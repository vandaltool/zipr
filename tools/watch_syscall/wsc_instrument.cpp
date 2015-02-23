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


#include "wsc_instrument.hpp"
#include "Rewrite_Utility.hpp"
#include <stdlib.h>
#include <string>
#include <iostream>



using namespace std;
using namespace libIRDB;

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



bool WSC_Instrument::add_wsc_instrumentation(Instruction_t *site)
{

//cout<<"Found syscall to instrument "<<site->getDisassembly()<<endl;

        virtual_offset_t postCallbackReturn = getAvailableAddress(firp);
	char tmpbuf[100];
        sprintf(tmpbuf,"push  0x%x", postCallbackReturn);

	Instruction_t *tmp=site, *callback=NULL, *post_callback=NULL;
        tmp=insertAssemblyAfter(firp,tmp,"lea esp, [esp-4096]");
        tmp=insertAssemblyAfter(firp,tmp,"pushf");
        tmp=insertAssemblyAfter(firp,tmp,"pusha");
        tmp=insertAssemblyAfter(firp,tmp,tmpbuf);	 // push <ret addr>
        callback=tmp=insertAssemblyAfter(firp,tmp,"nop");
        post_callback=tmp=insertAssemblyAfter(firp,tmp,"popa");
        tmp=insertAssemblyAfter(firp,tmp,"popf");
        tmp=insertAssemblyAfter(firp,tmp,"lea esp, [esp+4096]");
        post_callback->GetAddress()->SetVirtualOffset(postCallbackReturn);
	callback->SetCallback("zipr_post_allocate_watcher");
	return true;
}

bool WSC_Instrument::add_init_call()
{

	Instruction_t* insn=NULL;
	for(InstructionSet_t::iterator it=firp->GetInstructions().begin();
		it!=firp->GetInstructions().end();
		++it
	   )
	{
		insn=*it;
		if(insn->GetIndirectBranchTargetAddress() && 
			insn->GetIndirectBranchTargetAddress()->GetVirtualOffset()==(virtual_offset_t)elfiop->get_entry())
			break;
		
	}

	insertAssemblyBefore(firp,insn,"nop");

	for ( int i = 0; i < elfiop->sections.size(); ++i )
	{

        	int flags = elfiop->sections[i]->get_flags();
        	/* not a loaded section */
        	if( (flags & SHF_ALLOC) != SHF_ALLOC)
                	continue;
        	if( (flags & SHF_EXECINSTR) == SHF_EXECINSTR)
                	continue;

        	int first=elfiop->sections[i]->get_address();
        	int second=elfiop->sections[i]->get_address()+elfiop->sections[i]->get_size();

		cout<<"Adding "<<first <<"-"<<second<<" as range"<<endl;

        	virtual_offset_t postCallbackReturn = getAvailableAddress(firp);
		char tmpbuf[100];
	
		Instruction_t *tmp=insn, *callback=NULL, *post_callback=NULL;
        	tmp=insertAssemblyAfter(firp,tmp,"lea esp, [esp-4096]");
        	tmp=insertAssemblyAfter(firp,tmp,"pushf");
        	tmp=insertAssemblyAfter(firp,tmp,"pusha");
        	sprintf(tmpbuf,"push  0x%x", second);
        	tmp=insertAssemblyAfter(firp,tmp,tmpbuf);	 // push second
        	sprintf(tmpbuf,"push  0x%x", first);
        	tmp=insertAssemblyAfter(firp,tmp,tmpbuf);	 // push first
        	sprintf(tmpbuf,"push  0x%x", postCallbackReturn);
        	tmp=insertAssemblyAfter(firp,tmp,tmpbuf);	 // push <ret addr>
        	callback=tmp=insertAssemblyAfter(firp,tmp,"nop");
       	post_callback=		// indented oddly to look like labels.
		tmp=insertAssemblyAfter(firp,tmp,"lea esp, [esp+8]"); // pop first and second 
        	tmp=insertAssemblyAfter(firp,tmp,"popa");
        	tmp=insertAssemblyAfter(firp,tmp,"popf");
        	tmp=insertAssemblyAfter(firp,tmp,"lea esp, [esp+4096]");
	last_startup_insn=tmp;
        	post_callback->GetAddress()->SetVirtualOffset(postCallbackReturn);
		callback->SetCallback("zipr_init_addrs");

	
	}
	return true;
}

bool WSC_Instrument::add_allocation_instrumentation()
{

	bool success=true;

	for(SyscallSiteSet_t::iterator it=syscalls.GetSyscalls().begin();
		it!=syscalls.GetSyscalls().end();
		++it)
	{
		SyscallSite_t ss=*it;
		Instruction_t *site=ss.GetSyscallSite();
		SyscallNumber_t num=ss.GetSyscallNumber();
		if(num==SNT_allocate)
		{
			success = success && add_wsc_instrumentation(site);
		}
	}

	/* return an exit code */
	return success; /* success? */
}


static const ARGTYPE* FindMemoryArgument(const DISASM &d)
{
	 if((d.Argument1.ArgType & MEMORY_TYPE) == MEMORY_TYPE)
		return &d.Argument1;
	 if((d.Argument2.ArgType & MEMORY_TYPE) == MEMORY_TYPE)
		return &d.Argument2;
	 if((d.Argument3.ArgType & MEMORY_TYPE) == MEMORY_TYPE)
		return &d.Argument3;
	 if((d.Argument4.ArgType & MEMORY_TYPE) == MEMORY_TYPE)
		return &d.Argument4;

	return NULL;
}


bool WSC_Instrument::needs_wsc_segfault_checking(Instruction_t* insn, const DISASM& d)
{

	// start, for now, by instrumenting one insn we know faults
	// movsx  ecx,BYTE PTR [ebp+ecx*1-0x54]
	// later, we can hopefully get input from someplace and 
	// so we don't have to instrument everything.
	if(strstr(d.CompleteInstr,"[ebp+ecx+"))
		return true;
	if(strstr(d.CompleteInstr,"[ebp+ecx-"))
		return true;

	/* lea's and nops don't access memory */
	if(strstr(d.CompleteInstr,"lea") || strstr(d.CompleteInstr,"nop") )
		return false;

	const ARGTYPE *mem=FindMemoryArgument(d);	
	if(!mem)
		return false;

	/* if there's indexing happening, we need to check, even if it's on the stack */
	if(mem->Memory.IndexRegister!=0)
		return true;

	/* esp+<optional_scale>+<disp> is OK */
	if(mem->Memory.BaseRegister==REG4)
		return false;

	/* ebp+<optional scale>+disp  is OK if there's a base pointer */
	if(mem->Memory.BaseRegister==REG5 && insn->GetFunction() && insn->GetFunction()->GetUseFramePointer())
		return false;

	/* if there's no base reg, it must be an [<disp>] address, which is OK. */
	if(mem->Memory.BaseRegister==0)
		return false;

	/* else, there's a non-stack base reg, so we should check */ 
	return true;
}


static string get_memory_addr(const DISASM& d)
{
	string s=d.CompleteInstr;
	size_t pos=s.find('[');

	assert(pos!=string::npos);

	s.replace(0,pos-1,"");

	pos=s.find(']');
	s.replace(pos+1,s.length(),"");

	return s;
}

Relocation_t* WSC_Instrument::create_reloc(Instruction_t* insn, string type, int offset )
{
        Relocation_t* reloc=new Relocation_t;
        insn->GetRelocations().insert(reloc);
        firp->GetRelocations().insert(reloc);

	reloc->SetType(type);
	reloc->SetOffset(offset);

        return reloc;
}



Instruction_t* WSC_Instrument::GetCallbackCode()
{
	static Instruction_t* callback_dispatch=NULL;
	if(callback_dispatch != NULL)
		return callback_dispatch;


      	virtual_offset_t postCallbackReturn = getAvailableAddress(firp);
		
	Instruction_t *tmp=NULL, *callback=NULL, *post_callback=NULL, *ret=NULL;
	char tmpbuf[100];


	callback_dispatch=tmp=addNewAssembly(firp,NULL,"pushf");
	tmp=insertAssemblyAfter(firp,tmp,"push eax");	 // push eax -- addr to check
	sprintf(tmpbuf,"push 0x%x", postCallbackReturn);
	tmp=insertAssemblyAfter(firp,tmp,tmpbuf);	 // push <ret addr>
callback=	// indented oddly to look like labels.
	tmp=insertAssemblyAfter(firp,tmp,"nop");
post_callback=
       	tmp=insertAssemblyAfter(firp,tmp,"lea esp, [esp+4]");	// pop eax
       	tmp=insertAssemblyAfter(firp,tmp,"popf");
ret=
       	tmp=insertAssemblyAfter(firp,tmp,"ret");
	create_reloc(ret,"cf::safe",0);

       	post_callback->GetAddress()->SetVirtualOffset(postCallbackReturn);
	callback->SetCallback("zipr_is_addr_ok");


	return callback_dispatch;
              
}


bool	WSC_Instrument::add_segfault_checking(Instruction_t* insn)
{
	assert(insn);
	DISASM d;
	insn->Disassemble(d);
	char tmpbuf[100];

	Instruction_t* callback=GetCallbackCode(), *tmp=insn;

cout<<"Adding callback to "<<d.CompleteInstr<<endl;


       	insertAssemblyBefore(firp,insn,"pusha");
       	sprintf(tmpbuf,"lea  eax, %s", get_memory_addr(d).c_str());
       	tmp=insertAssemblyAfter(firp,tmp,tmpbuf);	 // lea addr,  [ expression ]
       	tmp=insertAssemblyAfter(firp,tmp,"call 0x0", callback);
       	tmp=insertAssemblyAfter(firp,tmp,"popa");

	return true;
}


bool	WSC_Instrument::add_segfault_checking()
{
	int success=true;
	cout<<"Checking "<<to_protect.size()<< " instructions for protections "<<endl;
	for(InstructionSet_t::iterator it=to_protect.begin();
		it!=to_protect.end();
		++it)
	{
		Instruction_t *insn=*it;
		cout<<"Testing "<<insn->getDisassembly()<<endl;
		DISASM d;
		insn->Disassemble(d);
		if(insn->GetBaseID()!=BaseObj_t::NOT_IN_DATABASE && needs_wsc_segfault_checking(insn,d))
			success=success && add_segfault_checking(insn);
	}

	return success;
}

bool WSC_Instrument::add_receive_limit(Instruction_t* site)
{

//cout<<"Found syscall to instrument "<<site->getDisassembly()<<endl;

const int receive_limit=64;
	char tmpbuf[100];

	Instruction_t *tmp=site, *int_insn=NULL;
        sprintf(tmpbuf,"cmp  edx, 0x%x", receive_limit);
        insertAssemblyBefore(firp,tmp,tmpbuf);
	tmp->SetComment(string(tmpbuf)+"ReceiveLimit");
	int_insn=tmp->GetFallthrough();
	tmp=insertAssemblyAfter(firp,tmp,"jle 0x0");
	tmp->SetTarget(int_insn);
        sprintf(tmpbuf,"mov  edx, 0x%x", receive_limit);
	tmp=insertAssemblyAfter(firp,tmp,tmpbuf);

	return true;
}

bool WSC_Instrument::add_receive_limit()
{

	bool success=true;

	for(SyscallSiteSet_t::iterator it=syscalls.GetSyscalls().begin();
		it!=syscalls.GetSyscalls().end();
		++it)
	{
		SyscallSite_t ss=*it;
		Instruction_t *site=ss.GetSyscallSite();
		SyscallNumber_t num=ss.GetSyscallNumber();
		if(num==SNT_receive && to_protect.find(site)!=to_protect.end())	// if it's receive, and it might overflow
		{
			cout<<"Adding receive limit to "<<site->GetBaseID()<<":"<<site->getDisassembly()<<endl;
			success = success && add_receive_limit(site);
		}
	}

	/* return an exit code */
	return success; /* success? */
}

bool WSC_Instrument::FindInstructionsToProtect(std::string s)
{
	bool success=true;

	// no filename, skip this step
	if(s=="")
	{
		cout<<"No filename provided for warnings, skipping selective application, applying to all instructions"<<endl;
		return false;
	}

	ifstream fin(s.c_str(), ios_base::in);
	if(!fin)
	{
		cerr<<"Cannot open file: "<<s<<endl;
		exit(1);
	}

	// forget what we've protected before, which might be everything.
	to_protect.clear();

	libIRDB::virtual_offset_t addr;
	while(  fin>>std::hex>>addr )
	{
		Instruction_t* insn=FindInstruction(addr);
		if(insn)
		{
			cout<<"Found instruction to protect "<<insn->getDisassembly()<<" at "<<hex<<addr<<endl;
			to_protect.insert(insn);
		}
		else
		{
			success=false;
			cerr<<"***************************************************************************************************************************"<<endl;
			cerr<<"***************************************************************************************************************************"<<endl;
			cerr<<"					Cannot find insn for addr "<<hex<<addr<<endl;
			cerr<<"***************************************************************************************************************************"<<endl;
			cerr<<"***************************************************************************************************************************"<<endl;
		}
	}
	
	return success;	
}

libIRDB::Instruction_t* WSC_Instrument::FindInstruction(libIRDB::virtual_offset_t addr)
{
	for(InstructionSet_t::iterator it=firp->GetInstructions().begin();
		it!=firp->GetInstructions().end();
		++it
	   )
	{
		Instruction_t* insn=*it;
		if( insn->GetAddress()->GetVirtualOffset()==addr)
			return insn;
		if(insn->GetIndirectBranchTargetAddress() && 
			insn->GetIndirectBranchTargetAddress()->GetVirtualOffset()==addr)
			return insn;
	}

	return NULL;	// can't find
}



bool WSC_Instrument::execute()
{
	bool success=true;

	success = success && add_init_call();
	success = success && add_allocation_instrumentation();
	success = success && add_segfault_checking();
	success = success && add_receive_limit();

	return success;
}


