/*
 * Copyright (c) 2015 - University of Virginia
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
 */

#include <stdlib.h>
#include <string>
#include <iostream>

#include "buffrecv_instrument.hpp"
#include "Rewrite_Utility.hpp"


using namespace std;
using namespace libIRDB;

virtual_offset_t getAvailableAddress(FileIR_t *p_virp)
{
        static int counter = -16;
        counter += 16;
        return 0xf0080000 + counter;
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

// site should be: int 0x80 instruction in receive() wrapper
bool BuffRecv_Instrument::_add_buffered_receive_instrumentation(Instruction_t *site)
{
        string bits;
        bits.resize(1);
        bits[0]=0x90;
        site->SetDataBits(bits);	 // convert site to nop instruction

//cout<<"Found syscall to instrument "<<site->getDisassembly()<<endl;

        virtual_offset_t postCallbackReturn = getAvailableAddress(firp);
	char tmpbuf[100];
        sprintf(tmpbuf,"push  0x%x", postCallbackReturn);

	Instruction_t *tmp=site, *callback=NULL, *post_callback=NULL;
        tmp=insertAssemblyAfter(firp,tmp,"pushf");
        tmp=insertAssemblyAfter(firp,tmp,"pusha");
        tmp=insertAssemblyAfter(firp,tmp,tmpbuf);	 // push <ret addr>
        callback=tmp=insertAssemblyAfter(firp,tmp,"nop");
        post_callback=tmp=insertAssemblyAfter(firp,tmp,"popa");
        tmp=insertAssemblyAfter(firp,tmp,"popf");
//        tmp=insertAssemblyAfter(firp,tmp,"mov eax, 0");
        post_callback->GetAddress()->SetVirtualOffset(postCallbackReturn);
	callback->SetCallback("buffered_receive");
	return true;
}

bool BuffRecv_Instrument::add_buffered_receive_instrumentation()
{

	bool success=true;

	for(SyscallSiteSet_t::iterator it=syscalls.GetSyscalls().begin();
		it!=syscalls.GetSyscalls().end();
		++it)
	{
		SyscallSite_t ss=*it;
		Instruction_t *site=ss.GetSyscallSite();
		SyscallNumber_t num=ss.GetSyscallNumber();
		if(num==SNT_receive) 
		{
			cout << "Found RECEIVE syscall - instrument: " << site->getDisassembly() << " " << hex << site->GetAddress()->GetVirtualOffset() << dec << endl;
			success = success && _add_buffered_receive_instrumentation(site);
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

static bool has_index_register(Instruction_t* i)
{
	DISASM d;
	i->Disassemble(d);
	const ARGTYPE* arg=FindMemoryArgument(d);

	if(!arg)
		return false;

	if(arg->Memory.Scale)
		return true;
	return false;
	
}

static string regToRegstring(size_t regno)
{
	switch(regno)
	{
		case REG0: return "eax";
		case REG1: return "ecx";
		case REG2: return "edx";
		case REG3: return "ebx";
		case REG4: return "esp";
		case REG5: return "ebp";
		case REG6: return "esi";
		case REG7: return "edi";
		default: assert(0);
	}
}

std::ostream& BuffRecv_Instrument::displayStatistics(std::ostream &os)
{
}

bool BuffRecv_Instrument::execute()
{
	bool success=true;

	success = success && add_buffered_receive_instrumentation();

	return success;
}


