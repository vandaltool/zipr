

#include <libIRDB.hpp>
#include <utils.hpp> // to_string function from libIRDB
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <map>
#include "beaengine/BeaEngine.h"
#include <string.h>
#include <assert.h>

using namespace libIRDB;
using namespace std;


//
// map an instruction from the new variant back to the old variant;
//
map<Instruction_t*,Instruction_t*> insnMap;

//
// create a label for the given instruction
//
static string labelfy(Instruction_t* insn)
{
	return string("Label_insn_") + to_string(insn->GetBaseID());
}


//
// return the address for an instruction.  If the instruction has no absolute address, return the label for this instruction.
//
static string addressify(Instruction_t* insn)
{
	stringstream s;

	Instruction_t* old_insn=insnMap[insn];
	if(!old_insn)
		return labelfy(insn);

	s<<"0x"<<std::hex<<old_insn->GetAddress()->GetVirtualOffset();

	return s.str();
	

}

//
// emit this instruction as spri code.
//
void emit_spri_instruction(Instruction_t *newinsn, ostream& fout)
{
	// disassemble using BeaEngine
	DISASM disasm;
	memset(&disasm, 0, sizeof(DISASM));

	disasm.Options = NasmSyntax + PrefixedNumeral;
	disasm.Archi = 32;
	disasm.EIP = (UIntPtr)newinsn->GetDataBits().c_str();
	disasm.VirtualAddr = newinsn->GetAddress()->GetVirtualOffset();

	/* Disassemble the instruction */
	int instr_len = Disasm(&disasm);

	string label=labelfy(newinsn);
	string complete_instr=string(disasm.CompleteInstr);
	string address_string=string(disasm.Argument1.ArgMnemonic);

	fout << "\t"+label+"\t ** ";

        if(
                (disasm.Instruction.BranchType!=0) &&                  // it is a branch
                (disasm.Instruction.BranchType!=RetType) &&            // and not a return
                (disasm.Argument1.ArgType & CONSTANT_TYPE)!=0          // and has a constant argument type 1
          )
	{

		/* if we have a target instruction in the database */
		if(newinsn->GetTarget())
		{
			/* change the target to be symbolic */
	
			/* first get the new target */
			string new_target=labelfy(newinsn->GetTarget());

			/* find the location in the disassembled string of the old target */
			int start=complete_instr.find(address_string,0);

			/* and build up a new string that has the label of the target instead of the address */
			string final=complete_instr.substr(0,start) + new_target + complete_instr.substr(start+address_string.length());
	
			/* sanity, no segment registers for absolute mode */
			assert(disasm.Argument1.SegmentReg==0);

			/* emit */
			fout<<final;
		}
		else 	/* this instruction has a target, but it's not in the DB */
		{
			/* so we'll just emit the instruction and let it go back to the application text. */	
			fout<<complete_instr;
		}
	}
	else
	{
		/* no target, just emit the instrution */
		fout<<disasm.CompleteInstr;
	}
	fout<<endl;
}

//
// check to see if this instruction needs a spri rewrite rule.
//
bool needs_spri_rule(Instruction_t* newinsn,Instruction_t* oldinsn)
{
	// check if this is an inserted instruction 
	if(newinsn->GetOriginalAddressID()==-1)
		return true;

	assert(oldinsn);
	assert(newinsn->GetOriginalAddressID()==oldinsn->GetAddress()->GetBaseID());


	/* We moved the instruction  to a new address*/
	if(newinsn->GetAddress()->GetVirtualOffset()!=oldinsn->GetAddress()->GetVirtualOffset())
		return true;

	/* We moved the instruction to a new file? */
	if(newinsn->GetAddress()->GetFileID()!=oldinsn->GetAddress()->GetFileID())
	{
		//	
		// coders:  verify this is OK before allowing an insn to change files. 
		//
		assert(0);
		return true;
	}


	Instruction_t *newFT=newinsn->GetFallthrough();
	Instruction_t *newTG=newinsn->GetTarget();
	Instruction_t *oldFT=oldinsn->GetFallthrough();
	Instruction_t *oldTG=oldinsn->GetTarget();

	//
	// check that both have a fallthrough or both don't have a fallthrough
	//
	if(!!newFT != !!oldFT)
		return true;
	//
	// check that both have a target or both don't have a target
	//
	if(!!newTG != !!oldTG)
		return true;

	// if there's a fallthrough, but it is different, return true
	if(newFT && newFT->GetOriginalAddressID()!=oldFT->GetBaseID())
		return true;
		
	// if there's a target, but it is different, return true
	if(newTG && newTG->GetOriginalAddressID()!=oldFT->GetBaseID())
		return true;

	// data bits themselves changed
	if(newinsn->GetDataBits() != oldinsn->GetDataBits())
		return true;

	
}

//
// emit the spri rule to redirect this instruction.
//
void emit_spri_rule(Instruction_t* newinsn, ostream& fout)
{

#if 0
We need to emit a rule of this form
	L_insn_id -> .	
	. ** data bits with pc rel address taken care of.
	. -> fallthrough label
#endif


	fout << addressify(newinsn) <<" -> ."<<endl;

	emit_spri_instruction(newinsn, fout);


	/* if there's a fallthrough instruction, jump to it. */
	if(newinsn->GetFallthrough())
		fout << ". -> " << labelfy(newinsn->GetFallthrough())<<endl;

}


//
// generate spri for the entire database
//
void generate_spri(VariantIR_t *varirp, VariantIR_t *orig_varirp, ostream &fout)
{
	//
	// for each instruction, compare the new instruction with the original instruction and see if 
	// they are the same.  If so, do nothing, otherwise emit a rewrite rule for this instruction.
	//
	for(
		std::set<Instruction_t*>::const_iterator it=varirp->GetInstructions().begin();
		it!=varirp->GetInstructions().end();
		++it
	   )
	{
		Instruction_t* newinsn=*it;
		Instruction_t* oldinsn=insnMap[newinsn];

		assert(newinsn);

		if(needs_spri_rule(newinsn,oldinsn))
		{
			emit_spri_rule(newinsn,fout);
		}
	}

}


//
// generate a map from new instructions to old instructions
//
void generate_insn_to_insn_maps(VariantIR_t *varirp, VariantIR_t *orig_varirp)
{
	/* since a variant does not hold a pointer to the original code, we need to create that mapping */
	/* we do it in two steps.  the first step is to make a map from ids in the original code to instructions in the original code */
	/* the second step is to is to create the final mapping using the first map */

	map<db_id_t,Instruction_t*> idMap;

	/* loop through each insn in the original program */
	for(
		std::set<Instruction_t*>::const_iterator it=orig_varirp->GetInstructions().begin();
		it!=orig_varirp->GetInstructions().end();
		++it
	   )
	{
		/* get the insn */
		Instruction_t *insn=*it;
		assert(insn);

		/* get it's ID */
		db_id_t address_id=insn->GetAddress()->GetBaseID();
		assert(address_id!=-1);

		/* sanity check */
		assert(insn->GetAddress()->GetFileID()!=-1);	
		assert(insn->GetAddress()->GetVirtualOffset()!=0);	

		/* insert into map */
		idMap[address_id]=insn;
	}

	/* loop through the new variant and create the final mapping of new insn to old insn */
	for(
		std::set<Instruction_t*>::const_iterator it=varirp->GetInstructions().begin();
		it!=varirp->GetInstructions().end();
		++it
	   )
	{
		/* get the insn */
		Instruction_t *insn=*it;
		assert(insn);

		db_id_t orig_addr=insn->GetOriginalAddressID();

		/* no mapping if this is true */
		if(orig_addr==-1)
			continue;

		assert(idMap[orig_addr]!=NULL);

		insnMap[insn]=idMap[orig_addr];
		
	}
}

//
// main routine to generate spri rules for a variant.
//
main(int argc, char* argv[])
{
	if(argc!=2 && argc!=3)
	{
		cerr<<"Usage: generate_spri.exe <variant id> [<output_file>]"<<endl;
		exit(-1);
	}

	string filename;
	ostream *fout;
	if(argc==3)
		fout=new ofstream(argv[3], ios::out);
	else
		fout=&cerr;


	VariantID_t *varidp=NULL;
	VariantIR_t *varirp=NULL;
	VariantID_t *orig_varidp=NULL;
	VariantIR_t *orig_varirp=NULL;

	/* setup the interface to the sql server */
	pqxxDB_t pqxx_interface;
	BaseObj_t::SetInterface(&pqxx_interface);

	try 
	{

		cout<<"Looking up variant "<<string(argv[1])<<" from database." << endl;
		varidp=new VariantID_t(atoi(argv[1]));
		cout<<"Looking up variant "<<varidp->GetOriginalVariantID()<<" from database." << endl;
		orig_varidp=new VariantID_t(varidp->GetOriginalVariantID());

		assert(varidp->IsRegistered()==true);
		assert(orig_varidp->IsRegistered()==true);

		// read the db  
		cout<<"Reading variant "<<string(argv[1])<<" from database." << endl;
		varirp=new VariantIR_t(*varidp);
		cout<<"Reading variant "<<varidp->GetOriginalVariantID()<<" from database." << endl;
		orig_varirp=new VariantIR_t(*orig_varidp);

		generate_insn_to_insn_maps(varirp, orig_varirp);
		generate_spri(varirp, orig_varirp, *fout);

	}
	catch (DatabaseError_t pnide)
	{
		cout<<"Unexpected database error: "<<pnide<<endl;
		exit(-1);
        }

	cout<<"Done!"<<endl;

	delete varidp;
	delete varirp;
	delete orig_varidp;
	delete orig_varirp;
}


