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

#ifndef zipr_impl_h
#define zipr_impl_h

class ZiprOptions_t;
class Stats_t;

class ZiprImpl_t : public Zipr_t
{
	public:
		ZiprImpl_t(libIRDB::FileIR_t* p_firp, ZiprOptions_t &p_opts) :
			m_firp(p_firp), 
			m_opts(p_opts), 
			m_stats(NULL), 
			elfiop(new ELFIO::elfio), 
			start_of_new_space(0),
			memory_space(&p_opts), 
			plugman(&memory_space, elfiop, p_firp, (Zipr_SDK::Options_t*)&p_opts, &final_insn_locations)
		{ 
			bss_needed=0;
			use_stratafier_mode=false;

			// init  pinned addresses map.
			RecordPinnedInsnAddrs();
 		};

		void CreateBinaryFile(const std::string &name);

		int DetermineWorstCaseInsnSize(libIRDB::Instruction_t*);
		Zipr_SDK::RangeAddress_t PlopInstruction(libIRDB::Instruction_t* insn, Zipr_SDK::RangeAddress_t addr);
		Zipr_SDK::RangeAddress_t PlopWithTarget(libIRDB::Instruction_t* insn, Zipr_SDK::RangeAddress_t at);
		Zipr_SDK::RangeAddress_t PlopWithCallback(libIRDB::Instruction_t* insn, Zipr_SDK::RangeAddress_t at);

	private:

		// data for the stuff we're rewriting.
		libIRDB::FileIR_t* m_firp;
		ZiprOptions_t& m_opts;
		Stats_t *m_stats;

		Zipr_SDK::RangeAddress_t _PlopInstruction(libIRDB::Instruction_t*, Zipr_SDK::RangeAddress_t);
		int _DetermineWorstCaseInsnSize(libIRDB::Instruction_t* );
		void FindFreeRanges(const std::string &name);
		void AddPinnedInstructions();
		void ReservePinnedInstructions();
		void PreReserve2ByteJumpTargets();
		void ExpandPinnedInstructions();
		void Fix2BytePinnedInstructions();
		void OptimizePinnedInstructions();
		void OptimizePinnedFallthroughs();
		void AskPluginsAboutPlopping();
		void PlopTheUnpinnedInstructions();
		void UpdateCallbacks();
		void PrintStats();
		void RecordPinnedInsnAddrs();


		// patching
		void PatchJump(RangeAddress_t at_addr, RangeAddress_t to_addr);
		void ApplyPatches(libIRDB::Instruction_t* insn);
		void PatchInstruction(RangeAddress_t addr, libIRDB::Instruction_t* insn);
		void RewritePCRelOffset(RangeAddress_t from_addr,RangeAddress_t to_addr, int insn_length, int offset_pos);
		void ApplyPatch(RangeAddress_t from_addr, RangeAddress_t to_addr);
		void PatchCall(RangeAddress_t at_addr, RangeAddress_t to_addr);
		void CallToNop(RangeAddress_t at_addr);

		// outputing new .exe
		void FillSection(ELFIO::section* sec, FILE* fexe);
		void OutputBinaryFile(const std::string &name);


		// helpers.
		void ProcessUnpinnedInstruction(const UnresolvedUnpinned_t &uu, const Patch_t &p);
		void InsertNewSegmentIntoExe(std::string old_file, std::string new_file, RangeAddress_t sec_start);
		std::string AddCallbacksToNewSegment(const std::string& tmpname, RangeAddress_t end_of_new_space);
		RangeAddress_t FindCallbackAddress(RangeAddress_t end_of_new_space,RangeAddress_t start_addr, const std::string &callback);
		libIRDB::Instruction_t *FindPinnedInsnAtAddr(RangeAddress_t addr);
		bool ShouldPinImmediately(libIRDB::Instruction_t *upinsn);

		// support
		RangeAddress_t extend_section(ELFIO::section *sec, ELFIO::section *next_sec);



	private:
		// structures necessary for ZIPR algorithm.
		std::set<UnresolvedUnpinned_t> unresolved_unpinned_addrs;
		std::set<UnresolvedPinned_t> unresolved_pinned_addrs; 
		std::multimap<UnresolvedUnpinned_t,Patch_t> patch_list;

		// map of where bytes will actually go.
//		std::map<RangeAddress_t,char> byte_map;

		// structures to pinned things.
		std::set<UnresolvedPinned_t> two_byte_pins; 
		std::map<UnresolvedPinned_t,RangeAddress_t> five_byte_pins; 

		// final mapping of instruction to address.
		std::map<libIRDB::Instruction_t*,RangeAddress_t> final_insn_locations; 
		std::map<RangeAddress_t,libIRDB::Instruction_t*> m_InsnAtAddrs; 

		// unpatched callbacks
		std::set<std::pair<libIRDB::Instruction_t*,RangeAddress_t> > unpatched_callbacks; 

		std::map<std::string,RangeAddress_t> callback_addrs;

		// way to read elf headers, etc.
		ELFIO::elfio*    elfiop;

		// records where we will insert extra bytes into the program.
		RangeAddress_t start_of_new_space;

		ZiprMemorySpace_t memory_space;

	        RangeAddress_t bss_needed;
		bool use_stratafier_mode;

		ZiprPluginManager_t plugman;

		std::map<libIRDB::Instruction_t*,DLFunctionHandle_t> plopping_plugins;
};

#endif
