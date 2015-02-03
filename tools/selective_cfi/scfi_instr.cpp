
#include "utils.hpp"
#include "scfi_instr.hpp"
#include "Rewrite_Utility.hpp"
#include <stdlib.h>



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


unsigned int SCFI_Instrument::GetNonce(Instruction_t* insn)
{
	/* in time we look up the nonce category for this insn */
	/* for now, it's just f4 as the nonce */
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
			type="cfi_nonce=";
			type+=to_string(GetNonce(insn));

			Relocation_t* reloc=create_reloc(insn);
			reloc->SetOffset(-GetNonceSize(insn));
			reloc->SetType(type);
			cout<<"Found indtarget  for "<<std::dec<<insn->GetBaseID()<<":"<<insn->GetComment()<<endl;
		}
	}
	cout<<"#ATTRIBUTE ind_targets_found="<<std::dec<<ind_targets<<endl;
	cout<<"#ATTRIBUTE targets_found="<<std::dec<<targets<<endl;
	return true;
}

void SCFI_Instrument::AddReturnCFI(Instruction_t* insn)
{
	string reg="ecx";	// 32-bit reg 
	if(firp->GetArchitectureBitWidth()==64)
		reg="rcx";	// 64-bit reg.

#ifdef CGC
	// insert the pop/checking code.
//	Instruction_t* after=insertAssemblyBefore(firp,insn,string("pop ")+reg);
	Instruction_t* after=insn;

	string jmpBits=getJumpDataBits();
	
        after->SetDataBits(jmpBits);
        after->SetComment(insn->getDisassembly()+" ; scfi");
	createNewRelocation(firp,after,"slow_cfi_path",0);
	after->SetFallthrough(NULL);
	after->SetTarget(after);
	return;
	
#else

	string decoration="";
	int nonce_size=GetNonceSize(insn);
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
		" ["+reg+"-"+to_string(nonce_size)+"], "+to_string(nonce));
    jne=tmp=insertAssemblyAfter(firp,tmp,"jne 0");

	// convert the ret instruction to a jmp ecx
	cout<<"Converting "<<dec<<tmp->GetFallthrough()->GetBaseID()<<":"<<tmp->GetFallthrough()->getDisassembly()<<"to jmp+reg"<<endl;
	setInstructionAssembly(firp,tmp->GetFallthrough(), string("jmp ")+reg, NULL,NULL);

	// set the jne's target to itself, and create a reloc that zipr/strata will have to resolve.
	jne->SetTarget(jne);	// needed so spri/spasm/irdb don't freak out about missing target for new insn.
	Relocation_t* reloc=create_reloc(jne);
	reloc->SetType("slow_cfi_path");
	reloc->SetOffset(0);

	return;
#endif
}

bool SCFI_Instrument::instrument_jumps() 
{
	InstructionSet_t to_instrument;

	// we do this in two passes.  first pass:  find instructions.
	for(InstructionSet_t::iterator it=firp->GetInstructions().begin();
		it!=firp->GetInstructions().end();
		++it)
	{
		Instruction_t* insn=*it;
		DISASM d;
		insn->Disassemble(d);
	
		switch(d.Instruction.BranchType)
		{
			case  JmpType:
			case  CallType:
				// not yet implemented.
				break;
			case  RetType: 
				to_instrument.insert(insn);
				break;

			default:
				break;
		}
	}
	

	int cfi_checks=0;

	// second pass, insert checking.  can't do this on pass one because
	// we add IBs, which we'd later decide instrument
	for(InstructionSet_t::iterator it=to_instrument.begin();
		it!=to_instrument.end();
		++it)
	{
		cfi_checks++;
		Instruction_t* insn=*it;
		AddReturnCFI(insn);
	}
	cout<<"#ATTRIBUTE cfi_checks="<<std::dec<<cfi_checks<<endl;
	return true;
}


bool SCFI_Instrument::execute()
{

	bool success=true;

	success = success && instrument_jumps();	// to handle moving of relocs properly if
							// an insn is both a IBT and a IB,
							// we instrument first, then add relocs for targets
	success = success && mark_targets();

	return success;
}


