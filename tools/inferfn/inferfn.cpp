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

#include "Rewrite_Utility.hpp"
#include "inferfn.hpp"

using namespace std;

#define SPRI_AVAIL_ADDRESS 0xff08ff00

InferFn::InferFn(FileIR_t *p_firp)
{
	m_firp = p_firp;

	m_cg.AddFile(m_firp);
//	m_cg.Dump(cout);

	int elfoid=m_firp->GetFile()->GetELFOID();
	pqxx::largeobject lo(elfoid);
	libIRDB::pqxxDB_t *interface=dynamic_cast<libIRDB::pqxxDB_t*>(libIRDB::BaseObj_t::GetInterface());
	assert(interface);
	lo.to_file(interface->GetTransaction(),"tmp.exe");

	m_elfiop=new ELFIO::elfio;
	m_elfiop->load("tmp.exe");
	ELFIO::dump::header(std::cout,*m_elfiop);
	ELFIO::dump::section_headers(std::cout,*m_elfiop);
	ELFIO::dump::segment_headers(std::cout,*m_elfiop);
}

void InferFn::pinAllFunctionEntryPoints()
{
	Function_t* fn=NULL;
	for(FunctionSet_t::iterator it=m_firp->GetFunctions().begin();
		it!=m_firp->GetFunctions().end();
		++it
	   )
	{
		fn=*it;
		if (!fn) continue;
		Instruction_t *insn = fn->GetEntryPoint();
		
		if(insn && insn->GetAddress() && insn->GetAddress()->GetVirtualOffset() > 0)
		{
			insn->SetIndirectBranchTargetAddress(insn->GetAddress());
printf("inferfn: pinning function entry point: %p\n", insn->GetAddress()->GetVirtualOffset());
		}
	}
}

void InferFn::addInferenceCallback(Instruction_t *site)
{
        virtual_offset_t postCallbackReturn = SPRI_AVAIL_ADDRESS;
	char tmpbuf[200];
        sprintf(tmpbuf,"push  0x%x", postCallbackReturn);

	Instruction_t *tmp=site, *callback=NULL, *post_callback=NULL;
//	Instruction_t *fallthrough = site->GetFallthrough();
        tmp=insertAssemblyAfter(m_firp,tmp,"lea esp, [esp-4096]"); 
        tmp=insertAssemblyAfter(m_firp,tmp,"pushf");
        tmp=insertAssemblyAfter(m_firp,tmp,"pusha");
        tmp=insertAssemblyAfter(m_firp,tmp,tmpbuf);	 // push <ret addr>
        callback=tmp=insertAssemblyAfter(m_firp,tmp,"nop");
        post_callback=tmp=insertAssemblyAfter(m_firp,tmp,"popa");
        tmp=insertAssemblyAfter(m_firp,tmp,"popf");
        tmp=insertAssemblyAfter(m_firp,tmp,"lea esp, [esp+4096]");
        post_callback->GetAddress()->SetVirtualOffset(postCallbackReturn);
	callback->SetCallback("inference_handler");

//	tmp->SetFallthrough(fallthrough);
}

Instruction_t* InferFn::findEntryPoint()
{
	Instruction_t* insn=NULL;
	for(InstructionSet_t::iterator it=m_firp->GetInstructions().begin();
		it!=m_firp->GetInstructions().end();
		++it
	   )
	{
		insn=*it;
		if(insn->GetIndirectBranchTargetAddress() && 
			insn->GetIndirectBranchTargetAddress()->GetVirtualOffset()==(virtual_offset_t)m_elfiop->get_entry())
		{
			cout << "mallard: entry point is at 0x" << hex << m_elfiop->get_entry() << dec << endl;
			return insn;	
		}
		
	}

	return NULL;
}

bool InferFn::execute()
{
	Instruction_t *entryPoint = findEntryPoint();	
	assert(entryPoint);

	insertAssemblyBefore(m_firp, entryPoint, "nop");
	addInferenceCallback(entryPoint);
	
	// pin functions
	pinAllFunctionEntryPoints();

	return true;
}
