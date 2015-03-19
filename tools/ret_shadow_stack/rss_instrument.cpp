/*
 * Copyright (c) 2014 - Zephyr Software
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


#include "rss_instrument.hpp"
#include "MEDS_SafeFuncAnnotation.hpp"
#include "FuncExitAnnotation.hpp"
#include "MEDS_ProblemFuncAnnotation.hpp"
#include "Rewrite_Utility.hpp"
#include <stdlib.h>



using namespace std;
using namespace libIRDB;
using namespace MEDS_Annotation;

virtual_offset_t getAvailableAddress(FileIR_t *p_virp)
{
/*
        // traverse all instructions
        // grab address

        // @todo: lookup instruction size so that we don't waste any space
        // for some reason the max available address is incorrect! was ist los?

        virtual_offset_t availableAddressOffset = 0;
        for(   
                set<Instruction_t*>::const_iterator it=p_virp->GetInstructions().begin();
                it!=p_virp->GetInstructions().end();
                ++it
           )
        {
                Instruction_t* insn=*it;
                if (!insn) continue;

                AddressID_t* addr = insn->GetAddress();
                virtual_offset_t offset = addr->GetVirtualOffset();

                if (offset > availableAddressOffset)
                {
                        availableAddressOffset = offset;
                }
        }
// availableAddressOffset + 16;
*/

        static int counter = -16;
        counter += 16;
        return 0xf0010000 + counter;
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



static void create_tls_reloc(FileIR_t* firp, Instruction_t* insn)
{
        Relocation_t* reloc=new Relocation_t;
	reloc->SetOffset(0);
	reloc->SetType("tls_ss_start");
        insn->GetRelocations().insert(reloc);
        firp->GetRelocations().insert(reloc);
}


static 	bool add_rss_push(FileIR_t* firp, Instruction_t* insn)
{

	if(getenv("RSS_VERBOSE")!=NULL)
	{
		DISASM d; 
		insn->Disassemble(d);
		cout<<"Adding push instrumentation at 0x"<<std::hex<<insn->GetAddress()->GetVirtualOffset()
			<< " disasm="<<d.CompleteInstr <<endl;
	}

	/* this moves insn to a new instructiona fter insn, and then overwrites insn */
	insertAssemblyBefore(firp,insn,"push rax");

	/* now we insert after that insn */
	Instruction_t* tmp=insertAssemblyAfter(firp,insn,"push rcx");
	tmp=insertAssemblyAfter(firp,tmp,"mov rcx, [rsp+16]");	// load return address 
	tmp=insertAssemblyAfter(firp,tmp,"mov rax, [fs:0x12345678] ");
	create_tls_reloc(firp,tmp);
	tmp=insertAssemblyAfter(firp,tmp,"mov [rax], rcx");
	tmp=insertAssemblyAfter(firp,tmp,"lea rax, [rax+8]");
	tmp=insertAssemblyAfter(firp,tmp,"mov [fs:0x12345678], rax ");
	create_tls_reloc(firp,tmp);
	tmp=insertAssemblyAfter(firp,tmp,"pop rcx");
	tmp=insertAssemblyAfter(firp,tmp,"pop rax");


	if(getenv("tss_print_stack")!=NULL)
		addCallbackHandlerSequence (firp,tmp,true,"tss_print_stack");
	

	return true;

	
}


static 	bool add_rss_pop(FileIR_t* firp, Instruction_t* insn)
{
	Instruction_t *jmp_insn=NULL, *ret_to_app=NULL, *tmp=NULL, *pop_chk=NULL;
	Instruction_t *jmp_insn2=NULL, *hlt_insn=NULL;

	if(getenv("RSS_VERBOSE")!=NULL)
	{
		DISASM d; 
		insn->Disassemble(d);
		cout<<"Adding pop instrumentation at 0x"<<std::hex<<insn->GetAddress()->GetVirtualOffset()
			<< " disasm="<<d.CompleteInstr <<endl;
	}


	/* this moves insn to a new instructiona fter insn, and then overwrites insn */
	insertAssemblyBefore(firp,insn,"push rcx");

	/* now we insert after that insn */
	tmp=insertAssemblyAfter(firp,insn,"pushfq");
	pop_chk=tmp=insertAssemblyAfter(firp,tmp,"mov rcx, [fs:0x12345678] "); create_tls_reloc(firp,tmp);
	tmp=insertAssemblyAfter(firp,tmp,"lea rcx, [rcx-8]");
	tmp=insertAssemblyAfter(firp,tmp,"mov [fs:0x12345678], rcx "); create_tls_reloc(firp,tmp);

	/* if tss_print_stack is on, we want to zero the old location just for easy printing. */
	/* doing so requires an extra register */
	if(getenv("tss_print_stack")!=NULL)
	{
		tmp=insertAssemblyAfter(firp,tmp,"push rax");
		tmp=insertAssemblyAfter(firp,tmp,"mov rax, rcx");
	}

	// load the old value 
	tmp=insertAssemblyAfter(firp,tmp,"mov rcx, [rcx]");

	/* if tss_print_stack is on, we want to zero the old location just for easy printing. */
	if(getenv("tss_print_stack")!=NULL)
	{
		tmp=insertAssemblyAfter(firp,tmp,"mov dword [rax], 0");
		tmp=insertAssemblyAfter(firp,tmp,"pop rax");
	}
	tmp=insertAssemblyAfter(firp,tmp,"sub rcx, [rsp+16]");
	jmp_insn=tmp=insertDataBitsAfter(firp,tmp,getJecxzDataBits()); // jecxz L1

/* 
 * here we've failed the fast check, try the slow check for longjmp and exception handling.
 */
	/* reload rcx */
	tmp=insertAssemblyAfter(firp,tmp,"mov rcx, [fs:0x12345678] "); create_tls_reloc(firp,tmp);
	tmp=insertAssemblyAfter(firp,tmp,"mov rcx, [rcx]");		// reload the TOS pointer
	jmp_insn2=tmp=insertDataBitsAfter(firp,tmp,getJecxzDataBits()); // jecxz L2
/*L2*/	hlt_insn=tmp=insertAssemblyAfter(firp,tmp,"hlt");
/*L1*/	ret_to_app=
	tmp=insertAssemblyAfter(firp,tmp,"popfq");
	tmp=insertAssemblyAfter(firp,tmp,"pop rcx");

	/* link jump instruction to restore code */
	jmp_insn->SetTarget(ret_to_app);

	/* link jmp2 instruction to hlt if rcx is zero, and back to pop another value if non-zero */
	jmp_insn2->SetTarget(hlt_insn);
	jmp_insn2->SetFallthrough(pop_chk);


	/* add a call to print_stack after the push */
	if(getenv("tss_print_stack")!=NULL)
		addCallbackHandlerSequence (firp,tmp,true,"tss_print_stack");
	
	return true;
}

static bool is_exit_instruction(Instruction_t *insn, MEDS_AnnotationParser *meds_ap)
{
	DISASM d;
	insn->Disassemble(d);
	if(strstr(d.CompleteInstr,"ret")!=0)
		return true;

        assert(meds_ap);
        std::pair<MEDS_Annotations_t::iterator,MEDS_Annotations_t::iterator> ret;

	virtual_offset_t irdb_vo = insn->GetAddress()->GetVirtualOffset();
	VirtualOffset vo(irdb_vo);

        /* find it in the annotations */
        ret = meds_ap->getAnnotations().equal_range(vo);
        MEDS_FuncExitAnnotation annotation;
        MEDS_FuncExitAnnotation* p_annotation;

        /* for each annotation for this instruction */
        for (MEDS_Annotations_t::iterator it = ret.first; it != ret.second; ++it)
        {
		/* is this annotation a funcSafe annotation? */
		p_annotation=dynamic_cast<MEDS_FuncExitAnnotation*>(it->second);
		if(p_annotation==NULL)
			continue;

		annotation = *p_annotation;

		/* bad annotation? */
		if(!annotation.isValid())
			continue;
		
		return true;
        }

        /* couldn't find this insn as a function exit. */
	return false;	
}

static bool add_rss_instrumentation(FileIR_t* firp, Function_t* func, MEDS_AnnotationParser *meds_ap)
{
	bool success=true;
	if(func->GetEntryPoint()==NULL)
		return false;

	if(getenv("RSS_VERBOSE")!=NULL)
		cout<<"Transforming function "<<func->GetName()<<endl;


	for(
		set<Instruction_t*>::iterator it=func->GetInstructions().begin();
		it!=func->GetInstructions().end();
		++it
	   )
	{
		Instruction_t* insn=*it;
		if(is_exit_instruction(insn, meds_ap))
			success&=add_rss_pop(firp, insn);
	}

	/* need to do this second, as the function entry may actually change due to popping that may happen */
	success&=add_rss_push(firp, func->GetEntryPoint());

	return success;
}


static bool is_safe_func(Function_t* func, MEDS_AnnotationParser* meds_ap)
{
	assert(meds_ap);
	if(!func->GetEntryPoint())
		return false;

	std::pair<MEDS_FuncAnnotations_t::iterator,MEDS_FuncAnnotations_t::iterator> ret;

	/* find it in the annotations */
	ret = meds_ap->getFuncAnnotations().equal_range(func->GetName());
	MEDS_SafeFuncAnnotation annotation;
	MEDS_SafeFuncAnnotation* p_annotation;

	/* for each annotation for this instruction */
	for (MEDS_FuncAnnotations_t::iterator it = ret.first; it != ret.second; ++it)
	{
			/* is this annotation a funcSafe annotation? */
                        p_annotation=dynamic_cast<MEDS_SafeFuncAnnotation*>(it->second);
                        if(p_annotation==NULL)
                                continue;
			annotation = *p_annotation;

			/* bad annotation? */
			if(!annotation.isValid())
				continue;

			/* that marks the function safe? */
			if(annotation.isSafe())
				return true;
	}

	/* couldn't find the func marked as safe */
	return false;
		
}

static bool is_problem_func(Function_t* func, MEDS_AnnotationParser* meds_ap)
{
	assert(meds_ap);
	if(!func->GetEntryPoint())
		return false;


	std::pair<MEDS_FuncAnnotations_t::iterator,MEDS_FuncAnnotations_t::iterator> ret;

	/* find it in the annotations */
	ret = meds_ap->getFuncAnnotations().equal_range(func->GetName());
	MEDS_ProblemFuncAnnotation annotation;
	MEDS_ProblemFuncAnnotation* p_annotation;

	/* for each annotation for this instruction */
	for (MEDS_FuncAnnotations_t::iterator it = ret.first; it != ret.second; ++it)
	{
			/* is this annotation a funcSafe annotation? */
                        p_annotation=dynamic_cast<MEDS_ProblemFuncAnnotation*>(it->second);
                        if(p_annotation==NULL)
                                continue;
			annotation = *p_annotation;

			/* bad annotation? */
			if(!annotation.isValid())
				continue;

			/* that marks the function safe? */
			return true;
	}

	/* couldn't find the func marked as safe */
	return false;
		
}

static int safe_funcs=0,problem_funcs=0, instr_funcs=0;

static bool needs_rss_instrumentation(Function_t* func, MEDS_AnnotationParser* meds_ap)
{
	if(is_safe_func(func,meds_ap))
	{
		safe_funcs++;
		return false; // safe functions need no instrumentation
	}

	if(is_problem_func(func,meds_ap))
	{
		problem_funcs++;
		return false; // problem funcs can't have instrumentation
	}


	/* otherwise, we need to instrument */
	instr_funcs++;
	return true;

}

bool RSS_Instrument::execute()
{

	bool success=false;

	for(set<Function_t*>::iterator it=firp->GetFunctions().begin(); 
		it!=firp->GetFunctions().end(); 
		++it
	   )
	{
		Function_t* func=*it;
		if(needs_rss_instrumentation(func,meds_ap))
		{
			cout<<"Function "<<func->GetName()<<" gets instrumentation!";
			if(func->GetEntryPoint())
				cout<<"( "<<std::hex<<func->GetEntryPoint()->GetAddress()->GetVirtualOffset()<<")";
			cout<<endl;
			success|=add_rss_instrumentation(firp,func, meds_ap);
		}
		else
		{
			cout<<"Function "<<func->GetName()<<" no instrumentation!\n";
		}
		
	}

	cout << "# ATTRIBUTE safe_funcs=" <<std::dec<<safe_funcs<<endl;
	cout << "# ATTRIBUTE problem_funcs=" <<problem_funcs<<endl;
	cout << "# ATTRIBUTE instr_funcs=" <<instr_funcs<<endl;

	/* return an exit code */
	if(success)
		return 0; /* success? */
	
	return 1;
}


