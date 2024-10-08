/*
 * Copyright (c) 2014 - Zephyr Software LLC
 *
 * This file may be used and modified for non-commercial purposes as long as
 * all copyright, permission, and nonwarranty notices are preserved.
 * Redistribution is prohibited without prior written consent from Zephyr
 * Software.
 *
 * Please contact the authors for restrictions applying to commercial use.
 *
 * THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: Zephyr Software
 * e-mail: jwd@zephyr-software.com
 * URL   : http://www.zephyr-software.com/
 *
 */



#include <libIRDB-core.hpp>
#include <irdb-cfg.hpp>
#include <iostream>
#include <stdlib.h>
#include <cctype>
#include <assert.h>
#include <stdlib.h>


#include <exeio.h>

//#include <elf.h>
#include "targ-config.h"
#include "elfio/elfio.hpp"
//#include "elfio/elfio_dump.hpp"

// elfio doesn't have this, and elf.h does, but including both is troublesome.
#ifndef SHT_GNU_HASH
#define SHT_GNU_HASH	  0x6ffffff6
#endif



using namespace libIRDB;
using namespace std;
using namespace EXEIO;

#define arch_ptr_bytes() (uintptr_t)(firp->GetArchitectureBitWidth()/8)

bool is_string_character(char c)
{
	if(c=='\n') return true;
	return isprint(c);
}


/* the stuff we need for reading an elf file */
typedef struct elf_info {
#if 0
       	::Elf64_Off sec_hdr_off, sec_off;
       	::Elf_Half secnum, strndx;
       	::Elf_Word secsize;
	::Elf64_Addr got;
#endif
	int secnum;
	VirtualOffset_t got;
	char const **sec_data;
	exeio *elfiop;
} elf_info_t;

void found_string(string s, void* addr)
{
	char *buff=(char*)malloc(s.length()+2);
	char *old_p=buff, *p;
	// use .data() instead of c_str(); can find multiple C-strings in one string
	memcpy(buff,s.data(),s.length());
	buff[s.length()]=0;

	do {
		// look for new lines in the string
		// if found, split it up and print out each one as it's own thing.
		while( ( p=strchr(old_p,'\n')) )
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

void load_section(elf_info_t &ei, int i, bool alloc)
{
	if( alloc && !ei.elfiop->sections[i]->isLoadable()) // (ei.elfiop->sections[i]->get_flags() & SHF_ALLOC) != SHF_ALLOC)
	{
		cerr<<"Cannot load non-alloc section\n";
		assert(0);
	}

	if(ei.sec_data[i]==NULL)
	{
		ei.sec_data[i]=ei.elfiop->sections[i]->get_data(); 
		// if(ei.elfiop->sections[i]->get_type()==SHT_NOBITS)
		if(ei.elfiop->sections[i]->isBSS())
		{
			/* no need to read anything for NOBITS sections */
			ei.sec_data[i]=(char*)calloc(ei.elfiop->sections[i]->get_size(),1);
		}
	}
}

void check_for_string(const char* p, void* addr)
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

void is_string_pointer(void* addr, elf_info_t &ei)
{
	auto intaddr=(uintptr_t)(addr);

	for(int i=0;i<ei.secnum;i++)
	{
//cout << "is_string_pointer(): address: " << std::hex << intaddr << std::dec << "looking at section number: " << i << endl;
		/* only look at loaded sections */
		// if( (ei.elfiop->sections[i]->get_flags() & SHF_ALLOC) != SHF_ALLOC)
		if( !ei.elfiop->sections[i]->isLoadable())
			continue;

		if(ei.elfiop->sections[i]->get_address() <= intaddr 
			&& intaddr <= (ei.elfiop->sections[i]->get_address()+ei.elfiop->sections[i]->get_size()))
		{
			/* we found a pointer into a loadable segment */
			load_section(ei,i,true);

			check_for_string(ei.sec_data[i]+((long long int)addr-ei.elfiop->sections[i]->get_address()),addr);
		}
	}

}

void is_string_constant(const DecodedInstruction_t& disasm)
{
	void *addr;

	//if(disasm.Argument1.ArgType != MEMORY_TYPE || disasm.Argument2.ArgType == MEMORY_TYPE
	//   || disasm.Argument1.ArgSize != FileIR_t::GetArchitectureBitWidth() )
	if(!disasm.getOperand(0).isMemory() || 
	   disasm.getOperand(1).isMemory() || 
	   (uintptr_t)disasm.getOperand(0).getArgumentSizeInBits() != (uintptr_t)FileIR_t::GetArchitectureBitWidth() )
		return;

	// addr = (void*)disasm.Instruction.Immediat;
	addr = (void*)disasm.getImmediate();

	/* consider that this constant itself may be a string */
	unsigned char byte1=(((long long unsigned int)addr)>>24)&0xff;
	unsigned char byte2=(((long long unsigned int)addr)>>16)&0xff;
	unsigned char byte3=(((long long unsigned int)addr)>>8)&0xff;
	unsigned char byte4=(long long unsigned int)addr&0xff;
	
        /*  
               mov reg, 0x6161 
                       addr       = 0x00006161
                       addr >> 24 = 0          byte1
                       addr >> 16 = 0          byte2
                       addr >> 8 = 61          byte3
                       addr >> 0 = 61          byte4
               mov reg, 0x61616161 

               mov reg, 0x0000000061616161 
        */
// cout << "address: " << std::hex << addr << std::dec << byte1 << " " << byte2 << " " << byte3 << " " << byte4 << endl;
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

void handle_argument(const DecodedOperand_t &arg, elf_info_t &ei, Instruction_t *insn)
{
        // if( (arg->ArgType & MEMORY_TYPE) == MEMORY_TYPE )
        if(arg.isMemory())
	{
		/* Only check without GOT offset if type is executable */
	
       		// if( ((arg->ArgType & ABSOLUTE_) == ABSOLUTE_)  && !ei.elfiop->isDLL() )
       		if( !arg.isPcrel()  && !ei.elfiop->isDLL() )
			//  && ei.elfiop->get_type() == ET_EXEC ) -- checks for .so/.dll vrs .exe.
			// is_string_pointer((void*)arg->Memory.Displacement,ei);
			is_string_pointer((void*)arg.getMemoryDisplacement(),ei);
		else
			//is_string_pointer((void*)(arg->Memory.Displacement + insn->GetDataBits().size()), ei);
			is_string_pointer((void*)(arg.getMemoryDisplacement() + insn->GetDataBits().size()), ei);

		/* Check with GOT offset if present */
		// if ( ei.got && arg->Memory.BaseRegister == REG3 /* ebx */ )
		if ( ei.got && arg.hasBaseRegister() && arg.getBaseRegister() == 3 /* ebx */ )
			// is_string_pointer((void*)(arg->Memory.Displacement + ei.got),ei);
			is_string_pointer((void*)(arg.getMemoryDisplacement() + ei.got),ei);
	}
}


void read_elf_info(elf_info_t &ei, FileIR_t* firp)
{


        /* Read ELF header */
//        ei.sec_hdr_off = ei.elfiop->get_sections_offset();
        ei.secnum = ei.elfiop->sections.size();
        assert(ei.secnum>0);
//        ei.strndx = ei.elfiop->get_section_name_str_index();

	ei.sec_data=(char const**)calloc(ei.secnum,sizeof(void*));

	ei.got = 0;
	/* Get .got or .got.plt address, if any */
//	if (ei.strndx != SHN_UNDEF)
//	{
//		int shstr_sec;
//		if (ei.strndx < SHN_LORESERVE)
//			shstr_sec = ei.strndx;
//		else
//			shstr_sec = ei.elfiop->sections[0]->get_link();
//		assert(shstr_sec < ei.secnum);
//		load_section(ei,shstr_sec,false);
//		IRDB_Elf_Shdr *shstr_sec_hdr = ei.sechdrs + shstr_sec;
		for (int i=0;i<ei.secnum;i++)
		{
// name works oddly here.  we can just get the name safely using elfio
//			assert(ei.sechdrs[i].sh_name < shstr_sec_hdr->sh_size);
			if (ei.elfiop->sections[i]->get_name()==".got.plt") // !strcmp(ei.sec_data[shstr_sec]+ei.sechdrs[i].sh_name, ".got.plt"))
			{
				// Prefer .got.plt to .got
				ei.got = ei.elfiop->sections[i]->get_address();
				break;
			}
			if (ei.elfiop->sections[i]->get_name()==".got") // if (!strcmp(ei.sec_data[shstr_sec]+ei.sechdrs[i].sh_name, ".got"))
				ei.got = ei.elfiop->sections[i]->get_address();
		}
//	}
}

void free_elf_info(elf_info_t &ei)
{
	#define FREE_IF_NOT_NULL(a) { if(a) { free(a); a=NULL; } }
	FREE_IF_NOT_NULL(ei.sec_data);
}

void find_strings_in_instructions(FileIR_t* firp, elf_info_t& ei)
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
		if(!(*fit)->GetEntryPoint())
			continue;
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
				//DISASM disasm;
				char *str = NULL;

				//int res=Disassemble(insn,disasm);
				auto disasm=DecodedInstruction_t(insn);
				int res=disasm.length();
				assert(res);

				// Concatenate printable strings from consecutive store immediates to SP-relative stack addresses
				size_t size = 0;
				unsigned int olddisp = 0;
				unsigned int newdisp = 0;
				unsigned int basereg = 0;
				while(iit!=(*bit)->GetInstructions().end())
				{
//					cout<<"Pass 1: Checking insn: "<<disasm.CompleteInstr<<" id: "<<(*iit)->GetBaseID()<<" category: " << (int) (disasm.Instruction.Category & 0xFFFF0000) << " ibta: " << (*iit)->GetIndirectBranchTargetAddress() << endl;

					// Break if not assignment of an immediate to an esp/ebp/eax offset
//					if (disasm.Argument1.ArgType != MEMORY_TYPE
//					    || (disasm.Argument1.Memory.BaseRegister != REG4 /* esp */
//					        && disasm.Argument1.Memory.BaseRegister != REG5 /* ebp */
//					        && disasm.Argument1.Memory.BaseRegister != REG0 /* eax */)
//					    || (basereg && disasm.Argument1.Memory.BaseRegister != basereg)
//					    || disasm.Argument2.ArgType == MEMORY_TYPE
//					    || ((disasm.Instruction.Category & 0XFFFF0000) != GENERAL_PURPOSE_INSTRUCTION)) 
					if (!disasm.getOperand(0).isMemory()
					    || !disasm.getOperand(0).hasBaseRegister()
					    || (disasm.getOperand(0).getBaseRegister() != 4 /* esp */
					        && disasm.getOperand(0).getBaseRegister() != 5 /* ebp */
					        && disasm.getOperand(0).getBaseRegister() != 0 /* eax */)
					    || (basereg && disasm.getOperand(0).getBaseRegister() != basereg)
					    || disasm.getOperand(1).isMemory()
					   )
					{
						// mark visited
						visited_insns.insert(*iit);
						is_string_constant(disasm);
						break;
					}
					// basereg = disasm.Argument1.Memory.BaseRegister;
					basereg = disasm.getOperand(0).getBaseRegister();
					unsigned int disp = disasm.getOperand(0).getMemoryDisplacement();
					// break if displacement moved backward
					if (newdisp && (disp < newdisp || disp == olddisp))
						break;
					// mark visited
					visited_insns.insert(*iit);
					// check for a printable argument
					// unsigned int imm = disasm.Instruction.Immediat;
					unsigned int imm = disasm.getImmediate();
					unsigned char byte1=(imm>>24)&0xff;
					unsigned char byte2=(imm>>16)&0xff;
					unsigned char byte3=(imm>>8)&0xff;
					unsigned char byte4=imm&0xff;
					// size_t argsize = disasm.Argument1.ArgSize / 8;
					size_t argsize = disasm.getOperand(0).getArgumentSizeInBytes(); // .ArgSize / 8;

					if ( imm!=0 /* special case 0 which is likely from push <reg> insns, etc. */ && 
					    (((is_string_character(byte1) || byte1==0) || argsize < 4) &&
					    ((is_string_character(byte2) || byte2==0) || argsize < 4) &&
					    ((is_string_character(byte3) || byte3==0) || argsize < 2) &&
					    (is_string_character(byte4) || byte4==0)))
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
					{
						break;
					}
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
					// res=Disassemble(insn,disasm);
					disasm=DecodedInstruction_t(insn);
					res=disasm.length();
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

                //DISASM disasm;
		//int res=Disassemble(insn,disasm);
		const auto disasm=DecodedInstruction_t(insn);
		int res=disasm.length();
		assert(res);
//	cout<<"Pass 2: Checking insn: "<<disasm.CompleteInstr<<" id: "<<insn->GetBaseID()<<endl;

		// check for immediate string pointers in non-PIC code
		if ( !ei.elfiop->isDLL()) // ei.elfiop->get_type() == ET_EXEC )
			// is_string_pointer((void*)disasm.Instruction.Immediat,ei);
			is_string_pointer((void*)disasm.getImmediate(),ei);
		// always check for string pointers in memory argument displacements

		handle_argument(disasm.getOperand(0),ei, insn);
		handle_argument(disasm.getOperand(1),ei, insn);
		handle_argument(disasm.getOperand(2),ei, insn);

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


void find_strings_in_data(FileIR_t* firp, elf_info_t& ei)
{
	ELFIO::elfio *the_elfiop=reinterpret_cast<ELFIO::elfio *>(ei.elfiop->get_elfio());
	if(!the_elfiop)
		return;
	for(int i=0;i<ei.secnum;i++)
	{
		/* skip executable, hash, string table, nonloadable, and tiny sections */
		if( (the_elfiop->sections[i]->get_flags() & SHF_EXECINSTR)
		    || the_elfiop->sections[i]->get_type() == SHT_HASH
		    || the_elfiop->sections[i]->get_type() == SHT_GNU_HASH
		    || the_elfiop->sections[i]->get_type() == SHT_STRTAB
		    || (the_elfiop->sections[i]->get_flags() & SHF_ALLOC) != SHF_ALLOC
		    || the_elfiop->sections[i]->get_size() < arch_ptr_bytes())
			continue;

		auto offset = 0U;
		auto step = 0U;
		/* step over relocation info */
		switch( the_elfiop->sections[i]->get_type() )
		{
			case SHT_REL:
				if(arch_ptr_bytes()==4)
					step = sizeof(ELFIO::Elf32_Rel);
				else
					step = sizeof(ELFIO::Elf64_Rel);
				break;
			case SHT_RELA:
				if(arch_ptr_bytes()==4)
					step = sizeof(ELFIO::Elf32_Rela);
				else
					step = sizeof(ELFIO::Elf64_Rela);
				break;
			case SHT_SYMTAB:
			case SHT_DYNSYM:
				if(arch_ptr_bytes()==4)
				{
					offset = sizeof(ELFIO::Elf32_Word);
					step = sizeof(ELFIO::Elf32_Sym);
				}
				else
				{
					offset = sizeof(ELFIO::Elf64_Word);
					step = sizeof(ELFIO::Elf64_Sym);
				}
				break;
			default:
				step = 1;
		}

		load_section(ei,i,true);
        	for(auto j=offset;(uintptr_t)(j+arch_ptr_bytes())<=(uintptr_t)(ei.elfiop->sections[i]->get_size());j+=step)
        	{
			void* p;
			if(arch_ptr_bytes()==4)
                		p=(void*)(uintptr_t)*((int*)(ei.sec_data[i]+j));
			else
                		p=*((void**)(ei.sec_data[i]+j));
                	is_string_pointer(p,ei);
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

	pqxx::largeobject lo(elfoid);
	lo.to_file(pqxx_interface->GetTransaction(),"readeh_tmp_file.exe");
	EXEIO::exeio    elfiop;
	elfiop.load(string("readeh_tmp_file.exe"));
	//EXEIO::dump::header(cout,elfiop);
	//EXEIO::dump::section_headers(cout,elfiop);




	elf_info_t ei;
	ei.elfiop=&elfiop;
	read_elf_info(ei,firp);

	find_strings_in_instructions(firp, ei);
	find_strings_in_data(firp, ei);

	free_elf_info(ei);

	cout << "# ATTRIBUTE find_strings::filename="<<firp->GetFile()->GetURL()<<endl;
}

int main(int argc, char* argv[])
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
	return 0;
}

