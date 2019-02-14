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


#include <string>
#include <algorithm>
#include "push64_relocs.h"

using namespace IRDB_SDK;
using namespace Zipr_SDK;
using namespace std;

#define ALLOF(a) begin(a), end(a)

Push64Relocs_t::Push64Relocs_t(Zipr_SDK::Zipr_t* zipr_object)
	:
		m_memory_space      (*zipr_object->getMemorySpace()),
		m_firp              (*zipr_object->getFileIR()),
		final_insn_locations(*zipr_object->getLocationMap())
{
	auto global= zipr_object->getOptionsManager()->getNamespace("global");
	m_verbose  = global->getBooleanOption("verbose");
}

bool Push64Relocs_t::IsRelocationWithType(Relocation_t *reloc,std::string type)
{
	return (reloc->getType().find(type) != std::string::npos);
}

// would be nice to have a FindRelocation function that takes a parameterized type.
Relocation_t* Push64Relocs_t::FindRelocationWithType(Instruction_t* insn, std::string type)
{
	Instruction_t* first_slow_path_insn=NULL;
	RelocationSet_t::iterator rit = insn->getRelocations().begin();
	for(rit; rit!=insn->getRelocations().end(); rit++)
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
	// Instruction_t *push_insn = NULL, *jmp_insn = NULL;
	VirtualOffset_t push_addr = 0;
	string databits = "";
	uint8_t push_data_bits[PUSH_DATA_BITS_MAX_LEN] = {0,};
	int push_data_bits_len = 0;

	plopped_relocs.insert(insn);	

	auto push_insn = insn;
	auto jmp_insn = insn->getFallthrough();
	assert(jmp_insn);

	push_data_bits_len = push_insn->getDataBits().length();
	assert(push_data_bits_len<PUSH_DATA_BITS_MAX_LEN);
	memcpy(push_data_bits,
	       (uint8_t*)push_insn->getDataBits().c_str(),
				 push_data_bits_len);
	/*
	 * Because we know that this is a push instruction,
	 * we know that the opcode is one byte.
	 * The pushed value will start at the 1th offset.
	 */
	push_addr = *((VirtualOffset_t*)(&push_data_bits[1]));

	if (*m_verbose)
		cout << "push_addr: 0x" << std::hex << push_addr << endl;
	assert(push_addr != 0);

	/* 
	 * Step 0: Add the add instruction and its address.
	 */
	auto add_addr=m_firp.addNewAddress(push_insn->getAddress()->getFileID(), 0);
	auto add_insn=m_firp.addNewInstruction(
			add_addr,
			push_insn->getFunction()
			);
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
	insn->setDataBits(databits);
	insn->setTarget(add_insn); // Comment
	insn->setFallthrough(NULL);
	insn->setComment(push_insn->getComment()+" Thunk part");

		
	/* 
	 * Step 2: Create the add instruction.
	 */
// this is OK, but could we consider the insn->Assemble() method for readability? 
	databits = "";
	if(m_firp.getArchitectureBitWidth()==64)
		databits+=(char)0x48;	 // rex prefix to convert esp->rsp
	databits+=(char)0x81;
	databits+=(char)0x2c;	
	databits+=(char)0x24;
	databits+=(char)0xff;
	databits+=(char)0xff;
	databits+=(char)0xff;
	databits+=(char)0xff;
	add_insn->setDataBits(databits);

	/*
	 * Step 3: Put the relocation on the add instruction.
	 */
	auto add_reloc=m_firp.addNewRelocation(add_insn,push_addr,"add64");

	if (*m_verbose)
		cout << "Adding an add/sub with reloc offset 0x" 
		     << std::hex << add_reloc->getOffset() 
				 << endl;
	/*
	 * Step 4: Tell the add insn to fallthrough to the call.
	 */
	add_insn->setFallthrough(jmp_insn);
}

void Push64Relocs_t::HandlePush64Relocs()
{
	int push64_relocations_count=0;
	int pcrel_relocations_count=0;
	// for each instruction 
	InstructionSet_t::iterator iit = m_firp.getInstructions().begin();
	for(iit; iit!=m_firp.getInstructions().end(); iit++)
	{
		Instruction_t *insn=*iit;

		Relocation_t *reloc=NULL;
		// caution, side effect in if statement.
		if (reloc = FindPushRelocation(insn))
		{
			if (*m_verbose)
				cout << "Found a Push relocation:" << insn->getDisassembly()<<endl;
			HandlePush64Relocation(insn,reloc);
			push64_relocations_count++;
		}
		// caution, side effect in if statement.
		else if (reloc = FindPcrelRelocation(insn))
		{
			if (*m_verbose)
				cout << "Found a pcrel relocation." << endl;
			plopped_relocs.insert(insn);
			pcrel_relocations_count++;
		}
	}

	cout<<"# ATTRIBUTE Push_Relocations::push_relocations_count="
	    <<std::dec<<push64_relocations_count
			<<endl;
	cout<<"# ATTRIBUTE Push_Relocations::pcrel_relocations_count="
	    <<std::dec<<pcrel_relocations_count
			<<endl;
}


void Push64Relocs_t::UpdatePush64Adds()
{
	if (*m_verbose)
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
			add = call->getTarget();

			assert(call && add);

			call_addr = final_insn_locations[call];
			add_addr = final_insn_locations[add];
			Instruction_t* wrt_insn=dynamic_cast<Instruction_t*>(reloc->getWRT());
			if(wrt_insn)
				wrt_addr=final_insn_locations[wrt_insn];

			if (call_addr == 0 || add_addr == 0)
			{
				if (*m_verbose)
					cout << "push64:Call/Add pair not plopped?" << endl;
				continue;
			}

			add_reloc = FindAdd64Relocation(add);
			assert(add_reloc && "push64:Add in Call/Add pair must have relocation.");

			add_offset = add_reloc->getOffset();

			/*
			 * Stupid call will push the NEXT instruction address.
			 */
			call_addr+=call->getDataBits().length();


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
			const auto rex_skip=m_firp.getArchitectureBitWidth()==64 ? 1 : 0;
			if (change_to_add)
			{
				char add = (char)0x04;
				m_memory_space.plopBytes(add_addr+rex_skip+1, (const char*)&add, 1);
			}
			m_memory_space.plopBytes(add_addr+rex_skip+3, (const char*)&relocated_value, 4);
		}
	}
}

extern "C" 
Zipr_SDK::ZiprPluginInterface_t* GetPluginInterface(
	Zipr_SDK::Zipr_t* zipr_object)
{
	return new Push64Relocs_t(zipr_object);
}
