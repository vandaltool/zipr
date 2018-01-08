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


#include <stdlib.h>
#include <cmath>

#include "utils.hpp"
#include "scdi_instr.hpp"
#include "Rewrite_Utility.hpp"

using namespace std;
using namespace libIRDB;

virtual_offset_t getAvailableAddress(FileIR_t *p_virp)
{

        static int counter = -16;
        counter += 16;
        return 0xf0020000 + counter;
}

template< typename T >
std::string int_to_hex_string( T i )
{
  std::stringstream stream;
  stream << "0x" 
         << std::hex << i;
  return stream.str();
}



#if 0
// moved to Rewrite_Utility.cpp
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
#endif

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




bool SimpleCDI_Instrument::add_scdi_instrumentation(Instruction_t* insn)
{
	bool success=true;

	if(getenv("SimpleCDI_VERBOSE")!=NULL)
	{
		cout<<"Found that "<<insn->GetBaseID()<<":"<<insn->getDisassembly()<<" can be converted to CDI"<<endl;
	}

	ICFS_t* ibts=insn->GetIBTargets();
	DISASM d;
	insn->Disassemble(d);

	if(getenv("SimpleCDI_VERBOSE")!=NULL && ibts)
	{
		cout <<"["<<string(d.CompleteInstr)<<"] [" << string(d.Instruction.Mnemonic)<< "] IBTargets size: " << ibts->size() << " analysis_status: " << ibts->GetAnalysisStatus() << endl;
	}

	if (is_return(insn))
	{
		// instrumentation must be coordinated with needs_scdi_instrumentation()
		if (ibts && ibts->IsComplete() && ibts->size() == 1)
		{
			Instruction_t *return_site = NULL;
			for(ICFS_t::iterator it=ibts->begin(); it!=ibts->end(); ++it)
			{
				return_site=*it; 
			}

			Instruction_t *ret;
			if (firp->GetArchitectureBitWidth() == 64)
				ret = insertAssemblyBefore(firp,insn,"lea rsp, [rsp+8]");
			else if (firp->GetArchitectureBitWidth() == 32)
				ret = insertAssemblyBefore(firp,insn,"lea esp, [esp+4]");
			else
				assert(0);
	
			ret->Assemble("jmp 0");
			ret->SetFallthrough(NULL);
			ret->SetTarget(return_site);
			ret->SetIBTargets(NULL);

			cout<<hex<<insn->GetAddress()->GetVirtualOffset();
			cout<<": Converted ret into a direct jmp: "<<insn->getDisassembly();
			cout<<"   jmp back to: "<<ret->GetTarget()->GetAddress()->GetVirtualOffset()<<": "<<ret->GetTarget()->getDisassembly()<<dec<<endl;
			single_target_set_returns++;
			return true;
		}
	}

	assert(strstr("ret ", d.Instruction.Mnemonic)==NULL);
	assert(strstr("retn ", d.Instruction.Mnemonic)==NULL);
	
	// pre-instrument
	// push reg
	// mov reg, <target>
	string reg="rcx";
	string addr_mode=(strstr(d.CompleteInstr," "));

	Instruction_t* tmp=insn;
	insertAssemblyBefore(firp,tmp,"push "+reg);
	tmp=insertAssemblyAfter(firp,tmp,"mov "+reg+", "+addr_mode);

	for(ICFS_t::iterator it=ibts->begin(); it!=ibts->end(); ++it)
	{
		Instruction_t* target=*it; 
		// add:  
		//	<t> pop reg  -> fallthrough to <target>
		// insert before:
		//	cmp reg, <target>; 
		//	je <t>
		assert(target && target->GetIndirectBranchTargetAddress() 
			&& target->GetIndirectBranchTargetAddress()->GetVirtualOffset());
	
		if(getenv("SimpleCDI_VERBOSE")!=NULL)
			cout<<"Adding check for "<<hex<<target->GetIndirectBranchTargetAddress()->GetVirtualOffset()<<endl;

		Instruction_t *t=addNewAssembly(firp,NULL, string("pop ")+reg);
		t->SetFallthrough(target);

		tmp=insertAssemblyAfter(firp,tmp, "cmp "+reg+", "+ 
			int_to_hex_string(target->GetIndirectBranchTargetAddress()->GetVirtualOffset()));
		tmp=insertAssemblyAfter(firp,tmp,"je 0x0",t); 
	}

	// add hlt instrution and/or controlled exit callback.
	tmp=insertAssemblyAfter(firp,tmp,"hlt"); 
	
	// leave original instruction, because i'm lazy.
	return success;
}

bool SimpleCDI_Instrument::is_return(Instruction_t* insn)
{
	if (insn) 
	{
		DISASM d;
		insn->Disassemble(d);
		return string(d.Instruction.Mnemonic) == string("ret "); 

		// FIXME: handle retn immd, but this means the instrumentation should pop/lea immd
	/*	return (string(d.Instruction.Mnemonic) == string("ret ") ||
		    string(d.Instruction.Mnemonic) == string("retn "));
	*/
	}

	return false;
}

// only complete returns need to be instrumented
bool SimpleCDI_Instrument::needs_scdi_instrumentation(Instruction_t* insn, int target_size_threshold)
{
	const bool isReturn = is_return(insn);

	if (isReturn)
		num_returns++;

	ICFS_t* ibts=insn->GetIBTargets();
	if(!ibts)
		return false;

	if (ibts->IsComplete() && ibts->size() > 0)
	{
		num_complete_ibts++;
		if (isReturn)
			num_complete_returns++;
	}

	if (isReturn)
	{
		if (ibts->IsComplete())
		{
			if (target_set_threshold < 0)
				return true;
			else 
				return ibts->size() <= target_size_threshold;
		}
		else 
			return false;
	}

	return false;
}

bool SimpleCDI_Instrument::convert_ibs()
{
	bool success=true;

        // we do this in two passes.  first pass:  find instructions.
        for(InstructionSet_t::iterator it=firp->GetInstructions().begin();
                it!=firp->GetInstructions().end();
                ++it)
        {
		Instruction_t* insn=*it;
		if(needs_scdi_instrumentation(insn, target_set_threshold))
			success = success && add_scdi_instrumentation(insn);
	}

	return success;
}

void SimpleCDI_Instrument::display_stats(std::ostream &out)
{
	float fraction = NAN;
	out << "# ATTRIBUTE Simple_Control_Data_Integrity::target_set_threshold=" << dec << target_set_threshold << endl;
	out << "# ATTRIBUTE Simple_Control_Data_Integrity::complete_ibts=" << dec << num_complete_ibts << endl;
	out << "# ATTRIBUTE Simple_Control_Data_Integrity::num_returns=" << num_returns << endl;
	if (num_complete_returns>0)
		fraction = (float)num_complete_returns/num_returns;
	out << "# ATTRIBUTE Simple_Control_Data_Integrity::num_complete_returns=" << num_complete_returns << endl;
	out << "# ATTRIBUTE Simple_Control_Data_Integrity::complete_returns_fraction=" << fraction << endl;
	out << "# ATTRIBUTE Simple_Control_Data_Integrity::complete_returns_pct=" << fraction*100.00<<"%" << endl;
	out << "# ATTRIBUTE Simple_Control_Data_Integrity::single_target_set_jumps=" << single_target_set_jumps << endl;
	out << "# ATTRIBUTE Simple_Control_Data_Integrity::single_target_set_returns=" << single_target_set_returns << endl;

	fraction = NAN;
	if (num_complete_ibts > 0)
		fraction = (float)(single_target_set_returns)/num_returns;
	out << "# ATTRIBUTE Simple_Control_Data_Integrity::single_target_set_return_fraction=" << fraction << endl;
	out << "# ATTRIBUTE Simple_Control_Data_Integrity::single_target_set_return_pct=" << fraction*100.00<<"%" << endl;
}

/* CDI: control data isolation */
bool SimpleCDI_Instrument::execute()
{

	bool success=true;

	success = success && convert_ibs();

	display_stats(cout);

	return success;
}


