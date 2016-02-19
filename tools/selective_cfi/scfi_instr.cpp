/*
 * Copyright (c) 2014-2015 - Zephyr Software LLC
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


#include "utils.hpp"
#include "scfi_instr.hpp"
#include "Rewrite_Utility.hpp"
#include "color_map.hpp"
#include <stdlib.h>
#include <memory>
#include <math.h>



using namespace std;
using namespace libIRDB;

virtual_offset_t getAvailableAddress(FileIR_t *p_virp)
{

        static int counter = -16;
        counter += 16;
        return 0xf0020000 + counter;
}




static Instruction_t* addNewAssembly(FileIR_t* firp, Instruction_t *p_instr, string p_asm)
{
        Instruction_t* newinstr;
        if (p_instr)
                newinstr = allocateNewInstruction(firp,p_instr->GetAddress()->GetFileID(), p_instr->GetFunction());
        else   
                newinstr = allocateNewInstruction(firp,BaseObj_t::NOT_IN_DATABASE, NULL);

        firp->RegisterAssembly(newinstr, p_asm);

        if (p_instr)
        {
                newinstr->SetFallthrough(p_instr->GetFallthrough());
                p_instr->SetFallthrough(newinstr);
        }

        return newinstr;
}


static Instruction_t* registerCallbackHandler64(FileIR_t* firp, Instruction_t *p_orig, string p_callbackHandler, int p_numArgs)
{

        Instruction_t *instr;
        Instruction_t *first;
        char tmpbuf[1024];

        // save flags and 16 registers (136 bytes)
        // call pushes 8 bytes
        // Total: 8 * 18 = 144
        first = instr = addNewAssembly(firp,NULL, "push rsp");
        instr = addNewAssembly(firp,instr, "push rbp");
        instr = addNewAssembly(firp,instr, "push rdi");
        instr = addNewAssembly(firp,instr, "push rsi");
        instr = addNewAssembly(firp,instr, "push rdx");
        instr = addNewAssembly(firp,instr, "push rcx");
        instr = addNewAssembly(firp,instr, "push rbx");
        instr = addNewAssembly(firp,instr, "push rax");
        instr = addNewAssembly(firp,instr, "push r8");
        instr = addNewAssembly(firp,instr, "push r9");
        instr = addNewAssembly(firp,instr, "push r10");
        instr = addNewAssembly(firp,instr, "push r11");
        instr = addNewAssembly(firp,instr, "push r12");
        instr = addNewAssembly(firp,instr, "push r13");
        instr = addNewAssembly(firp,instr, "push r14");
        instr = addNewAssembly(firp,instr, "push r15");
        instr = addNewAssembly(firp,instr, "pushf");

        // handle the arguments (if any): rdi, rsi, rdx, rcx, r8, r9
        // first arg starts at byte +144
        instr = addNewAssembly(firp,instr, "mov rdi, rsp");

	if (p_numArgs >= 1)
                instr = addNewAssembly(firp,instr,  "mov rsi, [rsp+144]");
        if (p_numArgs >= 2)
                instr = addNewAssembly(firp,instr,  "mov rdx, [rsp+152]");
        if (p_numArgs >= 3)
                instr = addNewAssembly(firp,instr,  "mov rcx, [rsp+160]");
        if (p_numArgs >= 4)
                instr = addNewAssembly(firp,instr,  "mov r8, [rsp+168]");
        if (p_numArgs > 4)
                assert(0); // only handle up to 5 args

        // pin the instruction that follows the callback handler
        Instruction_t* postCallback = allocateNewInstruction(firp, BaseObj_t::NOT_IN_DATABASE, NULL);
        virtual_offset_t postCallbackReturn = getAvailableAddress(firp);
        postCallback->GetAddress()->SetVirtualOffset(postCallbackReturn);

        // push the address to return to once the callback handler is invoked
        sprintf(tmpbuf,"mov rax, 0x%x", postCallbackReturn);
        instr = addNewAssembly(firp,instr, tmpbuf);

        instr = addNewAssembly(firp,instr, "push rax");

        // use a nop instruction for the actual callback
        instr = addNewAssembly(firp,instr, "nop");
        instr->SetComment(" -- callback: " + p_callbackHandler);
        instr->SetCallback(p_callbackHandler);
        instr->SetFallthrough(postCallback);


        // need to make sure the post callback address is pinned
        // (so that ILR and other transforms do not relocate it)
        AddressID_t *indTarg = new AddressID_t();
        firp->GetAddresses().insert(indTarg);
        indTarg->SetVirtualOffset(postCallback->GetAddress()->GetVirtualOffset());
        indTarg->SetFileID(BaseObj_t::NOT_IN_DATABASE); // SPRI global namespace
        postCallback->SetIndirectBranchTargetAddress(indTarg);

        // restore registers
        firp->RegisterAssembly(postCallback, "popf");


        instr = addNewAssembly(firp,postCallback, "pop r15");
        instr = addNewAssembly(firp,instr, "pop r14");
        instr = addNewAssembly(firp,instr, "pop r13");
        instr = addNewAssembly(firp,instr, "pop r12");
        instr = addNewAssembly(firp,instr, "pop r11");
        instr = addNewAssembly(firp,instr, "pop r10");
        instr = addNewAssembly(firp,instr, "pop r9");
        instr = addNewAssembly(firp,instr, "pop r8");
        instr = addNewAssembly(firp,instr, "pop rax");
        instr = addNewAssembly(firp,instr, "pop rbx");
        instr = addNewAssembly(firp,instr, "pop rcx");
        instr = addNewAssembly(firp,instr, "pop rdx");
        instr = addNewAssembly(firp,instr, "pop rsi");
        instr = addNewAssembly(firp,instr, "pop rdi");
        instr = addNewAssembly(firp,instr, "pop rbp");
        instr = addNewAssembly(firp,instr, "lea rsp, [rsp+8]");

        instr = addNewAssembly(firp,instr, "ret");

        // return first instruction in the callback handler chain
        return first;

}


// x86-64
// 20140421
static void ConvertCallToCallbackHandler64(FileIR_t* firp, Instruction_t *p_orig, string p_callbackHandler, int p_numArgs)
{
	static std::map<std::string, Instruction_t*> m_handlerMap;
        // nb: if first time, register and cache callback handler sequence
        if (m_handlerMap.count(p_callbackHandler) == 0)
        {
                m_handlerMap[p_callbackHandler] = registerCallbackHandler64(firp,p_orig, p_callbackHandler, p_numArgs);
        }

        if (p_orig)
                p_orig->SetTarget(m_handlerMap[p_callbackHandler]);
}


static Instruction_t* addCallbackHandlerSequence
	(
	  FileIR_t* firp,
  	  Instruction_t *p_orig, 
	  bool before,
	  std::string p_detector
	)
{

	if(before)
		insertAssemblyBefore(firp,p_orig,"lea rsp, [rsp-128]");
	else
		assert(0); // add handling  for inserting lea after given insn

        p_orig->SetComment("callback: " + p_detector);


        Instruction_t* call =insertAssemblyAfter(firp,p_orig,"call 0");

        ConvertCallToCallbackHandler64(firp, call, p_detector, 0); // no args for now

	insertAssemblyAfter(firp,call,"lea rsp, [rsp + 128 + 0]"); // no args for nwo 

        return p_orig;
}

Relocation_t* SCFI_Instrument::FindRelocation(Instruction_t* insn, string type)
{
        RelocationSet_t::iterator rit;
        for( rit=insn->GetRelocations().begin(); rit!=insn->GetRelocations().end(); ++rit)
        {
                Relocation_t& reloc=*(*rit);
                if(reloc.GetType()==type)
                {
                        return &reloc;
                }
        }
        return NULL;
}




Relocation_t* SCFI_Instrument::create_reloc(Instruction_t* insn)
{
        Relocation_t* reloc=new Relocation_t;
        insn->GetRelocations().insert(reloc);
        firp->GetRelocations().insert(reloc);

	return reloc;
}






bool SCFI_Instrument::add_scfi_instrumentation(Instruction_t* insn)
{
	bool success=true;

	if(getenv("SCFI_VERBOSE")!=NULL)
		;


	return success;
}





bool SCFI_Instrument::needs_scfi_instrumentation(Instruction_t* insn)
{
	return false;

}

unsigned int SCFI_Instrument::GetNonceOffset(Instruction_t* insn)
{
	if(color_map)
	{
		assert(insn->GetIBTargets());
		return (color_map->GetColorOfIB(insn).GetPosition()+1) * GetNonceSize(insn);
	}
	return GetNonceSize(insn);
}

NonceValueType_t SCFI_Instrument::GetNonce(Instruction_t* insn)
{
	/* in time we look up the nonce category for this insn */
	/* for now, it's just f4 as the nonce */
	if(color_map)
	{
		assert(insn->GetIBTargets());
		return color_map->GetColorOfIB(insn).GetNonceValue();
	}
	return 0xf4;
}

unsigned int SCFI_Instrument::GetNonceSize(Instruction_t* insn)
{
	/* in time we look up the nonce size for this insn */
	/* for now, it's just f4 as the nonce */
	return 1;
}

bool SCFI_Instrument::mark_targets() 
{
	int targets=0, ind_targets=0;
	for(InstructionSet_t::iterator it=firp->GetInstructions().begin();
		it!=firp->GetInstructions().end();
		++it)
	{
		targets++;
		Instruction_t* insn=*it;
		if(insn->GetIndirectBranchTargetAddress())
		{
			ind_targets++;
			string type;
			if(do_coloring)
			{
				ColoredSlotValues_t v=color_map->GetColorsOfIBT(insn);
				int size=1;
				for(int i=0;i<v.size();i++)
				{
					if(!v[i].IsValid())
						continue;
					int position=v[i].GetPosition();

					// convert the colored "slot" into a position in the code.
					position++;
					position*=size;
					position = - position;

					NonceValueType_t noncevalue=v[i].GetNonceValue();
					type=string("cfi_nonce=(pos=") +  to_string(position) + ",nv="
						+ to_string(noncevalue) + ",sz="+ to_string(size)+ ")";
					Relocation_t* reloc=create_reloc(insn);
					reloc->SetOffset(-position*size);
					reloc->SetType(type);
					cout<<"Created reloc='"+type+"' for "<<std::dec<<insn->GetBaseID()<<":"<<insn->getDisassembly()<<endl;
				}
			}
			else
			{
				type="cfi_nonce=";
				type+=to_string(GetNonce(insn));

				Relocation_t* reloc=create_reloc(insn);
				reloc->SetOffset(-GetNonceOffset(insn));
				reloc->SetType(type);
				cout<<"Found nonce="+type+"  for "<<std::dec<<insn->GetBaseID()<<":"<<insn->getDisassembly()<<endl;
			}
		}
	}
	cout<<"# ATTRIBUTE ind_targets_found="<<std::dec<<ind_targets<<endl;
	cout<<"# ATTRIBUTE targets_found="<<std::dec<<targets<<endl;
	return true;
}

/*
 * targ_change_to_push - use the mode in the insnp to create a new instruction that is a push instruction.
 */
static string change_to_push(Instruction_t *insn)
{
	string newbits=insn->GetDataBits();

	DISASM d; 
	insn->Disassemble(d);

	int opcode_offset=0;


	// FIXME: assumes REX is only prefix on jmp insn.  
	// does not assume rex exists.
	if(d.Prefix.REX.state == InUsePrefix)
		opcode_offset=1;

	unsigned char modregrm = (newbits[1+opcode_offset]);
	modregrm &= 0xc7;
	modregrm |= 0x30;
	newbits[0+opcode_offset] = 0xFF;
	newbits[1+opcode_offset] = modregrm;

        return newbits;
}


void mov_reloc(Instruction_t* from, Instruction_t* to, string type )
{
	for(
		/* start */
		RelocationSet_t::iterator it=from->GetRelocations().begin();

		/* continue */
		it!=from->GetRelocations().end();
		
		/* increment */
		/* empty */
	   )
	{
		Relocation_t* reloc=*it;

		if(reloc->GetType()==type)
		{
			to->GetRelocations().insert(reloc);	
	
			// odd standards-conforming way to delete object while iterating.
			from->GetRelocations().erase(it++);	
		}
		else
		{
			it++;
		}
	}
		
}


void SCFI_Instrument::AddJumpCFI(Instruction_t* insn)
{
	if(!do_jumps)
		return;
	assert(do_rets);
	ColoredSlotValue_t v2;
	if(insn->GetIBTargets() && color_map)
		v2=color_map->GetColorOfIB(insn);
	ColoredSlotValue_t *v=&v2;
	string reg="ecx";	// 32-bit reg 
	if(firp->GetArchitectureBitWidth()==64)
		reg="rcx";	// 64-bit reg.

	string pushbits=change_to_push(insn);
	cout<<"Converting ' "<<insn->getDisassembly()<<"' to '";
	Instruction_t* after=insertDataBitsBefore(firp,insn,pushbits); 
#ifdef CGC
	// insert the pop/checking code.
	cout<<insn->getDisassembly()<<"+jmp slowpath'"<<endl;

	string jmpBits=getJumpDataBits();
        after->SetDataBits(jmpBits);
        after->SetComment(insn->getDisassembly()+" ; scfi");
	assert(do_common_slow_path); /* fixme:  this defaults to the slow_cfi path.  need to color accordingly */
	createNewRelocation(firp,after,"slow_cfi_path",0);
	after->SetFallthrough(NULL);
	after->SetTarget(after);
	return;
#else
	
	after->SetDataBits(getRetDataBits());
	cout <<insn->getDisassembly()<<" + ret' "<<endl ;

	// move any pc-rel relocation bits to the push, which will access memory now 
	mov_reloc(after,insn,"pcrel");
	after->SetIBTargets(insn->GetIBTargets());
	insn->SetIBTargets(NULL);

	AddReturnCFI(after,v);
	// cout<<"Warning, JUMPS not CFI's yet"<<endl;
	return;
#endif
}


void SCFI_Instrument::AddReturnCFI(Instruction_t* insn, ColoredSlotValue_t *v)
{

	if(!do_rets)
		return;
	ColoredSlotValue_t v2; 
	if(v==NULL && color_map)
	{
		v2=color_map->GetColorOfIB(insn);
		v=&v2;
	}


	string reg="ecx";	// 32-bit reg 
	if(firp->GetArchitectureBitWidth()==64)
		reg="r11";	// 64-bit reg.

	string rspreg="esp";	// 32-bit reg 
	if(firp->GetArchitectureBitWidth()==64)
		rspreg="rsp";	// 64-bit reg.

	string worddec="dword";	// 32-bit reg 
	if(firp->GetArchitectureBitWidth()==64)
		worddec="qword";	// 64-bit reg.

	
	DISASM d;
	insn->Disassemble(d);
	if(d.Argument1.ArgType!=NO_ARGUMENT)
	{
		unsigned int sp_adjust=d.Instruction.Immediat-firp->GetArchitectureBitWidth()/8;
		cout<<"Found relatively rare ret_with_pop insn: "<<d.CompleteInstr<<endl;
		char buf[30];
		sprintf(buf, "pop %s [%s+%d]", worddec.c_str(), rspreg.c_str(), sp_adjust);
		Instruction_t* newafter=insertAssemblyBefore(firp,insn,buf);

		if(sp_adjust>0)
		{
			sprintf(buf, "lea %s, [%s+%d]", rspreg.c_str(), rspreg.c_str(), sp_adjust);
		}

		// rewrite the "old" isntruction, as that's what insertAssemblyBefore returns
		insn=newafter;
	}
		
	int size=1;
	int position=0;
	string slow_cfi_path_reloc_string;
	if(do_coloring && !do_common_slow_path)
	{
		slow_cfi_path_reloc_string="slow_cfi_path=(pos=-1,nv=244,sz=1)";
		if( v && v->IsValid())
		{
			slow_cfi_path_reloc_string="slow_cfi_path=(pos=-"+ to_string(v->GetPosition()+1) +",nv="
						  + to_string(v->GetNonceValue())+",sz="+ to_string(size) +")";
			size=v->GetPosition();
		}
	}
	else
	{
		slow_cfi_path_reloc_string="slow_cfi_path";
	}

	
	cout<<"Cal'd slow-path cfi reloc as: "<<slow_cfi_path_reloc_string<<endl;
// fixme:  would like to mark a slow path per nonce type using the variables calc'd above.
	
	

#ifdef CGC
	// insert the pop/checking code.
	Instruction_t* after=insn;

	string jmpBits=getJumpDataBits();
	
        after->SetDataBits(jmpBits);
        after->SetComment(insn->getDisassembly()+" ; scfi");
	createNewRelocation(firp,after,slow_cfi_path_reloc_string,0);
	after->SetFallthrough(NULL);
	after->SetTarget(after);
	return;
	
#else

	string decoration="";
	int nonce_size=GetNonceSize(insn);
	int nonce_offset=GetNonceOffset(insn);
	unsigned int nonce=GetNonce(insn);
	Instruction_t* jne=NULL, *tmp=NULL;
	

	// convert a return to:
	// 	pop ecx
	// 	cmp <nonce size> PTR [ecx-<nonce size>], Nonce
	// 	jne slow	; reloc such that strata/zipr can convert slow to new code
	//			; to handle places where nonce's can't be placed. 
	// 	jmp ecx

	switch(nonce_size)
	{
		case 1:
			decoration="byte ";
			break;
		case 2:	// handle later
		case 4: // handle later
		default:
			cerr<<"Cannot handle nonce of size "<<std::dec<<nonce_size<<endl;
			assert(0);
		
	}

	// insert the pop/checking code.
	insertAssemblyBefore(firp,insn,string("pop ")+reg);
	tmp=insertAssemblyAfter(firp,insn,string("cmp ")+decoration+
		" ["+reg+"-"+to_string(nonce_offset)+"], "+to_string(nonce));
    jne=tmp=insertAssemblyAfter(firp,tmp,"jne 0");

	// convert the ret instruction to a jmp ecx
	cout<<"Converting "<<dec<<tmp->GetFallthrough()->GetBaseID()<<":"<<tmp->GetFallthrough()->getDisassembly()<<"to jmp+reg"<<endl;
	setInstructionAssembly(firp,tmp->GetFallthrough(), string("jmp ")+reg, NULL,NULL);

	// set the jne's target to itself, and create a reloc that zipr/strata will have to resolve.
	jne->SetTarget(jne);	// needed so spri/spasm/irdb don't freak out about missing target for new insn.
	Relocation_t* reloc=create_reloc(jne);
	reloc->SetType(slow_cfi_path_reloc_string); 
	reloc->SetOffset(0);
	cout<<"Setting slow path for: "<<slow_cfi_path_reloc_string<<endl;

	return;
#endif
}

static void display_histogram(std::ostream& out, std::string attr_label, std::map<int,int> & p_map)
{
	if (p_map.size()) 
	{
		out<<"# ATTRIBUTE " << attr_label << "=";
		out<<"{ibt_size:count,";
		bool first_time=true;
		for (map<int,int>::iterator it = p_map.begin(); 
			it != p_map.end(); ++it)
		{
			if (!first_time)
				out << ",";
			out << it->first << ":" << it->second;
			first_time = false;
		}
		out<<"}"<<endl;
	}
}

bool SCFI_Instrument::instrument_jumps() 
{
	int cfi_checks=0;
	int cfi_branch_jmp_checks=0;
	int cfi_branch_jmp_complete=0;
	int cfi_branch_call_checks=0;
	int cfi_branch_call_complete=0;
	int cfi_branch_ret_checks=0;
	int cfi_branch_ret_complete=0;
	int ibt_complete=0;
	double cfi_branch_jmp_complete_ratio = NAN;
	double cfi_branch_ret_complete_ratio = NAN;

	std::map<int, int> jmps;
	std::map<int, int> rets;

	// build histogram of target sizes

	// for each instruction
	for(InstructionSet_t::iterator it=firp->GetInstructions().begin();
		it!=firp->GetInstructions().end();
		++it)
	{
		Instruction_t* insn=*it;

		if(insn->GetBaseID()==BaseObj_t::NOT_IN_DATABASE)
			continue;

		// if marked safe
		if(FindRelocation(insn,"cf::safe"))
			continue;

		DISASM d;
		insn->Disassemble(d);

	
		switch(d.Instruction.BranchType)
		{
			case  JmpType:
				if((d.Argument1.ArgType&MEMORY_TYPE)==MEMORY_TYPE)
				{
					cfi_checks++;
					cfi_branch_jmp_checks++;
					if (insn->GetIBTargets() && insn->GetIBTargets()->IsComplete())
					{
						cfi_branch_jmp_complete++;
						jmps[insn->GetIBTargets()->size()]++;
					}
					AddJumpCFI(insn);
				}
				break;

			case  CallType:
				if((d.Argument1.ArgType&MEMORY_TYPE)==MEMORY_TYPE)
				{
					// not yet implemented.	
					assert(0); // fix calls should conver these to jumps
					cfi_branch_call_checks++;
					if (insn->GetIBTargets() && insn->GetIBTargets()->IsComplete())
						cfi_branch_call_complete++;
					cfi_checks++;
				}
				break;
			case  RetType: 
				cfi_branch_ret_checks++;
				if (insn->GetIBTargets() && insn->GetIBTargets()->IsComplete())
				{
					cfi_branch_ret_complete++;
					rets[insn->GetIBTargets()->size()]++;
				}
				cfi_checks++;
				AddReturnCFI(insn);
				break;

			default:
				break;
		}
	}
	
	cout<<"# ATTRIBUTE cfi_jmp_checks="<<std::dec<<cfi_branch_jmp_checks<<endl;
	cout<<"# ATTRIBUTE cfi_jmp_complete="<<std::dec<<cfi_branch_jmp_complete<<endl;

	display_histogram(cout, "cfi_jmp_complete_histogram", jmps);

/*
	cout<<"# ATTRIBUTE cfi_branch_call_checks="<<std::dec<<cfi_branch_call_checks<<endl;
	cout<<"# ATTRIBUTE cfi_branch_call_complete="<<std::dec<<cfi_branch_call_complete<<endl;
*/
	cout<<"# ATTRIBUTE cfi_ret_checks="<<std::dec<<cfi_branch_ret_checks<<endl;
	cout<<"# ATTRIBUTE cfi_ret_complete="<<std::dec<<cfi_branch_ret_complete<<endl;
	display_histogram(cout, "cfi_ret_complete_histogram", rets);

	cout<<"# ATTRIBUTE cfi_checks="<<std::dec<<cfi_checks<<endl;
	ibt_complete = cfi_branch_jmp_complete + cfi_branch_call_complete + cfi_branch_ret_complete;
	cout<<"# ATTRIBUTE ibt_complete="<<std::dec<<ibt_complete<<endl;

	if (cfi_branch_jmp_checks > 0) 
		cfi_branch_jmp_complete_ratio = (double)cfi_branch_jmp_complete / cfi_branch_jmp_checks;

	if (cfi_branch_ret_checks > 0) 
		cfi_branch_ret_complete_ratio = (double)cfi_branch_ret_complete / cfi_branch_ret_checks;

	double cfi_branch_complete_ratio = NAN;
	if (ibt_complete > 0)
		cfi_branch_complete_ratio = (double) cfi_checks / ibt_complete;
	

	cout << "# ATTRIBUTE cfi_jmp_complete_ratio=" << cfi_branch_jmp_complete_ratio << endl;

	cout << "# ATTRIBUTE cfi_ret_complete_ratio=" << cfi_branch_ret_complete_ratio << endl;
	cout << "# ATTRIBUTE cfi_complete_ratio=" << cfi_branch_ret_complete_ratio << endl;

	return true;
}


bool SCFI_Instrument::execute()
{

	bool success=true;

	if(do_coloring)
	{
		color_map=new ColoredInstructionNonces_t(firp); 
		assert(color_map);
		success = success && color_map->build();
	
	}
	
	success = success && instrument_jumps();	// to handle moving of relocs properly if
							// an insn is both a IBT and a IB,
							// we instrument first, then add relocs for targets
	success = success && mark_targets();


	delete color_map;
	color_map=NULL;

	return success;
}


