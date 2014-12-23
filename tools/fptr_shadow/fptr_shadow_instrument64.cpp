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

/*
 * @todo: 
 *      insertAssemblyBefore()
 *      insertAssemblyAfter()
 *      design callbacks for def/check shadow
 */
#include <stdlib.h>

#include <libIRDB-core.hpp>
#include "MEDS_FPTRShadowAnnotation.hpp"
#include "fptr_shadow_instrument64.hpp"

#include "Rewrite_Utility.hpp"

using namespace std;
using namespace libIRDB;
using namespace MEDS_Annotation;

#define CALLBACK_FPTR_SHADOW_DEFINE_64 "fptr_shadow_define_64"
#define CALLBACK_FPTR_SHADOW_CHECK_64  "fptr_shadow_check_64"

#define BASE_SPRI_ADDRESS 0xf00f0000
static virtual_offset_t getAvailableAddress(FileIR_t *p_virp)
{
        static int counter = -16;
        counter += 16;
        return BASE_SPRI_ADDRESS + counter;
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
  	  Instruction_t *p_insn, 
	  std::string p_detector,
	  int p_numArgs
	)
{
	Instruction_t* call =insertAssemblyAfter(firp,p_insn,"call 0");

	ConvertCallToCallbackHandler64(firp, call, p_detector, p_numArgs); 

	return call;
}

// #define CALLBACK_FPTR_SHADOW_DEFINE_64 "fptr_shadow_define_64"
// #define CALLBACK_FPTR_SHADOW_CHECK_64  "fptr_shadow_check_64"
static Instruction_t* addShadowDefine64CallbackHandler(FileIR_t* p_firp, Instruction_t* p_insn, const MEDS_FPTRShadowAnnotation* p_annot)
{
	char tmp[1024];
	// need to push the 3 args: PC, shadowId, shadowValue
	// rsi <- PC
	// rdx <- shadowId
	// rcx <- value
	const int numArgs = 3;
	int shadowId = p_annot->getShadowId();
	
	Instruction_t* originalFallthrough = p_insn->GetFallthrough();

	// red zone + flags
	Instruction_t* lea = insertAssemblyBefore(p_firp,p_insn,"lea rsp, [rsp-128]");
	p_insn->SetComment("callback: " + string(CALLBACK_FPTR_SHADOW_DEFINE_64));
    Instruction_t* saveFlags = addNewAssembly(p_firp, lea, "pushf");

	// push the 3 arguments
	// arg#1: push PC
	unsigned PC = p_insn->GetAddress()->GetVirtualOffset(); 
	sprintf(tmp,"push 0x%016x", PC);
    Instruction_t* i = insertAssemblyAfter(p_firp, saveFlags, tmp);
	cout << "addShadowDefine(): PC: " << hex << PC << dec << endl;

	// arg#2: push shadowId
	sprintf(tmp,"push 0x%016x", shadowId);
    i = insertAssemblyAfter(p_firp, i, tmp);
	cout << "addShadowDefine(): shadowId: " << shadowId << endl;

	// arg#3: push shadow value given in [reg+offset]
	const int registerOffset = p_annot->getRegisterOffset();
	const char *reg = Register::toString(p_annot->getRegister()).c_str();
	if (registerOffset == 0)
		sprintf(tmp,"push [%s]", reg);
	else if (registerOffset > 0)
		sprintf(tmp,"push qword [%s+%d]", reg, registerOffset);
	else
		sprintf(tmp,"push qword [%s%d]", reg, registerOffset);
    i = insertAssemblyAfter(p_firp, i, tmp);
	cout << "addShadowDefine(): reg+offset: " << tmp << endl;

	// call the callback handler sequence
	Instruction_t* ch = addCallbackHandlerSequence(p_firp, i, CALLBACK_FPTR_SHADOW_DEFINE_64, numArgs);

	// pop the args
	sprintf(tmp, "lea rsp, [rsp+%d]", numArgs * 8);
	i = insertAssemblyAfter(p_firp, ch, tmp);  
	// restore flags
	i = insertAssemblyAfter(p_firp, i, "popf");  
	// undo the red zone
	i = insertAssemblyAfter(p_firp, i, "lea rsp, [rsp+128]");  

	i->SetFallthrough(originalFallthrough);

	return lea;
}

static Instruction_t* addShadowCheck64CallbackHandler(FileIR_t* p_firp, Instruction_t *p_orig, MEDS_FPTRShadowAnnotation* p_annot)
{
	// need to push the 4 args: PC, shadowId, shadowValue, (OUT) success
	return NULL;
}

//--------------------------------------------------------------
// Start FPTRShadow_instrument64
//--------------------------------------------------------------

FPTRShadow_Instrument64::FPTRShadow_Instrument64(libIRDB::FileIR_t *p_firp, MEDS_AnnotationParser* p_ap) : m_firp(p_firp), m_annotationParser(p_ap)
{
}

MEDS_Annotations_t& FPTRShadow_Instrument64::getAnnotations() 
{ 
	return m_annotationParser->getAnnotations(); 
}

bool FPTRShadow_Instrument64::execute()
{
///	FunctionSet_t func = m_firp->GetFunctions();
	cout << "size of annotation set: " << getAnnotations().size() << endl;

	InstructionSet_t func = m_firp->GetInstructions();
	for(
	  set<Instruction_t*>::const_iterator it=func.begin();
	  it!=func.end();
	  ++it)
	{
		Instruction_t* insn=*it;

		if (insn && insn->GetAddress())
		{
			virtual_offset_t irdb_vo = insn->GetAddress()->GetVirtualOffset();
			if (irdb_vo == 0) continue;


			VirtualOffset vo(irdb_vo);

			cout << endl << "Handling instruction: " << vo.toString() << " " << insn->getDisassembly() << endl;
			if (getAnnotations().count(vo) == 0)
			{
				continue;
			}

			cout << "   found annotation: " << endl;

			std::pair<MEDS_Annotations_t::iterator,MEDS_Annotations_t::iterator> ret;
			ret = getAnnotations().equal_range(vo);
			MEDS_FPTRShadowAnnotation* annotation; 
			for ( MEDS_Annotations_t::iterator it = ret.first; it != ret.second; ++it)
			{
				MEDS_AnnotationBase *base_type=(it->second);
				if (base_type)
					cout << "base annotation: " << base_type->toString() << endl;
				annotation = dynamic_cast<MEDS_FPTRShadowAnnotation*>(base_type);
				if(annotation && annotation->isValid()) 
					break; // just handle one annotation for now 
			}

			bool success = false;
			if (!annotation)
			{
				cout << "no FPTR annotation found" << endl;						
				continue;
			}

			// here we have a valid annotation, what's the type?
			if (annotation->isDefineShadowId())
			{
				cout << "would add shadow entry: " << annotation->toString();
				success = addShadowEntry(insn, annotation);
			}
			else if (annotation->isCheckShadowId())
			{
				cout << "would check shadow entry: " << annotation->toString();
				cout << "not yet implemented -- skip";
				continue;
				success = checkShadowEntry(insn, annotation);
			}
			else
			{
				// error
				cerr << "bad annotation: " << annotation->toString();
				continue;
			}

			// something went wrong with an instrumentation, get out of here
			if (!success) 
				return false;
		}
	}

	return true;
}

// refactor into utility library
//
//     insertBefore();
//     insertCallbackHandlerBefore();
//	Instruction_t* tmp=insertAssemblyAfter(firp,insn,"push rcx");
//	tmp=insertAssemblyAfter(firp,tmp,"mov rcx, [rsp+16]");	// load return address 
// 

bool FPTRShadow_Instrument64::addShadowEntry(Instruction_t *p_insn, const MEDS_FPTRShadowAnnotation *p_annot)
{
	addShadowDefine64CallbackHandler(m_firp, p_insn, p_annot);
	return true;
}

bool FPTRShadow_Instrument64::checkShadowEntry(Instruction_t *p_insn, const MEDS_FPTRShadowAnnotation *p_annot)
{
	return true;
}
