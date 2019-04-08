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

using namespace Push64Relocs;

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

bool Push64Relocs_t::IsRelocationWithType(Relocation_t *reloc,string type)
{
	return (reloc->getType().find(type) != string::npos);
}

// would be nice to have a FindRelocation function that takes a parameterized type.
Relocation_t* Push64Relocs_t::FindRelocationWithType(Instruction_t* insn, string type)
{
	for(auto reloc : insn->getRelocations())
	{
		if (IsRelocationWithType(reloc, type))
			return reloc;
	}
	return nullptr;
}

void Push64Relocs_t::HandlePush64Relocation(Instruction_t *insn, Relocation_t *reloc)
{
	const auto PUSH_DATA_BITS_MAX_LEN  = 16;
	auto push_data_bits = vector<uint8_t>(PUSH_DATA_BITS_MAX_LEN);

	plopped_relocs.insert(insn);	

	auto push_insn = insn;
	auto jmp_insn = insn->getFallthrough();
	assert(jmp_insn);

	auto push_data_bits_len = push_insn->getDataBits().length();
	assert(push_data_bits_len < PUSH_DATA_BITS_MAX_LEN);
	memcpy(push_data_bits.data(), (uint8_t*)push_insn->getDataBits().c_str(), push_data_bits_len);

	/*
	 * Because we know that this is a push instruction,
	 * we know that the opcode is one byte.
	 * The pushed value will start at the 1th offset.
	 */
	auto push_addr = VirtualOffset_t(0);
	memcpy(&push_addr,&push_data_bits[1], 4);

	if (*m_verbose)
		cout << "push_addr: 0x" << hex << push_addr << endl;
	assert(push_addr != 0);

	/* 
	 * Step 0: Add the add instruction and its address.
	 */
	auto add_addr=m_firp.addNewAddress(push_insn->getAddress()->getFileID(), 0);
	auto add_insn=m_firp.addNewInstruction( add_addr, push_insn->getFunction());

	/* 
	 * Step 1: Change the push to a call 0.
	 */

	auto call_databits = string({int8_t(0xe8), 0x00, 0x00, 0x00, 0x00 });
	insn->setDataBits(call_databits);
	insn->setTarget(add_insn); // Comment
	insn->setFallthrough(nullptr);
	insn->setComment(push_insn->getComment()+" Thunk part");

		
	/* 
	 * Step 2: Create the add instruction.
	 */
	auto add_databits = string();
	if(m_firp.getArchitectureBitWidth()==64)
		add_databits+=string({0x48});	 // rex prefix to convert esp->rsp
	add_databits+=string({(int8_t)0x81, 0x2c, 0x24, (int8_t)0xff, (int8_t)0xff, (int8_t)0xff, (int8_t)0xff} );
	add_insn->setDataBits(add_databits);

	/*
	 * Step 3: Put the relocation on the add instruction.
	 */
	auto add_reloc=m_firp.addNewRelocation(add_insn, push_addr, "add64");

	if (*m_verbose)
		cout << "Adding an add/sub with reloc offset 0x" 
		     << hex << add_reloc->getOffset() << endl;
	/*
	 * Step 4: Tell the add insn to fallthrough to the call.
	 */
	add_insn->setFallthrough(jmp_insn);
}

void Push64Relocs_t::HandlePush64Relocs()
{
	auto push64_relocations_count = 0;
	auto pcrel_relocations_count  = 0;

	// for each instruction 
	for(auto insn : m_firp.getInstructions())
	{
		const auto push_reloc = FindPushRelocation(insn);
		if (push_reloc != nullptr)  
		{
			if (*m_verbose)
				cout << "Found a Push relocation:" << insn->getDisassembly() << endl;
			HandlePush64Relocation(insn,push_reloc);
			push64_relocations_count++;
		}
		const auto pcrel_reloc = FindPcrelRelocation(insn);
		if (pcrel_reloc != nullptr)
		{
			if (*m_verbose)
				cout << "Found a pcrel relocation." << endl;
			plopped_relocs.insert(insn);
			pcrel_relocations_count++;
		}
	}

	cout << "# ATTRIBUTE Push_Relocations::push_relocations_count="
	     << dec << push64_relocations_count << endl;
	cout << "# ATTRIBUTE Push_Relocations::pcrel_relocations_count="
	     << dec << pcrel_relocations_count << endl;
}


void Push64Relocs_t::UpdatePush64Adds()
{
	if (*m_verbose)
		cout << "push64:UpdatePush64Adds()" << endl;

	for(auto insn : plopped_relocs)
	{
		auto reloc = FindPushRelocation(insn);
		if (reloc)
		{
			auto call      = insn;                             assert(call != nullptr);
			auto add       = call->getTarget();                assert(add  != nullptr);
			auto call_addr = final_insn_locations[call];
			auto add_addr  = final_insn_locations[add];
			auto wrt_insn  = dynamic_cast<Instruction_t*>(reloc->getWRT());
			auto wrt_addr  = wrt_insn ?  final_insn_locations[wrt_insn] : RangeAddress_t(0);

			if (call_addr == 0 || add_addr == 0)
			{
				if (*m_verbose)
					cout << "push64:Call/Add pair not plopped?" << endl;
				continue;
			}

			auto add_reloc = FindAdd64Relocation(add);
			assert(add_reloc != nullptr); 

			auto add_offset = add_reloc->getOffset();

			/*
			 * Stupid call will push the NEXT instruction address.
			 */
			auto ret_addr = call_addr + call->getDataBits().length();
			auto change_to_add   = false;

#if 1
			const auto reloc_adjustnent_value = wrt_insn ? wrt_addr : (m_firp.getArchitecture()->getFileBase() + add_offset);
			const auto relocated_value = ret_addr - reloc_adjustnent_value;
#else
			auto relocated_value = uint32_t(0);
			if ((size_t)add_offset > (size_t)ret_addr)
			{
				change_to_add = true;
				if(wrt_insn)
					relocated_value = wrt_addr   - ret_addr;
				else
					relocated_value = add_offset - ret_addr;
			}
			else
			{
				if(wrt_insn)
					relocated_value = ret_addr - wrt_addr;
				else
					relocated_value = ret_addr - (firp->getArchtecture()->getFileBase() + add_offset);
			}
#endif

			cout << "Push64:Relocating a(n) " << ((change_to_add) ? "add":"sub") << " from " 
			     << hex << ret_addr << " at " << hex << add_addr << endl;

			cout << "push64:Using 0x" << hex << relocated_value 
			     << " as the updated offset." << endl;

			cout << "Using 0x" << hex << add_offset 
			     << " as the base offset." << endl;

			const auto rex_skip=m_firp.getArchitectureBitWidth()==64 ? 1 : 0;
#if 0
			if (change_to_add)
			{
				char add = (char)0x04;
				m_memory_space.plopBytes(add_addr+rex_skip+1, (const char*)&add, 1);
			}
#endif
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
