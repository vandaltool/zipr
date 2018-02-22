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
#include <libIRDB-core.hpp>
#include <string>
#include <algorithm>
#include "utils.hpp"
#include "Rewrite_Utility.hpp"
#include "push64_relocs.h"

using namespace libIRDB;
using namespace std;
using namespace Zipr_SDK;
using namespace ELFIO;

#define ALLOF(a) begin(a), end(a)

/*
bool arg_has_relative(const ARGTYPE &arg)
{
	// if it's relative memory, watch out! 
	if(arg.ArgType&MEMORY_TYPE)
		if(arg.ArgType&RELATIVE_)
			return true;
	
	return false;
}
*/

Push64Relocs_t::Push64Relocs_t(MemorySpace_t *p_ms,
	elfio *p_elfio,
	FileIR_t *p_firp,
	InstructionLocationMap_t *p_fil) :
		m_memory_space(*p_ms), 
		m_elfio(*p_elfio),
		m_firp(*p_firp),
		final_insn_locations(*p_fil),
		m_verbose("verbose")
{
}
ZiprOptionsNamespace_t *Push64Relocs_t::RegisterOptions(ZiprOptionsNamespace_t *global) {
	global->AddOption(&m_verbose);
	return NULL;
}

bool Push64Relocs_t::IsRelocationWithType(Relocation_t *reloc,std::string type)
{
	return (reloc->GetType().find(type) != std::string::npos);
}

// would be nice to have a FindRelocation function that takes a parameterized type.
Relocation_t* Push64Relocs_t::FindRelocationWithType(Instruction_t* insn, std::string type)
{
	Instruction_t* first_slow_path_insn=NULL;
	RelocationSet_t::iterator rit = insn->GetRelocations().begin();
	for(rit; rit!=insn->GetRelocations().end(); rit++)
	{
		Relocation_t *reloc=*rit;
		if (IsRelocationWithType(reloc, type))
			return reloc;
	}
	return NULL;
}

#define PUSH_DATA_BITS_MAX_LEN 16
void Push64Relocs_t::HandlePush64Relocation(Instruction_t *insn, Relocation_t *reloc)
{
	Instruction_t *add_insn = new Instruction_t;
	AddressID_t *add_addr = new AddressID_t;
	Instruction_t *push_insn = NULL, *jmp_insn = NULL;
	Relocation_t *add_reloc = new Relocation_t;
	virtual_offset_t push_addr = 0;
	string databits = "";
	uint8_t push_data_bits[PUSH_DATA_BITS_MAX_LEN] = {0,};
	int push_data_bits_len = 0;

	plopped_relocs.insert(insn);	

	push_insn = insn;
	jmp_insn = insn->GetFallthrough();
	assert(jmp_insn);

	push_data_bits_len = push_insn->GetDataBits().length();
	assert(push_data_bits_len<PUSH_DATA_BITS_MAX_LEN);
	memcpy(push_data_bits,
	       (uint8_t*)push_insn->GetDataBits().c_str(),
				 push_data_bits_len);
	/*
	 * Because we know that this is a push instruction,
	 * we know that the opcode is one byte.
	 * The pushed value will start at the 1th offset.
	 */
	push_addr = *((virtual_offset_t*)(&push_data_bits[1]));

	if (m_verbose)
		cout << "push_addr: 0x" << std::hex << push_addr << endl;
	assert(push_addr != 0);

	/* 
	 * Step 0: Add the add instruction and its address.
	 */
	add_addr->SetFileID(push_insn->GetAddress()->GetFileID());
	add_insn->SetAddress(add_addr);
	add_insn->SetFunction(push_insn->GetFunction());
	m_firp.GetAddresses().insert(add_addr);
	m_firp.GetInstructions().insert(add_insn);

	/* 
	 * Step 1: Change the push to a call 0.
	 */

// this is OK, but could we consider the insn->Assemble() method for readability? 
	databits.resize(5);
	databits[0] = 0xe8;
	databits[1] = 0x00;
	databits[2] = 0x00;
	databits[3] = 0x00;
	databits[4] = 0x00;
	insn->SetDataBits(databits);
	insn->SetTarget(add_insn); // Comment
	insn->SetFallthrough(NULL);
	insn->SetComment(push_insn->GetComment()+" Thunk part");
		
	/* 
	 * Step 2: Create the add instruction.
	 */
// this is OK, but could we consider the insn->Assemble() method for readability? 
	databits = "";
	databits.resize(8);
	databits[0]=0x48;
	databits[1]=0x81;
	databits[2]=0x2c;	
	databits[3]=0x24;
	databits[4]=0xff;
	databits[5]=0xff;
	databits[6]=0xff;
	databits[7]=0xff;
	add_insn->SetDataBits(databits);

	/*
	 * Step 3: Put the relocation on the add instruction.
	 */
	add_reloc->SetOffset(push_addr);
	add_reloc->SetType("add64");
	add_insn->GetRelocations().insert(add_reloc);
	m_firp.GetRelocations().insert(add_reloc);

	if (m_verbose)
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
	int push64_relocations_count=0;
	int pcrel_relocations_count=0;
	// for each instruction 
	InstructionSet_t::iterator iit = m_firp.GetInstructions().begin();
	for(iit; iit!=m_firp.GetInstructions().end(); iit++)
	{
		Instruction_t *insn=*iit;

		Relocation_t *reloc=NULL;
		// caution, side effect in if statement.
		if (reloc = FindPushRelocation(insn))
		{
			if (m_verbose)
				cout << "Found a Push relocation:" << insn->getDisassembly()<<endl;
			HandlePush64Relocation(insn,reloc);
			push64_relocations_count++;
		}
		// caution, side effect in if statement.
		else if (reloc = FindPcrelRelocation(insn))
		{
			if (m_verbose)
				cout << "Found a pcrel relocation." << endl;
			plopped_relocs.insert(insn);
			pcrel_relocations_count++;
		}
	}

	cout<<"#ATTRIBUTE push_relocations_count="
	    <<std::dec<<push64_relocations_count
			<<endl;
	cout<<"#ATTRIBUTE pcrel_relocations_count="
	    <<std::dec<<pcrel_relocations_count
			<<endl;
}


void Push64Relocs_t::UpdatePush64Adds()
{
	if (m_verbose)
		cout << "push64:UpdatePush64Adds()" << endl;
	InstructionSet_t::iterator insn_it = plopped_relocs.begin();
	for (insn_it; insn_it != plopped_relocs.end(); insn_it++)
	{
		Relocation_t *reloc = NULL;
		Instruction_t *insn = *insn_it;
		// caution, side effect in if statement.
		if (reloc = FindPushRelocation(insn))
		{
// would consider updating this if statement to be a function call for simplicity/readability.
			bool change_to_add = false;
			RangeAddress_t call_addr = 0;
			RangeAddress_t add_addr = 0;
			RangeAddress_t wrt_addr = 0;
			int add_offset = 0;
			uint32_t relocated_value = 0;
			Instruction_t *call = NULL, *add = NULL;
			Relocation_t *add_reloc = NULL;

			call = *insn_it;
			add = call->GetTarget();

			assert(call && add);

			call_addr = final_insn_locations[call];
			add_addr = final_insn_locations[add];
			Instruction_t* wrt_insn=dynamic_cast<Instruction_t*>(reloc->GetWRT());
			if(wrt_insn)
				wrt_addr=final_insn_locations[wrt_insn];

			if (call_addr == 0 || add_addr == 0)
			{
				if (m_verbose)
					cout << "push64:Call/Add pair not plopped?" << endl;
				continue;
			}

			add_reloc = FindAdd64Relocation(add);
			assert(add_reloc && "push64:Add in Call/Add pair must have relocation.");

			add_offset = add_reloc->GetOffset();

			/*
			 * Stupid call will push the NEXT instruction address.
			 */
			call_addr+=call->GetDataBits().length();


// would this be simpler if we always used an add (or sub)
// and just signed the sign of the value we are adding (or subbing)?
			if (add_offset>call_addr)
			{
				change_to_add = true;
				if(wrt_insn)
					relocated_value= wrt_addr - call_addr;
				else
					relocated_value = add_offset - call_addr;
			}
			else
			// never covert it, a sub with a negative value is just fine.
			{
				if(wrt_insn)
					relocated_value= call_addr - wrt_addr;
				else
					relocated_value = call_addr - add_offset;
			}

			cout << "Push64:Relocating a(n) "<< ((change_to_add) ? "add":"sub") << " from " 
			     << std::hex << call_addr 
					 << " at "
					 << std::hex << add_addr
					 << endl
			     << "push64:Using 0x" << std::hex << relocated_value 
			     << " as the updated offset." << endl
					 << "Using 0x" << std::hex << add_offset 
					 << " as the base offset." << endl;
			if (change_to_add)
			{
				char add = (char)0x04;
				m_memory_space.PlopBytes(add_addr+2, (const char*)&add, 1);
			}
			m_memory_space.PlopBytes(add_addr+4, (const char*)&relocated_value, 4);
		}
		// handle basic pcrel relocations.
		// zipr_unpin_plugin handles pcrel + WRT
		// caution, side effect in if statement.
		else if ( (reloc = FindPcrelRelocation(insn)) != NULL && reloc->GetWRT()==NULL)
		{
// would consider updating this if statement to be a function call for simplicity/readability.
			uint8_t memory_offset = 0;
			int32_t existing_offset = 0;
			int32_t new_offset = 0;
			uint32_t insn_addr = 0;
			int existing_offset_size = 0;
			uint8_t *insn_bytes = NULL;
			int insn_bytes_len = 0;
			//DISASM d;
			//ARGTYPE *arg=NULL;
#if 1	
			insn_addr = final_insn_locations[insn];
			if (insn_addr == 0)
			{
				if (m_verbose)
					cout << "push64:Skipping unplopped Pcrel relocation." << endl;
				continue;
			}
			assert(insn_addr != 0);

			insn_bytes_len = sizeof(uint8_t)*insn->GetDataBits().length();
			insn_bytes=(uint8_t*)malloc(insn_bytes_len);
			memcpy(insn_bytes, insn->GetDataBits().c_str(), insn_bytes_len);

			DecodedInstruction_t d(insn);
			/* Disassemble(insn,d);

			if(arg_has_relative(d.Argument1))
				arg=&d.Argument1;
			if(arg_has_relative(d.Argument2))
				arg=&d.Argument2;
			if(arg_has_relative(d.Argument3))
				arg=&d.Argument3;
			assert(arg);
			*/
			const auto operands=d.getOperands();
			const auto arg_it=find_if(ALLOF(operands),[](const DecodedOperand_t& op) { return op.isMemory() && op.isPcrel(); });
			assert(arg_it!=operands.end());
			const auto arg=*arg_it;

			memory_offset = d.getMemoryDisplacementOffset(arg, insn); // arg->Memory.DisplacementAddr-d.EIP;
			existing_offset_size = arg.getMemoryDisplacementEncodingSize(); // arg->Memory.DisplacementSize;
			assert(memory_offset>=0 && memory_offset <=15 &&
			      (existing_offset_size==1 || 
			       existing_offset_size==2 || 
						 existing_offset_size==4 || 
						 existing_offset_size==8));

			memcpy((uint8_t*)&existing_offset, 
			       (uint8_t*)&insn_bytes[memory_offset], 
						 existing_offset_size);

			new_offset = existing_offset-insn_addr; 
			if (m_verbose)
				cout << "Relocating a pcrel relocation with 0x" 
				     << std::hex << existing_offset
						 << " existing offset at 0x" 
						 << insn_addr << "." << endl
						 << "Based on: " << d.getDisassembly() /*CompleteInstr*/ << endl
						 << "New address: 0x" << std::hex << new_offset << endl;
			
			m_memory_space.PlopBytes(insn_addr+memory_offset,
			                         (const char*)&new_offset,
															 existing_offset_size);
#endif
		}
	}
}

extern "C" 
Zipr_SDK::ZiprPluginInterface_t* GetPluginInterface(
	Zipr_SDK::Zipr_t* zipr_object)
{
	Zipr_SDK::MemorySpace_t *p_ms=zipr_object->GetMemorySpace(); 
	ELFIO::elfio *p_elfio=zipr_object->GetELFIO(); 
	libIRDB::FileIR_t *p_firp=zipr_object->GetFileIR();
	Zipr_SDK::InstructionLocationMap_t *p_fil=zipr_object->GetLocationMap(); 
	return new Push64Relocs_t(p_ms,p_elfio,p_firp,p_fil);
}
