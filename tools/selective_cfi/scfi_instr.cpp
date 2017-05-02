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
#include <exeio.h>
#include <elf.h>
#include "elfio/elfio.hpp"
#include "elfio/elfio_dump.hpp"




using namespace std;
using namespace libIRDB;

virtual_offset_t getAvailableAddress(FileIR_t *p_virp)
{

        static int counter = -16;
        counter += 16;
        return 0xf0020000 + counter;
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
        sprintf(tmpbuf,"mov rax, 0x%x", (unsigned int)postCallbackReturn);
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

bool SCFI_Instrument::isSafeFunction(Instruction_t* insn)
{
	return (insn && insn->GetFunction() && insn->GetFunction()->IsSafe());
}

bool SCFI_Instrument::isCallToSafeFunction(Instruction_t* insn)
{
	if (insn && insn->GetTarget() && insn->GetTarget()->GetFunction())
	{
		if(getenv("SCFI_VERBOSE")!=NULL)
		{
			if (insn->GetTarget()->GetFunction()->IsSafe())
			{
				cout << "Function " << insn->GetTarget()->GetFunction()->GetName() << " is deemed safe" << endl;
			}
		}

		return insn->GetTarget()->GetFunction()->IsSafe();
	}

	return false;
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
assert(0);
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

			// make sure there are no fallthroughs to nonces.
			for(InstructionSet_t::iterator pred_it=preds[insn].begin(); pred_it!=preds[insn].end(); ++pred_it)
			{
				Instruction_t* the_pred=*pred_it;
				if(the_pred->GetFallthrough()==insn)
				{
					Instruction_t* jmp=addNewAssembly(firp,NULL, "jmp 0x0");
					the_pred->SetFallthrough(jmp);
					jmp->SetTarget(insn);
				}
			}

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

					// cfi_nonce=(pos=-1,nv=0x33,sz=1)
					NonceValueType_t noncevalue=v[i].GetNonceValue();
					type=string("cfi_nonce=(pos=") +  to_string(position) + ",nv="
						+ to_string(noncevalue) + ",sz="+ to_string(size)+ ")";
					Relocation_t* reloc=create_reloc(insn);
					reloc->SetOffset(-position*size);
					reloc->SetType(type);
					cout<<"Created reloc='"+type+"' for "<<std::hex<<insn->GetBaseID()<<":"<<insn->getDisassembly()<<endl;
				}
			}
			else
			{
				// cfi_nonce=f4.
				type="cfi_nonce=";
				type+=to_string(GetNonce(insn));

				Relocation_t* reloc=create_reloc(insn);
				reloc->SetOffset(-GetNonceOffset(insn));
				reloc->SetType(type);
				cout<<"Found nonce="+type+"  for "<<std::hex<<insn->GetBaseID()<<":"<<insn->getDisassembly()<<endl;
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


static void move_relocs(Instruction_t* from, Instruction_t* to)
{
	for(auto it=from->GetRelocations().begin(); it!=from->GetRelocations().end(); ) 
	{
		auto current=it++;
		Relocation_t* reloc=*current;
		if(reloc->GetType()=="fix_call_fallthrough")
		{
			// don't move it.
		}
		else
		{
			to->GetRelocations().insert(reloc);
			from->GetRelocations().erase(current);
		}
	}
}

void SCFI_Instrument::AddJumpCFI(Instruction_t* insn)
{
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
	move_relocs(after,insn);
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
	after->SetIBTargets(NULL); // lose information about ib targets.
	insn->SetIBTargets(NULL); // lose information about ib targets.
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


void SCFI_Instrument::AddCallCFIWithExeNonce(Instruction_t* insn)
{
	// make a stub to call

	// the stub is:
	//	push [target]
	// 	jmp slow
	string pushbits=change_to_push(insn);
	Instruction_t* stub=addNewDatabits(firp,NULL,pushbits);
	stub->SetComment(insn->GetComment()+" cfi stub");
	

	string jmpBits=getJumpDataBits();
	Instruction_t* jmp=insertDataBitsAfter(firp, stub, jmpBits);

	assert(stub->GetFallthrough()==jmp);

	// create a reloc so the stub goes to the slow path, eventually
	createNewRelocation(firp,jmp,"slow_cfi_path",0);
	jmp->SetFallthrough(NULL);
	jmp->SetTarget(jmp);	// looks like infinite loop, but will go to slow apth

	// convert the indirct call to a direct call to the stub.
	string call_bits=insn->GetDataBits();
	call_bits.resize(5);
	call_bits[0]=0xe8;
	insn->SetTarget(stub);
	insn->SetDataBits(call_bits);
	insn->SetComment("Direct call to cfi stub");
	insn->SetIBTargets(NULL);	// lose info about branch targets.

}

void SCFI_Instrument::AddExecutableNonce(Instruction_t* insn)
{
// this is now done by the nonce plugin's PlopDollopEntry routine.
//	insertDataBitsAfter(firp, insn, ExecutableNonceValue, NULL);
}


void SCFI_Instrument::AddReturnCFIForExeNonce(Instruction_t* insn, ColoredSlotValue_t *v)
{
	assert(!do_coloring);

	if(!do_rets)
		return;

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
		
#ifdef CGC
#if 0
	ret -> jmp shared

	shared: 
		and [rsp], 0x7ffffffff
		mov reg <- [ rsp ] 
		cmp [reg], exe_nonce_value
		jne slow
		ret

#endif

	string jmpBits=getJumpDataBits();

#ifndef CGC
	assert(0); // not ported to non-cgc mode
#endif
	if(!ret_shared)
	{
        	string clamp_str="and "+worddec+"["+rspreg+"], 0x7fffffff";
		Instruction_t* tmp=NULL;
		ret_shared=
#ifdef CGC
        	tmp=addNewAssembly(firp,tmp,clamp_str); // FIXME???
		tmp=addNewAssembly(firp, tmp, "mov "+reg+", ["+rspreg+"]");
#else
		tmp=addNewAssembly(firp, "mov "+reg+", ["+rspreg+"]");
#endif
// fixme:  get value from ExecutableNonceString -- somewhat challening
		tmp=addNewAssembly(firp, tmp, "cmp byte ["+reg+"], 0x90");
		tmp=addNewAssembly(firp, tmp, "jne 0");
		createNewRelocation(firp,tmp,"slow_cfi_path",0);
		tmp->SetTarget(tmp);
		tmp=addNewAssembly(firp, tmp, "ret");

	}
	
        insn->SetDataBits(jmpBits);
	insn->SetTarget(ret_shared);

	return;
#else
	assert(0);
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
	cout<<"Converting "<<hex<<tmp->GetFallthrough()->GetBaseID()<<":"<<tmp->GetFallthrough()->getDisassembly()<<"to jmp+reg"<<endl;
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

bool SCFI_Instrument::is_plt_style_jmp(Instruction_t* insn) 
{
	DISASM d;
	insn->Disassemble(d);
	if((d.Argument1.ArgType&MEMORY_TYPE)==MEMORY_TYPE)
	{
		if(d.Argument1.Memory.BaseRegister == 0 && d.Argument1.Memory.IndexRegister == 0)  
			return true;
		return false;
	}
	return false;
}

bool SCFI_Instrument::is_jmp_a_fixed_call(Instruction_t* insn) 
{
	if(preds[insn].size()!=1)
		return false;

	Instruction_t* pred=*(preds[insn].begin());
	assert(pred);

	if(pred->GetDataBits()[0]==0x68)
		return true;
	return false;
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
	int cfi_safefn_jmp_skipped=0;
	int cfi_safefn_ret_skipped=0;
	int cfi_safefn_call_skipped=0;
	int ibt_complete=0;
	double cfi_branch_jmp_complete_ratio = NAN;
	double cfi_branch_call_complete_ratio = NAN;
	double cfi_branch_ret_complete_ratio = NAN;

	std::map<int, int> calls;
	std::map<int, int> jmps;
	std::map<int, int> rets;

	// build histogram of target sizes

	// for each instruction
	for(InstructionSet_t::iterator it=firp->GetInstructions().begin();
		it!=firp->GetInstructions().end();
		++it)
	{
		Instruction_t* insn=*it;
		DISASM d;
		insn->Disassemble(d);


		// we always have to protect the zestcfi dispatcher, that we just added.
		if(zestcfi_function_entry==insn)
		{
			cout<<"Protecting zestcfi function for external entrances"<<endl;
			cfi_checks++;
			AddJumpCFI(insn);
			continue;
		}

		if(insn->GetBaseID()==BaseObj_t::NOT_IN_DATABASE)
			continue;

		if(string(d.Instruction.Mnemonic)==string("call ") && (protect_safefn && !do_exe_nonce_for_call))
		{
                	cerr<<"Fatal Error: Found call instruction!"<<endl;
                	cerr<<"FIX_CALLS_FIX_ALL_CALLS=1 should be set in the environment, or"<<endl;
                	cerr<<"--step-option fix_calls:--fix-all should be passed to ps_analyze."<<endl;
			exit(1);
		}

		// if marked safe
		if(FindRelocation(insn,"cf::safe"))
			continue;

		bool safefn = isSafeFunction(insn);

		if (safefn) {
			if (insn->GetFunction())
				cerr << insn->GetFunction()->GetName() << " is safe" << endl;
		}

		if (insn->GetFunction())
			cerr<<"Looking at: "<<insn->getDisassembly()<< " from func: " << insn->GetFunction()->GetName() << endl;
		else
			cerr<<"Looking at: "<<insn->getDisassembly()<< " but no associated function" << endl;
	
		
		switch(d.Instruction.BranchType)
		{
			case  JmpType:
			{
				if((d.Argument1.ArgType&CONSTANT_TYPE)!=CONSTANT_TYPE)
				{
					bool is_fixed_call=is_jmp_a_fixed_call(insn);
					bool is_plt_style=is_plt_style_jmp(insn);
					bool is_any_call_style = (is_fixed_call || is_plt_style);
					if(do_jumps && !is_any_call_style)
					{
						if (insn->GetIBTargets() && insn->GetIBTargets()->IsComplete())
						{
							cfi_branch_jmp_complete++;
							jmps[insn->GetIBTargets()->size()]++;
						}

						cfi_checks++;
						cfi_branch_jmp_checks++;

						AddJumpCFI(insn);
					}
					else if(do_calls && is_any_call_style)
					{
						if (insn->GetIBTargets() && insn->GetIBTargets()->IsComplete())
						{
							cfi_branch_call_complete++;
							calls[insn->GetIBTargets()->size()]++;
						}

						cfi_checks++;
						cfi_branch_call_checks++;
						
						AddJumpCFI(insn);
					}
					else 
					{	
						cout<<"Eliding protection for "<<insn->getDisassembly()<<std::boolalpha
							<<" is_fixed_call="<<is_fixed_call
							<<" is_plt_style="<<is_plt_style
							<<" is_any_call_style="<<is_any_call_style
							<<" do_jumps="<<do_jumps
							<<" do_calls="<<do_calls<<endl;
					}
				}
	
				break;
			}
			case  CallType:
			{

				// should only see calls if we are not CFI'ing safe functions
				// be sure to use with: --no-fix-safefn in fixcalls
				//    (1) --no-fix-safefn in fixcalls leaves call as call (instead of push/jmp)
				//    (2) and here, we don't plop down a nonce
				//    see (3) below where we don't instrument returns for safe functions
				if (!protect_safefn)
				{
					bool isDirectCall = (d.Argument1.ArgType&CONSTANT_TYPE)==CONSTANT_TYPE;

					if (safefn || (isDirectCall && isCallToSafeFunction(insn)))
					{
						cfi_safefn_call_skipped++;
						continue;
					}
				}

				AddExecutableNonce(insn);	// for all calls
				if((d.Argument1.ArgType&CONSTANT_TYPE)!=CONSTANT_TYPE)
				{
					// for indirect calls.
					AddCallCFIWithExeNonce(insn);
				}
				break;
			}
			case  RetType: 
			{
				if (insn->GetFunction())
					cerr << "found ret type  protect_safefn: " << protect_safefn << "  safefn: " << safefn <<  " function: " << insn->GetFunction()->GetName() << endl;
				else
					cerr << "found ret type  protect_safefn: " << protect_safefn << "  safefn: " << safefn << " no functions associated with instruction!! wtf???" << endl;
				if (insn->GetIBTargets() && insn->GetIBTargets()->IsComplete())
				{
					cfi_branch_ret_complete++;
					rets[insn->GetIBTargets()->size()]++;
				}

				// (3) and here, we don't instrument returns for safe function
				if (!protect_safefn && safefn)
				{
					cerr << "Skip ret instructions in function: " << insn->GetFunction()->GetName() << endl;
					cfi_safefn_ret_skipped++;
					continue;
				}

				cfi_checks++;
				cfi_branch_ret_checks++;

				if(do_exe_nonce_for_call)
					AddReturnCFIForExeNonce(insn);
				else
					AddReturnCFI(insn);
				break;

			}
			default:
			{
				break;
			}
		}
	}
	
	cout<<"# ATTRIBUTE cfi_jmp_checks="<<std::dec<<cfi_branch_jmp_checks<<endl;
	cout<<"# ATTRIBUTE cfi_jmp_complete="<<cfi_branch_jmp_complete<<endl;
	display_histogram(cout, "cfi_jmp_complete_histogram", jmps);

	cout<<"# ATTRIBUTE cfi_branch_call_checks="<<std::dec<<cfi_branch_call_checks<<endl;
	cout<<"# ATTRIBUTE cfi_branch_call_complete="<<std::dec<<cfi_branch_call_complete<<endl;
	display_histogram(cout, "cfi_call_complete_histogram", calls);

	cout<<"# ATTRIBUTE cfi_ret_checks="<<std::dec<<cfi_branch_ret_checks<<endl;
	cout<<"# ATTRIBUTE cfi_ret_complete="<<std::dec<<cfi_branch_ret_complete<<endl;
	display_histogram(cout, "cfi_ret_complete_histogram", rets);


	// 0 or 1 checks.
	cout<<"# ATTRIBUTE multimodule_checks="<< (unsigned int)(zestcfi_function_entry!=NULL) <<endl;

	cout<<"# ATTRIBUTE cfi_checks="<<std::dec<<cfi_checks<<endl;
	ibt_complete = cfi_branch_jmp_complete + cfi_branch_call_complete + cfi_branch_ret_complete;
	cout<<"# ATTRIBUTE ibt_complete="<<std::dec<<ibt_complete<<endl;

	if (cfi_branch_jmp_checks > 0) 
		cfi_branch_jmp_complete_ratio = (double)cfi_branch_jmp_complete / cfi_branch_jmp_checks;

	if (cfi_branch_call_checks > 0) 
		cfi_branch_call_complete_ratio = (double)cfi_branch_call_complete / cfi_branch_call_checks;

	if (cfi_branch_ret_checks > 0) 
		cfi_branch_ret_complete_ratio = (double)cfi_branch_ret_complete / cfi_branch_ret_checks;

	double cfi_branch_complete_ratio = NAN;
	if (ibt_complete > 0)
		cfi_branch_complete_ratio = (double) cfi_checks / ibt_complete;
	

	cout << "# ATTRIBUTE cfi_jmp_complete_ratio=" << cfi_branch_jmp_complete_ratio << endl;
	cout << "# ATTRIBUTE cfi_call_complete_ratio=" << cfi_branch_call_complete_ratio << endl;
	cout << "# ATTRIBUTE cfi_ret_complete_ratio=" << cfi_branch_ret_complete_ratio << endl;
	cout << "# ATTRIBUTE cfi_complete_ratio=" << cfi_branch_ret_complete_ratio << endl;

	cout<<"# ATTRIBUTE cfi_safefn_jmp_skipped="<<cfi_safefn_jmp_skipped<<endl;
	cout<<"# ATTRIBUTE cfi_safefn_ret_skipped="<<cfi_safefn_ret_skipped<<endl;
	cout<<"# ATTRIBUTE cfi_safefn_call_skipped="<<cfi_safefn_call_skipped<<endl;

	return true;
}

// use this to determine whether a scoop has a given name.
static struct ScoopFinder : binary_function<const DataScoop_t*,const string,bool>
{
	// declare a simple scoop finder function that finds scoops by name
	bool operator()(const DataScoop_t* scoop, const string& name) const
	{
		return (scoop->GetName() == name);
	};
} finder;

static DataScoop_t* find_scoop(FileIR_t *firp,const string &name)
{
	auto it=find_if(firp->GetDataScoops().begin(), firp->GetDataScoops().end(), bind2nd(finder, name)) ;
	if( it != firp->GetDataScoops().end() )
		return *it;
	return NULL;
};

static unsigned int  add_to_scoop(const string &str, DataScoop_t* scoop) 
{
	// assert that this scoop is unpinned.  may need to enable --step move_globals --step-option move_globals:--cfi
	assert(scoop->GetStart()->GetVirtualOffset()==0);
	int len=str.length();
	scoop->SetContents(scoop->GetContents()+str);
	virtual_offset_t oldend=scoop->GetEnd()->GetVirtualOffset();
	virtual_offset_t newend=oldend+len;
	scoop->GetEnd()->SetVirtualOffset(newend);
	return oldend+1;
};

template<int ptrsize>
static void insert_into_scoop_at(const string &str, DataScoop_t* scoop, FileIR_t* firp, const unsigned int at) 
{
	// assert that this scoop is unpinned.  may need to enable --step move_globals --step-option move_globals:--cfi
	assert(scoop->GetStart()->GetVirtualOffset()==0);
	int len=str.length();
	string new_scoop_contents=scoop->GetContents();
	new_scoop_contents.insert(at,str);
	scoop->SetContents(new_scoop_contents);

	virtual_offset_t oldend=scoop->GetEnd()->GetVirtualOffset();
	virtual_offset_t newend=oldend+len;
	scoop->GetEnd()->SetVirtualOffset(newend);

	// update each reloc to point to the new location.
	for_each(scoop->GetRelocations().begin(), scoop->GetRelocations().end(), [str,at](Relocation_t* reloc)
	{
		if(reloc->GetOffset()>=at)
			reloc->SetOffset(reloc->GetOffset()+str.size());
		
	});

	// check relocations for pointers to this object.
	// we'll update dataptr_to_scoop relocs, but nothing else
	// so assert if we find something else
	for_each(firp->GetRelocations().begin(), firp->GetRelocations().end(), [scoop](Relocation_t* reloc)
	{
		DataScoop_t* wrt=dynamic_cast<DataScoop_t*>(reloc->GetWRT());
		assert(wrt != scoop || reloc->GetType()=="dataptr_to_scoop");
	});

	// for each scoop
	for_each(firp->GetDataScoops().begin(), firp->GetDataScoops().end(), [&str,scoop,firp,at](DataScoop_t* scoop_to_update)
	{
		// for each relocation for that scoop
		for_each(scoop_to_update->GetRelocations().begin(), scoop_to_update->GetRelocations().end(), [&str,scoop,firp,scoop_to_update,at](Relocation_t* reloc)
		{
			// if it's a reloc that's wrt scoop
			DataScoop_t* wrt=dynamic_cast<DataScoop_t*>(reloc->GetWRT());
			if(wrt==scoop)
			{
				// then we need to update the scoop
				if(reloc->GetType()=="dataptr_to_scoop")
				{
					string contents=scoop_to_update->GetContents();
					// subtract the stringsize from the (implicitly stored) addend
					// taking pointer size into account.
					switch(ptrsize)
					{
						case 4:
						{
							unsigned int val=*((unsigned int*)&contents.c_str()[reloc->GetOffset()]); 
							if(val>=at)
								val +=str.size();
							contents.replace(reloc->GetOffset(), ptrsize, (const char*)&val, ptrsize);
							break;
						
						}
						case 8:
						{
							unsigned long long val=*((long long*)&contents.c_str()[reloc->GetOffset()]); 
							if(val>=at)
								val +=str.size();
							contents.replace(reloc->GetOffset(), ptrsize, (const char*)&val, ptrsize);
							break;

						}
						default: 
							assert(0);
					}
					scoop_to_update->SetContents(contents);
				}
			}	

		});
		
	});
};

template<int ptrsize>
static void prefix_scoop(const string &str, DataScoop_t* scoop, FileIR_t* firp) 
{
	insert_into_scoop_at<ptrsize>(str,scoop,firp,0);
};


template<typename T_Elf_Sym, typename T_Elf_Rela, typename T_Elf_Dyn, int reloc_type, int rela_shift, int ptrsize>
bool SCFI_Instrument::add_dl_support()
{
	bool success=true;
	success = success &&  add_libdl_as_needed_support<T_Elf_Sym,T_Elf_Rela, T_Elf_Dyn, rela_shift, reloc_type, ptrsize>();
	success = success &&  add_got_entries<T_Elf_Sym,T_Elf_Rela, T_Elf_Dyn, reloc_type, rela_shift, ptrsize>();

	return success;
}

template<typename T_Elf_Sym, typename T_Elf_Rela, typename T_Elf_Dyn, int reloc_type, int rela_shift, int ptrsize>
Instruction_t* SCFI_Instrument::find_runtime_resolve(DataScoop_t* gotplt_scoop)
{
	// find any data_to_insn_ptr reloc for the gotplt scoop
	auto it=find_if(gotplt_scoop->GetRelocations().begin(), gotplt_scoop->GetRelocations().end(), [](Relocation_t* reloc)
	{
		return reloc->GetType()=="data_to_insn_ptr";
	});
	// there _should_ be one.
	assert(it!=gotplt_scoop->GetRelocations().end());

	Relocation_t* reloc=*it;
	Instruction_t* wrt=dynamic_cast<Instruction_t*>(reloc->GetWRT());
	assert(wrt);	// should be a WRT
	assert(wrt->getDisassembly().find("push ") != string::npos);	// should be push K insn
	return wrt->GetFallthrough();	// jump to the jump, or not.. doesn't matter.  zopt will fix
}

template<typename T_Elf_Sym, typename T_Elf_Rela, typename T_Elf_Dyn, int reloc_type, int rela_shift, int ptrsize>
void SCFI_Instrument::add_got_entry(const std::string& name)
{
	// find relevant scoops
	auto dynamic_scoop=find_scoop(firp,".dynamic");
	// auto gotplt_scoop=find_scoop(firp,".got.plt");
	//auto got_scoop=find_scoop(firp,".got");
	auto dynstr_scoop=find_scoop(firp,".dynstr");
	auto dynsym_scoop=find_scoop(firp,".dynsym");
	auto relaplt_scoop=find_scoop(firp,".rela.dyn coalesced w/.rela.plt");
	auto relplt_scoop=find_scoop(firp,".rel.dyn coalesced w/.rel.plt");
	auto relscoop=relaplt_scoop!=NULL ?  relaplt_scoop : relplt_scoop;

	// add 0-init'd pointer to table
	string new_got_entry_str(ptrsize,0);	 // zero-init a pointer-sized string
	//auto dl_got_entry_pos=add_to_scoop(new_got_entry_str,gotplt_scoop);


	// create a new, unpinned, rw+relro scoop that's an empty pointer.
	AddressID_t* start_addr=new AddressID_t(BaseObj_t::NOT_IN_DATABASE, firp->GetFile()->GetBaseID(), 0);
	AddressID_t* end_addr=new AddressID_t(BaseObj_t::NOT_IN_DATABASE, firp->GetFile()->GetBaseID(), ptrsize-1);
	DataScoop_t* external_func_addr_scoop=new DataScoop_t(BaseObj_t::NOT_IN_DATABASE,
		name, start_addr,end_addr, NULL, 6, true, new_got_entry_str);

	firp->GetAddresses().insert(start_addr);
	firp->GetAddresses().insert(end_addr);
	firp->GetDataScoops().insert(external_func_addr_scoop);

	// add string to string table 
	auto dl_str_pos=add_to_scoop(name+'\0', dynstr_scoop);

	// add symbol to dlsym
	T_Elf_Sym dl_sym;
	memset(&dl_sym,0,sizeof(T_Elf_Sym));
	dl_sym.st_name=dl_str_pos;
	dl_sym.st_info=((STB_GLOBAL<<4)| (STT_OBJECT));
	string dl_sym_str((const char*)&dl_sym, sizeof(T_Elf_Sym));
	unsigned int dl_pos=add_to_scoop(dl_sym_str,dynsym_scoop);

	// find the rela count.  can't insert before that.
	int rela_count=0;
	for(int i=0;i+sizeof(T_Elf_Dyn)<dynamic_scoop->GetSize(); i+=sizeof(T_Elf_Dyn))
	{
		T_Elf_Dyn &dyn_entry=*(T_Elf_Dyn*)&dynamic_scoop->GetContents().c_str()[i];
		if(dyn_entry.d_tag==DT_RELACOUNT)	 // diff than rela size.
		{
			// add to the size
			rela_count=dyn_entry.d_un.d_val;
			break;
		}
	}

	// create the new reloc 
	T_Elf_Rela dl_rel;
	memset(&dl_rel,0,sizeof(dl_rel));
	dl_rel.r_info= ((dl_pos/sizeof(T_Elf_Sym))<<rela_shift) | reloc_type;
	string dl_rel_str((const char*)&dl_rel, sizeof(dl_rel));

// need to fixup relocs
	unsigned int at=rela_count*sizeof(T_Elf_Rela);
	insert_into_scoop_at<ptrsize>(dl_rel_str, relscoop, firp, at);

	Relocation_t* dl_reloc=new Relocation_t(BaseObj_t::NOT_IN_DATABASE,  at+((uintptr_t)&dl_rel.r_offset -(uintptr_t)&dl_rel), "dataptr_to_scoop", external_func_addr_scoop);
	relscoop->GetRelocations().insert(dl_reloc);
	firp->GetRelocations().insert(dl_reloc);

	for(int i=0;i+sizeof(T_Elf_Dyn)<dynamic_scoop->GetSize(); i+=sizeof(T_Elf_Dyn))
	{
		// cast the index'd c_str to an Elf_Dyn pointer and deref it to assign to a 
		// reference structure.  That way editing the structure directly edits the string.
		T_Elf_Dyn &dyn_entry=*(T_Elf_Dyn*)&dynamic_scoop->GetContents().c_str()[i];
		if(dyn_entry.d_tag==DT_RELASZ)
			// add to the size
			dyn_entry.d_un.d_val+=sizeof(T_Elf_Rela);

		// we insert the zest_cfi_dispatch symbol after the relative relocs.
		// but we need to adjust the start if there are no relative relocs.
		if(at == 0  && dyn_entry.d_tag==DT_RELA)
			// subtract from the start.
			dyn_entry.d_un.d_val-=sizeof(T_Elf_Rela);

	}
}

template<typename T_Elf_Sym, typename T_Elf_Rela, typename T_Elf_Dyn, int reloc_type, int rela_shift, int ptrsize>
bool SCFI_Instrument::add_got_entries()
{

	// find all the necessary scoops;
	auto dynamic_scoop=find_scoop(firp,".dynamic");
	auto gotplt_scoop=find_scoop(firp,".got.plt");
	auto got_scoop=find_scoop(firp,".got");
	auto dynstr_scoop=find_scoop(firp,".dynstr");
	auto dynsym_scoop=find_scoop(firp,".dynsym");
	auto relaplt_scoop=find_scoop(firp,".rela.dyn coalesced w/.rela.plt");
	auto relplt_scoop=find_scoop(firp,".rel.dyn coalesced w/.rel.plt");
	auto relscoop=relaplt_scoop!=NULL ?  relaplt_scoop : relplt_scoop;

	Instruction_t* to_dl_runtime_resolve=find_runtime_resolve<T_Elf_Sym,T_Elf_Rela, T_Elf_Dyn, rela_shift, reloc_type, ptrsize>(gotplt_scoop);


	// add necessary GOT entries.
	add_got_entry<T_Elf_Sym,T_Elf_Rela,T_Elf_Dyn,reloc_type,rela_shift,ptrsize>("zest_cfi_dispatch");


	// also add a zest cfi "function" that's exported so dlsym can find it.
	auto zestcfi_str_pos=add_to_scoop(string("zestcfi")+'\0', dynstr_scoop);

	// add zestcfi symbol to binary
	T_Elf_Sym zestcfi_sym;
	memset(&zestcfi_sym,0,sizeof(T_Elf_Sym));
	zestcfi_sym.st_name=zestcfi_str_pos;
	zestcfi_sym.st_size=1234;
	zestcfi_sym.st_info=((STB_GLOBAL<<4)| (STT_FUNC));
	string zestcfi_sym_str((const char*)&zestcfi_sym, sizeof(T_Elf_Sym));
	unsigned int zestcfi_pos=add_to_scoop(zestcfi_sym_str,dynsym_scoop);

	// add "function" for zestcfi"
	// for now, return that the target is allowed.  the nonce plugin will have to have a slow path for this later.
	assert(firp->GetArchitectureBitWidth()==64); // fixme for 32-bit, should jmp to ecx.
	zestcfi_function_entry=addNewAssembly(firp,NULL,"jmp r11");

	// this jump can target any IBT in the module.
	ICFS_t *newicfs=new ICFS_t;
	for_each(firp->GetInstructions().begin(), firp->GetInstructions().end(), [&](Instruction_t* insn)
	{
		if(insn->GetIndirectBranchTargetAddress() != NULL )
			newicfs->insert(insn);
	});
	zestcfi_function_entry->SetIBTargets(newicfs);
	firp->GetAllICFS().insert(newicfs);
	firp->AssembleRegistry();
	

	// add a relocation so that the zest_cfi "function"  gets pointed to by the symbol
	Relocation_t* zestcfi_reloc=new Relocation_t(BaseObj_t::NOT_IN_DATABASE,  zestcfi_pos+((uintptr_t)&zestcfi_sym.st_value - (uintptr_t)&zestcfi_sym), "data_to_insn_ptr", zestcfi_function_entry);
	dynsym_scoop->GetRelocations().insert(zestcfi_reloc);
	firp->GetRelocations().insert(zestcfi_reloc);


	// update strtabsz after got/etc entries are added.
	for(int i=0;i+sizeof(T_Elf_Dyn)<dynamic_scoop->GetSize(); i+=sizeof(T_Elf_Dyn))
	{
		T_Elf_Dyn &dyn_entry=*(T_Elf_Dyn*)&dynamic_scoop->GetContents().c_str()[i];
		if(dyn_entry.d_tag==DT_STRSZ)
		{
			dyn_entry.d_un.d_val=dynstr_scoop->GetContents().size();
		}
	}


	return true;
}

template<typename T_Elf_Sym, typename T_Elf_Rela, typename T_Elf_Dyn, int reloc_type, int rela_shift, int ptrsize>
bool SCFI_Instrument::add_libdl_as_needed_support()
{
	DataScoopSet_t::iterator it;
	// use this to determine whether a scoop has a given name.

	auto dynamic_scoop=find_scoop(firp,".dynamic");
	auto gotplt_scoop=find_scoop(firp,".got.plt");
	auto dynstr_scoop=find_scoop(firp,".dynstr");
	auto dynsym_scoop=find_scoop(firp,".dynsym");
	auto relaplt_scoop=find_scoop(firp,".rela.dyn coalesced w/.rela.plt");
	auto relplt_scoop=find_scoop(firp,".rel.dyn coalesced w/.rel.plt");


	// they all have to be here, or none are.
	assert( (dynamic_scoop==NULL && gotplt_scoop==NULL && dynstr_scoop==NULL && dynsym_scoop==NULL ) || 
		(dynamic_scoop!=NULL && gotplt_scoop!=NULL && dynstr_scoop!=NULL && dynsym_scoop!=NULL )
		);


	// not dynamic executable w/o a .dynamic section.
	if(!dynamic_scoop)
		return true;

	// may need to enable --step move_globals --step-option move_globals:--cfi
	if(relaplt_scoop == NULL && relplt_scoop==NULL)
	{
		cerr<<"Cannot find relocation-scoop pair:  Did you enable '--step move_globals --step-option move_globals:--cfi' ? "<<endl;
		exit(1);
	}

	assert(relaplt_scoop == NULL || relplt_scoop==NULL); // can't have both
	assert(relaplt_scoop != NULL || relplt_scoop!=NULL); // can't have neither


	auto libld_str_pos=add_to_scoop(string("libzestcfi.so")+'\0', dynstr_scoop);


	// a new dt_needed entry for libdl.so
	T_Elf_Dyn new_dynamic_entry;
	memset(&new_dynamic_entry,0,sizeof(new_dynamic_entry));
	new_dynamic_entry.d_tag=DT_NEEDED;
	new_dynamic_entry.d_un.d_val=libld_str_pos;
	string new_dynamic_entry_str((const char*)&new_dynamic_entry, sizeof(T_Elf_Dyn));
	// a null terminator
	T_Elf_Dyn null_dynamic_entry;
	memset(&null_dynamic_entry,0,sizeof(null_dynamic_entry));
	string null_dynamic_entry_str((const char*)&null_dynamic_entry, sizeof(T_Elf_Dyn));

	// declare an entry for the .dynamic section and add it.
	int index=0;
	while(1)
	{
		// assert we don't run off the end.
		assert((index+1)*sizeof(T_Elf_Dyn) <= dynamic_scoop->GetContents().size());

		T_Elf_Dyn* dyn_ptr=(T_Elf_Dyn*) & dynamic_scoop->GetContents().c_str()[index*sizeof(T_Elf_Dyn)];
	
		if(memcmp(dyn_ptr,&null_dynamic_entry,sizeof(T_Elf_Dyn)) == 0 )
		{
			cout<<"Inserting new DT_NEEDED at index "<<dec<<index<<endl;
			// found a null terminator entry.
			for(unsigned int i=0; i<sizeof(T_Elf_Dyn); i++)
			{
				// copy new_dynamic_entry ontop of null entry.
				dynamic_scoop->GetContents()[index*sizeof(T_Elf_Dyn) + i ] = ((char*)&new_dynamic_entry)[i];
			}

			// check if there's room for the new null entry
			if((index+2)*sizeof(T_Elf_Dyn) <= dynamic_scoop->GetContents().size())
			{
				/* yes */
				T_Elf_Dyn* next_entry=(T_Elf_Dyn*)&dynamic_scoop->GetContents().c_str()[(index+1)*sizeof(T_Elf_Dyn)];
				// assert it's actually null 
				assert(memcmp(next_entry,&null_dynamic_entry,sizeof(T_Elf_Dyn)) == 0 );
			}
			else
			{
				// add to the scoop 
				add_to_scoop(null_dynamic_entry_str,dynamic_scoop);
			}
			break;
		}

		index++;
	}

#if 0
	cout<<".dynamic contents after scfi update:"<<hex<<endl;
	const string &dynstr_contents=dynamic_scoop->GetContents();
	for(unsigned int i=0;i<dynstr_contents.size(); i+=16)
	{
		cout<<*(long long*) &dynstr_contents.c_str()[i] <<" "
		    <<*(long long*) &dynstr_contents.c_str()[i+8] <<endl;
	}
#endif

	return true;

}



bool SCFI_Instrument::execute()
{

	bool success=true;

	// this adds an instruction that needs instrumenting by future phases.
	// do not move later.
	if(do_multimodule)
	{
		if(firp->GetArchitectureBitWidth()==64)
			success = success && add_dl_support<ELFIO::Elf64_Sym, ELFIO::Elf64_Rela, ELFIO::Elf64_Dyn, R_X86_64_GLOB_DAT, 32, 8>();
		else
			success = success && add_dl_support<ELFIO::Elf32_Sym, ELFIO::Elf32_Rel, ELFIO::Elf32_Dyn, R_386_GLOB_DAT, 8, 4>();
	}


	// this selects colors and is used in instrument jumps.
	// do not move later.
	if(do_coloring)
	{
		color_map.reset(new ColoredInstructionNonces_t(firp)); 
		assert(color_map);
		success = success && color_map->build();
	
	}
	
	success = success && instrument_jumps();	// to handle moving of relocs properly if
							// an insn is both a IBT and a IB,
							// we instrument first, then add relocs for targets

	success = success && mark_targets();		// put relocs on all targets so that the backend can put nonces in place.

	return success;
}


