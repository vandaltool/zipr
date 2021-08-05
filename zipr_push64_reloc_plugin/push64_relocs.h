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

namespace Push64Relocs
{

	using namespace std;
	using namespace IRDB_SDK;
	using namespace Zipr_SDK;

	class Push64Relocs_t : public ZiprPluginInterface_t
	{
		public:
			Push64Relocs_t(Zipr_t* zipr_object);

			virtual void doPinningEnd() override
			{ 
				if(m_firp.getArchitecture()->getFileType()==adftELFEXE)
				{
					cout<<"Push64_reloc: elide PinningEnd as type==ET_EXEC"<<endl;
					return;
				}
				cout<<"Push64Plugin: Ending  pinning, applying push64 relocs."<<endl;
				HandlePush64Relocs(); 
			}
			virtual void doCallbackLinkingEnd() override
			{
				if(m_firp.getArchitecture()->getFileType()==adftELFEXE)
				{
					cout<<"Push64_reloc: elide CallbackLinkingEnd as type==ET_EXEC"<<endl;
					return;
				}
				cout<<"Push64Plugin: CBLinkEnd, updating adds."  <<endl;
				UpdatePush64Adds(); 
			}

		private:
			// main workhorses
			void HandlePush64Relocs();
			void UpdatePush64Adds();

			// subsidiary workhorses 
			void HandlePush64Relocation(Instruction_t* insn, Relocation_t *reloc);

			// helpers
			bool IsPcrelRelocation(Relocation_t *reloc)
			{ return IsRelocationWithType(reloc,"pcrel"); }
			bool IsAdd64Relocation(Relocation_t *reloc)
			{ return IsRelocationWithType(reloc,"add64"); }
			bool IsPush64Relocation(Relocation_t *reloc)
			{ return IsRelocationWithType(reloc,"push64"); }
			bool Is32BitRelocation(Relocation_t *reloc)
			{ return IsRelocationWithType(reloc,"push64"); }

			Relocation_t* FindPcrelRelocation(Instruction_t* insn)
			{ return FindRelocationWithType(insn,"pcrel"); }
			Relocation_t* FindAdd64Relocation(Instruction_t* insn)
			{ return FindRelocationWithType(insn,"add64"); }
			Relocation_t* FindPush64Relocation(Instruction_t* insn)
			{ return FindRelocationWithType(insn,"push64"); }
			Relocation_t* Find32BitRelocation(Instruction_t* insn)
			{ return FindRelocationWithType(insn,"32-bit"); }

			Relocation_t* FindPushRelocation(Instruction_t* insn)
			{ 
				auto reloc=FindPush64Relocation(insn);
				if(reloc != nullptr)
				{
					return reloc; 
				}
				reloc=Find32BitRelocation(insn);
				if(reloc != nullptr)
				{
					return reloc; 
				}
				return nullptr;
			}

			bool IsRelocationWithType(Relocation_t *reloc, string type);
			Relocation_t* FindRelocationWithType(Instruction_t* insn, string type);


			// references to input
			MemorySpace_t &m_memory_space;	
			FileIR_t& m_firp;
			InstructionLocationMap_t &final_insn_locations;

			// local data.
			InstructionSet_t plopped_relocs;

			ZiprBooleanOption_t *m_verbose;

	};

}
#endif
