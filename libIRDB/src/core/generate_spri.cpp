

#include <all.hpp>
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
// the set of insturctions that have control 
// transfers to them (including fallthrough type control transfers) from instructions that do not need a spri rule.
//
set<Instruction_t*> unmoved_insn_targets;
	

//
// map an instruction from the new variant back to the old variant;
//
static map<Instruction_t*,Instruction_t*> insnMap;

//
// does this instruction need a spri rule 
//
static bool needs_spri_rule(Instruction_t* newinsn,Instruction_t* oldinsn);


//
// return the address as a string for this instruction
//
static string addressify(Instruction_t* insn);


//
// determine if this branch has a short offset that can't be represented as a long branch
//
static int needs_short_branch_rewrite(const DISASM &disasm)
{
	return   strstr(disasm.Instruction.Mnemonic, "jecxz" ) || strstr(disasm.Instruction.Mnemonic, "loop" ) || 
		 strstr(disasm.Instruction.Mnemonic, "loopne") || strstr(disasm.Instruction.Mnemonic, "loope") ;
}


//
// create a label for the given instruction
//
static string labelfy(Instruction_t* insn)
{
	if(!needs_spri_rule(insn, insnMap[insn]))
		return addressify(insn);

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

static string get_short_branch_label(Instruction_t *newinsn)
{
	if (!newinsn)
		return string("");
	else
		return "short_jump_" + labelfy(newinsn);
}

static string getPostCallbackLabel(Instruction_t *newinsn)
{
	if (!newinsn)
		return string("");
	else
		return "post_callback_" + labelfy(newinsn);
}

//
// emit this instruction as spri code.
//
static string emit_spri_instruction(Instruction_t *newinsn, ostream& fout)
{
	string original_target;
	Instruction_t* old_insn=insnMap[newinsn];

	// disassemble using BeaEngine
	DISASM disasm;
	memset(&disasm, 0, sizeof(DISASM));

	disasm.Options = NasmSyntax + PrefixedNumeral; //  + ShowSegmentRegs;
	disasm.Archi = 32;
	disasm.EIP = (UIntPtr)newinsn->GetDataBits().c_str();
	disasm.VirtualAddr = old_insn ? old_insn->GetAddress()->GetVirtualOffset() : 0;

	/* Disassemble the instruction */
	int instr_len = Disasm(&disasm);


	/* if this instruction has a prefix, re-disassemble it showing the segment regs */
	if(
		disasm.Prefix.FSPrefix || 
		disasm.Prefix.SSPrefix || 
		disasm.Prefix.GSPrefix || 
		disasm.Prefix.ESPrefix || 
		disasm.Prefix.CSPrefix || 
		disasm.Prefix.DSPrefix 
	  )
	
	{
		memset(&disasm, 0, sizeof(DISASM));

		disasm.Options = NasmSyntax + PrefixedNumeral + ShowSegmentRegs;
		disasm.Archi = 32;
		disasm.EIP = (UIntPtr)newinsn->GetDataBits().c_str();
		disasm.VirtualAddr = old_insn ? old_insn->GetAddress()->GetVirtualOffset() : 0;

		/* Disassemble the instruction */
		int instr_len = Disasm(&disasm);
	}


	string label=labelfy(newinsn);
	string complete_instr=string(disasm.CompleteInstr);
	string address_string=string(disasm.Argument1.ArgMnemonic);

        /* Emit any callback functions */
	if (!newinsn->GetCallback().empty())
	{
		fout << "\t"+label+"\t () " << newinsn->GetCallback() << endl;
		fout << "\t"+ getPostCallbackLabel(newinsn)+" ** ";
	}
	else
	{
		fout << "\t"+label+"\t ** ";
	}

	/* emit the actual instruction from the database */

	/* if it's a brnach instruction, we have extra work to do */
        if(
                (disasm.Instruction.BranchType!=0) &&                  // it is a branch
                (disasm.Instruction.BranchType!=RetType) &&            // and not a return
                (disasm.Argument1.ArgType & CONSTANT_TYPE)!=0          // and has a constant argument type 1
          )
	{

		/* if we have a target instruction in the database */
		if(newinsn->GetTarget() || needs_short_branch_rewrite(disasm))
		{
			/* change the target to be symbolic */
	
			/* first get the new target */
			string new_target;
			if(newinsn->GetTarget())
				new_target=labelfy(newinsn->GetTarget());
			/* if this is a short branch, write this branch to jump to the next insn */
			if(needs_short_branch_rewrite(disasm))
			{
				new_target=get_short_branch_label(newinsn);

				/* also get the real target if it's a short branch */
				if(newinsn->GetTarget())
					original_target=labelfy(newinsn->GetTarget());
				else
					original_target=address_string;
			}

			/* find the location in the disassembled string of the old target */
			int start=complete_instr.find(address_string,0);

			/* and build up a new string that has the label of the target instead of the address */
			string final=complete_instr.substr(0,start) + new_target + complete_instr.substr(start+address_string.length());
	
			/* sanity, no segment registers for absolute mode */
			assert(disasm.Argument1.SegmentReg==0);

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

		/* beaEngine kinda sucks and does some non-nasmness. */
		
		/* in this case, we look for an "lea <reg>, dword [ addr ]" and remove the "dword" part */
		if(strstr(disasm.CompleteInstr,"lea ") != NULL )
		{
			char* a=strstr(disasm.CompleteInstr, "dword ");
			if(a!=NULL)
			{
				a[0]=' ';	 // d
				a[1]=' ';	 // w
				a[2]=' ';	 // o
				a[3]=' ';	 // r
				a[4]=' ';	 // d
			}
		}

		/* In this case, we look for "mov*x dstreg, srcreg" and convert srcreg to an appropriate size */
		if(	strstr(disasm.CompleteInstr, "movzx ") || 
			strstr(disasm.CompleteInstr, "movsx ") )
		{
			if( disasm.Instruction.Opcode==0xfbe || disasm.Instruction.Opcode==0xfb6 ) 
			{
				char* comma=strstr(disasm.CompleteInstr, ",");
				assert(comma);
				if(comma[2]=='e' && comma[3]=='a' && comma[4]=='x') // eax -> al 
					comma[2]=' ', comma[3],'a', comma[4]='l';
				if(comma[2]=='e' && comma[3]=='b' && comma[4]=='x') // ebx -> bl 
					comma[2]=' ', comma[3],'b', comma[4]='l';
				if(comma[2]=='e' && comma[3]=='c' && comma[4]=='x') // ecx -> cl 
					comma[2]=' ', comma[3],'c', comma[4]='l';
				if(comma[2]=='e' && comma[3]=='d' && comma[4]=='x') // edx -> dl 
					comma[2]=' ', comma[3],'d', comma[4]='l';
				if(comma[2]=='e' && comma[3]=='s' && comma[4]=='p') // esp -> ah 
					comma[2]=' ', comma[3],'a', comma[4]='h';
				if(comma[2]=='e' && comma[3]=='b' && comma[4]=='p') // ebp -> bh 
					comma[2]=' ', comma[3],'b', comma[4]='h';
				if(comma[2]=='e' && comma[3]=='s' && comma[4]=='i') // esi -> ch 
					comma[2]=' ', comma[3],'c', comma[4]='l';
				if(comma[2]=='e' && comma[3]=='d' && comma[4]=='i') // edi -> dh 
					comma[2]=' ', comma[3],'d', comma[4]='l';
			}
			else if( disasm.Instruction.Opcode==0xfbf || disasm.Instruction.Opcode==0xfb7 ) 
			{
				char* comma=strstr(disasm.CompleteInstr, ",");
				assert(comma);
				if(strstr(&comma[2], "word [") == NULL)  // if it's not a memory operation 
				{
					assert(comma[2]=='e');
					comma[2]=' ';
				}
			}
			else
				assert(0); // wtf?
		}
		// look for an fld st0, st0, and convert it to fld st0 
		else if(strcmp("fld st0 , st0", disasm.CompleteInstr)==0)
		{
			disasm.CompleteInstr[8]='\0';
		}
			
		fout<<disasm.CompleteInstr;
	}
	fout<<endl;


	return original_target;

}

//
// check to see if this instruction needs a spri rewrite rule.
//
static bool needs_spri_rule(Instruction_t* newinsn,Instruction_t* oldinsn)
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
	if(newFT && newFT->GetOriginalAddressID()!=oldFT->GetAddress()->GetBaseID())
		return true;
		
	// if there's a target, but it is different, return true
	if(newTG && newTG->GetOriginalAddressID()!=oldTG->GetAddress()->GetBaseID())
		return true;

	// data bits themselves changed
	if(newinsn->GetDataBits() != oldinsn->GetDataBits())
		return true;

	return false;
}

//
// emit the spri rule to redirect this instruction.
//
static void emit_spri_rule(Instruction_t* newinsn, ostream& fout)
{

#if 0
We need to emit a rule of this form
	L_insn_id -> .	
	. ** data bits with pc rel address taken care of.
	. -> fallthrough label
#endif

	Instruction_t* old_insn=insnMap[newinsn];

	fout << "# Orig addr: "<<addressify(newinsn)<<" insn_id: "<< std::dec << newinsn->GetBaseID()<<" with comment "<<newinsn->GetComment()<<endl;
	if (newinsn->GetIndirectBranchTargetAddress())
		fout << "# Orig addr: "<<addressify(newinsn)<<" indirect branch target: "<<newinsn->GetIndirectBranchTargetAddress()->GetVirtualOffset() << endl;

	if(addressify(newinsn).c_str()[0]=='0')
	{
		if(
		   // if it's an indirect branch target 
		   newinsn->GetIndirectBranchTargetAddress() || 
		   // or the target of an unmodified instruction 
		   unmoved_insn_targets.find(newinsn) != unmoved_insn_targets.end()
		  )
		{
			fout << addressify(newinsn) <<" -> ."<<endl;
		}
		else
		{
			fout << "# eliding, no indirect targets"<<endl;
			fout << addressify(newinsn) <<" -> 0x0 " <<endl; 
		}
		
	}
	else if (newinsn->GetIndirectBranchTargetAddress()) 
	{
		fout << "0x" << std::hex << newinsn->GetIndirectBranchTargetAddress()->GetVirtualOffset() <<" -> ."<<endl;
		fout << ". -> "<< getPostCallbackLabel(newinsn) <<endl;
	}

	string original_target=emit_spri_instruction(newinsn, fout);


	/* if there's a fallthrough instruction, jump to it. */
	if(newinsn->GetFallthrough())
	{	
		fout << ". -> " << labelfy(newinsn->GetFallthrough())<<endl;
	}
	else
	{
		DISASM disasm;
		disasm.Options = NasmSyntax + PrefixedNumeral + ShowSegmentRegs;
		disasm.Archi = 32;
		disasm.EIP = (UIntPtr)newinsn->GetDataBits().c_str();
		disasm.VirtualAddr = old_insn ? old_insn->GetAddress()->GetVirtualOffset() : 0;

		/* Disassemble the instruction */
		int instr_len = Disasm(&disasm);

		if( disasm.Instruction.BranchType!=RetType && disasm.Instruction.BranchType!=JmpType ) 
		{
			assert(old_insn);	/* it's an error to insert a new, non-unconditional branch instruction
						 * and not specify it's fallthrough */
			fout << ". -> 0x" << std::hex << old_insn->GetAddress()->GetVirtualOffset()+instr_len <<endl;
		}
	}

	fout<<endl;

	/* if the original target string is set, we need to emit 
	 * a rule for this instruction so that short branches can always be resolved 
	 */
	if(!original_target.empty())
	{
		fout << "\t" << get_short_branch_label(newinsn) << "\t -> \t " << original_target << endl;
	}

}



//
// generate a map from new instructions to old instructions
//
static void generate_insn_to_insn_maps(FileIR_t *varirp, FileIR_t *orig_varirp)
{
	static map<Instruction_t*,Instruction_t*> new_insnMap;
	insnMap=new_insnMap; // re-init the global instruction map.

	/* since a variant does not hold a pointer to the original code, we need to create that mapping 
	 * we do it in two steps.  the first step is to make a map from ids in the original code 
	 * to instructions in the original code 
	 * the second step is to is to create the final mapping using the first map 
 	 */

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
// GenerateSPRI -  spri for the entire database
//
void FileIR_t::GenerateSPRI(ostream &fout)
{
	if(orig_variant_ir_p==NULL)
	{
		VariantID_t orig_varidp(progid.GetOriginalVariantID());
		assert(orig_varidp.IsRegistered()==true);
		orig_variant_ir_p=new FileIR_t(orig_varidp);
	}


	this->GenerateSPRI(orig_variant_ir_p,fout);
}


//
// generate_unmoved_insn_targets_set --  create the set of insturctions that have control 
// transfers to them (including fallthrough type control transfers) from instructions that do not need a spri rule.
//
static void generate_unmoved_insn_targets_set(FileIR_t* varirp)
{
	for(
		set<Instruction_t*>::const_iterator it=varirp->GetInstructions().begin();
		it!=varirp->GetInstructions().end();
		++it
	   )
	{
		Instruction_t *insn=*it;

		if(
			// this instruction corresponds to an old instructino 
			insnMap[insn] && 
			// and we need a spri rule for it 
			!needs_spri_rule(insn, insnMap[insn])
		  )
		{
			unmoved_insn_targets.insert(insn->GetTarget());
			unmoved_insn_targets.insert(insn->GetFallthrough());
		}

	}

}


void FileIR_t::GenerateSPRI(FileIR_t *orig_varirp, ostream &fout)
{
	// give 'this' a name
	FileIR_t *varirp=this;

	SetBaseIDS(); // need unique ID to generate unique label name

	// generate the map from new instruction to old instruction needed for this transform.
	generate_insn_to_insn_maps(varirp, orig_varirp);

	// generate unmoved_insn_targets_set --  the set of insturctions that have control 
	// transfers to them (including fallthrough type control transfers) from instructions that do not need a spri rule.
	generate_unmoved_insn_targets_set(varirp);

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

