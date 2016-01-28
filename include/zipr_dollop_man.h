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

#include <dollop.h>

class ZiprDollopManager_t {
	public:
		ZiprDollopManager_t() : m_refresh_stats(true) {}
		void AddDollops(Dollop_t *dollop_head);
		Zipr_SDK::Dollop_t *AddNewDollops(libIRDB::Instruction_t *start);
		Zipr_SDK::Dollop_t *GetContainingDollop(libIRDB::Instruction_t *insn);
		size_t Size() {
			return m_dollops.size();
		}
		void AddDollopPatch(Zipr_SDK::DollopPatch_t *new_patch) {
			m_patches_to_dollops[new_patch->Target()].push_back(new_patch);
		}
		std::list<DollopPatch_t*> PatchesToDollop(Dollop_t *target) {
			/*
			 * FIXME: This will throw an exception if target is
			 * not already in the list. [] fixes that but allocates.
			 * Decide what to do.
			 */
			return m_patches_to_dollops.at(target);
		}
		void PrintDollopPatches(const std::ostream &);
		bool UpdateTargets(Dollop_t *);
		void UpdateAllTargets();
		std::list<Dollop_t*>::const_iterator dollops_begin() {
			return m_dollops.begin();
		}
		std::list<Dollop_t*>::const_iterator dollops_end() {
			return m_dollops.end();
		}
		friend std::ostream &operator<<(std::ostream &out, const ZiprDollopManager_t &dollop_man);
		void PrintStats(std::ostream &out);
		void PrintPlacementMap(const ZiprMemorySpace_t &memory_space,
		                       const std::string &map_filename);
	private:
		void AddDollop(Dollop_t *dollop);
		void CalculateStats();

		std::list<Dollop_t*> m_dollops;
		std::map<libIRDB::Instruction_t*,Dollop_t*> m_insn_to_dollop;
		std::list<DollopPatch_t*> m_patches;
		std::map<Dollop_t*, std::list<DollopPatch_t*>> m_patches_to_dollops;
		bool m_refresh_stats;

		size_t m_total_dollop_space, m_total_dollop_entries;
		unsigned int m_total_dollops, m_truncated_dollops;
};
#endif
