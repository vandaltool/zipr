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

#ifndef zipr_h
#define zipr_h

class Options_t;
class Stats_t;

class Zipr_t
{
	public:
		Zipr_t(libIRDB::FileIR_t* p_firp, Options_t &p_opts)
			: m_firp(p_firp), m_opts(p_opts)
		{ 
                	total_dollops=0;
                	total_dollop_space=0;
                	total_dollop_instructions=0;
                	total_trampolines=0;
                	total_2byte_pins=0;
                	total_5byte_pins=0;
                	total_tramp_space=0;
                	total_other_space=0;
			truncated_dollops=0;
		};

		void CreateBinaryFile(const std::string &name);

	protected:

		// data for the stuff we're rewriting.
		libIRDB::FileIR_t* m_firp;
		Options_t& m_opts;
		Stats_t *m_stats;

		// phases of rewriting.
		void FindFreeRanges(const std::string &name);
		void AddPinnedInstructions();
		void ResolvePinnedInstructions();
		void ReservePinnedInstructions();
		void PreReserve2ByteJumpTargets();
		void ExpandPinnedInstructions();
		void Fix2BytePinnedInstructions();
		void OptimizePinnedInstructions();
		void PlopTheUnpinnedInstructions();
		void PrintStats();


		// range operatations
		void SplitFreeRange(RangeAddress_t addr);
		void MergeFreeRange(RangeAddress_t addr);
		std::list<Range_t>::iterator FindFreeRange(RangeAddress_t addr);
		Range_t GetFreeRange(int size);

		// queries about free areas.
		bool AreBytesFree(RangeAddress_t addr, int num_bytes);
		bool IsByteFree(RangeAddress_t addr);

		//  emitting bytes.
		void PlopByte(RangeAddress_t addr, char the_byte);
		void PlopBytes(RangeAddress_t addr, const char the_byte[], int num);
		void PlopJump(RangeAddress_t addr);

		// emiting instructions
		RangeAddress_t PlopInstruction(libIRDB::Instruction_t* insn,RangeAddress_t addr);
		RangeAddress_t PlopWithTarget(libIRDB::Instruction_t* insn, RangeAddress_t at);


		// patching
		void PatchJump(RangeAddress_t at_addr, RangeAddress_t to_addr);
		void ApplyPatches(libIRDB::Instruction_t* insn);
		void PatchInstruction(RangeAddress_t addr, libIRDB::Instruction_t* insn);
		void RewritePCRelOffset(RangeAddress_t from_addr,RangeAddress_t to_addr, int insn_length, int offset_pos);
		void ApplyPatch(RangeAddress_t from_addr, RangeAddress_t to_addr);


		// outputing new .exe
		void FillSection(ELFIO::section* sec, FILE* fexe);
		void OutputBinaryFile(const std::string &name);


		// helpers.
		void ProcessUnpinnedInstruction(const UnresolvedUnpinned_t &uu, const Patch_t &p);
		void InsertNewSegmentIntoExe(std::string old_file, std::string new_file, RangeAddress_t sec_start);
		void AddCallbacksTONewSegment(const std::string& tmpname, RangeAddress_t end_of_new_space);

		libIRDB::Instruction_t *FindPinnedInsnAtAddr(RangeAddress_t addr);
		bool ShouldPinImmediately(libIRDB::Instruction_t *upinsn);


	private:
		// structures necessary for ZIPR algorithm.
		std::set<UnresolvedUnpinned_t> unresolved_unpinned_addrs;
		std::list<Range_t> free_ranges;   // keep ordered
		std::set<UnresolvedPinned_t> unresolved_pinned_addrs; 
		std::list<Range_t> pinned_ranges; // keep ordered
		std::multimap<UnresolvedUnpinned_t,Patch_t> patch_list;

		// map of where bytes will actually go.
		std::map<RangeAddress_t,char> byte_map;

		// structures to pinned things.
		std::set<UnresolvedPinned_t> two_byte_pins; 
		std::map<UnresolvedPinned_t,RangeAddress_t> five_byte_pins; 

		// final mapping of instruction to address.
		std::map<libIRDB::Instruction_t*,RangeAddress_t> final_insn_locations; 


		// way to read elf headers, etc.
		ELFIO::elfio*    elfiop;

		// stats
		int total_dollops;
		int total_dollop_space;
		int total_dollop_instructions;
		int total_trampolines;
		int total_2byte_pins;
		int total_5byte_pins;
		int total_tramp_space;
		int total_other_space;
		int truncated_dollops;

		// records where we will insert extra bytes into the program.
		RangeAddress_t start_of_new_space;


};

#endif
