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


bool arg_has_relative(const ARGTYPE &arg)
{
	/* if it's relative memory, watch out! */
	if(arg.ArgType&MEMORY_TYPE)
		if(arg.ArgType&RELATIVE_)
			return true;
	
	return false;
}

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
bool Push64Relocs_t::IsPcrelRelocation(Relocation_t *reloc)
{
	return (reloc->GetType().find("pcrel") != std::string::npos);
}

Relocation_t* Push64Relocs_t::FindPcrelRelocation(Instruction_t* insn)

{
	Instruction_t* first_slow_path_insn=NULL;
	RelocationSet_t::iterator rit = insn->GetRelocations().begin();
	for(rit; rit!=insn->GetRelocations().end(); rit++)
	{
		Relocation_t *reloc=*rit;
		if (IsPcrelRelocation(reloc))
			return reloc;
	}
	return NULL;
}

void Push64Relocs_t::HandlePush64Relocation(Instruction_t *insn, Relocation_t *reloc)
{
	Instruction_t *add_insn = new Instruction_t;
	AddressID_t *add_addr = new AddressID_t;
	Instruction_t *jmp_insn = NULL;
	virtual_offset_t next_addr = 0;
	string databits = "";
	Relocation_t *add_reloc = new Relocation_t;

	plopped_relocs.insert(insn);	

	jmp_insn = insn->GetFallthrough();
	assert(jmp_insn);

	next_addr = insn->GetAddress()->GetVirtualOffset() + insn->GetDataBits().length();

	/*
	 * Change the push64 to a call/add pair.
	 */

	/* 
	 * Step 0: Add the add instruction and its address.
	 */
	add_addr->SetFileID(insn->GetAddress()->GetFileID());
	add_insn->SetAddress(add_addr);
	add_insn->SetFunction(insn->GetFunction());
	m_firp.GetAddresses().insert(add_addr);
	m_firp.GetInstructions().insert(add_insn);

	/* 
	 * Step 1: Change the push to a call 0.
	 */
	databits.resize(5);
	databits[0] = 0xe8;
	databits[1] = 0x00;
	databits[2] = 0x00;
	databits[3] = 0x00;
	databits[4] = 0x00;
	insn->SetDataBits(databits);
	insn->SetTarget(add_insn); // Comment
	insn->SetComment(insn->GetComment()+" Thunk part");
		
	/* 
	 * Step 2: Create the add instruction.
	 */
	databits = "";
	databits.resize(7);
	databits[0]=0x81;
	databits[1]=0x2c;	
	databits[2]=0x24;
	databits[3]=0xff;
	databits[4]=0xff;
	databits[5]=0xff;
	databits[6]=0xff;
	add_insn->SetDataBits(databits);

	/*
	 * Step 3: Put the relocation on the add instruction.
	 */
	add_reloc->SetOffset(next_addr);
	add_reloc->SetType("add64");
	add_insn->GetRelocations().insert(add_reloc);
	m_firp.GetRelocations().insert(add_reloc);

	if (m_opts.GetVerbose())
		cout << "Adding an add/sub with reloc offset 0x" 
		     << std::hex << add_reloc->GetOffset() 
				 << endl;
	/*
	 * Step 4: Tell the add insn to fallthrough to the call.
	 */
	add_insn->SetFallthrough(jmp_insn);
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
		Instruction_t *insn=*iit;
		insns++;

		Relocation_t* reloc=NULL;
		if (reloc = FindPush64Relocation(insn))
		{
			if (m_opts.GetVerbose())
				cout << "Found a Push64 relocation." << endl;
			HandlePush64Relocation(insn,reloc);
			handled++;
		}
		else if (reloc = FindPcrelRelocation(insn))
		{
			if (m_opts.GetVerbose())
				cout << "Found a pcrel relocation." << endl;
			plopped_relocs.insert(insn);
			handled++;
		}
	}

	cout<<"#ATTRIBUTE push64_relocations="<< std::dec<<handled<<endl;
}


void Push64Relocs_t::UpdatePush64Adds()
{
	if (m_opts.GetVerbose())
		cout << "UpdatePush64Adds()" << endl;
	InstructionSet_t::iterator insn_it = plopped_relocs.begin();
	for (insn_it; insn_it != plopped_relocs.end(); insn_it++)
	{
		Relocation_t *reloc = NULL;
		Instruction_t *insn = *insn_it;
		if (reloc = FindPush64Relocation(insn))
		{
			bool change_to_add = false;
			RangeAddress_t call_addr = 0;
			RangeAddress_t add_addr = 0;
			int add_offset = 0;
			uint32_t relocated_value = 0;
			Instruction_t *call = NULL, *add = NULL;
			Relocation_t *add_reloc = NULL;

			call = *insn_it;
			add = call->GetTarget();

			assert(call && add);

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
		else if (reloc = FindPcrelRelocation(insn))
		{
			uint8_t memory_offset = 0;
			int32_t existing_offset = 0;
			int32_t new_offset = 0;
			uint32_t insn_addr = 0;
			int existing_offset_size = 0;
			uint8_t *insn_bytes = NULL;
			int insn_bytes_len = 0;
			DISASM d;
			ARGTYPE *arg=NULL;
			
			insn_addr = final_insn_locations[insn];
			if (insn_addr == 0)
			{
				if (m_opts.GetVerbose())
					cout << "Skipping unplopped Pcrel relocation." << endl;
				continue;
			}
			assert(insn_addr != 0);

			insn_bytes_len = sizeof(uint8_t)*insn->GetDataBits().length();
			insn_bytes=(uint8_t*)malloc(insn_bytes_len);
			memcpy(insn_bytes, insn->GetDataBits().c_str(), insn_bytes_len);

			insn->Disassemble(d);

			if(arg_has_relative(d.Argument1))
				arg=&d.Argument1;
			if(arg_has_relative(d.Argument2))
				arg=&d.Argument2;
			if(arg_has_relative(d.Argument3))
				arg=&d.Argument3;

			assert(arg);

			memory_offset = arg->Memory.DisplacementAddr-d.EIP;
			existing_offset_size = arg->Memory.DisplacementSize;
			assert(memory_offset>=0 && memory_offset <=15 &&
			      (existing_offset_size==1 || 
			       existing_offset_size==2 || 
						 existing_offset_size==4 || 
						 existing_offset_size==8));

			memcpy((uint8_t*)&existing_offset, 
			       (uint8_t*)&insn_bytes[memory_offset], 
						 existing_offset_size);

			new_offset = existing_offset-insn_addr; 
			if (m_opts.GetVerbose())
				cout << "Relocating a pcrel relocation with 0x" 
				     << std::hex << existing_offset
						 << " existing offset at 0x" 
						 << insn_addr << "." << endl
						 << "Based on: " << d.CompleteInstr << endl
						 << "New address: 0x" << std::hex << new_offset << endl;
			
			m_memory_space.PlopBytes(insn_addr+memory_offset,
			                         (const char*)&new_offset,
															 existing_offset_size);
		}
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
