

#include <libIRDB.hpp>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <assert.h>

#include "beaengine/BeaEngine.h"

int odd_target_count=0;
int bad_target_count=0;
int bad_fallthrough_count=0;

using namespace libIRDB;
using namespace std;

void populate_instruction_map
	(
		map< pair<db_id_t,virtual_offset_t>, Instruction_t*> &insnMap,
		VariantIR_t *virp
	)
{
	for(
		set<Instruction_t*>::const_iterator it=virp->GetInstructions().begin();
		it!=virp->GetInstructions().end(); 
		++it
	   )
	{
		Instruction_t *insn=*it;
		db_id_t fileID=insn->GetAddress()->GetFileID();
		virtual_offset_t vo=insn->GetAddress()->GetVirtualOffset();

		pair<db_id_t,virtual_offset_t> p(fileID,vo);

		assert(insnMap[p]==NULL);
		insnMap[p]=insn;
	}

}

void set_fallthrough
	(
	map< pair<db_id_t,virtual_offset_t>, Instruction_t*> &insnMap,
	DISASM *disasm, Instruction_t *insn, VariantIR_t *virp
	)
{
	assert(disasm);
	assert(insn);

	if(insn->GetFallthrough())
		return;
	
	// check for branches with targets 
	if(
		(disasm->Instruction.BranchType==JmpType) ||			// it is a unconditional branch 
		(disasm->Instruction.BranchType==RetType)			// or a return
	  )
	{
		// this is a branch with no fallthrough instruction
		return;
	}

	/* get the address of the next instrution */
	
	int virtual_offset=insn->GetAddress()->GetVirtualOffset() + insn->GetDataBits().size();

	/* create a pair of offset/file */
	pair<db_id_t,virtual_offset_t> p(insn->GetAddress()->GetFileID(),virtual_offset);
	
	/* lookup the target insn from the map */
	Instruction_t *fallthrough_insn=insnMap[p];

	/* sanity, note we may see odd control transfers to 0x0 */
	if(fallthrough_insn==NULL &&   virtual_offset!=0)
	{
		cout<<"Cannot set fallthrough for "<<std::hex<<insn->GetAddress()->GetVirtualOffset()<<"."<<endl;
		bad_fallthrough_count++;
	}

	/* set the target for this insn */
	if(fallthrough_insn!=0)
		insn->SetFallthrough(fallthrough_insn);

}


void set_target
	(
	map< pair<db_id_t,virtual_offset_t>, Instruction_t*> &insnMap,
	DISASM *disasm, Instruction_t *insn, VariantIR_t *virp
	)
{

	assert(insn);
	assert(disasm);

	if(insn->GetTarget())
		return;
	
	// check for branches with targets 
	if(
		(disasm->Instruction.BranchType!=0) &&			// it is a branch 
		(disasm->Instruction.BranchType!=RetType) && 		// and not a return
		(disasm->Argument1.ArgType & CONSTANT_TYPE)!=0		// and has a constant argument type 1
	  )
	{
//		cout<<"Found direct jump with addr=" << insn->GetAddress()->GetVirtualOffset() <<
//			" disasm="<<disasm->CompleteInstr<<" ArgMnemonic="<<
//			disasm->Argument1.ArgMnemonic<<"."<<endl;

		/* get the offset */
		int virtual_offset=strtoul(disasm->Argument1.ArgMnemonic, NULL, 16);

		/* create a pair of offset/file */
		pair<db_id_t,virtual_offset_t> p(insn->GetAddress()->GetFileID(),virtual_offset);
	
		/* lookup the target insn from the map */
		Instruction_t *target_insn=insnMap[p];

		/* sanity, note we may see odd control transfers to 0x0 */
		if(target_insn==NULL &&   virtual_offset!=0)
		{
			unsigned char first_byte=0;
			if(insn->GetFallthrough())
				first_byte=(insn->GetFallthrough()->GetDataBits().c_str())[0];
			int jump_dist=virtual_offset-(insn->GetAddress()->GetVirtualOffset()+(insn->GetDataBits()).size());
			if(	
				// jump 1 byte forward
				jump_dist == 1 &&

				// and we calculated the fallthrough
				insn->GetFallthrough()!=NULL &&

				// and the fallthrough starts with a lock prefix
				first_byte==0xf0
			  )
			{
				odd_target_count++;
			}
			else
			{
				cout<<"Cannot set target for "<<std::hex<<insn->GetAddress()->GetVirtualOffset()<<"."<<endl;
				bad_target_count++;
			}
		}

		/* set the target for this insn */
		if(target_insn!=0)
			insn->SetTarget(target_insn);

	}
}

void fill_in_cfg(VariantIR_t *virp)
{

	map< pair<db_id_t,virtual_offset_t>, Instruction_t*> insnMap;
	populate_instruction_map(insnMap, virp);

	cout << "Found "<<virp->GetInstructions().size()<<" instructions." <<endl;

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

		set_fallthrough(insnMap, &disasm, insn, virp);
		set_target(insnMap, &disasm, insn, virp);
		
	}
	if(bad_target_count>0)
		cout<<std::dec<<"Found "<<bad_target_count<<" bad targets."<<endl;
	if(bad_target_count>0)
		cout<<"Found "<<bad_fallthrough_count<<" bad fallthroughs."<<endl;
	if(odd_target_count>0)
		cout<<std::dec<<"Found "<<odd_target_count<<" odd targets (to jump over lock prefix)."<<endl;

}

main(int argc, char* argv[])
{

	if(argc!=2)
	{
		cerr<<"Usage: create_variant <id>"<<endl;
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


		fill_in_cfg(virp);

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
