

#include <libIRDB-core.hpp>
#include <libIRDB-cfg.hpp>
#include <utils.hpp>
#include <iostream>
#include <stdlib.h>
#include "beaengine/BeaEngine.h"
#include <assert.h>
#include <string.h>
#include <elf.h>



using namespace libIRDB;
using namespace std;

long long no_target_insn=0;
long long target_not_in_function=0;
long long call_to_not_entry=0;
long long thunk_check=0;
long long found_pattern=0;

pqxxDB_t pqxx_interface;



bool check_entry(bool &found, ControlFlowGraph_t* cfg)
{

	BasicBlock_t *entry=cfg->GetEntry();
	found=false;

	for(
		std::vector<Instruction_t*>::const_iterator it=entry->GetInstructions().begin();
		it!=entry->GetInstructions().end();
		++it
	   )
	{
		DISASM disasm;
		Instruction_t* insn=*it;
		insn->Disassemble(disasm);
		if(Instruction_t::SetsStackPointer(&disasm))
			return false;

		if(strstr(disasm.CompleteInstr, "[esp]"))
		{
			found=true;
			return true;
		}
		
	}
	return false;
}


bool call_needs_fix(Instruction_t* insn)
{
	Instruction_t *target=insn->GetTarget();
	Instruction_t *fallthru=insn->GetFallthrough();
	DISASM disasm;

	string pattern;;

	/* if the target isn't in the IR */
	if(!target)
	{
		/* call 0's aren't to real locations */
		insn->Disassemble(disasm);
		if(strcmp(disasm.CompleteInstr, "call 0x00000000")==0)
		{
			return false;
		}
		no_target_insn++;
		/* then we need to fix it */
		return true;
	}

	/* no fallthrough instruction, something is odd here */
	if(!fallthru)
		return true;

	/* if the location after the call is marked as an IBT, then 
	 * this location might be used for walking the stack 
  	 */
	if(fallthru->GetIndirectBranchTargetAddress()!=NULL)
		return true;


	Function_t* func=target->GetFunction();

	/* if there's no function for this instruction */
	if(!func)
	{
		target_not_in_function++;
		/* we need to fix it */
		return true;
	}


	/* build a cfg for this function */
	ControlFlowGraph_t* cfg=new ControlFlowGraph_t(func);

	assert(cfg->GetEntry());

	/* if the call instruction isn't to a function entry point */
	if(cfg->GetEntry()->GetInstructions()[0]!=target)
	{
		call_to_not_entry++;
		/* then we need to fix it */
		return true;
	}


	/* check the entry block for thunks, etc. */
	bool found;
	bool ret=check_entry(found,cfg);
	if(found)
	{
		if(ret)
			thunk_check++;
		return ret;
	}

	/* now, search the function for stack references  */


	/* determine what the stack ref. would look like */
	if(func->GetUseFramePointer())
	{
		pattern="[ebp+0x04]";
	}
	else
	{
		pattern="[esp+"+to_string(func->GetStackFrameSize())+"]";
	}


	/* check each instruction */
	for(
		std::set<Instruction_t*>::const_iterator it=func->GetInstructions().begin();
		it!=func->GetInstructions().end();
		++it
	   )
	{
		Instruction_t* itrinsn=*it;
		/* if the disassembly contains the string mentioned */
		DISASM disasm;
		itrinsn->Disassemble(disasm);
		if(strstr(disasm.CompleteInstr, pattern.c_str())!=NULL) 
		{
			found_pattern++;
			/* then we need to fix this callsite */ 
			return true;
		}
	}

	/* otherwise, we think it's safe */
	return false;

}



	




void fix_call(Instruction_t* insn, VariantIR_t *virp)
{
	/* record the possibly new indirect branch target if this call gets fixed */
	Instruction_t* newindirtarg=insn->GetFallthrough();

	/* disassemble */
        DISASM disasm;
        memset(&disasm, 0, sizeof(DISASM));

        disasm.Options = NasmSyntax + PrefixedNumeral;
        disasm.Archi = 32;
        disasm.EIP = (UIntPtr)insn->GetDataBits().c_str();
        disasm.VirtualAddr = insn->GetAddress()->GetVirtualOffset();

        /* Disassemble the instruction */
        int instr_len = Disasm(&disasm);


	/* if this instruction is an inserted call instruction and we don't need to 
	 * convert it for correctness' sake.
	 */
	if(insn->GetAddress()->GetVirtualOffset()==0)
		return;

	virtual_offset_t next_addr=insn->GetAddress()->GetVirtualOffset() + insn->GetDataBits().length();

	/* create a new instruction and a new addresss for it that do not correspond to any original program address */
	Instruction_t *callinsn=new Instruction_t();
	AddressID_t *calladdr=new AddressID_t;
       	calladdr->SetFileID(insn->GetAddress()->GetFileID());
	insn->SetAddress(calladdr);

	/* set the fields in the new instruction */
	callinsn->SetAddress(calladdr);
	callinsn->SetTarget(insn->GetTarget());
	callinsn->SetFallthrough(NULL);
	callinsn->SetFunction(insn->GetFunction());
	callinsn->SetComment(insn->GetComment()+" Jump part");

	/* set the new instruction's data bits to be a jmp instead of a call */
	string newbits=insn->GetDataBits();
	switch((unsigned char)newbits[0])
	{
		case 0xff:
			if(((((unsigned char)newbits[1])&0x38)>>3) == 2)
			{
				newbits[1]&=0xC7;	// remove old bits
				newbits[1]|=(0x4<<3);	// set r/m field to 4
			}
			else if(((((unsigned char)newbits[1])&0x38)>>3) == 3)
			{
				newbits[1]&=0xC7;	// remove old bits
				newbits[1]|=(0x5<<3);	// set r/m field to 5
			}
			else
				assert(0);
			break;
		case 0x9A:
			newbits[0]=0xEA;
			break;
		case 0xE8:
			newbits[0]=0xE9;
			break;
		default:
			assert(0);
	}
	callinsn->SetDataBits(newbits);

	/* add the new insn and new address into the list of valid calls and addresses */
	virp->GetAddresses().insert(calladdr);
	virp->GetInstructions().insert(callinsn);

	/* Convert the old call instruction into a push return_address instruction */
	insn->SetFallthrough(callinsn);
	insn->SetTarget(NULL);
	newbits=string("");
	newbits.resize(5);
	newbits[0]=0x68;	/* assemble an instruction push next_addr */
	newbits[1]=(next_addr>>0) & 0xff;
	newbits[2]=(next_addr>>8) & 0xff;
	newbits[3]=(next_addr>>16) & 0xff;
	newbits[4]=(next_addr>>24) & 0xff;
	insn->SetDataBits(newbits);
	insn->SetComment(insn->GetComment()+" Push part");


	/* If the fallthrough is not marked as indirectly branchable-to, then mark it so */
	if(newindirtarg && !newindirtarg->GetIndirectBranchTargetAddress())
	{
		/* create a new address for the IBTA */
		AddressID_t* newaddr = new AddressID_t;
		assert(newaddr);
		newaddr->SetFileID(newindirtarg->GetAddress()->GetFileID());
		newaddr->SetVirtualOffset(newindirtarg->GetAddress()->GetVirtualOffset());

		/* set the insturction and include this address in the list of addrs */
		newindirtarg->SetIndirectBranchTargetAddress(newaddr);
		virp->GetAddresses().insert(newaddr);
	}

	
}


//
// return true if insn is a call
//
bool is_call(Instruction_t* insn)
{

        DISASM disasm;
        memset(&disasm, 0, sizeof(DISASM));

        disasm.Options = NasmSyntax + PrefixedNumeral;
        disasm.Archi = 32;
        disasm.EIP = (UIntPtr)insn->GetDataBits().c_str();
        disasm.VirtualAddr = insn->GetAddress()->GetVirtualOffset();

        /* Disassemble the instruction */
        int instr_len = Disasm(&disasm);
	

	return (disasm.Instruction.BranchType==CallType);
}

File_t* find_file(VariantIR_t* virp, db_id_t fileid)
{

        set<File_t*> &files=virp->GetFiles();

        for(
                set<File_t*>::iterator it=files.begin();
                it!=files.end();
                ++it
           )
        {
                File_t* thefile=*it;
                if(thefile->GetBaseID()==fileid)
                        return thefile;
        }
        return NULL;
}



//
// fix_all_calls - convert calls to push/jump pairs in the IR.  if fix_all is true, all calls are converted, 
// else we attempt to detect the calls it is safe to convert.
//
void fix_all_calls(VariantIR_t* virp, bool print_stats, bool fix_all)
{


	long long fixed_calls=0, not_fixed_calls=0, not_calls=0;

	for(
		set<Instruction_t*>::const_iterator it=virp->GetInstructions().begin();
		it!=virp->GetInstructions().end(); 
		++it
	   )
	{

		Instruction_t* insn=*it;

		if(is_call(insn)) 
		{
			if(fix_all || call_needs_fix(insn))
			{
				fixed_calls++;
				fix_call(insn, virp);
			}
			else
				not_fixed_calls++;
		}
		else
		{
			not_calls++;
		}
	}


	if(print_stats)
	{
		cout << "# ATTRIBUTE fixed_calls="<<std::dec<<fixed_calls<<endl;
		cout << "# ATTRIBUTE no_fix_needed_calls="<<std::dec<<not_fixed_calls<<endl;
		cout << "# ATTRIBUTE other_instructions="<<std::dec<<not_calls<<endl;
		cout << "# ATTRIBUTE fixed_ratio="<<std::dec<<(fixed_calls/((float)(not_fixed_calls+fixed_calls)))<<endl;
		cout << "# ATTRIBUTE remaining_ratio="<<std::dec<<(not_fixed_calls/((float)(not_fixed_calls+fixed_calls+not_calls)))<<endl;
		cout << "# ATTRIBUTE no_target_insn="<<std::dec<< no_target_insn << endl;
		cout << "# ATTRIBUTE target_not_in_function="<<std::dec<< target_not_in_function << endl;
		cout << "# ATTRIBUTE call_to_not_entry="<<std::dec<< call_to_not_entry << endl;
		cout << "# ATTRIBUTE thunk_check="<<std::dec<< thunk_check << endl;
		cout << "# ATTRIBUTE found_pattern="<<std::dec<< found_pattern << endl;
	}
}


//
// main rountine; convert calls into push/jump statements 
//
main(int argc, char* argv[])
{

	bool fix_all=false;

	if(argc!=2 && argc !=3)
	{
		cerr<<"Usage: fix_calls <id> (--fix-all) "<<endl;
		exit(-1);
	}

	if(argc==3)
	{
		if(strcmp("--fix-all", argv[2])!=0)
		{
			cerr<<"Unrecognized option: "<<argv[2]<<endl;
			exit(-1);
		}
		else
			fix_all=true;
	}

	VariantID_t *pidp=NULL;
	VariantIR_t *virp=NULL;

	/* setup the interface to the sql server */
	BaseObj_t::SetInterface(&pqxx_interface);

	cout<<"Reading variant "<<string(argv[1])<<" from database." << endl;
	try 
	{

		pidp=new VariantID_t(atoi(argv[1]));

		assert(pidp->IsRegistered()==true);

		// read the db  
		virp=new VariantIR_t(*pidp);


	}
	catch (DatabaseError_t pnide)
	{
		cout<<"Unexpected database error: "<<pnide<<endl;
		exit(-1);
        }

	assert(virp && pidp);
	
	cout<<"Fixing calls->push/jmp in variant "<<*pidp<< "." <<endl;

	fix_all_calls(virp,true,fix_all);


	cout<<"Writing variant "<<*pidp<<" back to database." << endl;
	virp->WriteToDB();

	pqxx_interface.Commit();
	cout<<"Done!"<<endl;

	delete pidp;
	delete virp;
}
