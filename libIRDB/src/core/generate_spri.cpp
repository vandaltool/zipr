

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

// forward decls for this file 
static string qualified_addressify(FileIR_t* fileIRp, Instruction_t *insn);
static string labelfy(Instruction_t* insn);


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
static int needs_short_branch_rewrite(const DISASM &disasm)
{
	return   strstr(disasm.Instruction.Mnemonic, "jecxz" ) || strstr(disasm.Instruction.Mnemonic, "loop" ) || 
		 strstr(disasm.Instruction.Mnemonic, "loopne") || strstr(disasm.Instruction.Mnemonic, "loope") ;
}


//
// create a label for the given instruction
//
static string qualified_labelfy(FileIR_t* fileIRp, Instruction_t* insn)
{
	if(!needs_spri_rule(insn, insnMap[insn]))
		return qualified_addressify(fileIRp, insn);

	return labelfy(insn);
}

static int label_offset=0;

static void update_label_offset(FileIR_t *firp)
{
	int max=0;
	for(set<Instruction_t*>::iterator it=firp->GetInstructions().begin();
            it!=firp->GetInstructions().end();
	    ++it)
	{
		Instruction_t *insn=*it;
		if(insn->GetBaseID()>max)
			max=insn->GetBaseID()+100;
	}
	label_offset+=max;
}

static int IDToSPRIID(int id)
{
	return id+label_offset;
}

static string labelfy(Instruction_t* insn)
{
	if(!needs_spri_rule(insn, insnMap[insn]))
		return addressify(insn);

	return string("LI_") + to_string(IDToSPRIID(insn->GetBaseID()));
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

static string URLToFile(string url)
{
	int loc=0;

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
	return URLToFile(fileIRp->GetFile()->GetURL()) + "+" ;
}


static string qualify_address(FileIR_t* fileIRp, int addr)
{
	stringstream ss;
	ss<< qualify(fileIRp) << "0x" << std::hex << (addr);
	return ss.str();
}

static string better_qualify_address(FileIR_t* fileIRp, AddressID_t* addr)
{
	db_id_t fileID=addr->GetFileID();
	virtual_offset_t off=addr->GetVirtualOffset();

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

static string getPostCallbackLabel(Instruction_t *newinsn)
{
	if (!newinsn)
		return string("");
	else
		return "pcb_" + labelfy(newinsn);
}


static void emit_relocation(FileIR_t* fileIRp, ostream& fout, int offset, string type, Instruction_t* insn) 
{
	fout<<"\t"<<labelfy(insn)<<" rl " << offset << " "<< type <<  " " << URLToFile(fileIRp->GetFile()->GetURL()) <<endl;
}

//
// emit this instruction as spri code.
//
static string emit_spri_instruction(FileIR_t* fileIRp, Instruction_t *newinsn, ostream& fout)
{
	string original_target;
	Instruction_t* old_insn=insnMap[newinsn];

	// disassemble using BeaEngine
	DISASM disasm;
#if 0
	memset(&disasm, 0, sizeof(DISASM));

	disasm.Options = NasmSyntax + PrefixedNumeral; //  + ShowSegmentRegs;
	disasm.Archi = 32;
	disasm.EIP = (UIntPtr)newinsn->GetDataBits().c_str();
	disasm.VirtualAddr = old_insn ? old_insn->GetAddress()->GetVirtualOffset() : 0;
#endif

	/* Disassemble the instruction */
	int instr_len = newinsn->Disassemble(disasm);


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
		if(sizeof(void*)==8)
			disasm.Archi = 64;
		else
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
		fout << "\t"+label+"\t () " << newinsn->GetCallback() << " # acts as a call <callback> insn" << endl;
		fout << "\t"+ getPostCallbackLabel(newinsn)+" ** ";
	}
	else
	{
		fout << "\t"+label+"\t ** ";
	}

	/* emit the actual instruction from the database */
	if( 
	   strstr(disasm.CompleteInstr,"jmp far")!=0 || 
	   strstr(disasm.CompleteInstr,"call far")!=0
	  )
	{
		fout<<"\t hlt " << endl;
	}

	/* if it's a branch instruction, we have extra work to do */
        else if(
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

			fout<<final<<endl;

			if (new_target.c_str()[0]=='0')
			{
				// if we're jumping to an absolute address vrs a label, we will need a relocation for this jump instruction
				if(
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
			}
		}
		else 	/* this instruction has a target, but it's not in the DB */
		{
			/* so we'll just emit the instruction and let it go back to the application text. */	
			fout<<complete_instr<<endl;
// needs relocation info.
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
					assert(strstr(fileIRp->GetFile()->GetURL().c_str(),"a.ncexe")!=0);
				}
			}
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
		fout<<endl;
	}

	for(set<Relocation_t*>::iterator it=newinsn->GetRelocations().begin(); it!=newinsn->GetRelocations().end(); ++it)
	{
		Relocation_t* this_reloc=*it;
		emit_relocation(fileIRp, fout, this_reloc->GetOffset(),this_reloc->GetType(), newinsn);
	}
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
static void emit_spri_rule(FileIR_t* fileIRp, Instruction_t* newinsn, ostream& fout, bool with_ilr)
{


	Instruction_t* old_insn=insnMap[newinsn];

	fout << endl << "# Orig addr: "<<addressify(newinsn)<<" insn_id: "<< std::dec 
	     << newinsn->GetBaseID()<<" with comment "<<newinsn->GetComment()<<endl;
	if (newinsn->GetIndirectBranchTargetAddress())
		fout << "# Orig addr: "<<addressify(newinsn)<<" indirect branch target: "
		     <<newinsn->GetIndirectBranchTargetAddress()->GetVirtualOffset() << endl;

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
	if (newinsn->GetIndirectBranchTargetAddress()) 
	{
		redirected_ibt=true;
		/* If the IBT address isn't this insns address, redirect appropriately.
		 * If this insn isn't an unmoved insn target, always redirect appropriately.
		 */
		if((old_insn && (*newinsn->GetIndirectBranchTargetAddress()) != (*old_insn->GetAddress()))
			||  !redirected_addr)
		{
			// use the better qualify address to check for file matches.
			fout << "# because has indir "<<endl;
			fout << better_qualify_address(fileIRp,newinsn->GetIndirectBranchTargetAddress()) 
		     	     <<" -> ."<<endl;
		}

		/* i don't understand this part.  hopefully this is right */
		if(!newinsn->GetCallback().empty())
			fout << ". -> "<< getPostCallbackLabel(newinsn) <<endl;
	}
	// if there's a corresponding "old" instruction (i.e., in Variant 0, aka from the binary) 
	if(old_insn)
	{
		/* the address of new insns with a corresponding old insn should start with 0x */
		assert(addressify(newinsn).c_str()[0]=='0');

		/* check to see if this address an IBT somewhere else */
		/* and we havne't already redirected it */
		if (ibts.find(*old_insn->GetAddress()) == ibts.end() && !redirected_ibt && !redirected_addr)
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
		assert(newinsn->GetCallback().empty());
		
	}

	string original_target=emit_spri_instruction(fileIRp, newinsn, fout);


	/* if there's a fallthrough instruction, jump to it. */
	if(newinsn->GetFallthrough())
	{	
		fout << ". -> " << qualified_labelfy(fileIRp,newinsn->GetFallthrough())<<endl;
	}
	else
	{
		DISASM disasm;
		disasm.Options = NasmSyntax + PrefixedNumeral + ShowSegmentRegs;
		if(sizeof(void*)==8)
			disasm.Archi = 64;
		else
			disasm.Archi = 32;
		disasm.EIP = (UIntPtr)newinsn->GetDataBits().c_str();
		disasm.VirtualAddr = old_insn ? old_insn->GetAddress()->GetVirtualOffset() : 0;

		/* Disassemble the instruction */
		int instr_len = Disasm(&disasm);

		if( disasm.Instruction.BranchType!=RetType && disasm.Instruction.BranchType!=JmpType ) 
		{
			assert(old_insn);	/* it's an error to insert a new, non-unconditional branch instruction
						 * and not specify it's fallthrough */
			fout << ". -> " << qualify(fileIRp)<< "0x" << std::hex << old_insn->GetAddress()->GetVirtualOffset()+instr_len <<endl;
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
		std::set<Instruction_t*>::const_iterator it=orig_fileIRp->GetInstructions().begin();
		it!=orig_fileIRp->GetInstructions().end();
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
		std::set<Instruction_t*>::const_iterator it=fileIRp->GetInstructions().begin();
		it!=fileIRp->GetInstructions().end();
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
void FileIR_t::GenerateSPRI(ostream &fout, bool with_ilr)
{
	VariantID_t orig_varidp(progid.GetOriginalVariantID());
	assert(orig_varidp.IsRegistered()==true);

	for(
		set<File_t*>::iterator it=orig_varidp.GetFiles().begin();
		it!=orig_varidp.GetFiles().end();
		++it
	   )
	{
                        File_t* the_file=*it;

			if(the_file->GetBaseID()==fileptr->orig_fid)
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
		AddressID_t *ibt=insn->GetIndirectBranchTargetAddress();

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
			unmoved_insn_targets.insert(insn->GetTarget());
			unmoved_insn_targets.insert(insn->GetFallthrough());
		}

	}

}


void FileIR_t::GenerateSPRI(FileIR_t *orig_fileIRp, ostream &fout, bool with_ilr)
{
	//Resolve (assemble) any instructions in the registry.
	AssembleRegistry();

	// give 'this' a name
	FileIR_t *fileIRp=this;

	SetBaseIDS(); // need unique ID to generate unique label name

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
}

