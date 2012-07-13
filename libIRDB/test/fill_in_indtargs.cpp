

#include <libIRDB-core.hpp>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <assert.h>
#include <elf.h>
#include <sys/mman.h>
#include <ctype.h>


#include "beaengine/BeaEngine.h"

int odd_target_count=0;
int bad_target_count=0;
int bad_fallthrough_count=0;

using namespace libIRDB;
using namespace std;

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

void process_ranges(FileIR_t* virp)
{
        for(
                set<Instruction_t*>::const_iterator it=virp->GetInstructions().begin();
                it!=virp->GetInstructions().end();
                ++it
           )
	{
                Instruction_t *insn=*it;
                DISASM disasm;
                memset(&disasm, 0, sizeof(DISASM));

                disasm.Options = NasmSyntax + PrefixedNumeral;
                disasm.Archi = 32;
                disasm.EIP = (UIntPtr) insn->GetDataBits().c_str();
                disasm.VirtualAddr = insn->GetAddress()->GetVirtualOffset();
                int instr_len = Disasm(&disasm);

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

void handle_argument(ARGTYPE *arg)
{
	if( arg->ArgType == MEMORY_TYPE ) 
		possible_target(arg->Memory.Displacement);
}

void mark_targets(FileIR_t *virp)
{
        for(
                set<Instruction_t*>::const_iterator it=virp->GetInstructions().begin();
                it!=virp->GetInstructions().end();
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
			virp->GetAddresses().insert(newaddr);
		}
		else
			insn->SetIndirectBranchTargetAddress(NULL);
		
	}

}
void get_instruction_targets(FileIR_t *virp)
{
        for(
                set<Instruction_t*>::const_iterator it=virp->GetInstructions().begin();
                it!=virp->GetInstructions().end();
                ++it
           )
        {
                Instruction_t *insn=*it;
                DISASM disasm;
                memset(&disasm, 0, sizeof(DISASM));

                disasm.Options = NasmSyntax + PrefixedNumeral;
                disasm.Archi = 32;
                disasm.EIP = (UIntPtr) insn->GetDataBits().c_str();
                disasm.VirtualAddr = insn->GetAddress()->GetVirtualOffset();
                int instr_len = Disasm(&disasm);

                assert(instr_len==insn->GetDataBits().size());



/* we are moving the marking of callsite indirects into fix calls
 * which has a better notion of whether the callsite indicates an 
 * indirect branch target at the next instruction 
 */
#if 0
		/* calls indicate an indirect target, pc+sizeof(instruction) */
		if(disasm.Instruction.BranchType==CallType)
		{
			possible_target(disasm.VirtualAddr+instr_len);
		}
		else 
#endif
		/* other branches can't indicate an indirect branch target */
		if(disasm.Instruction.BranchType)
			continue;

		/* otherwise, any immediate is a possible branch target */
		possible_target(disasm.Instruction.Immediat);

		handle_argument(&disasm.Argument1);
		handle_argument(&disasm.Argument2);
		handle_argument(&disasm.Argument3);



	}

}

void get_executable_bounds(Elf32_Shdr *shdr, FILE* fp, FileIR_t *virp)
{
	int flags = shdr->sh_flags;

	/* not a loaded section */
	if( (flags & SHF_ALLOC) != SHF_ALLOC)
		return;

	/* loaded, and contains instruction, record the bounds */
	if( (flags & SHF_EXECINSTR) != SHF_EXECINSTR)
		return;

	int first=shdr->sh_addr;
	int second=shdr->sh_addr+shdr->sh_size;

	bounds.insert(pair<int,int>(first,second));


}

void infer_targets(Elf32_Shdr *shdr, FILE* fp, FileIR_t *virp)
{
	int flags = shdr->sh_flags;

	if( (flags & SHF_ALLOC) != SHF_ALLOC)
		/* not a loaded section */
		return;

	if( (flags & SHF_EXECINSTR) == SHF_EXECINSTR)
		/* loaded, but contains instruction.  we'll look through the VariantIR for this section. */
		return;

	/* if the type is NOBITS, then there's no actual data to look through */
	if(shdr->sh_type==SHT_NOBITS)
		return;

	char* data=(char*)malloc(shdr->sh_size);

	fseek(fp,shdr->sh_offset, SEEK_SET);

	int res=fread(data, shdr->sh_size, 1, fp);
	assert(res==1);

	for(int i=0;i<=shdr->sh_size-sizeof(void*);i++)
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
void add_num_handle_fn_watches(FileIR_t * virp)
{
    /* Loop over the set of functions */
    for(
        set<Function_t*>::const_iterator it=virp->GetFunctions().begin();
            it!=virp->GetFunctions().end();
            ++it
        )
    {
        Function_t *func=*it;
        char *funcname=(char *)func->GetName().c_str();
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

void fill_in_indtargs(FileIR_t* virp, string elf_file, pqxxDB_t &pqxx_interface)
{
        Elf32_Off sec_hdr_off, sec_off;
        Elf32_Half secnum, strndx, secndx;
        Elf32_Word secsize;
        FILE *fp;

        fp = fopen(elf_file.c_str(),"rb");

	if(!fp)
	{
		cerr<<"Cannot open "<<elf_file<<"."<<endl;
		exit(-1);
	}

	/* allcoate memory  */
        Elf32_Ehdr elfhdr;

        /* Read ELF header */
        int res=fread(&elfhdr, sizeof(Elf32_Ehdr), 1, fp);
        assert(res==1);
        sec_hdr_off = elfhdr.e_shoff;
        secnum = elfhdr.e_shnum;
        strndx = elfhdr.e_shstrndx;

        /* Read Section headers */
        Elf32_Shdr *sechdrs=(Elf32_Shdr*)malloc(sizeof(Elf32_Shdr)*secnum);
        fseek(fp, sec_hdr_off, SEEK_SET);
        res=fread(sechdrs, sizeof(Elf32_Shdr), secnum, fp);
        assert(res==secnum);

	/* look through each section and record bounds */
        for (secndx=1; secndx<secnum; secndx++)
		get_executable_bounds(&sechdrs[secndx], fp, virp);

	/* look through each section and look for target possibilities */
        for (secndx=1; secndx<secnum; secndx++)
		infer_targets(&sechdrs[secndx], fp, virp);

	
	cout<<"========================================="<<endl;
	cout<<"Targets from data sections are: " << endl;
	cout<<"# ATTRIBUTE total_indirect_targets_pass1="<<std::dec<<targets.size()<<endl;
//	print_targets();
	cout<<"========================================="<<endl;

	/* look through the instructions in the program for targets */
	get_instruction_targets(virp);

	/* mark the entry point as a target */
	possible_target(elfhdr.e_entry);


	cout<<"========================================="<<endl;
	cout<<"All targets from data+instruction sections are: " << endl;
	cout<<"# ATTRIBUTE total_indirect_targets_pass2="<<std::dec<<targets.size()<<endl;
//	print_targets();
	cout<<"========================================="<<endl;

	/* Read the exception handler frame so that those indirect branches are accounted for */
	void read_ehframe(FileIR_t* virp, pqxxDB_t& pqxx_interface);
        read_ehframe(virp, pqxx_interface);

	cout<<"========================================="<<endl;
	cout<<"All targets from data+instruction+eh_header sections are: " << endl;
	cout<<"# ATTRIBUTE total_indirect_targets_pass3="<<std::dec<<targets.size()<<endl;
//	print_targets();
	cout<<"========================================="<<endl;


	/* now process the ranges that have exception handling */
	process_ranges(virp);
	cout<<"========================================="<<endl;
	cout<<"All targets from data+instruction+eh_header sections+eh_header_ranges are: " << endl;
	cout<<"# ATTRIBUTE total_indirect_targets_pass4="<<std::dec<<targets.size()<<endl;
	print_targets();
	cout<<"========================================="<<endl;

    /* Add functions containing unsigned int params to the list */
    add_num_handle_fn_watches(virp);

	/* set the IR to have some instructions marked as IB targets */
	mark_targets(virp);
	
}





main(int argc, char* argv[])
{

	if(argc!=3)
	{
		cerr<<"Usage: fill_in_indtargs <id> <elffile>"<<endl;
		exit(-1);
	}




	VariantID_t *pidp=NULL;
	FileIR_t * virp=NULL;

	try 
	{
		/* setup the interface to the sql server */
		pqxxDB_t pqxx_interface;
		BaseObj_t::SetInterface(&pqxx_interface);

		pidp=new VariantID_t(atoi(argv[1]));

		assert(pidp->IsRegistered()==true);

		cout<<"New Variant, after reading registration, is: "<<*pidp << endl;

		// read the db  
		virp=new FileIR_t(*pidp);

		// find all indirect branch targets
		fill_in_indtargs(virp,argv[2], pqxx_interface);

		// write the DB back and commit our changes 
		virp->WriteToDB();
		pqxx_interface.Commit();

	}
	catch (DatabaseError_t pnide)
	{
		cout<<"Unexpected database error: "<<pnide<<endl;
		exit(-1);
        }

	assert(virp && pidp);


	delete pidp;
	delete virp;
}
