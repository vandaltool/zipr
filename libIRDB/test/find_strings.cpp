

#include <libIRDB-core.hpp>
#include <libIRDB-cfg.hpp>
#include <iostream>
#include <stdlib.h>
#include <cctype>
#include <elf.h>
#include <assert.h>

using namespace libIRDB;
using namespace std;


bool is_string_character(char c)
{
	if(c=='\n') return true;
	return isprint(c);
}


/* the stuff we need for reading an elf file */
typedef struct elf_info {
       	Elf32_Off sec_hdr_off, sec_off;
       	Elf32_Half secnum, strndx;
       	Elf32_Ehdr elfhdr;
       	Elf32_Word secsize;
       	Elf32_Shdr *sechdrs;
	char **sec_data;
	Elf32_Addr got;
} elf_info_t;

void found_string(string s, void* addr)
{
	char buff[s.length()+2];
	char *old_p=buff, *p;
	// use .data() instead of c_str(); can find multiple C-strings in one string
	memcpy(buff,s.data(),s.length());
	buff[s.length()]=0;

	do {
		// look for new lines in the string
		// if found, split it up and print out each one as it's own thing.
		while(p=strchr(old_p,'\n'))
		{
			*p=0;
			if (*old_p != 0)
			{
				cout << "Found string: \""<<old_p<<"\" at "<<std::hex<<addr<<std::dec<<endl;
				old_p=p+1;
			}
		} 

		if (*old_p != 0)
			cout << "Found string: \""<<old_p<<"\" at "<<std::hex<<addr<<std::dec<<endl;
		old_p = p = old_p + strlen(old_p) + 1;
	} while (p < buff + s.length());
}

void load_section(elf_info_t &ei, int i, pqxx::largeobjectaccess &loa, bool alloc)
{
	if( alloc && (ei.sechdrs[i].sh_flags & SHF_ALLOC) != SHF_ALLOC)
	{
		cerr<<"Cannot load non-alloc section\n";
		assert(0);
	}

	if(ei.sec_data[i]==NULL)
	{
		ei.sec_data[i]=(char*)calloc(ei.sechdrs[i].sh_size,1);
		if(ei.sechdrs[i].sh_type==SHT_NOBITS)
		{
			/* no need to read anything for NOBITS sections */
		}
		else
		{
//			cout<<"Loading section "<<std::dec<<i<<" vaddr: "<<std::hex<<ei.sechdrs[i].sh_addr<< "  size: "
//				<<std::dec<<ei.sechdrs[i].sh_size<< endl;
        		loa.seek(ei.sechdrs[i].sh_offset, std::ios_base::beg);
        		loa.cread((char*)ei.sec_data[i], ei.sechdrs[i].sh_size);
		}
	}
}

void check_for_string(char* p, void* addr)
{
	assert(p);
	if(!is_string_character(*p))
		return;

	string s;
	while(*p)
	{
		if(!is_string_character(*p))
			return;
		s+=*p;
		p++;
	}
	found_string(s, addr);
}

void is_string_pointer(void* addr, elf_info_t &ei, pqxx::largeobjectaccess &loa)
{
	int intaddr=(int)(addr);

	for(int i=0;i<ei.secnum;i++)
	{
		/* only look at loaded sections */
		if( (ei.sechdrs[i].sh_flags & SHF_ALLOC) != SHF_ALLOC)
			continue;

		if(ei.sechdrs[i].sh_addr <= intaddr && intaddr <= (ei.sechdrs[i].sh_addr+ei.sechdrs[i].sh_size))
		{
			/* we found a pointer into a loadable segment */
			load_section(ei,i,loa,true);
//			cout<<"Checking address "<<std::hex<<addr<<endl;
			check_for_string(ei.sec_data[i]+((int)addr-ei.sechdrs[i].sh_addr),addr);
		}
	}

}

void is_string_constant(DISASM& disasm)
{
	void *addr;

	if(disasm.Argument1.ArgType != MEMORY_TYPE || disasm.Argument2.ArgType == MEMORY_TYPE
	   || disasm.Argument1.ArgSize < 16 || disasm.Argument1.ArgSize > 32)
		return;

	addr = (void*)disasm.Instruction.Immediat;

	/* consider that this constant itself may be a string */
	unsigned char byte1=(((unsigned int)addr)>>24)&0xff;
	unsigned char byte2=(((unsigned int)addr)>>16)&0xff;
	unsigned char byte3=(((unsigned int)addr)>>8)&0xff;
	unsigned char byte4=(unsigned int)addr&0xff;
	
	if(  
		(is_string_character(byte1) || byte1==0) &&
		(is_string_character(byte2) || byte2==0) &&
		(is_string_character(byte3) || byte3==0) &&
		(is_string_character(byte4) || byte4==0)
	  )
     {
	char s[5];
	s[0]=byte4;
	s[1]=byte3;
	s[2]=byte2;
	s[3]=byte1;
	s[4]=0;
	if(strlen(s)>0) // only find 2+ character strings this way.
		found_string(s, addr);
     }



} 

void handle_argument(ARGTYPE *arg, elf_info_t &ei, pqxx::largeobjectaccess &loa)
{
        if( arg->ArgType == MEMORY_TYPE )
	{
		if ( !ei.got )
			is_string_pointer((void*)arg->Memory.Displacement,ei,loa);
		if ( ei.got && arg->Memory.BaseRegister == REG3 /* ebx */ )
			is_string_pointer((void*)(arg->Memory.Displacement + ei.got),ei,loa);
	}
}


void read_elf_info(elf_info_t &ei, FileIR_t* firp, pqxx::largeobjectaccess &loa)
{


        /* Read ELF header */
        loa.cread((char*)&ei.elfhdr, sizeof(Elf32_Ehdr)* 1);
        ei.sec_hdr_off = ei.elfhdr.e_shoff;
        ei.secnum = ei.elfhdr.e_shnum;
        assert(ei.secnum>0);
        ei.strndx = ei.elfhdr.e_shstrndx;

        /* Read Section headers */
        ei.sechdrs=(Elf32_Shdr*)malloc(sizeof(Elf32_Shdr)*ei.secnum);
	assert(ei.sechdrs!=NULL);
        loa.seek(ei.sec_hdr_off, std::ios_base::beg);
        loa.cread((char*)ei.sechdrs, sizeof(Elf32_Shdr)* ei.secnum);

	ei.sec_data=(char**)calloc(ei.secnum,sizeof(void*));

	ei.got = 0;
	/* Get .got or .got.plt address, if any */
	if (ei.strndx != SHN_UNDEF)
	{
		int shstr_sec;
		if (ei.strndx < SHN_LORESERVE)
			shstr_sec = ei.strndx;
		else
			shstr_sec = ei.sechdrs[0].sh_link;
		assert(shstr_sec < ei.secnum);
		load_section(ei,shstr_sec,loa,false);
		Elf32_Shdr *shstr_sec_hdr = ei.sechdrs + shstr_sec;
		for (int i=0;i<ei.secnum;i++)
		{
			assert(ei.sechdrs[i].sh_name < shstr_sec_hdr->sh_size);
			if (!strcmp(ei.sec_data[shstr_sec]+ei.sechdrs[i].sh_name, ".got.plt"))
			{
				// Prefer .got.plt to .got
				ei.got = ei.sechdrs[i].sh_addr;
				break;
			}
			if (!strcmp(ei.sec_data[shstr_sec]+ei.sechdrs[i].sh_name, ".got"))
				ei.got = ei.sechdrs[i].sh_addr;
		}
	}
}

void free_elf_info(elf_info_t &ei)
{
	#define FREE_IF_NOT_NULL(a) { if(a) { free(a); a=NULL; } }
	FREE_IF_NOT_NULL(ei.sechdrs);
	for(int i=0;i<ei.secnum;i++)	
		FREE_IF_NOT_NULL(ei.sec_data[i]);
	FREE_IF_NOT_NULL(ei.sec_data);
}

void find_strings_in_instructions(FileIR_t* firp, elf_info_t& ei, pqxx::largeobjectaccess &loa)
{
	set<Instruction_t*> visited_insns;

	// First pass; get strings from basic blocks,
	// concatenating consecutive immediates stored to the stack

	// Loop over all functions
	for(
		set<Function_t*>::const_iterator fit=firp->GetFunctions().begin();
		fit!=firp->GetFunctions().end();
		++fit
	   )
	{
		ControlFlowGraph_t cfg = ControlFlowGraph_t(*fit) ;
		// Loop over basic blocks in function
		for(
			set<BasicBlock_t*>::const_iterator bit=cfg.GetBlocks().begin();
			bit!=cfg.GetBlocks().end();
			++bit
		   )
		{
			// Loop over instructions in basic block
			vector<Instruction_t*>::const_iterator iit=(*bit)->GetInstructions().begin();
			while(iit!=(*bit)->GetInstructions().end())
			{
				Instruction_t *insn=*iit;
				DISASM disasm;
				char *str = NULL;

				int res=insn->Disassemble(disasm);
				assert(res);

				// Concatenate printable strings from consecutive store immediates to SP-relative stack addresses
				size_t size = 0;
				unsigned int olddisp = 0;
				unsigned int newdisp = 0;
				unsigned int basereg = 0;
				while(iit!=(*bit)->GetInstructions().end())
				{
//						cout<<"Pass 1: Checking insn: "<<disasm.CompleteInstr<<" id: "<<insn->GetBaseID()<<endl;

					// Break if not assignment of an immediate to an esp offset
					if (disasm.Argument1.ArgType != MEMORY_TYPE
					    || (disasm.Argument1.Memory.BaseRegister != REG4 /* esp */
					        && disasm.Argument1.Memory.BaseRegister != REG0 /* eax */)
					    || (basereg && disasm.Argument1.Memory.BaseRegister != basereg)
					    || disasm.Argument2.ArgType == MEMORY_TYPE
					    || disasm.Argument1.ArgSize > 32)
					{
						// mark visited
						visited_insns.insert(*iit);
						is_string_constant(disasm);
						break;
					}
					basereg = disasm.Argument1.Memory.BaseRegister;
					unsigned int disp = disasm.Argument1.Memory.Displacement;
					// break if displacement moved backward
					if (newdisp && (disp < newdisp || disp == olddisp))
						break;
					// mark visited
					visited_insns.insert(*iit);
					// check for a printable argument
					unsigned int imm = disasm.Instruction.Immediat;
					unsigned char byte1=(imm>>24)&0xff;
					unsigned char byte2=(imm>>16)&0xff;
					unsigned char byte3=(imm>>8)&0xff;
					unsigned char byte4=imm&0xff;
					size_t argsize = disasm.Argument1.ArgSize / 8;
					if (((is_string_character(byte1) || byte1==0) || argsize < 4) &&
					    ((is_string_character(byte2) || byte2==0) || argsize < 4) &&
					    ((is_string_character(byte3) || byte3==0) || argsize < 2) &&
					    (is_string_character(byte4) || byte4==0))
					{
						// printable, concatenate to built string
						assert(str = (char *)realloc(str, size+argsize));
						size += argsize;
						memcpy(str + size - argsize, (char *) (&imm), argsize);
					}
					else
					{
						// not printable, check for other strings
						is_string_constant(disasm);
						break;
					}
					++iit;
					if (iit == (*bit)->GetInstructions().end())
						break;
					insn = *iit;
					// break if none
					if (insn == NULL)
						break;
					olddisp = disp;
					// calculate expected displacement
					if (newdisp)
						newdisp += argsize;
					else
						newdisp = disp + argsize;
					res=insn->Disassemble(disasm);
					assert(res);
				}
				
				// Handle string if one was found
				if (size > 1) // only find 2+ character strings this way.
				{
					if (str[size-1] != 0)
					{
						assert(str = (char *)realloc(str, size+1));
						str[size] = 0;
						size++;
					}
					std::string s(str, size);
					found_string(s, str);
					free(str);
				}
				// advance to next if we've visited this instruction
				if (iit != (*bit)->GetInstructions().end()
				    && visited_insns.find(*iit) != visited_insns.end())
					++iit;
			}
		}
	}

	// second pass; grab any leftover stringy bits without chunking
	for(
		set<Instruction_t*>::const_iterator it=firp->GetInstructions().begin();
		it!=firp->GetInstructions().end(); 
		++it
	   )
	{
                Instruction_t *insn=*it;

//		cout<<"Pass 2: Checking insn: "<<disasm.CompleteInstr<<" id: "<<insn->GetBaseID()<<endl;
                DISASM disasm;
		int res=insn->Disassemble(disasm);
		assert(res);

		// always check for string pointers
		is_string_pointer((void*)disasm.Instruction.Immediat,ei,loa);
		handle_argument(&disasm.Argument1,ei,loa);
		handle_argument(&disasm.Argument2,ei,loa);
		handle_argument(&disasm.Argument3,ei,loa);

		// if not in a function, check for string in immediate
		if (visited_insns.find(insn) != visited_insns.end())
		{
			assert(insn->GetFunction());
			continue;
		}

//		if (insn->GetFunction())
//			cerr << "Warning: instruction at address ID " << insn->GetOriginalAddressID()
//			     << " is in function " << insn->GetFunction()->GetName() << " but not in its CFG." << endl;

		is_string_constant(disasm);
	}
}

void find_strings_in_data(FileIR_t* firp, elf_info_t& ei, pqxx::largeobjectaccess &loa)
{
	for(int i=0;i<ei.secnum;i++)
	{
		/* only check loadable sections */
		if( (ei.sechdrs[i].sh_flags & SHF_ALLOC) != SHF_ALLOC)
			continue;

		load_section(ei,i,loa,true);
        	for(int j=0;j<=ei.sechdrs[i].sh_size-sizeof(void*);j++)
        	{
                	void* p=*((void**)(ei.sec_data[i]+j));
                	is_string_pointer(p,ei,loa);
        	}

		
	}
	
}

void find_strings(VariantID_t *pidp, FileIR_t* firp)
{

	assert(firp && pidp);

	cout<<"Searching variant for strings"<<*pidp<< "." <<endl;

	/* get a handle to the binary file */
        int elfoid=firp->GetFile()->GetELFOID();
	pqxxDB_t* pqxx_interface=dynamic_cast<pqxxDB_t*>(BaseObj_t::GetInterface());
        pqxx::largeobjectaccess loa(pqxx_interface->GetTransaction(), elfoid, PGSTD::ios::in);


	elf_info_t ei;
	read_elf_info(ei,firp,loa);

	find_strings_in_instructions(firp, ei, loa);
	find_strings_in_data(firp, ei, loa);

	free_elf_info(ei);

	cout << "# ATTRIBUTE filename="<<firp->GetFile()->GetURL()<<endl;
}

main(int argc, char* argv[])
{

	if(argc!=2)
	{
		cerr<<"Usage: ilr <id>"<<endl;
		exit(-1);
	}

	VariantID_t *pidp=NULL;
	FileIR_t *firp=NULL;

	/* setup the interface to the sql server */
	pqxxDB_t pqxx_interface;
	BaseObj_t::SetInterface(&pqxx_interface);

	cout<<"Reading variant "<<string(argv[1])<<" from database." << endl;
	try 
	{

		pidp=new VariantID_t(atoi(argv[1]));
		assert(pidp->IsRegistered()==true);


                for(set<File_t*>::iterator it=pidp->GetFiles().begin();
                        it!=pidp->GetFiles().end();
                        ++it
                    )
                {
                        File_t* this_file=*it;
                        assert(this_file);

			// read the db  
			firp=new FileIR_t(*pidp,this_file);
			
			// do the finding. 
			find_strings(pidp, firp);

//			firp->WriteToDB();

			delete firp;
		}


		pqxx_interface.Commit();

	}
	catch (DatabaseError_t pnide)
	{
		cout<<"Unexpected database error: "<<pnide<<endl;
		exit(-1);
        }

	cout<<"Done!"<<endl;

	delete pidp;
}

