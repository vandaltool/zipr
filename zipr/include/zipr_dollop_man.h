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

#ifndef zipr_dollop_man_h
#define zipr_dollop_man_h 

#include <zipr-sdk>

namespace zipr
{
	class DollopPatch_t;

	typedef		std::set<Dollop_t*> DollopList_t;
	typedef		std::map<IRDB_SDK::Instruction_t*,Dollop_t*>  InsnToDollopMap_t;
	typedef		std::list<DollopPatch_t*>  DollopPatchList_t;
	typedef		std::map<Dollop_t*, DollopPatchList_t > DollopToDollopPatchListMap_t;

	class ZiprDollopManager_t : public DollopManager_t 
	{
		public:
			ZiprDollopManager_t() : m_refresh_stats(true), m_zipr(nullptr) {}
			ZiprDollopManager_t(Zipr_SDK::Zipr_t *zipr) : m_refresh_stats(true), m_zipr(zipr) {}

			/*
			 * Adders.
			 */
			void addDollops(Dollop_t *dollop_head) { return AddDollops(dollop_head); }
			void AddDollops(Dollop_t *dollop_head);


			Zipr_SDK::Dollop_t *addNewDollops(IRDB_SDK::Instruction_t *start) { return AddNewDollops(start); } 
			Zipr_SDK::Dollop_t *AddNewDollops(IRDB_SDK::Instruction_t *start);

			/*
			 * Getters.
			 */
			Zipr_SDK::Dollop_t *getContainingDollop(IRDB_SDK::Instruction_t *insn);

			size_t getSize() { return Size(); } 
			size_t Size() {
				return m_dollops.size();
			}

			/*
			 * Patch functions.
			 */
			void addDollopPatch(DollopPatch_t *new_patch) { return AddDollopPatch(new_patch); } 
			void AddDollopPatch(DollopPatch_t *new_patch) ; 

			std::list<DollopPatch_t*> getPatchesToDollop(Dollop_t *target) { return PatchesToDollop(target); } 
			std::list<DollopPatch_t*> PatchesToDollop(Dollop_t *target) {
				/*
				 * FIXME: This will throw an exception if target is
				 * not already in the list. [] fixes that but allocates.
				 * Decide what to do.
				 */
				return m_patches_to_dollops.at(target);
			}

			/*
			 * Dollop target update functions.
			 */
			bool updateTargets(Dollop_t *t) { return UpdateTargets(t); } 
			bool UpdateTargets(Dollop_t *t);

			void updateAllTargets() { return UpdateAllTargets(); }
			void UpdateAllTargets();

			/*
			 * Iteration functions.
			 */
			DollopList_t::iterator begin() { return dollops_begin(); } 
			DollopList_t::iterator end()   { return dollops_end();   }  

			DollopList_t::iterator dollops_begin() {
				return m_dollops.begin();
			}
			DollopList_t::iterator dollops_end() {
				return m_dollops.end();
			}
			DollopList_t& getDollops() { return m_dollops; }
			const DollopList_t& getDollops() const { return m_dollops; }

			/*
			 * Printing/output functions.
			 */
			void printDollopPatches(const std::ostream &o) { return PrintDollopPatches(o); } 
			void PrintDollopPatches(const std::ostream &o);

			friend std::ostream &operator<<(std::ostream &out,
							const ZiprDollopManager_t &dollop_man);

			void printStats(std::ostream &out) { return PrintStats(out); } 
			void PrintStats(std::ostream &out);

			void printPlacementMap(const MemorySpace_t &memory_space, const std::string &map_filename) { return PrintPlacementMap(memory_space,map_filename); }
			void PrintPlacementMap(const MemorySpace_t &memory_space, const std::string &map_filename);
		
			/*
			 * Helper functions.
			 */
			size_t determineDollopEntrySize(DollopEntry_t *entry) { return DetermineDollopEntrySize(entry); } 
			size_t DetermineDollopEntrySize(DollopEntry_t *entry);

			Zipr_SDK::Zipr_t* GetZipr() const { return m_zipr; }
		private:
			/*
			 * Helper functions.
			 */
			void addDollop(Dollop_t *dollop);
			void CalculateStats();

			/*
			 * Support variables.
			 */
			DollopList_t m_dollops;
			InsnToDollopMap_t m_insn_to_dollop;
			DollopPatchList_t m_patches;
			DollopToDollopPatchListMap_t m_patches_to_dollops;

			/*
			 * Statistics.
			 */
			bool m_refresh_stats;
			size_t m_total_dollop_space, m_total_dollop_entries;
			unsigned int m_total_dollops, m_truncated_dollops;

			Zipr_SDK::Zipr_t* m_zipr;
	};
}
#endif
