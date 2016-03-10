#include "fix_canaries.hpp"

#include "Rewrite_Utility.hpp"
#include <assert.h>
#include <stdexcept>
#include "beaengine/BeaEngine.h"

using namespace libTransform;
using namespace ELFIO;
using namespace libIRDB;

FixCanaries::FixCanaries(FileIR_t *p_variantIR) :
	Transform(NULL, p_variantIR, NULL), m_verbose(true)
{
	
}

FixCanaries::~FixCanaries() 
{
}

void FixCanaries::set_callback(const std::string &callback) {
	m_callback = callback;
}

void FixCanaries::LoadElf()
{
	unsigned int elfoid=0;
	pqxxDB_t *interface=NULL;

	if (m_elfiop)
		return;

	elfoid = getFileIR()->GetFile()->GetELFOID();
	interface = dynamic_cast<pqxxDB_t*>(BaseObj_t::GetInterface());
		
	assert(interface);

	file_object.reset(
		new pqxx::largeobjectaccess(interface->GetTransaction(),
		                            elfoid,
		                            std::ios::in));
	
	file_object->to_file("tmp.exe");

	m_elfiop.reset(new ELFIO::elfio);
	m_elfiop->load("tmp.exe");
}

void FixCanaries::FindStartAddress()
{
	assert(m_elfiop != NULL);
	m_start_addr = m_elfiop->get_entry();
	cout << "m_start_addr: 0x" << std::hex << m_start_addr << endl;
}

Instruction_t *FixCanaries::add_instrumentation(Instruction_t *site,
	const char *canary_register, const char *callback_name, const char *lea)
{
	FileIR_t *firp = getFileIR();
	virtual_offset_t postCallbackReturn = getAvailableAddress();
	char pushRetBuf[100], movCanaryValueBuf[100], movfs0x28Buf[100], setRdx[100];
	sprintf(pushRetBuf,"push  0x%x", postCallbackReturn);
	sprintf(movCanaryValueBuf,"mov rsi, %s", canary_register);
	sprintf(movfs0x28Buf,"mov rdi, [fs:0x28]");

	if (lea != NULL)
		sprintf(setRdx,"lea rdx, [%s]", lea);

	Instruction_t *tmp=site, *callback=NULL, *post_callback=NULL;

	tmp=::insertAssemblyAfter(firp,tmp,"push rsp");
	tmp=insertAssemblyAfter(firp,tmp,"push rbp");
	tmp=insertAssemblyAfter(firp,tmp,"push rdi");
	tmp=insertAssemblyAfter(firp,tmp,"push rsi");
	tmp=insertAssemblyAfter(firp,tmp,"push rdx");
	tmp=insertAssemblyAfter(firp,tmp,"push rcx");
	tmp=insertAssemblyAfter(firp,tmp,"push rbx");
	tmp=insertAssemblyAfter(firp,tmp,"push rax");
	tmp=insertAssemblyAfter(firp,tmp,"push r8");
	tmp=insertAssemblyAfter(firp,tmp,"push r9");
	tmp=insertAssemblyAfter(firp,tmp,"push r10");
	tmp=insertAssemblyAfter(firp,tmp,"push r11");
	tmp=insertAssemblyAfter(firp,tmp,"push r12");
	tmp=insertAssemblyAfter(firp,tmp,"push r13");
	tmp=insertAssemblyAfter(firp,tmp,"push r14");
	tmp=insertAssemblyAfter(firp,tmp,"push r15");
	tmp=insertAssemblyAfter(firp,tmp,"pushf");
	tmp=insertAssemblyAfter(firp,tmp,movfs0x28Buf);
	tmp=insertAssemblyAfter(firp,tmp,movCanaryValueBuf);
	if (lea != NULL)
		tmp=insertAssemblyAfter(firp,tmp,setRdx);
	else
		tmp=insertAssemblyAfter(firp,tmp,"mov rdx, 0x0");
	/*
	 * The "bogus" return address that we push here
	 * will be popped by the callback handler 
	 * invocation code in zipr.
	 */
	tmp=insertAssemblyAfter(firp,tmp,pushRetBuf);	 // push <ret addr>

	callback=tmp=insertAssemblyAfter(firp,tmp,"nop");
	callback->SetCallback(callback_name);

	post_callback=tmp=insertAssemblyAfter(firp,tmp,"popf");
	post_callback->GetAddress()->SetVirtualOffset(postCallbackReturn);

	tmp=insertAssemblyAfter(firp,tmp,"pop r15");
	tmp=insertAssemblyAfter(firp,tmp,"pop r14");
	tmp=insertAssemblyAfter(firp,tmp,"pop r13");
	tmp=insertAssemblyAfter(firp,tmp,"pop r12");
	tmp=insertAssemblyAfter(firp,tmp,"pop r11");
	tmp=insertAssemblyAfter(firp,tmp,"pop r10");
	tmp=insertAssemblyAfter(firp,tmp,"pop r9");
	tmp=insertAssemblyAfter(firp,tmp,"pop r8");
	tmp=insertAssemblyAfter(firp,tmp,"pop rax");
	tmp=insertAssemblyAfter(firp,tmp,"pop rbx");
	tmp=insertAssemblyAfter(firp,tmp,"pop rcx");
	tmp=insertAssemblyAfter(firp,tmp,"pop rdx");
	tmp=insertAssemblyAfter(firp,tmp,"pop rsi");
	tmp=insertAssemblyAfter(firp,tmp,"pop rdi");
	tmp=insertAssemblyAfter(firp,tmp,"pop rbp");
	tmp=insertAssemblyAfter(firp,tmp,"lea rsp, [rsp+8]");

	return tmp;
}

int FixCanaries::execute()
{
	uint64_t adjusted_size = 1;

	LoadElf();
	FindStartAddress();
#if 0
	for(
	  set<Function_t*>::const_iterator itf=getFileIR()->GetFunctions().begin();
	  itf!=getFileIR()->GetFunctions().end();
	  ++itf
	  )
	{
		Function_t* func=*itf;
#endif
		for(
//		  set<Instruction_t*>::const_iterator it=func->GetInstructions().begin();
		  set<Instruction_t*>::const_iterator it=getFileIR()->GetInstructions().begin();
//			it!=func->GetInstructions().end();
		  it!=getFileIR()->GetInstructions().end();
		  ++it)
		{
			DISASM d;
			Instruction_t* insn = *it;

			insn->Disassemble(d);

			if (insn->GetAddress()->GetVirtualOffset() == m_start_addr) {
				if (m_verbose)
					cout << "Found and hooked the start instruction." << endl;
				add_instrumentation(insn, "rsp", "zipr_set_top_of_stack");
			}
			/*
			 * Check to see if this is the push canary
			 * operation.
			 * mov target_reg_name, [fs:0xfs_displacement]
			 * mov rsp_displacement(rsp_reg_name), target_reg_name
			 *     |---rsp_reg_and_offset-------|
			 */
			else if ((d.Instruction.Category & 0xFFFF) == DATA_TRANSFER &&
				  d.Argument2.SegmentReg == FSReg &&
					d.Argument2.Memory.Displacement == 0x28 &&
					d.Argument1.ArgType & REGISTER_TYPE) {
				DISASM md;
				Instruction_t *mov_insn;

				bool numeric_register = false;

				int i = 0;

				char *target_reg_name = NULL;
				uint16_t target_reg_value = 0;
				char target_ereg_name[256] = {0,};
				char target_sreg_name[256] = {0,};

				char *rsp_sreg_name = NULL, *rsp_reg_and_offset = NULL;
				char rsp_reg_name[256] = {0,};
				
				char asm_buffer[256] = {0,};
				int64_t fs_displacement = 0, rsp_displacement = 0;
				uint64_t abs_rsp_displacement = 0;
				
				if (!(mov_insn = insn->GetFallthrough()))
					continue;

				/*
				 * Setup the target register information.
				 */
				target_reg_value = d.Argument1.ArgType & 0xFFFF;
				target_reg_name = d.Argument1.ArgMnemonic;

				/*
				 * Two cases here: 
				 * 1: There is rax,rcx,rdx, etc.
				 * 2: There is r12, r11, etc.
				 */
				i = 0;
				while (target_reg_name[i] != '\0') {
					if (((int)target_reg_name[i]) >= ((int)'0') && 
					    ((int)target_reg_name[i]) <= ((int)'9'))
					{
						numeric_register = true;
						break;
					}
					i++;
				}
				if (!numeric_register)
				{
					strcpy(target_ereg_name, target_reg_name);
					strcpy(target_sreg_name, target_reg_name+1);
					target_ereg_name[0] = 'e';
				} else {
					strcpy(target_ereg_name, target_reg_name);
					strcpy(target_sreg_name, target_reg_name);
					strcat(target_ereg_name, "d");
					strcat(target_sreg_name, "w");
				}

				fs_displacement = d.Argument2.Memory.Displacement;

				mov_insn->Disassemble(md);

				if ((md.Instruction.Category & 0xFFFF) ==DATA_TRANSFER &&
				    (md.Argument1.Memory.BaseRegister == 0x10 ||
				     md.Argument1.Memory.BaseRegister == 0x20
						) &&
				    md.Argument2.ArgType & REGISTER_TYPE &&
						(md.Argument2.ArgType & 0xFFFF) == target_reg_value) {
					Instruction_t *tmp_insn;
					int i = 0;
					char *displacement_operation = NULL;

					rsp_displacement = md.Argument1.Memory.Displacement;
					rsp_reg_and_offset = md.Argument1.ArgMnemonic;
					strcpy(rsp_reg_name, rsp_reg_and_offset);
					while (rsp_reg_name[i] != '\0' &&
					       rsp_reg_name[i] != '+' &&
					       rsp_reg_name[i] != '-') i++; 
					rsp_reg_name[i] = '\0';
					rsp_sreg_name = rsp_reg_name + 1;
					
					if (m_verbose == true) {
						cout << "Load Canary Instr: " << d.CompleteInstr << endl;
						cout << "Push Canary Instr: " << md.CompleteInstr << endl;
						cout << "Mov Base Register: 0x" << std::hex << md.Argument1.Memory.BaseRegister <<endl;
						cout << "FS displacement: 0x" << std::hex << fs_displacement <<endl;
						cout << "Target register: " << endl;
						cout << "\tvalue: 0x" << std::hex << target_reg_value << endl;
						cout << "\tnames: " << target_reg_name << ", "
						     << target_ereg_name << ", "
						     << target_sreg_name << endl;
						cout << "RSP register: " << endl;
						cout << "\tregister and offset: " << rsp_reg_and_offset << endl;
						cout << "\tnames: " << rsp_reg_name << ", "
						     << rsp_sreg_name << endl;
						cout << "\tdisplacement: 0x"<<std::hex<<rsp_displacement <<endl;
					}

					/*
					 * Move canary reference value into register.
					 */
					sprintf(asm_buffer, "mov %s, [fs:0x%x]\n",
						target_reg_name,
						fs_displacement);
					if (m_verbose == true)
						cout << "asm_buffer: " << asm_buffer;
					setAssembly(insn, asm_buffer);
					memset(asm_buffer, 0, sizeof(asm_buffer));

					/*
					 * target register has 
					 * xxxxxxxxxxxxaaaa
					 * where aaaa is 16 bits of stack address
					 * and x...x is canary value.
					 */
				
					/*
					 * Calculate the difference between the stack pointer
					 * (the lowest 16 bits, actually!) and the previously 
					 * saved stack pointer (lower 16 bits of target register, 
					 * accessed through its 16-bit name).
					 */
					sprintf(asm_buffer, "sub %s, %s\n", target_sreg_name, rsp_sreg_name);
					if (m_verbose == true)
						cout << "asm_buffer: " << asm_buffer;
					tmp_insn = addNewAssembly(insn, asm_buffer);
					memset(asm_buffer, 0, sizeof(asm_buffer));

					/*
					 * target register contains the offset from current
					 * stack pointer to the location of the canary
					 * above us. 
					 */

					/*
					 * But, we need to be careful because we aren't actually
					 * now at the top of the stack with rsp. It's been adjusted
					 * by the size of the locals. Adjust based on that.
					 */
					if (rsp_displacement<0) {
						abs_rsp_displacement = rsp_displacement*-1;
						displacement_operation = "add";
					} else {
						displacement_operation = "sub";
						abs_rsp_displacement = rsp_displacement;
					}
					sprintf(asm_buffer, "%s %s, 0x%x\n",
						displacement_operation,
						target_sreg_name,
						abs_rsp_displacement);
					if (m_verbose == true)
						cout << "asm_buffer: " << asm_buffer;
					tmp_insn = addNewAssembly(tmp_insn, asm_buffer);
					memset(asm_buffer, 0, sizeof(asm_buffer));
			
					/*
					 * Finally, put the canary value on to the stack.
					 */
					sprintf(asm_buffer, "mov [%s], %s\n",
						rsp_reg_and_offset,
						target_reg_name);
					if (m_verbose == true)
						cout << "asm_buffer: " << asm_buffer;
					tmp_insn = addNewAssembly(tmp_insn, asm_buffer);
					memset(asm_buffer, 0, sizeof(asm_buffer));

					if (m_callback.length() != 0) {
						tmp_insn = add_instrumentation(
							tmp_insn,
							target_reg_name,
							m_callback.c_str(), rsp_reg_and_offset);
						tmp_insn->SetFallthrough(mov_insn);
					}
				
					/*
					 * Save canary+top of stack back to canary
					 * reference value.
					 */

					/*
					 * Keep all of the canary reference value
					 * except for the 16 bits that hold the
					 * top of stack value.
					 */
					sprintf(asm_buffer, "mov %s, 0xFFFFFFFFFFFF0000\n", target_reg_name);
					if (m_verbose == true)
						cout << "asm_buffer: " << asm_buffer;
					tmp_insn = addNewAssembly(tmp_insn, asm_buffer);
					memset(asm_buffer, 0, sizeof(asm_buffer));
					
					sprintf(asm_buffer, "and %s, [fs:0x%x]\n",
						target_reg_name,
						fs_displacement);
					if (m_verbose == true)
						cout << "asm_buffer: " << asm_buffer;
					tmp_insn = addNewAssembly(tmp_insn, asm_buffer);
					memset(asm_buffer, 0, sizeof(asm_buffer));

					/*
					 * Put the top of into those empty 16 bits. Use
					 * an add here and the 16-bit name of the target
					 * register to do it easily.
					 */
					sprintf(asm_buffer, "add %s, %s\n", target_sreg_name, rsp_sreg_name);
					if (m_verbose == true)
						cout << "asm_buffer: " << asm_buffer;
					tmp_insn = addNewAssembly(tmp_insn, asm_buffer);
					memset(asm_buffer, 0, sizeof(asm_buffer));
					
					if (rsp_displacement<0) {
						abs_rsp_displacement = rsp_displacement*-1;
						displacement_operation = "sub";
					} else {
						abs_rsp_displacement = rsp_displacement;
						displacement_operation = "add";
					}
					sprintf(asm_buffer, "%s %s, 0x%x\n",
						displacement_operation,
						target_sreg_name,
						abs_rsp_displacement);
					if (m_verbose == true)
						cout << "asm_buffer: " << asm_buffer;
					tmp_insn = addNewAssembly(tmp_insn, asm_buffer);
					memset(asm_buffer, 0, sizeof(asm_buffer));

					/*
					 * Now, deposit that value back into the
					 * canary reference value.
					 */
					sprintf(asm_buffer, "mov [fs:0x%x], %s\n",
						fs_displacement,
						target_sreg_name);
					if (m_verbose == true)
						cout << "asm_buffer: " << asm_buffer;
					setAssembly(mov_insn, asm_buffer);
					memset(asm_buffer, 0, sizeof(asm_buffer));

					if (m_callback.length() != 0) {
						Instruction_t *t = mov_insn->GetFallthrough();
						tmp_insn=add_instrumentation(
							mov_insn,
							target_reg_name,
							m_callback.c_str());
						tmp_insn->SetFallthrough(t);
					}
				}
			}
			/* 
			 * Check to see if this is the start of the pop canary
			 * operation.
			 */
			else if ((d.Instruction.Category & 0xFFFF) == DATA_TRANSFER) {
				DISASM xd;

				uint16_t target_reg_value = 0;
				char *target_reg_name = NULL;
				char target_ereg_name[256] = {0,};
				char target_sreg_name[256] = {0,};

				char *rsp_reg_and_offset = NULL;
				char rsp_reg_name[256] = {0,};
				char *rsp_sreg_name = NULL;

				int distance = 0, max_distance = 2;
				Instruction_t *xor_insn = NULL;
				bool followed_by_xor = false;
				
				/*
				 * We are looking for 
				 * mov target_reg_name, rsp_displacement(rsp_reg_name)
				 *                      |---rsp_reg_and_offset-------|
				 * followed (closely [within max_distance]) by
				 * xor target_reg_name, qword [fs:0xfs_displacement]
				 */
				if (d.Argument2.ArgType & MEMORY_TYPE &&
				    (d.Argument2.Memory.BaseRegister == 0x10 ||
				     d.Argument2.Memory.BaseRegister == 0x20
						) &&
						((d.Argument1.ArgType & 0xFFFF0000)==(REGISTER_TYPE+GENERAL_REG))) {

					target_reg_value = d.Argument1.ArgType & 0xFFFF;
					target_reg_name = d.Argument1.ArgMnemonic;

					xor_insn = insn->GetFallthrough();
					while (xor_insn != NULL && distance < max_distance) {
						xor_insn->Disassemble(xd);
						if ((xd.Instruction.Category & 0xFFFF) == LOGICAL_INSTRUCTION &&
						    xd.Argument2.SegmentReg == FSReg &&
								xd.Argument2.Memory.Displacement == 0x28 &&
								xd.Argument1.ArgType & REGISTER_TYPE &&
								(xd.Argument1.ArgType & 0xFFFF) == target_reg_value) {
							followed_by_xor = true;
							break;
						}
						distance++;
						xor_insn = xor_insn->GetFallthrough();
					}
					if (followed_by_xor == true) {
						char asm_buffer[256] = {0,};
						Instruction_t *tmp_insn;
						int64_t fs_displacement = 0;
						int i = 0;
						bool numeric_register = false;
				
						rsp_reg_and_offset = d.Argument2.ArgMnemonic;
						strcpy(rsp_reg_name, rsp_reg_and_offset);
						i = 0;
						while (rsp_reg_name[i] != '\0' &&
					       rsp_reg_name[i] != '+' &&
					       rsp_reg_name[i] != '-') i++; 
						rsp_reg_name[i] = '\0';
						rsp_sreg_name = rsp_reg_name + 1;

						/*
						 * Two cases here: 
						 * 1: There is rax,rcx,rdx, etc.
						 * 2: There is r12, r11, etc.
						 * 
						 * See (above) for additional description
						 * of how we are handling this.
						 */
						i = 0;
						while (target_reg_name[i] != '\0') {
							if (((int)target_reg_name[i]) >= ((int)'0') && 
							    ((int)target_reg_name[i]) <= ((int)'9'))
							{
								numeric_register = true;
								break;
							}
							i++;
						}
						if (!numeric_register)
						{
							strcpy(target_ereg_name, target_reg_name);
							strcpy(target_sreg_name, target_reg_name+1);
							target_ereg_name[0] = 'e';
						} else {
							strcpy(target_ereg_name, target_reg_name);
							strcpy(target_sreg_name, target_reg_name);
							strcat(target_ereg_name, "d");
							strcat(target_sreg_name, "w");
						}
						fs_displacement = xd.Argument2.Memory.Displacement;
						
						if (m_verbose == true) {
							cout << "Load Complete Instr: " << d.CompleteInstr << endl;
							cout << "XOR  Complete Instr: " << xd.CompleteInstr << endl;
							cout << "Load base register : 0x" << std::hex
							     << (d.Argument2.Memory.BaseRegister) << endl;
							cout << "FS displacement: 0x"<<std::hex << fs_displacement <<endl;
							cout << "Target register: " << endl;
							cout << "\tvalue: 0x" << std::hex << target_reg_value << endl;
							cout << "\tnames: " << target_reg_name << ", "
							     << target_ereg_name << ", "
							     << target_sreg_name << endl;
							cout << "RSP register: " << endl;
							cout << "\tregister and offset: " << rsp_reg_and_offset << endl;
							cout << "\tnames: " << rsp_reg_name << ", "
							     << rsp_sreg_name << endl;
						}

						/*
						 * Get the canary from the stack and put it in
						 * the target register.
						 */
						sprintf(asm_buffer, "mov %s, [%s]\n",
							target_reg_name,
							rsp_reg_and_offset);
						if (m_verbose == true)
							cout << "asm_buffer: " << asm_buffer;
						setAssembly(insn, asm_buffer);
						memset(asm_buffer, 0, sizeof(asm_buffer));

						/*
						 * Jump back to the previous top of stack
						 * using canary reference value
						 * and store that in the lower 16 of the target.
						 */
						sprintf(asm_buffer, "add %s, [fs:0x%x]\n",
							target_sreg_name,
							fs_displacement);
						if (m_verbose == true)
							cout << "asm_buffer: " << asm_buffer;
						tmp_insn = addNewAssembly(insn, asm_buffer);
						memset(asm_buffer, 0, sizeof(asm_buffer));
					
						/*
						 * Move just those updated 16 bits back into 
						 * the canary reference buffer.
						 */
						sprintf(asm_buffer, "mov [fs:0x%x], %s\n",
							fs_displacement, target_sreg_name);
						if (m_verbose == true)
							cout << "asm_buffer: " << asm_buffer;
						tmp_insn = addNewAssembly(tmp_insn, asm_buffer);
						memset(asm_buffer, 0, sizeof(asm_buffer));
						
						if (m_callback.length() != 0) {
							tmp_insn = add_instrumentation(
								tmp_insn,
								target_reg_name,
								m_callback.c_str());
							tmp_insn->SetFallthrough(xor_insn);
						}	

						/*
						 * Since this code did not touch the upper
						 * 64-16 bits of the canary value on the
						 * stack or the canary reference value, 
						 * any tampering should be evident through
						 * an xor.
						 */
						sprintf(asm_buffer, "xor %s, [fs:0x%x]\n",
							target_reg_name,
							fs_displacement);
						if (m_verbose == true)
							cout << "asm_buffer: " << asm_buffer;
						setAssembly(xor_insn, asm_buffer);
						memset(asm_buffer, 0, sizeof(asm_buffer));
					}
				}
			} 
		}
#if 0
	}
#endif
	return true;
}
