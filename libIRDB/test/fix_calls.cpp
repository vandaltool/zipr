

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


void fix_other_pcrel(FileIR_t* firp, Instruction_t *insn, UIntPtr offset);


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
	delete cfg;
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

/*
 * - adjust_esp_offset - take newbits, and determine if it has an esp+K type offset in a memory address.
 * if so, adjust k by offset, and return the new string.
 */
string adjust_esp_offset(string newbits, int offset)
{

        /*
         * call has opcodes of:
         *              E8 cw   call near, relative, displacement relative to next instruction
         *              E8 cd   call near, relative, displacement relative to next instruction
         *              FF /2   call near, absolute indirect, address given in r/m16
         *              FF /2   call near, absolute indirect, address given in r/m32
         *              9a cd   call far, absolute, address given in operand
         *              9a cp   call far, absolute, address given in operand
         *              FF /3   call far, absolute indirect, address given in m16:16
         *              FF /3   call far, absolute indirect, address given in m16:32
         *
         *              FF /4   jmp near, absolute indirect, address given in r/m16
         *              FF /4   jmp near, absolute indirect, address given in r/m32
         *              FF /5   jmp near, absolute indirect, address given in r/m16
         *              FF /5   jmp near, absolute indirect, address given in r/m32
         *
         *              /digit indicates that the Reg/Opcode field (bits 3-5) of the ModR/M byte (opcode[1])
         *              contains that value as an opcode instead of a register operand indicator.  The instruction only
         *              uses the R/M field (bits 0-2) as an operand.
         *
         *              cb,cw,cd,cp indicates a 1,2,4,6-byte following the opcode is used to specify a
         *                      code offset and possibly a new code segment
         *
         *      I believe we only care about these this version:
         *              FF /3   call far, absolute indirect, address given in m16:32
         *      as we're looking for an address on the stack.
         *
         *      The ModR/M byte must be Mod=10, Reg/Op=010, R/M=100 aka 0x94 or possibly
         *                              Mod=01, Reg/Op=100, R/M=100 aka 0x64 or possibly
         *                              Mod=10, Reg/Op=100, R/M=100 aka 0xa4 or possibly
         *                              Mod=01, Reg/Op=010, R/M=100 aka 0x54 or possibly
         *      R/M=100 indicates to read the SIB (see below)
         *      Mod=10 indicates an 32-bit displacement after the SIB byte
         *      Mod=01 indicates an  8-bit displacement after the SIB byte
         *      Reg/Op=010 indicates an opcode that has a /3 after it
         *      Reg/Op=100 indicates an opcode that has a /5 after it
         *
         *      The SIB byte (opcode[2]) needs to be scale=00, index=100, base=100, or 0x24
         *              indicating that the addresing mode is 0*%esp+%esp.
         *
         *      I believe that the SIB byte could also be 0x64, 0xA4, or 0xE4, but that
         *      these are very rarely used as they are redundant.
         *
         *
         *
         *
         */
        /* non-negative offsets please */
        assert(offset>=0);

        int sib_byte=(unsigned char)newbits[2];
        int sib_base=(unsigned char)sib_byte&0x7;
        int sib_ss=(sib_byte>>3)&0x7;
        int sib_index=(sib_byte>>3)&0x7;


        /* 32-bit offset */
        if ( (unsigned char)newbits[0] == 0xff &&                                	/* ff */
             ((unsigned char)newbits[1] == 0x94 || (unsigned char)newbits[1] == 0x64 )    && 		/* /2  or /4*/
             sib_base == 0x4 )                                          /* base==esp */
        {
		// reconstruct the old 32-bit value
		int oldval=((unsigned char)newbits[6]<<24)+((unsigned char)newbits[5]<<16)+((unsigned char)newbits[4]<<8)+((unsigned char)newbits[3]);

		// add the offset 
		int newval=oldval+offset;

		// break it back apart to store in the string.
		newbits[3]=(char)(newval>>0)&0xff;
		newbits[4]=(char)(newval>>8)&0xff;
		newbits[5]=(char)(newval>>16)&0xff;
		newbits[6]=(char)(newval>>24)&0xff;
        }

        /* 8-bit offset */
        else if ( (unsigned char)newbits[0] == 0xff &&                           	/* ff */
             ((unsigned char)newbits[1] == 0x54 || (unsigned char)newbits[1]==0xa4) &&    		/* /3 or /5 */
             sib_base == 0x4 )                                          /* base==esp */
        {
                /* We need to add 4 to the offset, but this may overflow an 8-bit quantity
                 * (note:  there's no 16-bit offset for this insn.  Only 8-bit or 32-bit offsets exist for this instr)
                 * if the const offset is over (0x7f-offset), adding offset will overflow it to be negative instead
                 * of positive
                 */
                if((unsigned char)newbits[3]>=(0x7f-offset))
                {
                        /* Careful, adding 4 to this will cause an overflow.
                         * We need to convert it to a 32-bit displacement
                         */
                        /* first, change addressing mode from 8-bit to 32-bit. */
                        newbits[1]&=0x3f;         /* remove upper 2 bits */
                        newbits[1]|=0x80;         /* make them 10 to indicate a 32-bit offset */

			// sign-extend to 32-bit int
			int oldval=(char)newbits[3];

			// add the offset 
			int newval=oldval+offset;
	
			// break it back apart to store in the string.
			assert(newbits.length() == 4);
			newbits[3]=(char)(newval>>0)&0xff;
			// 3 most significant bytes extend the instruction
			newbits+=(char)(newval>>8)&0xff;
			newbits+=(char)(newval>>16)&0xff;
			newbits+=(char)(newval>>24)&0xff;

                }
                else

                        newbits[3] += (char)offset;
        }
	return newbits;
}

	

/* 
 * convert_to_jump - assume newbits is a call insn, convert newbits to a jump, and return it.
 * Also: if newbits is a call [esp+k], add "offset" to k.
 */ 
static string convert_to_jump(string newbits, int offset)
{
	// on the opcode.  assume no prefix here 	
	switch((unsigned char)newbits[0])
	{
		// opcodes: ff /2 and ff /3
		case 0xff:	
		{
			// opcode: ff /2
			// call r/m32, call near, absolute indirect, address given in r/m32
			if(((((unsigned char)newbits[1])&0x38)>>3) == 2)
			{
				newbits=adjust_esp_offset(newbits,offset);
				// convert to jmp r/m32
				// opcode: FF /4
				newbits[1]&=0xC7;	// remove old bits
				newbits[1]|=(0x4<<3);	// set r/m field to 4
			}
			// opcode: ff /3
			// call m16:32, call far, absolute indirect, address given in m16:32	
			else if(((((unsigned char)newbits[1])&0x38)>>3) == 3)
			{
				// convert to jmp m16:32
				// opcode: FF /5
				newbits[1]&=0xC7;	// remove old bits
				newbits[1]|=(0x5<<3);	// set r/m field to 5
			}
			else
				assert(0);
			break;
		}
		// opcode: 9A cp
		// call ptr16:32, call far, absolute, address given in operand
		case 0x9A:	
		{
			// convert to jmp ptr16:32
			// opcode: EA cp
			newbits[0]=0xEA;
			break;
		}

		// opcode: E8 cd
		// call rel32, call near, relative, displacement relative to next insn.
		case 0xE8:
		{
			// convert to jmp rel32
			// opcode: E9 cd
			newbits[0]=0xE9;
			break;
		}

		// not a call
		default:
			assert(0);
	}
	return newbits;
}


void fix_call(Instruction_t* insn, FileIR_t *firp)
{
	/* record the possibly new indirect branch target if this call gets fixed */
	Instruction_t* newindirtarg=insn->GetFallthrough();

	/* disassemble */
        DISASM disasm;

        /* Disassemble the instruction */
        int instr_len = insn->Disassemble(disasm);


	/* if this instruction is an inserted call instruction than we don't need to 
	 * convert it for correctness' sake.
	 */
	if(insn->GetAddress()->GetVirtualOffset()==0)
		return;

	/* if the first byte isn't a call opcode, there's some odd prefixing and we aren't handling it.
	 * this comes up most frequently in a call gs:0x10 instruction where an override prefix specifes the gs: part.
	 */
	if( (insn->GetDataBits()[0]!=(char)0xff) && (insn->GetDataBits()[0]!=(char)0xe8) && (insn->GetDataBits()[0]!=(char)0x9a) )
		return;

	if(getenv("VERBOSE_FIX_CALLS"))
	{
		cout<<"Doing a fix_call on "<<std::hex<<insn->GetAddress()->GetVirtualOffset()<< " which is "<<disasm.CompleteInstr<<endl;
	}


	virtual_offset_t next_addr=insn->GetAddress()->GetVirtualOffset() + insn->GetDataBits().length();

	/* create a new instruction and a new addresss for it that do not correspond to any original program address */
	Instruction_t *callinsn=new Instruction_t();
	AddressID_t *calladdr=new AddressID_t;
       	calladdr->SetFileID(insn->GetAddress()->GetFileID());

	/* set the fields in the new instruction */
	callinsn->SetAddress(calladdr);
	callinsn->SetTarget(insn->GetTarget());
	callinsn->SetFallthrough(NULL);
	callinsn->SetFunction(insn->GetFunction());
	callinsn->SetComment(insn->GetComment()+" Jump part");

	// We need the control transfer instruction to be from the orig program because 
	// if for some reason it's fallthrough/target isn't in the DB, we need to correctly 
	// emit fallthrough/target rules
	callinsn->SetOriginalAddressID(insn->GetOriginalAddressID());
	insn->SetOriginalAddressID(BaseObj_t::NOT_IN_DATABASE);

	/* set the new instruction's data bits to be a jmp instead of a call */
	string newbits=insn->GetDataBits();

	newbits=convert_to_jump(newbits,sizeof(void*));		/* add 4 (8) if it's an esp(rsp) indirect branch for x86-32 (-64) */ 

	callinsn->SetDataBits(newbits);
	/* the jump instruction should NOT be indirectly reachable.  We should
	 * land at the push
	 */
	fix_other_pcrel(firp, callinsn, insn->GetAddress()->GetVirtualOffset());
	callinsn->SetIndirectBranchTargetAddress(NULL);

	/* add the new insn and new address into the list of valid calls and addresses */
	firp->GetAddresses().insert(calladdr);
	firp->GetInstructions().insert(callinsn);


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

	/* create a relocation for this instruction */
	Relocation_t* reloc=new Relocation_t;
	if(sizeof(void*)==4)
	{
		reloc->SetOffset(1);
		reloc->SetType("32-bit");
	}
	else
	{
		reloc->SetOffset(0);
		reloc->SetType("push64");
	}
	insn->GetRelocations().insert(reloc);
	firp->GetRelocations().insert(reloc);


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
		firp->GetAddresses().insert(newaddr);
	}

	
}


//
// return true if insn is a call
//
bool is_call(Instruction_t* insn)
{

        DISASM disasm;
        memset(&disasm, 0, sizeof(DISASM));
#if 0
        disasm.Options = NasmSyntax + PrefixedNumeral;
        disasm.Archi = 32;
        disasm.EIP = (UIntPtr)insn->GetDataBits().c_str();
        disasm.VirtualAddr = insn->GetAddress()->GetVirtualOffset();
#endif

        /* Disassemble the instruction */
        int instr_len = insn->Disassemble(disasm);
	

	return (disasm.Instruction.BranchType==CallType);
}

File_t* find_file(FileIR_t* firp, db_id_t fileid)
{
#if 0
        set<File_t*> &files=firp->GetFiles();

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
#endif
        assert(firp->GetFile()->GetBaseID()==fileid);
        return firp->GetFile();

}



//
// fix_all_calls - convert calls to push/jump pairs in the IR.  if fix_all is true, all calls are converted, 
// else we attempt to detect the calls it is safe to convert.
//
void fix_all_calls(FileIR_t* firp, bool print_stats, bool fix_all)
{


	long long fixed_calls=0, not_fixed_calls=0, not_calls=0;

	for(
		set<Instruction_t*>::const_iterator it=firp->GetInstructions().begin();
		it!=firp->GetInstructions().end(); 
		++it
	   )
	{

		Instruction_t* insn=*it;

		if(is_call(insn)) 
		{
			if(fix_all || call_needs_fix(insn))
			{
				fixed_calls++;
				fix_call(insn, firp);
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

bool arg_has_relative(const ARGTYPE &arg)
{
	/* if it's relative memory, watch out! */
	if(arg.ArgType&MEMORY_TYPE)
		if(arg.ArgType&RELATIVE_)
			return true;
	
	return false;
}


//
//  fix_other_pcrel - add relocations to other instructions that have pcrel bits
//
void fix_other_pcrel(FileIR_t* firp, Instruction_t *insn, UIntPtr virt_offset)
{
	DISASM disasm;
	insn->Disassemble(disasm);
	int is_rel= arg_has_relative(disasm.Argument1) || arg_has_relative(disasm.Argument2) || arg_has_relative(disasm.Argument3);

	/* if this has already been fixed, we can skip it */
	if(virt_offset==0 || virt_offset==-1)
		return;

	if(is_rel)
	{
		ARGTYPE* the_arg=NULL;
		if(arg_has_relative(disasm.Argument1))
			the_arg=&disasm.Argument1;
		if(arg_has_relative(disasm.Argument2))
			the_arg=&disasm.Argument2;
		if(arg_has_relative(disasm.Argument3))
			the_arg=&disasm.Argument3;

		assert(the_arg);

		int offset=the_arg->Memory.DisplacementAddr-disasm.EIP;
		assert(offset>=0 && offset <=15);
		int size=the_arg->Memory.DisplacementSize;
		assert(size==1 || size==2 || size==4 || size==8);

		if(getenv("VERBOSE_FIX_CALLS"))
		{
			cout<<"Found insn with pcrel memory operand: "<<disasm.CompleteInstr
		    	    <<" Displacement="<<std::hex<<the_arg->Memory.Displacement<<std::dec
		    	    <<" size="<<the_arg->Memory.DisplacementSize<<" Offset="<<offset;
		}

		/* convert [rip_pc+displacement] addresssing mode into [rip_0+displacement] where rip_pc is the actual PC of the insn, 
		 * and rip_0 is means that the PC=0. AKA, we are relocating this instruction to PC=0. Later we add a relocation to undo this transform at runtime 
		 * when we know the actual address.
		 */

		/* get the data */
		string data=insn->GetDataBits();
		char cstr[20]; 
		memcpy(cstr,data.c_str(), data.length());
		void *offsetptr=&cstr[offset];

		UIntPtr disp=the_arg->Memory.Displacement;
		UIntPtr oldpc=virt_offset;
		UIntPtr newdisp=disp+oldpc;

		assert(offset+size<=data.length());
		
		switch(size)
		{
			case 4:
				assert( (UIntPtr)(int)newdisp == (UIntPtr)newdisp);
				*(int*)offsetptr=newdisp;
				break;
			case 1:
			case 2:
			case 8:
			default:
				assert(("Cannot handle offset of given size", 0));
		}

		/* put the data back into the insn */
		data.replace(0, data.length(), cstr, data.length());
		insn->SetDataBits(data);
		insn->GetAddress()->SetVirtualOffset(0);	// going to end up in the SPRI file anyhow after changing the data bits 
			
		Relocation_t *reloc=new Relocation_t;
		reloc->SetOffset(0);
		reloc->SetType("pcrel");
		insn->GetRelocations().insert(reloc);
		firp->GetRelocations().insert(reloc);

		insn->Disassemble(disasm);
		if(getenv("VERBOSE_FIX_CALLS"))
			cout<<" Coverted to: "<<disasm.CompleteInstr<<endl;
	}
}

void fix_other_pcrel(FileIR_t* firp)
{

	for(
		set<Instruction_t*>::const_iterator it=firp->GetInstructions().begin();
		it!=firp->GetInstructions().end(); 
		++it
	   )
	{
		Instruction_t* insn=*it;
		fix_other_pcrel(firp,insn, insn->GetAddress()->GetVirtualOffset());
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
	FileIR_t *firp=NULL;

	/* setup the interface to the sql server */
	BaseObj_t::SetInterface(&pqxx_interface);

	cout<<"Reading variant "<<string(argv[1])<<" from database." << endl;
	try 
	{

		pidp=new VariantID_t(atoi(argv[1]));
		cout<<"Fixing calls->push/jmp in variant "<<*pidp<< "." <<endl;

		assert(pidp->IsRegistered()==true);

                for(set<File_t*>::iterator it=pidp->GetFiles().begin();
                        it!=pidp->GetFiles().end();
                        ++it
                    )
                {
                        File_t* this_file=*it;
                        assert(this_file);

			// read the db  
			firp=new FileIR_t(*pidp,this_file);
	
			assert(firp && pidp);
	
			fix_all_calls(firp,true,fix_all);
			fix_other_pcrel(firp);
			firp->WriteToDB();
			cout<<"Done!"<<endl;
			delete firp;

		}
		cout<<"Writing variant "<<*pidp<<" back to database." << endl;
		pqxx_interface.Commit();


	}
	catch (DatabaseError_t pnide)
	{
		cout<<"Unexpected database error: "<<pnide<<endl;
		exit(-1);
        }




	delete pidp;
}


