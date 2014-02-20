

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
#include "check_thunks.hpp"


#define arch_ptr_bytes() (firp->GetArchitectureBitWidth()/8)

int odd_target_count=0;
int bad_target_count=0;
int bad_fallthrough_count=0;

using namespace libIRDB;
using namespace std;
using namespace ELFIO;

// bool possible_target(int p, uintptr_t addr=0);
bool is_possible_target(int p, uintptr_t addr);

set< pair <int,int>  > bounds;
set<int> targets;

set< pair< int, int> > ranges;

// a way to map an instruction to it's set of predecessors. 
map< Instruction_t* , set<Instruction_t*> > preds;


void check_for_PIC_switch_table32(Instruction_t* insn, DISASM disasm, ELFIO::elfio* elfiop, const set<int>& thunk_bases);
void check_for_PIC_switch_table64(Instruction_t* insn, DISASM disasm, ELFIO::elfio* elfiop);

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

bool possible_target(int p, uintptr_t addr)
{
	if(is_possible_target(p,addr))
	{
		if(addr!=0 && getenv("IB_VERBOSE")!=NULL)
		{
			cout<<"Found address 0x"<<std::hex<<p<<" at 0x"<<addr<<std::dec<<endl;
		}
		targets.insert(p);
		return true;
	}
	return false;
}

bool is_possible_target(int p, uintptr_t addr)
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
		{
			return true;
		}
        }
	return false;

}

ELFIO::section*  find_section(int addr, ELFIO::elfio *elfiop)
{
         for ( int i = 0; i < elfiop->sections.size(); ++i )
         {   
                 ELFIO::section* pSec = elfiop->sections[i];
                 assert(pSec);
                 if(pSec->get_address() > addr)
                         continue;
                 if(addr > pSec->get_address()+pSec->get_size())
                         continue;

                 return pSec;
	}
	return NULL;
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
void get_instruction_targets(FileIR_t *firp, ELFIO::elfio* elfiop, const set<int>& thunk_bases)
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

		check_for_PIC_switch_table32(insn,disasm, elfiop, thunk_bases);
		check_for_PIC_switch_table64(insn,disasm, elfiop);

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
		// even on 64-bit, pointers might be stored as 32-bit, as a 
		// elf object has the 32-bit limitations.
		// there's no real reason to look for 64-bit pointers 
		int p=*(int*)&data[i];
		possible_target(p, i+shdr->get_address());
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

bool backup_until(const char* insn_type, Instruction_t *& prev, Instruction_t* orig)
{
	DISASM disasm;
	prev=orig;

	while(preds[prev].size()==1)
	{
        	// get the only item in the list.
		prev=*(preds[prev].begin());

        	// get I7's disassembly
        	prev->Disassemble(disasm);

        	// check it's the requested type
        	if(strstr(disasm.Instruction.Mnemonic, insn_type)!=NULL)
                	return true;

		// otherwise, try backing up again.

	}
	return false;
}


/*
 * check_for_PIC_switch_table32 - look for switch tables in PIC code for 32-bit code.
 */
void check_for_PIC_switch_table32(Instruction_t* insn, DISASM disasm, ELFIO::elfio* elfiop, const set<int> &thunk_bases)
{
#if 0

/* here's typical code */

I1: 080a9037 <gedit_floating_slider_get_type+0x607> call   0806938e <_gedit_app_ready+0x8e>  	// ebx=080a903c
I2: 080a903c <gedit_floating_slider_get_type+0x60c> add    $0x45fb8,%ebx			// ebx=<module_start>
...
I3: 080a90f8 <gedit_floating_slider_get_type+0x6c8> mov    -0x1ac14(%ebx,%esi,4),%eax		// table_start=<module_start-0x1ac14
												// table_offset=eax=table_start[esi]
I4: 080a90ff <gedit_floating_slider_get_type+0x6cf> add    %ebx,%eax				// switch_case=eax=<module_start>+table_offset
I5: 080a9101 <gedit_floating_slider_get_type+0x6d1> jmp    *%eax				// jump switch_case
...
I6: 0806938e <_gedit_app_ready+0x8e> mov    (%esp),%ebx
I7: 08069391 <_gedit_app_ready+0x91> ret


/* However, since the thunk and the switch table can be (and sometimes are) very control-flow distinct, 
 * we just collect all the places where a module can start by examining all the thunk/add pairs.
 * After that, we look for all jumps that match the I3-I5 pattern, and consider the offset against each
 * module start.  If we find that there are possible targets at <module_start>+table_offset, we record them.
 */

#endif

        Instruction_t* I5=insn;
        Instruction_t* I4=NULL;
        Instruction_t* I3=NULL;
        // check if I5 is a jump
        if(strstr(disasm.Instruction.Mnemonic, "jmp")==NULL)
		return;

	// return if it's a jump to a constant address, these are common
        if(disasm.Argument1.ArgType&CONSTANT_TYPE)
		return;

	// return if it's a jump to a memory address
        if(disasm.Argument1.ArgType&MEMORY_TYPE)
		return;

	// has to be a jump to a register now

	// backup and find the instruction that's an add before I8 
	if(!backup_until("add", I4, I5))
		return;

	// backup and find the instruction that's an movsxd before I7
	if(!backup_until("mov", I3, I4))
		return;

	// grab the offset out of the lea.
	DISASM d2;
	I3->Disassemble(d2);

	// get the offset from the thunk
	int table_offset=d2.Argument2.Memory.Displacement;
	if(table_offset==0)
		return;

cout<<hex<<"Found switch dispatch at "<<I3->GetAddress()->GetVirtualOffset()<< " with table_offset="<<table_offset<<endl;
		
	/* iterate over all thunk_bases/module_starts */
	for(set<int>::iterator it=thunk_bases.begin(); it!=thunk_bases.end(); ++it)
	{
		int thunk_base=*it;
		int table_base=*it+table_offset;

		// find the section with the data table
        	ELFIO::section *pSec=find_section(table_base,elfiop);
		if(!pSec)
			continue;

		// if the section has no data, abort 
        	const char* secdata=pSec->get_data();
		if(!secdata)
			continue;

		// get the base offset into the section
        	int offset=table_base-pSec->get_address();
		int i;
		for(i=0;i<3;i++)
		{
                	if(offset+i*4+sizeof(int) > pSec->get_size())
                        	break;

                	const int *table_entry_ptr=(const int*)&(secdata[offset+i*4]);
                	int table_entry=*table_entry_ptr;

			if(!is_possible_target(thunk_base+table_entry,table_base+i*4))
				break;	
		}
		/* did we finish the loop or break out? */
		if(i==3)
		{
			cout<<"Found switch table (thunk-relative) at "<<hex<<table_base+table_offset<<endl;
			// finished the loop.
			for(i=0;true;i++)
			{
                		if(offset+i*4+sizeof(int) > pSec->get_size())
                        		break;
	
                		const int *table_entry_ptr=(const int*)&(secdata[offset+i*4]);
                		int table_entry=*table_entry_ptr;
	
				cout<<"Found switch table (thunk-relative) entry["<<dec<<i<<"], "<<hex<<thunk_base+table_entry<<endl;
				if(!possible_target(thunk_base+table_entry,table_base+i*4))
					break;
			}
		}
		else
			cout<<"Found that  "<<hex<<table_base+table_offset<<endl;

		// now, try next thunk base 
	}


}


/* check if this instruction is an indirect jump via a register,
 * if so, see if we can trace back a few instructions to find a
 * the start of the table.
 */
void check_for_PIC_switch_table64(Instruction_t* insn, DISASM disasm, ELFIO::elfio* elfiop)
{

        /* here's the pattern we're looking for */
#if 0
I1:   0x000000000044425a <+218>:        cmp    DWORD PTR [rax+0x8],0xd   // bounds checking code, 0xd cases.
I2:   0x000000000044425e <+222>:        jbe    0x444320 <_gedit_tab_get_icon+416>

<snip>
I3:   0x0000000000444264 <+228>:        mov    rdi,rbp // default case, also jumped to via indirect branch below
<snip>
I4:   0x0000000000444320 <+416>:        mov    edx,DWORD PTR [rax+0x8]
I5:   0x0000000000444323 <+419>:        lea    rax,[rip+0x3e1b6]        # 0x4824e0
I6:   0x000000000044432a <+426>:        movsxd rdx,DWORD PTR [rax+rdx*4]
I7:   0x000000000044432e <+430>:        add    rax,rdx
I8:   0x0000000000444331 <+433>:        jmp    rax      // relatively standard switch dispatch code


D1:   0x4824e0: .long 0x4824e0-L1       // L1-LN are labels in the code where case statements start.
D2:   0x4824e0: .long 0x4824e0-L2
..
DN:   0x4824e0: .long 0x4824e0-LN
#endif


        // for now, only trying to find I4-I8.  ideally finding I1 would let us know the size of the
        // jump table.  We'll figure out N by trying targets until they fail to produce something valid.

        Instruction_t* I8=insn;
        Instruction_t* I7=NULL;
        Instruction_t* I6=NULL;
        Instruction_t* I5=NULL;
        // check if I8 is a jump
        if(strstr(disasm.Instruction.Mnemonic, "jmp")==NULL)
                return;

	// return if it's a jump to a constant address, these are common
        if(disasm.Argument1.ArgType&CONSTANT_TYPE)
		return;
	// return if it's a jump to a memory address
        if(disasm.Argument1.ArgType&MEMORY_TYPE)
		return;

	// has to be a jump to a register now

	// backup and find the instruction that's an add before I8 
	if(!backup_until("add", I7, I8))
		return;

	// backup and find the instruction that's an movsxd before I7
	if(!backup_until("movsxd", I6, I7))
		return;

	// backup and find the instruction that's an lea before I6
	if(!backup_until("lea", I5, I6))
		return;

	I5->Disassemble(disasm);

        if(!(disasm.Argument2.ArgType&MEMORY_TYPE))
                return;
        if(!(disasm.Argument2.ArgType&RELATIVE_))
                return;

        // note that we'd normally have to add the displacement to the
        // instruction address (and include the instruction's size, etc.
        // but, fix_calls has already removed this oddity so we can relocate
        // the instruction.
        int D1=strtol(disasm.Argument2.ArgMnemonic, NULL, 16);

        // find the section with the data table
        ELFIO::section *pSec=find_section(D1,elfiop);

        // sanity check there's a section
        if(!pSec)
                return;

        const char* secdata=pSec->get_data();

	// if the section has no data, abort 
	if(!secdata)
		return;

        int offset=D1-pSec->get_address();
        int entry=0;
        do
        {
                // check that we can still grab a word from this section
                if(offset+sizeof(int) > pSec->get_size())
                        break;

                const int *table_entry_ptr=(const int*)&(secdata[offset]);
                int table_entry=*table_entry_ptr;

                if(!possible_target(D1+table_entry))
                        break;

                cout<<"Found possible table entry, at: "<< std::hex << I8->GetAddress()->GetVirtualOffset()
		    << " insn: " << disasm.CompleteInstr<< " d1: "
                    << D1 << " table_entry:" << table_entry 
		    << " target: "<< D1+table_entry << std::dec << endl;

                offset+=sizeof(int);
                entry++;

        } while (1);

}

void calc_preds(FileIR_t* firp)
{
        preds.clear();
        for(
                set<Instruction_t*>::const_iterator it=firp->GetInstructions().begin();
                it!=firp->GetInstructions().end();
                ++it
           )
        {
                Instruction_t* insn=*it;
                if(insn->GetTarget());
                        preds[insn->GetTarget()].insert(insn);
                if(insn->GetFallthrough());
                        preds[insn->GetFallthrough()].insert(insn);
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

	set<int> thunk_bases;
	find_all_module_starts(firp,thunk_bases);


	// reset global vars
	bounds.clear();
	ranges.clear();
	targets.clear();

	calc_preds(firp);

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
	print_targets();
	cout<<"========================================="<<endl;

	/* look through the instructions in the program for targets */
	get_instruction_targets(firp, elfiop, thunk_bases);

	/* mark the entry point as a target */
	possible_target(elfiop->get_entry()); 


	cout<<"========================================="<<endl;
	cout<<"All targets from data+instruction sections are: " << endl;
	cout<<"# ATTRIBUTE total_indirect_targets_pass2="<<std::dec<<targets.size()<<endl;
	print_targets();
	cout<<"========================================="<<endl;

	/* Read the exception handler frame so that those indirect branches are accounted for */
	void read_ehframe(FileIR_t* firp, elfio* );
        read_ehframe(firp, elfiop);

	cout<<"========================================="<<endl;
	cout<<"All targets from data+instruction+eh_header sections are: " << endl;
	cout<<"# ATTRIBUTE total_indirect_targets_pass3="<<std::dec<<targets.size()<<endl;
	print_targets();
	cout<<"========================================="<<endl;


	/* now process the ranges that have exception handling */
	process_ranges(firp);
	cout<<"========================================="<<endl;
	cout<<"All targets from data+instruction+eh_header sections+eh_header_ranges are: " << endl;
	cout<<"# ATTRIBUTE total_indirect_targets_pass4="<<std::dec<<targets.size()<<endl;
	print_targets();
	cout<<"========================================="<<endl;

#if 1
	/* now process the ranges that have exception handling */
	check_for_thunks(firp, thunk_bases);
	cout<<"========================================="<<endl;
	cout<<"# ATTRIBUTE total_indirect_targets_pass5="<<std::dec<<targets.size()<<endl;
	print_targets();
	cout<<"========================================="<<endl;
#endif






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
