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

#ifndef unpin_h
#define unpin_h

#include <irdb-core>
#include <zipr-sdk>

class Unpin_t : public Zipr_SDK::ZiprPluginInterface_t
{
	public:
		Unpin_t( Zipr_SDK::Zipr_t* zipr_object) 
			: 
				zo(zipr_object), 
				unpins(0),
		                missed_unpins(0),
                		ms(*zo->getMemorySpace()),
                		locMap(*(zo->getLocationMap())),
                		firp(*(zo->getFileIR()))

		{ 

			auto global=zo->getOptionsManager()->getNamespace("global");
			auto unpin =zo->getOptionsManager()->getNamespace("unpin" );

			m_verbose =        global->getBooleanOption("verbose");
			m_should_cfi_pin = unpin ->getBooleanOption("should_cfi_pin", "Pin CFI instructions.", false);
			m_on =             unpin ->getBooleanOption("on","Turn unpin plugin on/off.", true);
			m_max_unpins =     unpin ->getIntegerOption("max-unpins","Set how many unpins are allowed, useful for debugging.",-1);

		}

		virtual ~Unpin_t() 
		{ } 

		virtual void doPinningBegin() override
		{
			if(!m_on) return;
			DoUnpin();
		}
		virtual void doCallbackLinkingEnd() override
		{
			if(!m_on) return;
			DoUpdate();
		}

		// virtual Zipr_SDK::ZiprOptionsNamespace_t *registerOptions(Zipr_SDK::ZiprOptionsNamespace_t *) override;

		Zipr_SDK::ZiprPreference retargetCallback(
			const Zipr_SDK::RangeAddress_t &callback_address,
			const Zipr_SDK::DollopEntry_t *callback_entry,
			Zipr_SDK::RangeAddress_t &target_address) override;
	protected:
		// designed for arch-specific override.
		virtual void HandleRetAddrReloc(IRDB_SDK::Instruction_t* from_insn,IRDB_SDK::Relocation_t* reloc)=0;
		virtual void HandlePcrelReloc(IRDB_SDK::Instruction_t* from_insn,IRDB_SDK::Relocation_t* reloc)=0;
		virtual void HandleAbsptrReloc(IRDB_SDK::Instruction_t* from_insn,IRDB_SDK::Relocation_t* reloc)=0;
		virtual void HandleImmedptrReloc(IRDB_SDK::Instruction_t* from_insn,IRDB_SDK::Relocation_t* reloc)=0;
		virtual void HandleCallbackReloc(IRDB_SDK::Instruction_t* from_insn,IRDB_SDK::Relocation_t* reloc)=0;

		bool should_cfi_pin(IRDB_SDK::Instruction_t* insn);

		// workhorses 
		void DoUnpin();
		void DoUnpinForScoops();
		void DoUnpinForFixedCalls();

		void DoUpdate();
		void DoUpdateForScoops();
		void DoUpdateForInstructions();

		Zipr_SDK::Zipr_t* zo;

		Zipr_SDK::ZiprBooleanOption_t *m_verbose;
		Zipr_SDK::ZiprBooleanOption_t *m_should_cfi_pin;
		Zipr_SDK::ZiprBooleanOption_t *m_on; 
		Zipr_SDK::ZiprIntegerOption_t *m_max_unpins; 

		int unpins;
		int missed_unpins=0;
		Zipr_SDK::MemorySpace_t& ms;
		Zipr_SDK::InstructionLocationMap_t& locMap;
		IRDB_SDK::FileIR_t& firp;

};


class UnpinX86_t : public Unpin_t
{
	public:
		UnpinX86_t( Zipr_SDK::Zipr_t* zipr_object) 
			: Unpin_t(zipr_object)

		{ 
		}
	protected:
		// designed for arch-specific override.
		void HandleRetAddrReloc(IRDB_SDK::Instruction_t* from_insn,IRDB_SDK::Relocation_t* reloc) override;
		void HandlePcrelReloc(IRDB_SDK::Instruction_t* from_insn,IRDB_SDK::Relocation_t* reloc) override;
		void HandleAbsptrReloc(IRDB_SDK::Instruction_t* from_insn,IRDB_SDK::Relocation_t* reloc) override;
		void HandleImmedptrReloc(IRDB_SDK::Instruction_t* from_insn,IRDB_SDK::Relocation_t* reloc) override;
		void HandleCallbackReloc(IRDB_SDK::Instruction_t* from_insn,IRDB_SDK::Relocation_t* reloc) override;


};

class UnpinAarch64_t : public Unpin_t
{
	public:
		UnpinAarch64_t( Zipr_SDK::Zipr_t* zipr_object) 
			: Unpin_t(zipr_object)

		{ 
		}
	protected:
		// designed for arch-specific override.
		void HandleRetAddrReloc(IRDB_SDK::Instruction_t* from_insn,IRDB_SDK::Relocation_t* reloc) override;
		void HandlePcrelReloc(IRDB_SDK::Instruction_t* from_insn,IRDB_SDK::Relocation_t* reloc) override;
		void HandleAbsptrReloc(IRDB_SDK::Instruction_t* from_insn,IRDB_SDK::Relocation_t* reloc) override;
		void HandleImmedptrReloc(IRDB_SDK::Instruction_t* from_insn,IRDB_SDK::Relocation_t* reloc) override;
		void HandleCallbackReloc(IRDB_SDK::Instruction_t* from_insn,IRDB_SDK::Relocation_t* reloc) override;


};
#endif
