#include "hook_dynamic_calls.hpp"

#include "Rewrite_Utility.hpp"
#include <assert.h>
#include <stdexcept>

using namespace libTransform;
using namespace ELFIO;
using namespace libIRDB;

HookDynamicCalls::HookDynamicCalls(FileIR_t *p_variantIR) :
	Transform(NULL, p_variantIR, NULL),
	m_plt_addresses(NULL)
{
	
}

HookDynamicCalls::~HookDynamicCalls() 
{
	if (m_plt_addresses)
		free(m_plt_addresses);
}

Elf64_Addr HookDynamicCalls::ReadAddressInSectionAtOffset(
	section *section, Elf64_Off offset)
{
	Elf64_Addr address;
	const char *data = NULL;
	LoadElf();

	data = section->get_data();
	cout << "data: " << std::hex << (void*)data << endl;
	cout << "seeked to: 0x"
	     << std::hex 
			 << offset
			 << endl;

	memcpy(&address, &(data[offset]), sizeof(Elf64_Addr));
	
	return address;
}

void HookDynamicCalls::LoadElf()
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

void HookDynamicCalls::LoadPltIndexTable()
{
	section *rela_plt_section = NULL;
	section *got_plt_section = NULL;
	int i = 0;
	Elf_Xword entry_index = 0;
	Elf64_Addr value = 0, offset = 0;
	Elf_Word type;
	Elf_Sxword addend, calcValue;
	string name;
	std::unique_ptr<relocation_section_accessor> pRsa;
	unsigned int plt_second_half_offset = 0x6;

	if (m_plt_addresses) return;

	rela_plt_section = m_elfiop->sections[".rela.plt"];
	got_plt_section = m_elfiop->sections[".got.plt"];
	assert(rela_plt_section && got_plt_section);

	pRsa.reset(new relocation_section_accessor(*m_elfiop, rela_plt_section));
	m_plt_addresses = (Elf64_Addr*)calloc(pRsa->get_entries_num()+1, sizeof(Elf64_Addr));

	for (entry_index = 0; entry_index < pRsa->get_entries_num(); entry_index++)
	{
		if (pRsa->get_entry(entry_index,
		                    offset,
		                    value,
		                    name,
		                    type,
		                    addend,
		                    calcValue))
		{
			Elf64_Addr plt_address = 0;
			Elf64_Off got_plt_offset = 0;

			got_plt_offset = offset - got_plt_section->get_address();
			plt_address = ReadAddressInSectionAtOffset(
				got_plt_section,
				got_plt_offset);
			m_plt_addresses[entry_index] = plt_address - plt_second_half_offset;
		}
	}

	for (i=0; m_plt_addresses[i] != 0; i++)
		cout << "m_plt_addresses[" << i << "]: "
		     << "0x" << std::hex << m_plt_addresses[i] << endl;
}

void HookDynamicCalls::MakeSymbolOffsetTable()
{
	string name;

	Elf_Xword entry_index = 0;
	Elf64_Addr value = 0, offset = 0;
	Elf_Word type;
	Elf_Sxword addend, calcValue;

	section *rela_plt_section = NULL;
	std::unique_ptr<relocation_section_accessor> pRsa;

	if (m_symbol_offset_table) return;

	LoadElf();
	LoadPltIndexTable();

	m_symbol_offset_table.reset(new std::map<std::string, ELFIO::Elf64_Addr>);

	rela_plt_section = m_elfiop->sections[".rela.plt"];
	assert(rela_plt_section);

	cout << "section: 0x"<<std::hex<< rela_plt_section->get_address() << endl;
	pRsa.reset(new relocation_section_accessor(*m_elfiop, rela_plt_section));
	for (entry_index = 0; entry_index < pRsa->get_entries_num(); entry_index++)
	{
		if (pRsa->get_entry(entry_index,
											 offset,
											 value,
											 name,
											 type,
											 addend,
											 calcValue))
		{
			m_symbol_offset_table->insert(std::pair<std::string, Elf64_Addr>(name, m_plt_addresses[entry_index]));
		}
	}
	
	std::map<std::string, Elf64_Addr>::iterator it;
	it = m_symbol_offset_table->begin();
	for (; it != m_symbol_offset_table->end(); it++)
	{
		string n = it->first;
		Elf64_Addr address = it->second;
		cout << "symbol:    " << n << "-" << endl;
		cout << "Address:   0x" << std::hex << address << endl;
	}

}

virtual_offset_t HookDynamicCalls::GetSymbolOffset(string &symbol)
{
	LoadElf();
	LoadPltIndexTable();
	MakeSymbolOffsetTable();

	Elf64_Addr address = 0;

	try {
		address = m_symbol_offset_table->at(symbol);
	} catch (const std::out_of_range &a) {
		address = -1;
	}

	cerr << "" << symbol << ": 0x" << std::hex << address << endl;
	return address;
}

Instruction_t *HookDynamicCalls::add_instrumentation(Instruction_t *site,unsigned long id)
{
	FileIR_t *firp = getFileIR();
	virtual_offset_t postCallbackReturn = getAvailableAddress();
	char pushRetBuf[100], movIdBuf[100], movRaxBuf[100], movRspBuf[100];
	sprintf(pushRetBuf,"push  0x%x", postCallbackReturn);
	sprintf(movIdBuf,"mov rdi, 0x%x", id);
	sprintf(movRaxBuf,"mov rsi, rax");
	sprintf(movRspBuf,"mov rdx, rbp");

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
	tmp=insertAssemblyAfter(firp,tmp,movIdBuf);
	tmp=insertAssemblyAfter(firp,tmp,movRaxBuf);
	tmp=insertAssemblyAfter(firp,tmp,movRspBuf);
	/*
	 * The "bogus" return address that we push here
	 * will be popped by the callback handler 
	 * invocation code in zipr.
	 */
	tmp=insertAssemblyAfter(firp,tmp,pushRetBuf);	 // push <ret addr>

	callback=tmp=insertAssemblyAfter(firp,tmp,"nop");
	callback->SetCallback("zipr_hook_dynamic_callback");

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

void HookDynamicCalls::SetToHook(map<string,int> to_hook)
{
	m_to_hook = to_hook;
}

bool HookDynamicCalls::GetPltCallTarget(Instruction_t *insn,
	virtual_offset_t &target_addr) {

	section *got_plt_section = NULL;
	Instruction_t *target = NULL;
	virtual_offset_t target_target = 0;
	string target_bits;
	Elf64_Addr target_target_target;

	LoadElf();
	got_plt_section = m_elfiop->sections[".got.plt"];

	/*
	 * PLT entry:
	 *
	 * L0: jmp *CALL_FIXED (a)
	 * L1: push CALL_FIXUP_IDX (b)
	 * L2: jmp  CALL_FIXER_UPPER (c)
	 * ...
	 * FORK_FIXED: &L1 (d)
	 */

	/*
	 * Is this an instruction with a target?
	 * That's a must.
	 */
	if (!(target = insn->GetTarget()))
		return false;

	/*
	 * a
	 */
	target_bits = target->GetDataBits();
	
	/*
	 * Determine the type of the operand
	 * based on the opcode. Is it relative?
	 * Is it absolute?
	 * 
	 * target_target: CALL_FIXED 
	 */
	switch ((uint8_t)target_bits[0])
	{
		case 0xFF:
			target_target = *((uint32_t*)&target_bits[2]) + target_bits.length();
			break;
		default:
			cout << "Not a handled control instruction opcode." << endl;
			break;
	}
	/*
	 * target_target contains the target of
	 * the target instruction, if one such
	 * exists.
	 */
	if (!target_target)
		return false;

	cout << "target_target: 0x"
	     << std::hex << target_target
			 << endl;
	/*
	 * Dereference the address at the target of the
	 * target to determine what /it/ points at.
	 * I.e., *CALL_FIXED
	 */
	target_target_target = ReadAddressInSectionAtOffset(
		got_plt_section,
		target_target - got_plt_section->get_address());
	cout << "target_target_target: 0x" 
	     << std::hex << target_target_target
			 << endl;

	/*
	 * Subtract 0x6 since that is the offset
	 * from the start of the PLT entry to the
	 * so-called second half (which starts at L1)
	 */
	target_addr = target_target_target - 0x6;
	return true;
}

int HookDynamicCalls::execute()
{
	assert(m_to_hook.size() != 0);
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
			Instruction_t* insn = *it;
			virtual_offset_t target;
			/*
			 * This is not quite done yet: Consider
			 * redirects that are rewritten as push/jmp
			 * combinations.
			 */
			if (GetPltCallTarget(insn, target))
			{
				cout << "target: " << std::hex << target << endl;
				map<string,int>::iterator m_to_hook_iterator = m_to_hook.begin();
				for (; m_to_hook_iterator != m_to_hook.end(); m_to_hook_iterator++)
				{
					string to_hook = m_to_hook_iterator->first;
					int hook_id = m_to_hook_iterator->second;
					if (target == GetSymbolOffset(to_hook))
					{
						cout << "hooking " << to_hook << " call at 0x" 
						     << std::hex << insn->GetAddress()->GetVirtualOffset()
								 << endl;
						add_instrumentation(insn, hook_id);
					}
				}
			}
		}
	}
	return false;
}
