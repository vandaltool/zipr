

#include <libIRDB.hpp>
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


set< pair <int,int>  > bounds;
set<int> targets;

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

void mark_targets(VariantIR_t *virp)
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
void get_instruction_targets(VariantIR_t *virp)
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
			possible_target(disasm.VirtualAddr+instr_len);
		}
		/* other branches can't indicate an indirect branch target */
		else if(disasm.Instruction.BranchType)
			continue;

		/* otherwise, any immediate is a possible branch target */
		possible_target(disasm.Instruction.Immediat);

		handle_argument(&disasm.Argument1);
		handle_argument(&disasm.Argument2);
		handle_argument(&disasm.Argument3);



	}

}

void get_executable_bounds(Elf32_Shdr *shdr, FILE* fp, VariantIR_t *virp)
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

void infer_targets(Elf32_Shdr *shdr, FILE* fp, VariantIR_t *virp)
{
	int flags = shdr->sh_flags;

	if( (flags & SHF_ALLOC) != SHF_ALLOC)
		/* not a loaded section */
		return;

	if( (flags & SHF_EXECINSTR) == SHF_EXECINSTR)
		/* loaded, but contains instruction.  we'll look through the VariantIR for this section. */
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
	int j=1;
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


void fill_in_indtargs(VariantIR_t* virp, string elf_file)
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

	
	cout<<"Targets from data sections are: " << endl;
	print_targets();

	/* look through the instructions in the program for targets */
	get_instruction_targets(virp);

	/* mark the entry point as a target */
	possible_target(elfhdr.e_entry);


	cout<<"All targets from data sections are: " << endl;
	print_targets();

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
	VariantIR_t * virp=NULL;

	try 
	{
		/* setup the interface to the sql server */
		pqxxDB_t pqxx_interface;
		BaseObj_t::SetInterface(&pqxx_interface);

		pidp=new VariantID_t(atoi(argv[1]));

		assert(pidp->IsRegistered()==true);

		cout<<"New Variant, after reading registration, is: "<<*pidp << endl;

		// read the db  
		virp=new VariantIR_t(*pidp);


		fill_in_indtargs(virp,argv[2]);

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
