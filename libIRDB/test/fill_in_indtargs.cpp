

#include <libIRDB-core.hpp>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <assert.h>
#include <sys/mman.h>
// #include <elf.h>
#include <ctype.h>


#include "elfio/elfio.hpp"
#include "elfio/elfio_dump.hpp"

#include "targ-config.h"

#include "beaengine/BeaEngine.h"


#define arch_ptr_bytes() (firp->GetArchitectureBitWidth()/8)

int odd_target_count=0;
int bad_target_count=0;
int bad_fallthrough_count=0;

using namespace libIRDB;
using namespace std;
using namespace ELFIO;

void possible_target(int p);

set< pair <int,int>  > bounds;
set<int> targets;

set< pair< int, int> > ranges;


void range(int start, int end)
{ 	
	pair<int,int> foo(start,end);
	ranges.insert(foo);
}

bool is_in_range(int p)
{
	for(
		set< pair <int,int>  >::iterator it=ranges.begin();
		it!=ranges.end();
		++it
	   )
	{
		pair<int,int> bound=*it;
		int start=bound.first;
		int end=bound.second;
		if(start<=p && p<=end)
			return true;
	}
	return false;
}

void process_ranges(FileIR_t* firp)
{
        for(
                set<Instruction_t*>::const_iterator it=firp->GetInstructions().begin();
                it!=firp->GetInstructions().end();
                ++it
           )
	{
                Instruction_t *insn=*it;
                DISASM disasm;
#if 0
                memset(&disasm, 0, sizeof(DISASM));

                disasm.Options = NasmSyntax + PrefixedNumeral;
                disasm.Archi = 32;
                disasm.EIP = (UIntPtr) insn->GetDataBits().c_str();
                disasm.VirtualAddr = insn->GetAddress()->GetVirtualOffset();
#endif
                int instr_len = insn->Disassemble(disasm);

                assert(instr_len==insn->GetDataBits().size());

                /* calls indicate an indirect target, pc+sizeof(instruction) */
                if(disasm.Instruction.BranchType==CallType)
                {
			if(is_in_range(disasm.VirtualAddr+instr_len))
                        	possible_target(disasm.VirtualAddr+instr_len);
                }
	}
}

void possible_target(int p)
{
	for(
		set< pair <int,int>  >::iterator it=bounds.begin();
		it!=bounds.end();
		++it
	   )
	{
		pair<int,int> bound=*it;
		int start=bound.first;
		int end=bound.second;
		if(start<=p && p<=end)
			targets.insert(p);

	}
}

void handle_argument(ARGTYPE *arg, Instruction_t* insn)
{
	if( (arg->ArgType&MEMORY_TYPE) == MEMORY_TYPE ) 
	{
		if((arg->ArgType&RELATIVE_)==RELATIVE_)
		{
			assert(insn);
			assert(insn->GetAddress());
			possible_target(arg->Memory.Displacement+insn->GetAddress()->GetVirtualOffset()+
				insn->GetDataBits().length());
		}
		else
			possible_target(arg->Memory.Displacement);
	}
}

void mark_targets(FileIR_t *firp)
{
        for(
                set<Instruction_t*>::const_iterator it=firp->GetInstructions().begin();
                it!=firp->GetInstructions().end();
                ++it
           )
        {
		Instruction_t *insn=*it;
		int addr=insn->GetAddress()->GetVirtualOffset();

		/* lookup in the list of targets */
		if(targets.find(addr)!=targets.end())
		{
			AddressID_t* newaddr = new AddressID_t;
			newaddr->SetFileID(insn->GetAddress()->GetFileID());
			newaddr->SetVirtualOffset(insn->GetAddress()->GetVirtualOffset());
			
			insn->SetIndirectBranchTargetAddress(newaddr);
			firp->GetAddresses().insert(newaddr);
		}
	}

}
void get_instruction_targets(FileIR_t *firp)
{
        for(
                set<Instruction_t*>::const_iterator it=firp->GetInstructions().begin();
                it!=firp->GetInstructions().end();
                ++it
           )
        {
                Instruction_t *insn=*it;
                DISASM disasm;
                int instr_len = insn->Disassemble(disasm);

                assert(instr_len==insn->GetDataBits().size());



		/* other branches can't indicate an indirect branch target */
		if(disasm.Instruction.BranchType)
			continue;

		/* otherwise, any immediate is a possible branch target */
		possible_target(disasm.Instruction.Immediat);

		handle_argument(&disasm.Argument1, insn);
		handle_argument(&disasm.Argument2, insn);
		handle_argument(&disasm.Argument3, insn);
	}

}

void get_executable_bounds(FileIR_t *firp, const section* shdr)
{
	int flags = shdr->get_flags();

	/* not a loaded section */
	if( (flags & SHF_ALLOC) != SHF_ALLOC)
		return;

	/* loaded, and contains instruction, record the bounds */
	if( (flags & SHF_EXECINSTR) != SHF_EXECINSTR)
		return;

	int first=shdr->get_address();
	int second=shdr->get_address()+shdr->get_size();

	bounds.insert(pair<int,int>(first,second));


}

void infer_targets(FileIR_t *firp, section* shdr)
{
	int flags = shdr->get_flags();

	if( (flags & SHF_ALLOC) != SHF_ALLOC)
		/* not a loaded section */
		return;

	if( (flags & SHF_EXECINSTR) == SHF_EXECINSTR)
		/* loaded, but contains instruction.  we'll look through the VariantIR for this section. */
		return;

	/* if the type is NOBITS, then there's no actual data to look through */
	if(shdr->get_type()==SHT_NOBITS)
		return;

	const char* data=shdr->get_data() ; // C(char*)malloc(shdr->sh_size);

	assert(arch_ptr_bytes()==4 || arch_ptr_bytes()==8);
	for(int i=0;i+arch_ptr_bytes()<=shdr->get_size();i++)
	{
		int p=*(int*)&data[i];
		possible_target(p);
	}

}


void print_targets()
{
	int j=0;
	for(
		set<int>::iterator it=targets.begin();
		it!=targets.end();
		++it, j++
	   )
	{
		int target=*it;
	
		cout<<std::hex<<target;
		if(j%10 == 0)
			cout<<endl; 
		else
			cout<<", ";
	}

	cout<<endl;
}

/* 
 *
 * add_num_handle_fn_watches - 
 *
 *  This function is a quick and dirty way to ensure that 
 *  certain function call watches are not interfered with by ILR
 *  This is done by marking the functions of interest as indirect targets
 *  so that they receive a spri rule of the form <original_addr> -> <newaddr>
 * 
 *  Current function list:  
 *      fread, fread_unlocked, 
 *      fwrite, fwrite_unlocked, 
 *      strncpy, strncat, strncmp, strxfrm
 *      memcpy, memmove, memcmp, memchr, memrchr, memset
 *      wcsncpy, wcsncat, wcsncmp, wcsxfrm
 *      wmemcpy, wmemmove, wmemcmp, wmemchr, memset
 *
 */
void add_num_handle_fn_watches(FileIR_t * firp)
{
    	/* Loop over the set of functions */
    for(
       	set<Function_t*>::const_iterator it=firp->GetFunctions().begin();
       	it!=firp->GetFunctions().end();
        ++it
        )
    {
        Function_t *func=*it;
        char *funcname=(char *)func->GetName().c_str();
	if(!func->GetEntryPoint())
		continue;
        virtual_offset_t the_offset=func->GetEntryPoint()->GetAddress()->GetVirtualOffset();

        /* macro to facilitate the checking */
#define CHECK_FN(fname)                         \
        if(strcmp(#fname, funcname)==0)         \
        {                                       \
            possible_target(the_offset);        \
        }

        /* 
         * if one that we want to watch, 
         * mark it as a possible target 
         */
        CHECK_FN(fread);
        CHECK_FN(_IO_fread);
        CHECK_FN(fread_unlocked);
        CHECK_FN(fwrite);
        CHECK_FN(_IO_fwrite);
        CHECK_FN(fwrite_unlocked);
        CHECK_FN(strncpy);
        CHECK_FN(strncmp);
        CHECK_FN(strxfrm);
        CHECK_FN(memcpy);
        CHECK_FN(memmove);
        CHECK_FN(memcmp);
        CHECK_FN(memchr);
        CHECK_FN(memrchr);
        CHECK_FN(memset);
        CHECK_FN(wcsncpy);
        CHECK_FN(wcsxfrm);
        CHECK_FN(wmemcpy);
        CHECK_FN(wmemmove);
        CHECK_FN(wmemcmp);
        CHECK_FN(wmemchr);
        CHECK_FN(wmemset);
        
    }

}

void fill_in_indtargs(FileIR_t* firp, elfio* elfiop)
{
	if(getenv("VERBOSE")!=0)
        	for(
                	set<Instruction_t*>::const_iterator it=firp->GetInstructions().begin();
                	it!=firp->GetInstructions().end();
                	++it
           	)
		{
			Instruction_t* insn=*it;
			if(insn->GetIndirectBranchTargetAddress())
				cout<<"Insn at "<<insn->GetAddress()->GetVirtualOffset()<<" already has ibt "<<
					insn->GetIndirectBranchTargetAddress()->GetVirtualOffset()<<endl;
			
		}
	// reset global vars
	bounds.clear();
	ranges.clear();
	targets.clear();

        ::Elf64_Off sec_hdr_off, sec_off;
        ::Elf_Half secnum, strndx, secndx;
        ::Elf_Word secsize;

        /* Read ELF header */
        sec_hdr_off = elfiop->get_sections_offset();
        secnum = elfiop->sections.size();
        strndx = elfiop->get_section_name_str_index();

	/* look through each section and record bounds */
        for (secndx=1; secndx<secnum; secndx++)
		get_executable_bounds(firp, elfiop->sections[secndx]);

	/* look through each section and look for target possibilities */
        for (secndx=1; secndx<secnum; secndx++)
		infer_targets(firp, elfiop->sections[secndx]);

	
	cout<<"========================================="<<endl;
	cout<<"Targets from data sections are: " << endl;
	cout<<"# ATTRIBUTE total_indirect_targets_pass1="<<std::dec<<targets.size()<<endl;
	cout<<"========================================="<<endl;

	/* look through the instructions in the program for targets */
	get_instruction_targets(firp);

	/* mark the entry point as a target */
	possible_target(elfiop->get_entry()); 


	cout<<"========================================="<<endl;
	cout<<"All targets from data+instruction sections are: " << endl;
	cout<<"# ATTRIBUTE total_indirect_targets_pass2="<<std::dec<<targets.size()<<endl;
	cout<<"========================================="<<endl;

	/* Read the exception handler frame so that those indirect branches are accounted for */
	void read_ehframe(FileIR_t* firp, elfio* );
        read_ehframe(firp, elfiop);

	cout<<"========================================="<<endl;
	cout<<"All targets from data+instruction+eh_header sections are: " << endl;
	cout<<"# ATTRIBUTE total_indirect_targets_pass3="<<std::dec<<targets.size()<<endl;
	cout<<"========================================="<<endl;


	/* now process the ranges that have exception handling */
	process_ranges(firp);
	cout<<"========================================="<<endl;
	cout<<"All targets from data+instruction+eh_header sections+eh_header_ranges are: " << endl;
	cout<<"# ATTRIBUTE total_indirect_targets_pass4="<<std::dec<<targets.size()<<endl;
	print_targets();
	cout<<"========================================="<<endl;

	/* now process the ranges that have exception handling */
	void check_for_thunks(FileIR_t* firp);
	check_for_thunks(firp);
	cout<<"========================================="<<endl;
	cout<<"# ATTRIBUTE total_indirect_targets_pass5="<<std::dec<<targets.size()<<endl;
	print_targets();
	cout<<"========================================="<<endl;






    	/* Add functions containing unsigned int params to the list */
    	add_num_handle_fn_watches(firp);
	/* now process the ranges that have exception handling */
	cout<<"========================================="<<endl;
	cout<<"# ATTRIBUTE total_indirect_targets_pass6="<<std::dec<<targets.size()<<endl;
	print_targets();
	cout<<"========================================="<<endl;




	/* set the IR to have some instructions marked as IB targets */
	mark_targets(firp);
}





main(int argc, char* argv[])
{

	if(argc!=2)
	{
		cerr<<"Usage: fill_in_indtargs <id>"<<endl;
		exit(-1);
	}




	VariantID_t *pidp=NULL;
	FileIR_t * firp=NULL;

	try 
	{
		/* setup the interface to the sql server */
		pqxxDB_t pqxx_interface;
		BaseObj_t::SetInterface(&pqxx_interface);

		pidp=new VariantID_t(atoi(argv[1]));

		assert(pidp->IsRegistered()==true);

		cout<<"New Variant, after reading registration, is: "<<*pidp << endl;

                for(set<File_t*>::iterator it=pidp->GetFiles().begin();
                        it!=pidp->GetFiles().end();
                        ++it
                    )
                {
                        File_t* this_file=*it;
                        assert(this_file);

			// read the db  
			firp=new FileIR_t(*pidp, this_file);

			int elfoid=firp->GetFile()->GetELFOID();
		        pqxx::largeobject lo(elfoid);
        		lo.to_file(pqxx_interface.GetTransaction(),"readeh_tmp_file.exe");

        		ELFIO::elfio*    elfiop=new ELFIO::elfio;
        		elfiop->load("readeh_tmp_file.exe");
		
        		ELFIO::dump::header(cout,*elfiop);
        		ELFIO::dump::section_headers(cout,*elfiop);


			// find all indirect branch targets
			fill_in_indtargs(firp, elfiop);
	
			// write the DB back and commit our changes 

			firp->WriteToDB();
			delete firp;
		}

		pqxx_interface.Commit();

	}
	catch (DatabaseError_t pnide)
	{
		cout<<"Unexpected database error: "<<pnide<<endl;
		exit(-1);
        }

	assert(firp && pidp);


	delete pidp;
}
