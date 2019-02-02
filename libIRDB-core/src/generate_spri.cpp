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



#include <all.hpp>
#include <irdb-util> // to_string function from libIRDB
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <map>
#include <string.h>
#include <assert.h>

#undef EIP

using namespace libIRDB;
using namespace std;

#if 0

// forward decls for this file 
static string qualified_addressify(FileIR_t* fileIRp, Instruction_t *insn);
static string labelfy(IRDB_SDK::Instruction_t* insn);


//
// the set of insturctions that have control 
// transfers to them (including fallthrough type control transfers) from instructions that do not need a spri rule.
//
static set<Instruction_t*> unmoved_insn_targets;
	
//
// The set of all addresses that are ibts
//
static set <AddressID_t> ibts;

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
static bool needs_short_branch_rewrite(Instruction_t* newinsn, const DecodedInstruction_t &disasm)
{
	if   (	   (disasm.getMnemonic()== "jecxz" ) 
		|| (disasm.getMnemonic()== "jrcxz" ) 
		|| (disasm.getMnemonic()== "loop"  ) 
		|| (disasm.getMnemonic()== "loopne") 
		|| (disasm.getMnemonic()== "loope" ) )
		return true;

	/* 64-bit has more needs than this */
	// if(disasm.Archi==32)
	if(FileIR_t::getArchitectureBitWidth()==32)
		return false;

	// if(disasm.Instruction.BranchType==0)		/* non-branches, jumps, calls and returns don't need this rewrite */
	if(!disasm.isBranch())
		return false;
	// if(disasm.Instruction.BranchType==JmpType)
	if(disasm.isUnconditionalBranch())
		return false;
	// if(disasm.Instruction.BranchType==CallType)
	if(disasm.isCall())
		return false;
	// if(disasm.Instruction.BranchType==RetType)
	if(disasm.isReturn())
		return false;

	/* all other branches (on x86-64) need further checking */
	if(!newinsn->getTarget())	/* no specified target, no need to modify it */
		return false;
	string new_target=labelfy(newinsn->getTarget());
	if (new_target.c_str()[0]=='0')	/* if we're jumping back to the base instruction */
		return true;
	return false;
}


//
// create a label for the given instruction
//
static string qualified_labelfy(FileIR_t* fileIRp, IRDB_SDK::Instruction_t* p_insn)
{
	auto insn=dynamic_cast<Instruction_t*>(p_insn);
	if(!needs_spri_rule(insn, insnMap[insn]))
		return qualified_addressify(fileIRp, insn);

	return labelfy(insn);
}

static long long int label_offset=0;

static void update_label_offset(FileIR_t *firp)
{
	int max=0;
	for(auto insn : firp->GetInstructions())
	{
		if(insn->getBaseID()>max)
			max=insn->getBaseID();
	}
	label_offset+=max+1;
}

static long long int IDToSPRIID(int id)
{
	return id+label_offset;
}

static string labelfy(IRDB_SDK::Instruction_t* p_insn)
{
	auto insn=dynamic_cast<Instruction_t*>(p_insn);

	if(!needs_spri_rule(insn, insnMap[insn]))
		return addressify(insn);

	return string("LI_") + to_string(IDToSPRIID(insn->getBaseID()));
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

	s<<"0x"<<std::hex<<old_insn->getAddress()->getVirtualOffset();

	return s.str();
	

}

static string URLToFile(string url)
{
	auto loc=(size_t)0;

	loc=url.find('/');
	while(loc!=string::npos)
	{
		url=url.substr(loc+1,url.length()-loc-1);

		loc=url.find('/');
	}
	// maybe need to check filename for odd characters 

	return url;
}

static string qualify(FileIR_t* fileIRp)
{
	return URLToFile(fileIRp->getFile()->GetURL()) + "+" ;
}


static string qualify_address(FileIR_t* fileIRp, int addr)
{
	stringstream ss;
	ss<< qualify(fileIRp) << "0x" << std::hex << (addr);
	return ss.str();
}

static string better_qualify_address(FileIR_t* fileIRp, IRDB_SDK::AddressID_t* addr)
{
	db_id_t fileID=addr->getFileID();
	virtual_offset_t off=addr->getVirtualOffset();

	/* in theory, we could have an address from any file..
	 * but this appears to be broken.  NOT_IN_DATABASE means we're in the spri address space,
	 * so that's all i'm checking for for now.
	 * do it the old way if it's in the DB, else the new way.
	 */
	if(fileID!=BaseObj_t::NOT_IN_DATABASE)
		return qualify_address(fileIRp,off);

	/* having a file not in the DB means we're int he spridd address space */
	stringstream s;
	s<<"spridd+0x"<<std::hex<<off;
	return s.str();
		
}

static string qualified_addressify(FileIR_t* fileIRp, Instruction_t *insn)
{
	string address=addressify(insn);
	if(address.c_str()[0]=='0')
		return qualify(fileIRp) + address;
	return address;	
	
}

static string get_short_branch_label(Instruction_t *newinsn)
{
	if (!newinsn)
		return string("");
	else
		return "sj_" + labelfy(newinsn);
}

static string get_data_label(Instruction_t *newinsn)
{
	if (!newinsn)
		return string("");
	else
		return "da_" + labelfy(newinsn);
}

static string getPostCallbackLabel(Instruction_t *newinsn)
{
	if (!newinsn)
		return string("");
	else
		return "pcb_" + labelfy(newinsn);
}


static string get_relocation_string(FileIR_t* fileIRp, ostream& fout, int offset, string type, Instruction_t* insn) 
{
	stringstream ss;
	ss<<labelfy(insn)<<" rl " << offset << " " << type <<  " " << URLToFile(fileIRp->getFile()->GetURL()) << endl;
	return ss.str();
}

static void emit_relocation(FileIR_t* fileIRp, ostream& fout, int offset, string type, Instruction_t* insn) 
{
	fout<<"\t"<<get_relocation_string(fileIRp,fout,offset,type,insn);
}


/* return true if converted */
bool convert_jump_for_64bit(Instruction_t* newinsn, string &final, string &emit_later, string new_target)
{
	/* skip for labeled addresses */
	if (new_target.c_str()[0]!='0')
		return false;

	string datalabel=get_data_label(newinsn);

	/* convert a "call <addr>" into "call qword [rel data_label] \n  data_label ** dq <addr>" */
	int start=final.find(new_target,0);

	string new_string=final.substr(0,start)+" qword [ rel " +datalabel + "]\n";
	emit_later="\t"+ datalabel + " ** dq "+final.substr(start) + "\n";

	final=new_string;

	return true;
}

void emit_jump(FileIR_t* fileIRp, ostream& fout, const DecodedInstruction_t& disasm, Instruction_t* newinsn, Instruction_t *old_insn, string & original_target, string &emit_later)
{

	string label=labelfy(newinsn);
	string complete_instr=disasm.getDisassembly();
	string address_string=disasm.getOperand(0)->getString();


	/* if we have a target instruction in the database */
	if(newinsn->getTarget() || needs_short_branch_rewrite(newinsn,disasm))
	{
		/* change the target to be symbolic */

		/* first get the new target */
		string new_target;
		if(newinsn->getTarget())
			new_target=labelfy(newinsn->getTarget());
		/* if this is a short branch, write this branch to jump to the next insn */
		if(needs_short_branch_rewrite(newinsn,disasm))
		{
			new_target=get_short_branch_label(newinsn);

			/* also get the real target if it's a short branch */
			if(newinsn->getTarget())
				original_target=labelfy(newinsn->getTarget());
			else
				original_target=address_string;
		}

		/* find the location in the disassembled string of the old target */
		int start=complete_instr.find(address_string,0);

		/* and build up a new string that has the label of the target instead of the address */
		string final=complete_instr.substr(0,start) + new_target + complete_instr.substr(start+address_string.length());

	
		/* sanity, no segment registers for absolute mode */
		//assert(disasm.Argument1.SegmentReg==0);

		// if(disasm.Archi==64)
		if(FileIR_t::getArchitectureBitWidth()==64)
		{
			auto converted=convert_jump_for_64bit(newinsn,final, emit_later,new_target);
			(void)converted; // unused?
		}

		fout<<final<<endl;

		if (new_target.c_str()[0]=='0')
		{
assert(0); 
#if 0
this will never work again?
			// if we converted to an indirect jump, do a 64-bit reloc 
			if(converted)
			{
				/* jumps have a 1-byte opcode */
				string reloc=get_relocation_string(fileIRp, fout,0,"64-bit",newinsn);
				emit_later=emit_later+"da_"+reloc;
			}
			// if we're jumping to an absolute address vrs a label, we will need a relocation for this jump instruction
			else if(
			   disasm.Instruction.Opcode==0xeb || 	 // jmp with 8-bit addr  -- should be recompiled to 32-bit
			   disasm.Instruction.Opcode==0xe8 || 	 // jmp with 32-bit addr 
			   disasm.Instruction.Opcode==0xe9 	 // call with 32-bit addr

			)
			{
				/* jumps have a 1-byte opcode */
				emit_relocation(fileIRp, fout,1,"32-bit",newinsn);
			}
			else
			{
				/* other jcc'often use a 2-byte opcode for far jmps (which is what spri will emit) */
				emit_relocation(fileIRp, fout,2,"32-bit",newinsn);
			}
#endif
		}
	}
	else 	/* this instruction has a target, but it's not in the DB */
	{
		/* so we'll just emit the instruction and let it go back to the application text. */	
		fout<<complete_instr<<endl;
// needs relocation info.
#if 0
// never to work again?
		if(complete_instr.compare("call 0x00000000")==0 ||
		   complete_instr.compare("jmp 0x00000000")==0
		  )
		{
			// just ignore these bogus instructions.
		}
		else
		{
			if(
			   disasm.Instruction.Opcode==0xeb || 	 // jmp with 8-bit addr 
			   disasm.Instruction.Opcode==0xe8 || 	 // jmp with 32-bit addr 
			   disasm.Instruction.Opcode==0xe9 	 // call with 32-bit addr
			  )
			{
				emit_relocation(fileIRp, fout,1,"32-bit",newinsn);
			}
			else
			{
				// assert this is the "main" file and no relocation is necessary.
				assert(strstr(fileIRp->getFile()->GetURL().c_str(),"a.ncexe")!=0);
			}
		}
#endif
	}
}



//
// emit this instruction as spri code.
//
static string emit_spri_instruction(FileIR_t* fileIRp, Instruction_t *newinsn, ostream& fout, string &emit_later)
{
	string original_target;
	Instruction_t* old_insn=insnMap[newinsn];

	// disassemble using BeaEngine
	// DISASM disasm;

	/* Disassemble the instruction */
	//int instr_len = Disassemble(newinsn,disasm);
	const auto p_disasm=DecodedInstruction_t::factory(newinsn);
	const auto &disasm=*p_disasm;

	string label=labelfy(newinsn);
	string complete_instr=disasm.getDisassembly(); 
	string address_string=disasm.getOperand(0)->getString(); 

	/* Emit any callback functions */
	if (!newinsn->getCallback().empty())
	{
		fout << "\t"+label+"\t () " << newinsn->getCallback() << " # acts as a call <callback> insn" << endl;
		fout << "\t"+ getPostCallbackLabel(newinsn)+" ** ";
	}
	else
	{
		fout << "\t"+label+"\t ** ";
	}

	/* emit the actual instruction from the database */
	if( 
	   strstr(disasm.getDisassembly().c_str(),"jmp far")!=0 || 
	   strstr(disasm.getDisassembly().c_str(),"call far")!=0
	  )
	{
		fout<<"\t hlt " << endl;
	}

	/* if it's a branch instruction, we have extra work to do */
	else if(
		//(disasm.Instruction.BranchType!=0) &&                  // it is a branch
		//(disasm.Instruction.BranchType!=RetType) &&            // and not a return
		//(disasm.Argument1.ArgType & CONSTANT_TYPE)!=0          // and has a constant argument type 1
		disasm.isBranch()  &&
		!disasm.isReturn() &&
		disasm.getOperand(0)->isConstant() 
	  )
	{
		emit_jump(fileIRp, fout, disasm,newinsn,old_insn, original_target, emit_later);
	}
	else
	{
		/* no target, just emit the instruction */

		/* beaEngine kinda sucks and does some non-nasmness. */
		
		/* in this case, we look for an "lea <reg>, dword [ addr ]" and remove the "dword" part */
		// if(strstr(disasm.CompleteInstr,"lea ") != NULL )

// not needed after bea and/or libdecode fixes.
#if 0
		if(disasm.getMnemonic()=="lea")
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
		//if(	strstr(disasm.CompleteInstr, "movzx ") || 
		//	strstr(disasm.CompleteInstr, "movsx ") )
		if(	disasm.getMnemonic()== "movzx" || disasm.getMnemonic()== "movsx" )
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
#if 0
/* this seems to be fixed in beaEngine -r175 */
				char* comma=strstr(disasm.CompleteInstr, ",");
				assert(comma);
				if(strstr(&comma[2], "word [") == NULL)  // if it's not a memory operation 
				{
					assert(comma[2]=='e');
					comma[2]=' ';
				}
#endif
			}
			else
				assert(0); // wtf?
		}
		// look for an fld st0, st0, and convert it to fld st0 
		else if(strcmp("fld st0 , st0", disasm.CompleteInstr)==0)
		{
			disasm.CompleteInstr[8]='\0';
		}
#endif
			
		fout<<disasm.getDisassembly();
		fout<<endl;
	}

	//for(set<Relocation_t*>::iterator it=newinsn->GetRelocations().begin(); it!=newinsn->GetRelocations().end(); ++it)
	for(auto this_reloc : newinsn->getRelocations())
	{
		//Relocation_t* this_reloc=*it;
		emit_relocation(fileIRp, fout, this_reloc->getOffset(),this_reloc->getType(), newinsn);
	}

	auto IB_targets = newinsn->getIBTargets();
	if (NULL != IB_targets) 
	{
		if (IB_targets->isComplete())
		{
			// Iterate through all IB targets and produce SPRI rules for IBTL (IB Target Limitation).
			for (auto TargIter = IB_targets->begin(); TargIter != IB_targets->end(); ++TargIter)
			{
			    fout << "\t" << labelfy(newinsn) << " IL " << qualified_labelfy(fileIRp, *TargIter) << endl;
			}
		}
	}

	return original_target;

}

//
// check to see if this instruction needs a spri rewrite rule.
//
static bool needs_spri_rule(Instruction_t* newinsn,Instruction_t* oldinsn)
{
	// check if this is an inserted instruction 
	if(newinsn->getOriginalAddressID()==BaseObj_t::NOT_IN_DATABASE)
		return true;

	assert(oldinsn);
	assert(newinsn->getOriginalAddressID()==oldinsn->getAddress()->getBaseID());


	/* We moved the instruction  to a new address*/
	if(newinsn->getAddress()->getVirtualOffset()!=oldinsn->getAddress()->getVirtualOffset())
		return true;

	/* We moved the instruction to a new file? */
	if(newinsn->getAddress()->getFileID()!=oldinsn->getAddress()->getFileID())
	{
		//	
		// coders:  verify this is OK before allowing an insn to change files. 
		//
		assert(0);
		return true;
	}


	auto newFT=newinsn->getFallthrough();
	auto newTG=newinsn->getTarget();
	auto oldFT=oldinsn->getFallthrough();
	auto oldTG=oldinsn->getTarget();

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
	if(newFT && newFT->getOriginalAddressID()!=oldFT->getAddress()->getBaseID())
		return true;
		
	// if there's a target, but it is different, return true
	if(newTG && newTG->getOriginalAddressID()!=oldTG->getAddress()->getBaseID())
		return true;

	// data bits themselves changed
	if(newinsn->getDataBits() != oldinsn->getDataBits())
		return true;

	return false;
}

//
// emit the spri rule to redirect this instruction.
//
static void emit_spri_rule(FileIR_t* fileIRp, Instruction_t* newinsn, ostream& fout, bool with_ilr)
{
	string emit_later;

	Instruction_t* old_insn=insnMap[newinsn];

	fout << endl << "# Orig addr: "<<addressify(newinsn)<<" insn_id: "<< std::dec 
	     << newinsn->getBaseID()<<" with comment "<<newinsn->getComment()<<endl;
	if (newinsn->getIndirectBranchTargetAddress())
		fout << "# Orig addr: "<<addressify(newinsn)<<" indirect branch target: "
		     <<newinsn->getIndirectBranchTargetAddress()->getVirtualOffset() << endl;

	bool redirected_addr=false;
	bool redirected_ibt=false;

	// if it's the target of an unmodified instruction 
	if( unmoved_insn_targets.find(newinsn) != unmoved_insn_targets.end() )
	{
		redirected_addr=true;
		if(old_insn)
		{
			// then we will need a rule so that the unmodified instruction gets here correctly
			fout << "# because its target of unmoved"<<endl;
			fout << qualified_addressify(fileIRp, newinsn) <<" -> ."<<endl;
		}
	}

	/* if this insn is an IB target, emit the redirect appropriately */
	if (newinsn->getIndirectBranchTargetAddress()) 
	{
		redirected_ibt=true;
		/* If the IBT address isn't this insns address, redirect appropriately.
		 * If this insn isn't an unmoved insn target, always redirect appropriately.
		 */
		// if((old_insn && (*newinsn->getIndirectBranchTargetAddress()) != (*old_insn->getAddress()))
		// 	||  !redirected_addr)
		assert(0); // couldn't make this work
		{
			// use the better qualify address to check for file matches.
			fout << "# because has indir "<<endl;
			fout << better_qualify_address(fileIRp,newinsn->getIndirectBranchTargetAddress()) 
			     <<" -> ."<<endl;
		}

		/* i don't understand this part.  hopefully this is right */
		if(!newinsn->getCallback().empty())
			fout << ". -> "<< getPostCallbackLabel(newinsn) <<endl;
	}
	// if there's a corresponding "old" instruction (i.e., in Variant 0, aka from the binary) 
	if(old_insn)
	{
		/* the address of new insns with a corresponding old insn should start with 0x */
		assert(addressify(newinsn).c_str()[0]=='0');

		/* check to see if this address an IBT somewhere else */
		/* and we havne't already redirected it */
		if (ibts.find(*old_insn->getAddress()) == ibts.end() && !redirected_ibt && !redirected_addr)
		{

			// with ILR turned off, we don't try to redirect to 0
			if(with_ilr)
			{	
				fout << "# eliding, no indirect targets"<<endl;
				fout << qualified_addressify(fileIRp, newinsn) <<" -> 0x0 " <<endl; 
			}
			else
			{
				fout << "# skipping elide because ilr is off (in this module) and "
					"no indirect targets, but emitting a rule anyhow"<<endl;
				fout << qualified_addressify(fileIRp, newinsn) <<" ->  ."  << endl;
			}
			
		}

		// not yet handling callbacks on original instructions.
		// this may be tricky because it seems we are overloading the indirect branch 
		// target address to mean two things for instructions with callbacks.  Good luck if 
		// you're reading this. 
		assert(newinsn->getCallback().empty());
		
	}

	string original_target=emit_spri_instruction(fileIRp, newinsn, fout, emit_later);


	/* if there's a fallthrough instruction, jump to it. */
	if(newinsn->getFallthrough())
	{	
		fout << ". -> " << qualified_labelfy(fileIRp,newinsn->getFallthrough())<<endl;
	}
	else
	{
		//DISASM disasm;
		//disasm.Options = NasmSyntax + PrefixedNumeral + ShowSegmentRegs;
		//disasm.Archi = fileIRp->getArchitectureBitWidth();
		//disasm.EIP = (UIntPtr)newinsn->getDataBits().c_str();
		//disasm.VirtualAddr = old_insn ? old_insn->getAddress()->getVirtualOffset() : 0;
		const auto disasm=DecodedInstruction_t(newinsn);

		/* Disassemble the instruction */
		//int instr_len = Disasm(&disasm);
		assert(disasm.valid());
		int instr_len = disasm.length();
		

		//if( disasm.Instruction.BranchType!=RetType && disasm.Instruction.BranchType!=JmpType ) 
		if( !disasm.isReturn() && !disasm.isUnconditionalBranch())
		{
			assert(old_insn);	/* it's an error to insert a new, non-unconditional branch instruction
						 * and not specify it's fallthrough */
			fout << ". -> " << qualify(fileIRp)<< "0x" << std::hex << old_insn->getAddress()->getVirtualOffset()+instr_len <<endl;
		}
	}

	fout<<endl;

	/* if the original target string is set, we need to emit 
	 * a rule for this instruction so that short branches can always be resolved 
	 */
	if(!original_target.empty())
	{
		/* qualify this target if necessary */
		if(original_target.c_str()[0]=='0')
			original_target=qualify(fileIRp)+original_target;
		fout << "\t" << get_short_branch_label(newinsn) << "\t -> \t " << original_target << endl;
	}
	fout<<emit_later<<endl;

}



//
// generate a map from new instructions to old instructions
//
static void generate_insn_to_insn_maps(FileIR_t *fileIRp, FileIR_t *orig_fileIRp)
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
		auto it=orig_fileIRp->getInstructions().begin();
		it!=orig_fileIRp->getInstructions().end();
		++it
	   )
	{
		/* get the insn */
		Instruction_t *insn=*it;
		assert(insn);

		/* get it's ID */
		db_id_t address_id=insn->getAddress()->getBaseID();
		assert(address_id!=-1);

		/* sanity check */
		assert(insn->getAddress()->getFileID()!=-1);	
		assert(insn->getAddress()->getVirtualOffset()!=0);	

		/* insert into map */
		idMap[address_id]=insn;
	}

	/* loop through the new variant and create the final mapping of new insn to old insn */
	for(
		std::set<Instruction_t*>::const_iterator it=fileIRp->GetInstructions().begin();
		it!=fileIRp->GetInstructions().end();
		++it
	   )
	{
		/* get the insn */
		Instruction_t *insn=*it;
		assert(insn);

		db_id_t orig_addr=insn->getOriginalAddressID();

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
void FileIR_t::GenerateSPRI(ostream &fout, bool with_ilr)
{
	VariantID_t orig_varidp(progid.GetOriginalVariantID());
	assert(orig_varidp.IsRegistered()==true);

	for(
		set<File_t*>::iterator it=orig_varidp.getFiles().begin();
		it!=orig_varidp.getFiles().end();
		++it
	   )
	{
			File_t* the_file=*it;

			if(the_file->getBaseID()==fileptr->orig_fid)
			{
				fout <<"# Generating spri for "<< the_file->GetURL()<<endl;

				orig_variant_ir_p=new FileIR_t(orig_varidp,the_file);
				this->GenerateSPRI(orig_variant_ir_p,fout,with_ilr);
				delete orig_variant_ir_p;
				orig_variant_ir_p=NULL;
			}
	
	}



}

//
// generate_IBT_set -- record all addresses in the program
//
static void generate_IBT_set(FileIR_t* fileIRp)
{
	ibts.clear();
	for(
		set<Instruction_t*>::const_iterator it=fileIRp->GetInstructions().begin();
		it!=fileIRp->GetInstructions().end();
		++it
	   )
	{
		Instruction_t *insn=*it;
		AddressID_t *ibt=insn->getIndirectBranchTargetAddress();

		if(ibt)
		{
			ibts.insert(*ibt);
		}

	}

}

//
// generate_unmoved_insn_targets_set --  create the set of insturctions that have control 
// transfers to them (including fallthrough type control transfers) from instructions that do not need a spri rule.
//
static void generate_unmoved_insn_targets_set(FileIR_t* fileIRp)
{
	unmoved_insn_targets.clear();
	for(
		set<Instruction_t*>::const_iterator it=fileIRp->GetInstructions().begin();
		it!=fileIRp->GetInstructions().end();
		++it
	   )
	{
		Instruction_t *insn=*it;

		if(
			// this instruction corresponds to an old instruction 
			insnMap[insn] && 
			// and we need a spri rule for it 
			!needs_spri_rule(insn, insnMap[insn])
		  )
		{
			unmoved_insn_targets.insert(insn->getTarget());
			unmoved_insn_targets.insert(insn->getFallthrough());
		}

	}

}
#endif

void FileIR_t::GenerateSPRI(FileIR_t *orig_fileIRp, ostream &fout, bool with_ilr)
{
	assert(0);
	/*
	//Resolve (assemble) any instructions in the registry.
	AssembleRegistry();

	// give 'this' a name
	FileIR_t *fileIRp=this;

	setBaseIDS(); // need unique ID to generate unique label name

	// generate the map from new instruction to old instruction needed for this transform.
	generate_insn_to_insn_maps(fileIRp, orig_fileIRp);

	// generate the set of all indirect branch target addressses for this fileIR
	generate_IBT_set(fileIRp);

	// generate unmoved_insn_targets_set --  the set of instructions that have control 
	// transfers to them (including fallthrough type control transfers) from instructions that do not need a spri rule.
	generate_unmoved_insn_targets_set(fileIRp);

	//
	// for each instruction, compare the new instruction with the original instruction and see if 
	// they are the same.  If so, do nothing, otherwise emit a rewrite rule for this instruction.
	//
	for(
		std::set<Instruction_t*>::const_iterator it=fileIRp->GetInstructions().begin();
		it!=fileIRp->GetInstructions().end();
		++it
	   )
	{
		Instruction_t* newinsn=*it;
		Instruction_t* oldinsn=insnMap[newinsn];

		assert(newinsn);

		if(needs_spri_rule(newinsn,oldinsn))
		{
			emit_spri_rule(fileIRp,newinsn,fout,with_ilr);
		}
	}
	update_label_offset(fileIRp);

	fout<<"#DEBUG: maximum ID is "<<label_offset<<endl;
	*/
}

