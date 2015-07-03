/*
 * Copyright (c) 2014, 2015 - University of Virginia 
 *
 * This file may be used and modified for non-commercial purposes as long as 
 * all copyright, permission, and nonwarranty notices are preserved.  
 * Redistribution is prohibited without prior written consent from the University 
 * of Virginia.
 *
 * Please contact the authors for restrictions applying to commercial use.
 *
 * THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: University of Virginia
 * e-mail: jwd@virginia.com
 * URL   : http://www.cs.virginia.edu/
 *
 */


#include "utils.hpp"
#include "c2e_instr.hpp"
#include "Rewrite_Utility.hpp"
#include <stdlib.h>


// for mmap param
#include <sys/mman.h>
#include <bits/syscall.h>





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

Instruction_t* Cgc2Elf_Instrument::insertTerminate(Instruction_t* after) 
{  
	char buf[100];
	sprintf(buf, "mov eax, %d", SYS_exit);

	after=insertAssemblyAfter(firp, after, buf);
	after=insertAssemblyAfter(firp, after, "int 0x80");
	
	return after; 
}

Instruction_t* Cgc2Elf_Instrument::insertTransmit(Instruction_t* after, int sysno, int force_fd)
{  
	Instruction_t *jmp2return=NULL, *jmp2error=NULL, *success=NULL, *error=NULL;
	char fdbuf[100];
	char buf[100];
	sprintf(buf, "mov eax, %d", sysno);
	if (force_fd >= 0)
	{
		sprintf(fdbuf, "mov ebx, %d", force_fd);
		after=insertAssemblyAfter(firp, after, "push ebx");	// push old fd
		after=insertAssemblyAfter(firp, after, fdbuf);	// force fd
	}
	after=insertAssemblyAfter(firp, after, "push esi");	 	// push tx_bytes
	after=insertAssemblyAfter(firp, after, buf);			// set eax to syscall #
	after=insertAssemblyAfter(firp, after, "int 0x80");		// make syscall
	after=insertAssemblyAfter(firp, after, "pop esi");	 	// pop tx_bytes
	if (force_fd >= 0)
	{
		after=insertAssemblyAfter(firp, after, "pop ebx");	// restore old fd
	}
	after=insertAssemblyAfter(firp, after, "cmp eax, -1");	 	// if return == -1
	jmp2error=after=insertAssemblyAfter(firp, after, "je 0x0");	 // jmp to error
	after=insertAssemblyAfter(firp, after, "cmp esi, 0");	 		// if tx_bytes == 0 
	jmp2return=after=insertAssemblyAfter(firp, after, "je 0x0");	 	// jmp to success
	after=insertAssemblyAfter(firp, after, "mov [esi], eax");			// store tx_bytes
	success=after=insertAssemblyAfter(firp, after, "mov eax, 0");	// return success
	error=after=insertAssemblyAfter(firp, after, "mov eax, 25");	// return error
	after=insertAssemblyAfter(firp, after, "nop");			// sync point.

	jmp2error->SetTarget(error);
	jmp2return->SetTarget(success);
	success->SetFallthrough(after);
	
	return after; 
}

Instruction_t* Cgc2Elf_Instrument::insertReadExitOnEOF(Instruction_t* after)
{  
	Instruction_t *jmp2return=NULL, *jmp2error=NULL, *success=NULL, *error=NULL;
	char buf[100];
	sprintf(buf, "mov eax, %d", SYS_read);
	after=insertAssemblyAfter(firp, after, "push esi");	 	// push tx_bytes
	after=insertAssemblyAfter(firp, after, buf);			// set eax to syscall #
	after=insertAssemblyAfter(firp, after, "int 0x80");		// make syscall

	after = insertAssemblyAfter(firp, after, "cmp eax, 0");
	Instruction_t* jmp2terminate=insertAssemblyAfter(firp, after, "je 0x0");	 // jmp to terminate

	after=insertAssemblyAfter(firp, jmp2terminate, "pop esi");	 	// pop tx_bytes
	after=insertAssemblyAfter(firp, after, "cmp eax, -1");	 	// if return == -1
	jmp2error=after=insertAssemblyAfter(firp, after, "je 0x0");	 // jmp to error
	after=insertAssemblyAfter(firp, after, "cmp esi, 0");	 		// if tx_bytes == 0 
	jmp2return=after=insertAssemblyAfter(firp, after, "je 0x0");	 	// jmp to success
	after=insertAssemblyAfter(firp, after, "mov [esi], eax");			// store tx_bytes
	success=after=insertAssemblyAfter(firp, after, "mov eax, 0");	// return success
	error=after=insertAssemblyAfter(firp, after, "mov eax, 25");	// return error
	after=insertAssemblyAfter(firp, after, "nop");			// sync point.

	jmp2error->SetTarget(error);
	jmp2return->SetTarget(success);
	success->SetFallthrough(after);
	
        // terminate sequence
        Instruction_t* n = addNewAssembly(firp, NULL, "nop"); 
        n->SetFallthrough(after); // pick anything here, it doesn't matter as we're terminating
	Instruction_t* term = insertTerminate(n);
	jmp2terminate->SetTarget(n);

	return after; 
}

Instruction_t* Cgc2Elf_Instrument::insertReceive(Instruction_t* after, bool force_stdin, bool exit_on_eof) 
{  
	if (force_stdin)
	{
		// force read from file descriptor 0
		after = insertAssemblyAfter(firp, after, "push ebx");		// save original fd
		after = insertAssemblyAfter(firp, after, "xor ebx, ebx");	// fd = 0
		if (exit_on_eof)
			after = insertReadExitOnEOF(after);
		else
			after = insertTransmit(after, SYS_read);
		after = insertAssemblyAfter(firp, after, "pop ebx");		// restore
		return after;
	}
	else
	{
		if (exit_on_eof)
			return insertReadExitOnEOF(after);
		else
			return insertTransmit(after, SYS_read);
	}
}

// eax          ebx          ecx              edx               esi                        edi
// int fdwait(int nfds, fd_set *readfds, fd_set *writefds, const struct timeval *timeout, int *readyfds)
// int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);

// %ebx, %ecx, %edx, %esi, %edi, %ebp
// force_no_timeout: set %edi to 0
Instruction_t* Cgc2Elf_Instrument::insertFdwait(Instruction_t* after)
{  
	Instruction_t *jmp2return=NULL, *jmp2error=NULL, *success=NULL, *error=NULL;
	char buf[100];
	sprintf(buf, "mov eax, %d", 142 /* SYS_newselect -- not defined? */);
	after=insertAssemblyAfter(firp, after, "push edi");	 	// push readyfds
	after=insertAssemblyAfter(firp, after, buf);			// set eax to syscall #
	after=insertAssemblyAfter(firp, after, "mov edi, esi");		// mov 4th param to fdwait  - fdwait(...,timeout,...) 
									// into 5th param to select - select(...,timeout,...)
	after=insertAssemblyAfter(firp, after, "mov esi, 0");		// set 4th param to select to 0	
	after=insertAssemblyAfter(firp, after, "int 0x80");		// make syscall
	after=insertAssemblyAfter(firp, after, "pop edi");	 	// pop readyfds
	after=insertAssemblyAfter(firp, after, "cmp eax, -1");	 	// if return == -1
	jmp2error=after=insertAssemblyAfter(firp, after, "je 0x0");	 // jmp to error
	after=insertAssemblyAfter(firp, after, "cmp edi, 0");	 		// if tx_bytes == 0 
	jmp2return=after=insertAssemblyAfter(firp, after, "je 0x0");	 	// jmp to success
	after=insertAssemblyAfter(firp, after, "mov [edi], eax");			// store return value into readyfds
	success=after=insertAssemblyAfter(firp, after, "mov eax, 0");	// return success
	error=after=insertAssemblyAfter(firp, after, "mov eax, 25");	// return error
	after=insertAssemblyAfter(firp, after, "nop");			// sync point.

	jmp2error->SetTarget(error);
	jmp2return->SetTarget(success);
	success->SetFallthrough(after);
	
	return after; 
}
Instruction_t* Cgc2Elf_Instrument::insertAllocate(Instruction_t* after) 
{  
	Instruction_t *jmp2return=NULL, *jmp2error=NULL, *success=NULL, *error=NULL;
	char buf[100];
	after=insertAssemblyAfter(firp, after, "push ebp");	 	// push ebp to save reg
	after=insertAssemblyAfter(firp, after, "push edx");	 	// push (void **)addr
	after=insertAssemblyAfter(firp, after, "push esi");	 	// save reg
	after=insertAssemblyAfter(firp, after, "push edi");	 	// save reg

#ifdef SYS_mmap2
	sprintf(buf, "mov eax, %d", SYS_mmap2);
#else
	assert(0);	// mmap2 required
#endif
	after=insertAssemblyAfter(firp, after, buf);			// set eax to syscall #



	after=insertAssemblyAfter(firp, after, "mov ebp, 0");		// mov 0 to 6th param to mmap2  - mmap2(...,0)
	after=insertAssemblyAfter(firp, after, "mov edi, -1");		// mov -1 to 5th param to mmap2  - mmap2(...,-1,0) 
	sprintf(buf, "mov esi, %d", MAP_PRIVATE|MAP_ANONYMOUS);
	after=insertAssemblyAfter(firp, after, buf);			// mov MAP_PRIV|ANON to 4th param to mmap2  - mmap2(...,PA,-1,0) 
	sprintf(buf, "mov edx, %d", PROT_READ|PROT_WRITE|PROT_EXEC);
	after=insertAssemblyAfter(firp, after, buf);			// mov RWX to 3rd param to mmap2  - mmap2(...RWX,,PA,-1,0) 
	after=insertAssemblyAfter(firp, after, "mov ecx, ebx");		// mov length to 2nd param to mmap2  - mmap2(...,len,RWX,,PA,-1,0) 
	after=insertAssemblyAfter(firp, after, "mov ebx,0");		// mov 0 to 1st param to mmap2  - mmap2(0,len,RWX,,PA,-1,0) 

	after=insertAssemblyAfter(firp, after, "int 0x80");		// make syscall
	after=insertAssemblyAfter(firp, after, "pop edi");	 	// restore reg
	after=insertAssemblyAfter(firp, after, "pop esi");	 	// restore reg
	after=insertAssemblyAfter(firp, after, "pop edx");	 	// pop (void**)addr
	after=insertAssemblyAfter(firp, after, "pop ebp");	 	// pop ebp to restore reg
	after=insertAssemblyAfter(firp, after, "cmp eax, -1");	 	// if return == -1
	jmp2error=after=insertAssemblyAfter(firp, after, "je 0x0");	 // jmp to error
	after=insertAssemblyAfter(firp, after, "cmp edx, 0");	 		// if addr == 0 
	jmp2return=after=insertAssemblyAfter(firp, after, "je 0x0");	 	// jmp to success
	after=insertAssemblyAfter(firp, after, "mov [edx], eax");			// store return value into readyfds
	success=after=insertAssemblyAfter(firp, after, "mov eax, 0");	// return success
	error=after=insertAssemblyAfter(firp, after, "mov eax, 25");	// return error
	after=insertAssemblyAfter(firp, after, "nop");			// sync point.

	jmp2error->SetTarget(error);
	jmp2return->SetTarget(success);
	success->SetFallthrough(after);
	
	return after; 
}

Instruction_t* Cgc2Elf_Instrument::insertDeallocate(Instruction_t* after) 
{  
	Instruction_t *jmp2return=NULL, *jmp2error=NULL, *success=NULL, *error=NULL;
	char buf[100];
	sprintf(buf, "mov eax, %d", SYS_munmap);
	after=insertAssemblyAfter(firp, after, buf);			// set eax to syscall #
	after=insertAssemblyAfter(firp, after, "int 0x80");		// make syscall
	after=insertAssemblyAfter(firp, after, "cmp eax, -1");	 	// if return == -1
	jmp2error=after=insertAssemblyAfter(firp, after, "je 0x0");	 // jmp to error
	success=after=insertAssemblyAfter(firp, after, "mov eax, 0");	// return success
	error=after=insertAssemblyAfter(firp, after, "mov eax, 25");	// return error
	after=insertAssemblyAfter(firp, after, "nop");			// sync point.

	jmp2error->SetTarget(error);
	success->SetFallthrough(after);
	
	return after; 
}

Instruction_t* Cgc2Elf_Instrument::insertRandom(Instruction_t* after) 
{
/*
   d:	85 d2                	test   %ecx,%ecx
J1:	7e 11                	jle    L4
  11:	b8 00 00 00 00       	mov    $0x0,%eax
L3:	88 04 01             	mov    %al,[%ebx+%eax*1]
  19:	83 c0 01             	add    $0x1,%eax
  1c:	39 d0                	cmp    %ecx,%eax
J2:	75 f6                	jne    L3
  20:	eb 05                	jmp    L2
L4:	ba 00 00 00 00       	mov    $0x0,%ecx
L2:	85 db                	test   %edx,%edx
J3:	74 02                	je     L1
  2b:	89 13                	mov    %ecx,(%edx)
L1:	b8 00 00 00 00       	mov    $0x0,%eax
*/

	Instruction_t *J1=NULL, *J2=NULL, *J3=NULL, *L1=NULL, *L2=NULL, *L3=NULL, *L4=NULL;

	after=insertAssemblyAfter(firp, after, "test ecx, ecx");
	J1=after=insertAssemblyAfter(firp, after, "jle 0x0");
	after=insertAssemblyAfter(firp, after, "mov eax, 0");
//	L3=after=insertAssemblyAfter(firp, after, "mov [ebx+eax], al");
	L3=after=insertAssemblyAfter(firp, after, "mov byte [ebx+eax], -1"); // give afl a better chance
	after=insertAssemblyAfter(firp, after, "add eax, 1");
	after=insertAssemblyAfter(firp, after, "cmp eax, ecx");
	J2=after=insertAssemblyAfter(firp, after, "jne 0x0");

	L4=after=insertAssemblyAfter(firp, after, "mov ecx, 0");
	L2=after=insertAssemblyAfter(firp, after, "test edx, edx");
	J3=after=insertAssemblyAfter(firp, after, "je 0x0");
	after=insertAssemblyAfter(firp, after, "mov [edx], ecx");
	L1=after=insertAssemblyAfter(firp, after, "mov eax, 0");

	J1->SetTarget(L4);
	J2->SetTarget(L3);
	J2->SetFallthrough(L2);
	J3->SetTarget(L1);


	return after;
}


bool Cgc2Elf_Instrument::add_c2e_instrumentation(libIRDB::Instruction_t* insn)
{

	assert(insn);
	cout<<"Adding CGC->Elf instrumentation for "<<insn->GetBaseID()<<":"<<insn->getDisassembly()<<endl;

	Instruction_t* tmp=insn;
	Instruction_t* termjmp=NULL, *terminsn=NULL;
	Instruction_t* transmitjmp=NULL, *transmitinsn=NULL;
	Instruction_t* receivejmp=NULL, *receiveinsn=NULL;
	Instruction_t* fdwaitjmp=NULL, *fdwaitinsn=NULL;
	Instruction_t* allocatejmp=NULL, *allocateinsn=NULL;
	Instruction_t* deallocatejmp=NULL, *deallocateinsn=NULL;
	Instruction_t* randomjmp=NULL, *randominsn=NULL;
	Instruction_t* old=insn;
	Instruction_t* failinsn=NULL;

	old=insertAssemblyBefore(firp,tmp,"cmp eax, 1"); // terminate
	terminsn=tmp;
        termjmp=tmp=insertAssemblyAfter(firp,tmp,"jne 0");
		tmp=insertTerminate(tmp);
		tmp->SetFallthrough(old);
        transmitinsn=tmp=addNewAssembly(firp,NULL,"cmp eax, 2"); //transmit
        transmitjmp=tmp=insertAssemblyAfter(firp,tmp,"jne 0");
		if (getForceWriteToStdout())
			tmp=insertTransmit(tmp, SYS_write, getForceWriteFd()); // force output on fd=1
		else
			tmp=insertTransmit(tmp); 
		tmp->SetFallthrough(old);
        receiveinsn=tmp=addNewAssembly(firp,NULL,"cmp eax, 3"); //receive
        receivejmp=tmp=insertAssemblyAfter(firp,tmp,"jne 0"); 
		tmp=insertReceive(tmp, getForceReadFromStdin(), getForceExitOnReadEOF());
		tmp->SetFallthrough(old);
        fdwaitinsn=tmp=addNewAssembly(firp,NULL,"cmp eax, 4"); //fdwait
        fdwaitjmp=tmp=insertAssemblyAfter(firp,tmp,"jne 0");
		tmp=insertFdwait(tmp);
		tmp->SetFallthrough(old);
        allocateinsn=tmp=addNewAssembly(firp,NULL,"cmp eax, 5"); //allocate
        allocatejmp=tmp=insertAssemblyAfter(firp,tmp,"jne 0");
		tmp=insertAllocate(tmp);
		tmp->SetFallthrough(old);
        deallocateinsn=tmp=addNewAssembly(firp,NULL,"cmp eax, 6"); //deallocate
        deallocatejmp=tmp=insertAssemblyAfter(firp,tmp,"jne 0");
		tmp=insertDeallocate(tmp);
		tmp->SetFallthrough(old);
        randominsn=tmp=addNewAssembly(firp,NULL,"cmp eax, 7"); // random
        randomjmp=tmp=insertAssemblyAfter(firp,tmp,"jne 0");
		tmp=insertRandom(tmp);
		tmp->SetFallthrough(old);
	failinsn=tmp=addNewAssembly(firp,NULL,"mov eax, 13"); // fail
	failinsn->SetFallthrough(old);

	termjmp->SetTarget(transmitinsn);
	transmitjmp->SetTarget(receiveinsn);
	receivejmp->SetTarget(fdwaitinsn);
	fdwaitjmp->SetTarget(allocateinsn);
	allocatejmp->SetTarget(deallocateinsn);
	deallocatejmp->SetTarget(randominsn);
	randomjmp->SetTarget(failinsn);
	
	// convert orig. insn to nop.
	string bits;  
	bits.resize(1); 
	bits[0]=0x90;
	old->SetDataBits(bits);

	return true;
}

bool Cgc2Elf_Instrument::needs_c2e_instrumentation(libIRDB::Instruction_t* insn)
{
	// instrument int instructions 
	DISASM d;
	insn->Disassemble(d);
	return strstr(d.CompleteInstr,"int 0x80")!=0;
}

bool Cgc2Elf_Instrument::instrument_ints()
{
	bool success=true;
        InstructionSet_t  allinsns=firp->GetInstructions();

        // we do this in two passes.  first pass:  find instructions.
        for(InstructionSet_t::iterator it=allinsns.begin();
                it!=allinsns.end();
                ++it)
        {
		Instruction_t* insn=*it;
		if(needs_c2e_instrumentation(insn))
			success = success && add_c2e_instrumentation(insn);
			
	}

	return success;
}



bool Cgc2Elf_Instrument::execute()
{

	bool success=true;

	success = success && instrument_ints();

	return success;
}


