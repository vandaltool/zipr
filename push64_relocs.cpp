/***************************************************************************
 * Copyright (c)  2014  Zephyr Software LLC. All rights reserved.
 *
 * This software is furnished under a license and/or other restrictive
 * terms and may be used and copied only in accordance with such terms
 * and the inclusion of the above copyright notice. This software or
 * any other copies thereof may not be provided or otherwise made
 * available to any other person without the express written consent
 * of an authorized representative of Zephyr Software LCC. Title to,
 * ownership of, and all rights in the software is retained by
 * Zephyr Software LCC.
 *
 * Zephyr Software LLC. Proprietary Information
 *
 * Unless otherwise specified, the information contained in this
 * directory, following this legend, and/or referenced herein is
 * Zephyr Software LLC. (Zephyr) Proprietary Information.
 *
 * CONTACT
 *
 * For technical assistance, contact Zephyr Software LCC. at:
 *
 *
 * Zephyr Software, LLC
 * 2040 Tremont Rd
 * Charlottesville, VA 22911
 *
 * E-mail: jwd@zephyr-software.com
 **************************************************************************/


#include <zipr_sdk.h>
#include <string>
#include <algorithm>
#include "utils.hpp"
#include "Rewrite_Utility.hpp"
#include "push64_relocs.h"


using namespace libIRDB;
using namespace std;
using namespace Zipr_SDK;
using namespace ELFIO;

Push64Relocs_t::Push64Relocs_t(MemorySpace_t *p_ms,
	elfio *p_elfio,
	FileIR_t *p_firp,
	Options_t *p_opts,
	InstructionLocationMap_t *p_fil) :
		m_memory_space(*p_ms), 
		m_elfio(*p_elfio),
		m_firp(*p_firp),
		m_opts(*p_opts),
		final_insn_locations(*p_fil)
{
}

bool Push64Relocs_t::IsAdd64Relocation(Relocation_t *reloc)
{
	return (reloc->GetType().find("add64") != std::string::npos);
}

Relocation_t* Push64Relocs_t::FindAdd64Relocation(Instruction_t* insn)
{
	Instruction_t* first_slow_path_insn=NULL;
	RelocationSet_t::iterator rit = insn->GetRelocations().begin();
	for(rit; rit!=insn->GetRelocations().end(); rit++)
	{
		Relocation_t *reloc=*rit;
		if (IsAdd64Relocation(reloc))
			return reloc;
	}
	return NULL;
}


bool Push64Relocs_t::IsPush64Relocation(Relocation_t *reloc)
{
	return (reloc->GetType().find("push64") != std::string::npos);
}

Relocation_t* Push64Relocs_t::FindPush64Relocation(Instruction_t* insn)
{
	Instruction_t* first_slow_path_insn=NULL;
	RelocationSet_t::iterator rit = insn->GetRelocations().begin();
	for(rit; rit!=insn->GetRelocations().end(); rit++)
	{
		Relocation_t *reloc=*rit;
		if (IsPush64Relocation(reloc))
			return reloc;
	}
	return NULL;
}

void Push64Relocs_t::HandlePush64Relocation(Instruction_t *insn, Relocation_t *reloc)
{
	std::unique_ptr<CallAddPair_t> reloc_pair(new CallAddPair_t(insn, insn->GetTarget()));
	call_add_pairs.insert(std::move(reloc_pair));	
}

void Push64Relocs_t::HandlePush64Relocs()
{
	int handled=0;
	int insns=0;
	int relocs=0;
	// for each instruction 
	InstructionSet_t::iterator iit = m_firp.GetInstructions().begin();
	for(iit; iit!=m_firp.GetInstructions().end(); iit++)
	{
		Instruction_t& insn=*(*iit);
		insns++;

		Relocation_t* reloc=FindPush64Relocation(&insn);
		if(reloc)
		{
			if (m_opts.GetVerbose())
				cout << "Found a Push64 relocation." << endl;
			HandlePush64Relocation(&insn,reloc);
			handled++;
		}
	}

	cout<<"#ATTRIBUTE push64_relocations="<< std::dec<<handled<<endl;
}


void Push64Relocs_t::UpdatePush64Adds()
{
	if (m_opts.GetVerbose())
		cout << "UpdatePush64Adds()" << endl;
	CallAddPairs_t::iterator cap_it = call_add_pairs.begin();
	for (cap_it; cap_it != call_add_pairs.end(); cap_it++)
	{
		bool change_to_add = false;
		RangeAddress_t call_addr = 0;
		RangeAddress_t add_addr = 0;
		int add_offset = 0;
		uint32_t relocated_value = 0;
		Instruction_t *call = NULL, *add = NULL;
		CallAddPair_t *cap = cap_it->get();
		Relocation_t *add_reloc = NULL;

		call = cap->first;
		add = cap->second;

		call_addr = final_insn_locations[call];
		add_addr = final_insn_locations[add];

		if (call_addr == 0 || add_addr == 0)
		{
			if (m_opts.GetVerbose())
				cout << "Call/Add pair not plopped?" << endl;
			continue;
		}

		add_reloc = FindAdd64Relocation(add);
		assert(add_reloc && "Add in Call/Add pair must have relocation.");

		add_offset = add_reloc->GetOffset();

		/*
		 * Stupid call will push the NEXT instruction address.
		 */
		call_addr+=call->GetDataBits().length();

		if (add_offset>call_addr)
		{
			change_to_add = true;
			relocated_value = add_offset-call_addr;
		}
		else
		{
			relocated_value = call_addr-add_offset;
		}

		cout << "Relocating a(n) "<< ((change_to_add) ? "add":"sub") << " from " 
		     << std::hex << call_addr 
				 << " at "
				 << std::hex << add_addr
				 << endl
		     << "Using 0x" << std::hex << relocated_value 
		     << " as the updated offset." << endl
				 << "Using 0x" << std::hex << add_offset 
				 << " as the base offset." << endl;
		if (change_to_add)
		{
			char add = (char)0x04;
			m_memory_space.PlopBytes(add_addr+1, (const char*)&add, 1);
		}
		m_memory_space.PlopBytes(add_addr+3, (const char*)&relocated_value, 4);
	}
}

extern "C" 
Zipr_SDK::ZiprPluginInterface_t* GetPluginInterface(
	Zipr_SDK::MemorySpace_t *p_ms, 
	ELFIO::elfio *p_elfio, 
	libIRDB::FileIR_t *p_firp, 
	Zipr_SDK::Options_t *p_opts,
	Zipr_SDK::InstructionLocationMap_t *p_fil) 
{
	return new Push64Relocs_t(p_ms,p_elfio,p_firp,p_opts,p_fil);
}
