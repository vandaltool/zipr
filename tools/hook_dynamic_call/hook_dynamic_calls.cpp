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
	char pushRetBuf[100], movIdBuf[100], movRaxBuf[100];
	sprintf(pushRetBuf,"push  0x%x", postCallbackReturn);
	sprintf(movIdBuf,"mov rdi, 0x%x", id);
	sprintf(movRaxBuf,"mov rsi, rax");

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
			Instruction_t *target = NULL;
			if (target = insn->GetTarget())
			{
				map<string,int>::iterator m_to_hook_iterator = m_to_hook.begin();
				for (; m_to_hook_iterator != m_to_hook.end(); m_to_hook_iterator++)
				{
					string to_hook = m_to_hook_iterator->first;
					int hook_id = m_to_hook_iterator->second;
					if (target->GetAddress()->GetVirtualOffset() == 
					    GetSymbolOffset(to_hook))
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
	return true;
}
