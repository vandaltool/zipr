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

#include <irdb-core>
#include <zipr-sdk>

class Push64Relocs_t : public Zipr_SDK::ZiprPluginInterface_t
{
	public:
		Push64Relocs_t(Zipr_SDK::Zipr_t* zipr_object);

		virtual void doPinningEnd() override
		{ 
			// if(m_elfio.get_type()==ET_EXEC)
			if(m_firp.getArchitecture()->getFileType()==IRDB_SDK::adftELFEXE)
			{
				std::cout<<"Push64_reloc: elide PinningEnd as type==ET_EXEC"<<std::endl;
				return;
			}
			std::cout<<"Push64Plugin: Ending  pinning, applying push64 relocs."<<std::endl;
			HandlePush64Relocs(); 
		}
		virtual void doCallbackLinkingEnd() override
		{
			// if(m_elfio.get_type()==ET_EXEC)
			if(m_firp.getArchitecture()->getFileType()==IRDB_SDK::adftELFEXE)
			{
				std::cout<<"Push64_reloc: elide CallbackLinkingEnd as type==ET_EXEC"<<std::endl;
				return;
			}
			std::cout<<"Push64Plugin: CBLinkEnd, updating adds."  <<std::endl;
			UpdatePush64Adds(); 
		}

		// virtual Zipr_SDK::ZiprOptionsNamespace_t *registerOptions(Zipr_SDK::ZiprOptionsNamespace_t *) override;
	private:
		// main workhorses
		void HandlePush64Relocs();
		void UpdatePush64Adds();

		// subsidiary workhorses 
		void HandlePush64Relocation(IRDB_SDK::Instruction_t* insn, IRDB_SDK::Relocation_t *reloc);

		// helpers
		bool IsPcrelRelocation(IRDB_SDK::Relocation_t *reloc)
		{ return IsRelocationWithType(reloc,"pcrel"); }
		bool IsAdd64Relocation(IRDB_SDK::Relocation_t *reloc)
		{ return IsRelocationWithType(reloc,"add64"); }
		bool IsPush64Relocation(IRDB_SDK::Relocation_t *reloc)
		{ return IsRelocationWithType(reloc,"push64"); }
		bool Is32BitRelocation(IRDB_SDK::Relocation_t *reloc)
		{ return IsRelocationWithType(reloc,"push64"); }

		IRDB_SDK::Relocation_t* FindPcrelRelocation(IRDB_SDK::Instruction_t* insn)
		{ return FindRelocationWithType(insn,"pcrel"); }
		IRDB_SDK::Relocation_t* FindAdd64Relocation(IRDB_SDK::Instruction_t* insn)
		{ return FindRelocationWithType(insn,"add64"); }
		IRDB_SDK::Relocation_t* FindPush64Relocation(IRDB_SDK::Instruction_t* insn)
		{ return FindRelocationWithType(insn,"push64"); }
		IRDB_SDK::Relocation_t* Find32BitRelocation(IRDB_SDK::Instruction_t* insn)
		{ return FindRelocationWithType(insn,"32-bit"); }

		IRDB_SDK::Relocation_t* FindPushRelocation(IRDB_SDK::Instruction_t* insn)
		{ 
			IRDB_SDK::Relocation_t* reloc=NULL;
			if(reloc=FindPush64Relocation(insn))
			{
				return reloc; 
			}
			if(reloc=Find32BitRelocation(insn))
			{
				return reloc; 
			}
			return NULL;
		}

		bool IsRelocationWithType(IRDB_SDK::Relocation_t *reloc, std::string type);
		IRDB_SDK::Relocation_t* FindRelocationWithType(IRDB_SDK::Instruction_t* insn, std::string type);




		// references to input
		Zipr_SDK::MemorySpace_t &m_memory_space;	
		IRDB_SDK::FileIR_t& m_firp;
		Zipr_SDK::InstructionLocationMap_t &final_insn_locations;

		// local data.
		IRDB_SDK::InstructionSet_t plopped_relocs;

		Zipr_SDK::ZiprBooleanOption_t *m_verbose;

};

#endif
