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

void FixCanaries::CalculateBaseAndSize()
{
	assert(m_elfiop != NULL);
	ELFIO::section *text_section = m_elfiop->sections[".text"];

	m_text_base = text_section->get_address();
	m_text_size = text_section->get_size();
}
Instruction_t *FixCanaries::add_instrumentation(Instruction_t *site,
	const char *canary_register)
{
	FileIR_t *firp = getFileIR();
	virtual_offset_t postCallbackReturn = getAvailableAddress();
	char pushRetBuf[100], movCanaryValueBuf[100], movfs0x28Buf[100];
	sprintf(pushRetBuf,"push  0x%x", postCallbackReturn);
	sprintf(movCanaryValueBuf,"mov rsi, %s", canary_register);
	sprintf(movfs0x28Buf,"mov rdi, [fs:0x28]");

	Instruction_t *tmp=site, *callback=NULL, *post_callback=NULL;

	tmp=insertAssemblyAfter(firp,tmp,"push rsp");
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
	/*
	 * The "bogus" return address that we push here
	 * will be popped by the callback handler 
	 * invocation code in zipr.
	 */
	tmp=insertAssemblyAfter(firp,tmp,pushRetBuf);	 // push <ret addr>

	callback=tmp=insertAssemblyAfter(firp,tmp,"nop");
	callback->SetCallback("zipr_debug_canary_callback");

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
	CalculateBaseAndSize();

	cout << "base: " << std::hex << m_text_base << endl;
	cout << "size: " << std::hex << m_text_size << endl;

	/*
	 * Let's find the biggest, without going over. 
	 * We need this to be a power of 2. 
	 */
	while (adjusted_size<m_text_size) adjusted_size |= adjusted_size<<1;
	m_text_size = adjusted_size>>1;
	m_text_size &= ~((uint64_t)8);

	cout << "m_text_size: " << std::hex << m_text_size << endl;

	/*
	 * So, anything between base and base+size
	 * are valid for us to pick values from.
	 */

	for(
	  set<Function_t*>::const_iterator itf=getFileIR()->GetFunctions().begin();
	  itf!=getFileIR()->GetFunctions().end();
	  ++itf
	  )
	{
		Function_t* func=*itf;
		for(
		  set<Instruction_t*>::const_iterator it=func->GetInstructions().begin();
		  it!=func->GetInstructions().end();
		  ++it)
		{
			DISASM d;
			Instruction_t* insn = *it;

			insn->Disassemble(d);

			/*
			 * Check to see if this is the push canary
			 * operation.
			 */
			if ((d.Instruction.Category & 0xFFFF) == DATA_TRANSFER &&
				  d.Argument2.SegmentReg == FSReg &&
					d.Argument2.Memory.Displacement == 0x28 &&
					d.Argument1.ArgType & REGISTER_TYPE) {
				int64_t displacement = 0;
				uint16_t reg_value = 0;
				char *reg_name = NULL;
				char asm_buffer[256] = {0,};
				char ereg_name[256] = {0,};
				Instruction_t *tmp_insn;
				Instruction_t *original_fallthrough;

				original_fallthrough = insn->GetFallthrough();
				assert(original_fallthrough);

				reg_value = d.Argument1.ArgType & 0xFFFF;
				displacement = d.Argument2.Memory.Displacement;
				
				reg_name = d.Argument1.ArgMnemonic;
				strcpy(ereg_name, reg_name);
				ereg_name[0] = 'e';

				if (m_verbose == true) {
					cout << "Complete Instr: " << d.CompleteInstr << endl;
					cout << "reg_value: 0x" << std::hex << reg_value << endl;
					cout << "reg name : " << reg_name << endl;
					cout << "displacement: 0x" << std::hex << displacement << endl;
				}
				/*
				 * The output of this instruction goes in reg_name.
				 * That value will be the value stored on the stack
				 * as the canary.
				 */

				sprintf(asm_buffer, "mov %s, 0xFFFFFFFF%08x\n", reg_name, m_text_size);
				cout << "asm_buffer: " << asm_buffer;
				setAssembly(insn, asm_buffer);
				memset(asm_buffer, 0, sizeof(asm_buffer));
				
				sprintf(asm_buffer, "and %s, [fs:0x%x]\n", reg_name, displacement);
				cout << "asm_buffer: " << asm_buffer;
				tmp_insn = addNewAssembly(insn, asm_buffer);
				memset(asm_buffer, 0, sizeof(asm_buffer));
				
				sprintf(asm_buffer, "add %s, 0x%x\n", reg_name, m_text_base);
				cout << "asm_buffer: " << asm_buffer;
				tmp_insn = addNewAssembly(tmp_insn, asm_buffer);
				memset(asm_buffer, 0, sizeof(asm_buffer));
				
				sprintf(asm_buffer, "mov [fs:0x%x], %s\n", displacement, reg_name);
				cout << "asm_buffer: " << asm_buffer;
				tmp_insn = addNewAssembly(tmp_insn, asm_buffer);
				memset(asm_buffer, 0, sizeof(asm_buffer));
				
				sprintf(asm_buffer, "mov %s, [%s]\n", reg_name, ereg_name);
				cout << "asm_buffer: " << asm_buffer;
				tmp_insn = addNewAssembly(tmp_insn, asm_buffer);
				memset(asm_buffer, 0, sizeof(asm_buffer));
				
				sprintf(asm_buffer, "shr %s, 32\n", reg_name);
				cout << "asm_buffer: " << asm_buffer;
				tmp_insn = addNewAssembly(tmp_insn, asm_buffer);
				memset(asm_buffer, 0, sizeof(asm_buffer));
				
				sprintf(asm_buffer, "shl %s, 32\n", reg_name);
				cout << "asm_buffer: " << asm_buffer;
				tmp_insn = addNewAssembly(tmp_insn, asm_buffer);
				memset(asm_buffer, 0, sizeof(asm_buffer));
				
				sprintf(asm_buffer, "xor %s, [fs:0x%x]\n", reg_name, displacement);
				cout << "asm_buffer: " << asm_buffer;
				tmp_insn = addNewAssembly(tmp_insn, asm_buffer);
				memset(asm_buffer, 0, sizeof(asm_buffer));
				
				sprintf(asm_buffer, "rol %s, 32\n", reg_name);
				cout << "asm_buffer: " << asm_buffer;
				tmp_insn = addNewAssembly(tmp_insn, asm_buffer);
				memset(asm_buffer, 0, sizeof(asm_buffer));

				/*
				 * Callback will print values of rax (what we are
				 * about to push as the canary value) and fs:0x28
				 * (the value of the key and index).
				 */
				tmp_insn = add_instrumentation(tmp_insn, reg_name);

				tmp_insn->SetFallthrough(original_fallthrough);
			}
			/* 
			 * Check to see if this is the start of the pop canary
			 * operation.
			 */
			if ((d.Instruction.Category & 0xFFFF) == DATA_TRANSFER) {
				uint16_t storage_reg_value = 0;
				int distance = 0, max_distance = 2;
				Instruction_t *xor_instruction = NULL;
				bool followed_by_xor = false;
				
				/*
				 * We are looking for 
				 * mov A, qword [rsp+0x??]
				 * followed (closely) by
				 * xor A, qword [fs:0x28]
				 */
				if (d.Argument2.ArgType & MEMORY_TYPE &&
				    /* d.Argument2.Memory.BaseRegister == 0x10 && *//* RSP */
						d.Argument1.ArgType & REGISTER_TYPE) {
					storage_reg_value = d.Argument1.ArgType & 0xFFFF;

					DISASM xd;
					xor_instruction = insn->GetFallthrough();
					while (xor_instruction != NULL && distance < max_distance) {
						xor_instruction->Disassemble(xd);
						if ((xd.Instruction.Category & 0xFFFF) == LOGICAL_INSTRUCTION &&
						    xd.Argument2.SegmentReg == FSReg &&
								xd.Argument2.Memory.Displacement == 0x28 &&
								xd.Argument1.ArgType & REGISTER_TYPE &&
								(xd.Argument1.ArgType & 0xFFFF) == storage_reg_value) {
							followed_by_xor = true;
							break;
						}
						distance++;
						xor_instruction = xor_instruction->GetFallthrough();
					}
					if (followed_by_xor == true) {
						if (m_verbose == true) {
							cout << "Complete Instr: " << d.CompleteInstr << endl;
							cout << "Complete Instr: " << xd.CompleteInstr << endl;
							cout << endl;
						}
						char *rsp;
						char *reg_name;
						char ereg_name[256] = {0,};
						char asm_buffer[256] = {0,};
						Instruction_t *tmp_insn;
						int64_t displacement = 0;
				
						rsp = d.Argument2.ArgMnemonic;

						reg_name = d.Argument1.ArgMnemonic;
						strcpy(ereg_name, reg_name);
						ereg_name[0] = 'e';

						displacement = xd.Argument2.Memory.Displacement;

						sprintf(asm_buffer, "mov %s, [%s]\n",reg_name,rsp);
						cout << "asm_buffer: " << asm_buffer;
						setAssembly(insn, asm_buffer);
						memset(asm_buffer, 0, sizeof(asm_buffer));
						
						sprintf(asm_buffer, "rol %s, 32\n",reg_name);
						cout << "asm_buffer: " << asm_buffer;
						tmp_insn = addNewAssembly(insn, asm_buffer);
						memset(asm_buffer, 0, sizeof(asm_buffer));
						
						sprintf(asm_buffer, "mov %s, [%s]\n",reg_name,ereg_name);
						cout << "asm_buffer: " << asm_buffer;
						tmp_insn = addNewAssembly(tmp_insn, asm_buffer);
						memset(asm_buffer, 0, sizeof(asm_buffer));
						
						sprintf(asm_buffer, "xor %s, [fs:0x%x]\n",reg_name,displacement);
						cout << "asm_buffer: " << asm_buffer;
						tmp_insn = addNewAssembly(tmp_insn, asm_buffer);
						memset(asm_buffer, 0, sizeof(asm_buffer));
						
						sprintf(asm_buffer, "shr %s, 32\n",reg_name);
						cout << "asm_buffer: " << asm_buffer;
						tmp_insn = addNewAssembly(tmp_insn, asm_buffer);
						memset(asm_buffer, 0, sizeof(asm_buffer));
						
						sprintf(asm_buffer, "xor %s, [%s]\n",reg_name,rsp);
						cout << "asm_buffer: " << asm_buffer;
						tmp_insn = addNewAssembly(tmp_insn, asm_buffer);
						tmp_insn->SetFallthrough(xor_instruction);
						memset(asm_buffer, 0, sizeof(asm_buffer));
						
						sprintf(asm_buffer, "cmp %s, 0x0\n",ereg_name);
						cout << "asm_buffer: " << asm_buffer;
						setAssembly(xor_instruction, asm_buffer);
						memset(asm_buffer, 0, sizeof(asm_buffer));
					}
				}
			}
		}
	}
	return true;
}
