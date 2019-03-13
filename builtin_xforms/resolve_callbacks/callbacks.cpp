#include "callbacks.hpp"

#include <assert.h>
#include <stdexcept>
#include <fstream>

using namespace ELFIO;
using namespace IRDB_SDK;

template <class T> void ignore_result(const T&){}; 

Callbacks::Callbacks(FileIR_t *p_variantIR) :
	Transform_t(p_variantIR),
	m_firp(p_variantIR),
	m_text_offset(0)
{
}

Callbacks::~Callbacks() 
{
}

Elf64_Addr Callbacks::ReadAddressInSectionAtOffset(
	section *section, Elf64_Off offset)
{
	Elf64_Addr address;
	const char *data = NULL;

	data = section->get_data();
	cout << "data: " << std::hex << (void*)data << endl;
	cout << "seeked to: 0x"
	     << std::hex 
			 << offset
			 << endl;

	memcpy(&address, &(data[offset]), sizeof(Elf64_Addr));
	
	return address;
}

bool Callbacks::LoadElf(string filename)
{
	if (m_elfiop)
		return true;

	m_elfiop.reset(new ELFIO::elfio);
	return m_elfiop->load(filename);
}

bool Callbacks::CreateCallbacksScoop()
{
	assert(m_elfiop);
	section *text_section = m_elfiop->sections[string(".text")];
	assert(text_section);

	auto startaddr = m_firp->addNewAddress(m_firp->getFile()->getBaseID(), 0);
	auto endaddr   = m_firp->addNewAddress(m_firp->getFile()->getBaseID(), 0);
	string the_contents;
	string strata_to_data_cmd = "$STRATAFIER/strata_to_data " + m_callback_file + " callbacks.dat 0";

	assert(startaddr);
	assert(endaddr);

	/* start address */
	//startaddr->setVirtualOffset(text_section->get_address());
	//startaddr->setVirtualOffset(0x0);
	//startaddr->SetFileID(m_firp->getFile()->getBaseID());
	//m_firp->getAddresses().insert(startaddr);

#ifdef USE_ELFIO_FOR_CONTENTS
	/* end address */
	endaddr->setVirtualOffset(text_section->get_size()-1);

	the_contents.resize(text_section->get_size()); 
	// deal with .bss segments that are 0 init'd.
	if (text_section->get_data())
		the_contents.assign(text_section->get_data(),text_section->get_size());
#else
	fstream callbacks_dot_dat;
	int file_length;
	char *callbacks_dot_dat_mem  = NULL;

	ignore_result(system(strata_to_data_cmd.c_str()));

	/*
	 * Open the file to read.
	 */
	callbacks_dot_dat.open("callbacks.dat", std::fstream::in | std::fstream::ate);
	/*
	 * read the file length and then seek to the beginning.
	 */
	file_length = callbacks_dot_dat.tellg();
	
	callbacks_dot_dat.seekg(0, ios::beg);

	endaddr->setVirtualOffset(file_length-1);

	/*
	 * allocate space for the contents.
	 */
	callbacks_dot_dat_mem = (char*)calloc(file_length, sizeof(char));
	the_contents.resize(file_length);

	/*
	 * read in the file.
	 */
	callbacks_dot_dat.read(callbacks_dot_dat_mem, file_length);

	/*
	 * Close the file.
	 */
	callbacks_dot_dat.close();

	/*
	 * Copy the data into the string
	 */
	the_contents.assign(callbacks_dot_dat_mem, file_length);

	/*
	 * Free the memory.
	 */
	free(callbacks_dot_dat_mem);
#endif

	// endaddr->SetFileID(m_firp->getFile()->getBaseID());
	// m_firp->getAddresses().insert(endaddr);

	int permissions= 
	  ( ((text_section->get_flags() & SHF_ALLOC) == SHF_ALLOC) << 2 ) | 
	  //( ((text_section->get_flags() & SHF_WRITE) == SHF_WRITE) << 1 ) | 
	  ( 0x1 << 1 ) | 
	  ( ((text_section->get_flags() & SHF_EXECINSTR) == SHF_EXECINSTR) << 0 );

	/*
	callbacks_scoop = new DataScoop_t(BaseObj_t::NOT_IN_DATABASE,
				".callback_text",
				startaddr,
				endaddr,
				NULL,
				permissions,
				false,
				the_contents);

	m_firp->getDataScoops().insert(callbacks_scoop);
	*/
	auto callbacks_scoop = m_firp->addNewDataScoop(
		".callback_text",
		startaddr,
		endaddr,
		NULL,
		permissions,
		false,
		the_contents);
	(void)callbacks_scoop; // just giving to the IR

	return true;
}

void Callbacks::MakeSymbolOffsetTable()
{
	section *symbol_table_section = NULL;
	std::unique_ptr<symbol_section_accessor> pSsa;
	Elf_Xword entry_index = 0;

	if (m_symbol_offset_table) return;

	m_symbol_offset_table.reset(new std::map<std::string, ELFIO::Elf64_Addr>);

	symbol_table_section = m_elfiop->sections[".symtab"];
	assert(symbol_table_section);

	cout << "section: 0x"<<std::hex<< symbol_table_section->get_address() << endl;
	pSsa.reset(new symbol_section_accessor(*m_elfiop, symbol_table_section));
	for (entry_index = 0; entry_index < pSsa->get_symbols_num(); entry_index++)
	{
		string name; 
		unsigned char bind, type, other;
		Elf64_Addr value;
		Elf_Xword size;
		Elf_Half section_index;

		if (pSsa->get_symbol(entry_index,
											 name,
											 value,
											 size,
											 bind,
											 type,
											 section_index,
											 other))
		{
			if (type == STT_FUNC)
				m_symbol_offset_table->insert(std::pair<std::string, Elf64_Addr>(name, value));
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

VirtualOffset_t Callbacks::GetSymbolOffset(string &symbol)
{
	Elf64_Addr address = 0;

	try {
		address = m_symbol_offset_table->at(symbol);
	} catch (const std::out_of_range &a) {
		address = -1;
	}

	cerr << "" << symbol << ": 0x" << std::hex << address << endl;
	return address;
}

void Callbacks::SetCallbackFile(string filename)
{
	m_callback_file = filename;
}

int Callbacks::execute()
{
	/*
	 * Look throught the elf file and pull out the interesting parts
	 * and put it into a scoop.
	 */
	if(!LoadElf(m_callback_file))
	{
		cout<<"Error!  Cannot find callbacks file"<<m_callback_file<<endl;
		exit(2);
	}		
	MakeSymbolOffsetTable();
	CreateCallbacksScoop();
	/*
	 * Look through the code and find those instructions that have
	 * callbacks!
	 */
	for(
	  set<Instruction_t*>::const_iterator it=getFileIR()->getInstructions().begin();
	  it!=getFileIR()->getInstructions().end();
	  it++
	  )
	{
			Instruction_t *insn = *it;
			string callback_name = insn->getCallback();

			if (callback_name != "") {
				ELFIO::Elf64_Addr callback_addr = GetSymbolOffset(callback_name);
				if (callback_addr != (ELFIO::Elf64_Addr)-1) {
					cout << "This callback is at " << std::hex 
					     << callback_addr << endl;
					cout << "This callback is at " << std::hex 
					     << callback_addr-m_text_offset << " relative to .text." << endl;

					/*
					Relocation_t *callback_reloc = new Relocation_t();
					callback_reloc->SetType("callback_to_scoop");
					callback_reloc->setAddend(callback_addr - m_text_offset);
					callback_reloc->setWRT(callbacks_scoop);
					insn->getRelocations().insert(callback_reloc);
					*/
					auto callback_reloc=m_firp->addNewRelocation(insn, 0, "callback_to_scoop", 
						       callbacks_scoop, callback_addr - m_text_offset);
					(void)callback_reloc; // just giving to IR

				}
				else 
				{
					cout<<"Strong Warning!  Could not find "<<callback_name<<" in callbacks file."<<endl;	
				}
			}
		
	}
	return true;
}
