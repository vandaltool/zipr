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

#ifndef push_relocs_h
#define push_relocs_h

#include <libIRDB-core.hpp>

class Push64Relocs_t : public Zipr_SDK::ZiprPluginInterface_t
{
	public:
		Push64Relocs_t(Zipr_SDK::MemorySpace_t *p_ms,
			ELFIO::elfio *p_elfio,
			libIRDB::FileIR_t *p_firp,
			Zipr_SDK::Options_t *p_opts,
			Zipr_SDK::InstructionLocationMap_t *p_fil);
		virtual void PinningBegin()
		{
		}
		virtual void PinningEnd()
		{ 
			cout<<"Push64Plugin: Ending  pinning, applying push64 relocs."<<endl;
			HandlePush64Relocs(); 
		}
		virtual void DollopBegin()
		{
		}
		virtual void DollopEnd()
		{
		}
		virtual void CallbackLinkingBegin()
		{
		}
		virtual void CallbackLinkingEnd()
		{
			cout<<"Push64Plugin: CBLinkEnd, updating adds."  <<endl;
			UpdatePush64Adds(); 
		}

	private:
		// main workhorses
		void HandlePush64Relocs();
		void UpdatePush64Adds();

		// helpers
		bool IsPcrelRelocation(libIRDB::Relocation_t *reloc);
		libIRDB::Relocation_t* FindPcrelRelocation(libIRDB::Instruction_t* insn);
		bool IsAdd64Relocation(libIRDB::Relocation_t *reloc);
		libIRDB::Relocation_t* FindAdd64Relocation(libIRDB::Instruction_t* insn);
		bool IsPush64Relocation(libIRDB::Relocation_t *reloc);
		libIRDB::Relocation_t* FindPush64Relocation(libIRDB::Instruction_t* insn);
		void HandlePush64Relocation(libIRDB::Instruction_t* insn, libIRDB::Relocation_t *reloc);

		// references to input
		Zipr_SDK::MemorySpace_t &m_memory_space;	
		ELFIO::elfio&  m_elfio;
		libIRDB::FileIR_t& m_firp;
		Zipr_SDK::Options_t& m_opts;
		Zipr_SDK::InstructionLocationMap_t &final_insn_locations;

		// local data.
		libIRDB::InstructionSet_t plopped_relocs;

};

#endif
