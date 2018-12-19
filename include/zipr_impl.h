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

#include <climits>
#include <arch/archbase.hpp>
#include <memory>
class Stats_t;

class pin_sorter_t 
{
	public:
		bool operator() (const UnresolvedPinned_t& p1, const UnresolvedPinned_t& p2)
		{
			assert(p1.GetInstruction());
			assert(p2.GetInstruction());
			assert(p1.GetInstruction()->GetIndirectBranchTargetAddress()
			       && p1.GetInstruction()->GetIndirectBranchTargetAddress()->GetVirtualOffset()!=0);
			assert(p2.GetInstruction()->GetIndirectBranchTargetAddress()
			       && p2.GetInstruction()->GetIndirectBranchTargetAddress()->GetVirtualOffset()!=0);

			return p1.GetInstruction()->GetIndirectBranchTargetAddress()->GetVirtualOffset() < 
				p2.GetInstruction()->GetIndirectBranchTargetAddress()->GetVirtualOffset() ;
		}
};

class ZiprImpl_t : public Zipr_t
{


	public:
		ZiprImpl_t(int argc, char **argv) :
			m_stats(NULL),
			m_firp(NULL),
			m_error(false),
			m_dollop_mgr(this),
			elfiop(new ELFIO::elfio), 
			start_of_new_space(0),
			memory_space(),
			m_zipr_options(argc-1, argv+1),
			m_output_filename("output", "b.out"),
			m_callbacks("callbacks"),
			m_objcopy("objcopy", "/usr/bin/objcopy"),
			m_replop("replop", false),
			m_verbose("verbose", false),
			m_vverbose("very_verbose", false),
			m_apply_nop("apply_nop", false),
			m_add_sections("add-sections", true),
			m_bss_opts("bss-opts", true),
			m_variant("variant"),
			m_architecture("architecture"),
			m_seed("seed", 0),
			m_dollop_map_filename("dollop_map_filename", "dollop.map"),
			m_paddable_minimum_distance("paddable_minimum_distance", 5*1024)
		{ 
			Init();
 		};
		~ZiprImpl_t();

		void CreateBinaryFile();
		bool Error() {
			return m_error;
		}
		/*
		 * DetermineWorstCaseDollopEntrySize
		 * 
		 * Determine the worst case dollop entry size.
		 * and account for the possibility that a plugin
		 * may be plopping this instruction and want
		 * to do some calculations.
		 */
		size_t DetermineWorstCaseDollopEntrySize(DollopEntry_t *entry, bool account_for_trampoline);

		Zipr_SDK::RangeAddress_t PlopDollopEntry(
			DollopEntry_t *,
			RangeAddress_t override_place = 0,
			RangeAddress_t override_target = 0);

		Zipr_SDK::RangeAddress_t PlopDollopEntryWithTarget(
			DollopEntry_t *,
			RangeAddress_t override_place = 0,
			RangeAddress_t override_target = 0);

		Zipr_SDK::RangeAddress_t PlopDollopEntryWithCallback(
			DollopEntry_t *,
			RangeAddress_t override_place = 0);

		ZiprOptionsNamespace_t *RegisterOptions(ZiprOptionsNamespace_t *);

		/*
		 * ()
		 *
		 * Input:
		 * Output:
		 * Effects:
		 *
		 *
		 *
		 */
		void AskPluginsAboutPlopping();
		bool AskPluginsAboutPlopping(libIRDB::Instruction_t *);

		void RecordNewPatch(const std::pair<RangeAddress_t, UnresolvedUnpinnedPatch_t>& p)
		{
				m_PatchAtAddrs.insert(p);
		}
		UnresolvedUnpinnedPatch_t FindPatch(const RangeAddress_t r) const
		{
			return m_PatchAtAddrs.at(r);
		}
		void RemovePatch(const RangeAddress_t r)
		{
			auto patch_it = m_PatchAtAddrs.find(r);
                        assert(patch_it != m_PatchAtAddrs.end());
                        m_PatchAtAddrs.erase(patch_it);
		}
		libIRDB::Instruction_t *FindPatchTargetAtAddr(RangeAddress_t addr);
		void PatchJump(RangeAddress_t at_addr, RangeAddress_t to_addr);
		void AddPatch(const UnresolvedUnpinned_t& uu, const Patch_t& thepatch)
		{
			patch_list.insert(pair<const UnresolvedUnpinned_t,Patch_t>(uu,thepatch));
		}


	private:

		void Init();

		Zipr_SDK::RangeAddress_t _PlopDollopEntry(DollopEntry_t*, Zipr_SDK::RangeAddress_t override_address=0);

		/*
		 * FindFreeRanges()
		 *
		 * Input: Nothing
		 * Output: Nothing
		 * Uses: elfio
		 * Effects: memory_space
		 *
		 * Adds available memory ranges to memory space.
		 * It also adds the "large" section.
		 */
		void FindFreeRanges(const std::string &name);

		/* 
		 * Input: A map of section addresses to integers in order of address
		 * Output:  A new scoop with RX perms for each allocatable/executable series-of-segments.
		 * Uses: elfio
		 * Effects: libIRDB IR.
 	         *
		 * Creates a scoop for the executable instructions in the IR.
		 */
		void CreateExecutableScoops(const std::map<RangeAddress_t, int> &ordered_sections);

        
	
		/* 
		 * Input: the libIRDB IR, memory_space
		 * Output:  a revised version of the IR taking memory into account 
		 * Uses: libIRDB IR
		 * Effects: libIRDB IR.
 	         *
		 * Creates a scoop for the executable instructions in the IR.
		 */
		void UpdateScoops();



		/*
		 * AddPinnedInstructions()
		 *
		 * Input: None
		 * Output: Output
		 * Uses: IRDB
		 * Effects: unresolved_pinned_addrs
		 *
		 * Creates a set of addresses of unresolved pins.
		 */
		void AddPinnedInstructions();
		/*
		 * ReservePinnedInstructions()
		 *
		 * Input: None
		 * Output: None
		 * Uses: unresolved_pinned_addrs, memory_space
		 * Effects: two_byte_pins
		 *
		 * Builds up two_byte_pins, a set of addresses of
		 * two byte memory ranges that correspond to pinned
		 * addresses.
		 *
		 * This function also handles cases where space for
		 * two byte pins do not exist at pinned addresses and
		 * puts in the appropriate workaround code.
		 *
		 */
		void ReservePinnedInstructions();
		/*
		 * PreReserve2ByteJumpTargets()
		 *
		 * Input: None
		 * Output: None
		 * Uses: two_byte_pins, memory_space
		 * Effects: two_byte_pins, memory_space
		 *
		 * Reserves nearby space (2/5 bytes) within
		 * range of two byte pins. Each two byte pin
		 * will have either two or five nearby bytes
		 * reserved by the end of this loop.
		 *
		 */
		void PreReserve2ByteJumpTargets();

		/*
		 * ExpandPinnedInstructions()
		 *
		 * Input: None
		 * Output: Output
		 * Uses: two_byte_pins, memory_space
		 * Effects: memory_space, five_byte_pins
		 *
		 * Turn reserved two byte pins into five
		 * byte pins where space allows. All new
		 * five byte pin locations are added to
		 * five_byte_pins. Those two byte pins
		 * that cannot be expanded are readded
		 * to two_byte_pins.
		 *
		 */
		void ExpandPinnedInstructions();
		/*
		 * Fix2BytePinnedInstructions()
		 *
		 * Input: None
		 * Output: None
		 * Uses: two_byte_pins, memory_space
		 * Effects: five_byte_pins, memory_space,
		 *          two_byte_pins
		 *
		 * Look at the pre-reserved space for each
		 * remaining two byte pin. If the prereserved
		 * space is five bytes, then we make the two
		 * byte pin point to the five byte space and
		 * make the five byte pin point to the insn
		 * (and add it to the list of five byte pins).
		 * However, if the prereserved space is only
		 * two bytes, then we make the existing two
		 * byte pin point to the preserved two byte
		 * pin and add it back to the list of two
		 * byte pins.
		 *
		 */
		void Fix2BytePinnedInstructions();

		/*
		 * OptimizedPinnedInstructions()
		 *
		 * Input:
		 * Output:
		 * Effects:
		 *
		 *
		 *
		 */
		void OptimizePinnedInstructions();
		/*
		 * CreateDollops()
		 *
		 * Input: None
		 * Output: None
		 * Uses: five_byte_pins, memory_space
		 * Effects: m_dollop_mgr, five_byte_pins
		 *
		 * Creates all the dollops starting with
		 * those pointed to by the five byte pins.
		 */
		void CreateDollops();
		void PlaceDollops();
		void WriteDollops();
		void UpdatePins();

		void RecalculateDollopSizes();

		void ReplopDollopEntriesWithTargets();
		/*
		 * OptimizePinnedFallthrough()
		 *
		 * Input: None
		 * Output: None
		 * Uses: five_byte_pins
		 * Effects: patch_list, five_byte_pins
		 *
		 * Iterates through all five byte pin spaces
		 * and converts them to patches to their
		 * target instructions. Those patches are
		 * added to patch_list and removed from
		 * five_byte_pins. At the end of the function,
		 * five_byte_pins *should* be empty.
		 *
		 */
		void OptimizePinnedFallthroughs();
		/*
		 * PlopTheUnpinnedInstructions()
		 *
		 * Input: None
		 * Output: None
		 * Uses patch_list
		 * Effects: patch_list
		 *
		 * Iterates through all the patches and
		 * plops down the target instructions.
		 * Dynamically adds patches to patch_list
		 * as necessary (when it plops an instruction
		 * that jumps to another place that is not
		 * already plopped or that needs to be
		 * replopped).
		 *
		 */
		void PlopTheUnpinnedInstructions();

		/*
		 * ()
		 *
		 * Input:
		 * Output:
		 * Effects:
		 *
		 *
		 *
		 */
		void UpdateCallbacks();

		/*
		 * ()
		 *
		 * Input:
		 * Output:
		 * Effects:
		 *
		 *
		 *
		 */
		void PrintStats();
		/*
		 * RecordPinnedInsnAddrs()
		 *
		 * Input: None
		 * Output: None
		 * Effects: m_InsnAtAddrs
		 *
		 * Builds a map of pinned addresses -> insns
		 * used by FindPinnedInsnsAtAddr.
		 */
		void RecordPinnedInsnAddrs();


		void PerformPinning();

		// zipr has some internal assumptions that multiple fallthroughs to the same instruction
		// are problematic.  this function adjusts the IR such that no multiple fallthroughs
		// exist by adding direct jump instructions where necessary to eliminate multiple fallthroughs.
		void  FixMultipleFallthroughs();

		// zipr assumes that jumps (jcc and jmp) are either 2 or 5 byte.
		// sometimes they include a prefix to help the branch predictor
		// remove unneeded prefixes to meet zipr assumptions.
		void  FixTwoByteWithPrefix();

		// zipr also has internal assumptions about conditional branches having targets/fallthrougsh.
		// patch any instructions with null fallthrougsh and targtes a halt instruction
		void  FixNoFallthroughs();



		/*
		 * DetermineWorstCaseInsnSize
		 * 
		 * Determine the worst case instruction size
		 * but do not account for the possibility that a plugin
		 * may be plopping this instruction and want
		 * to do some calculations.
		 */
		size_t DetermineWorstCaseInsnSize(libIRDB::Instruction_t*, bool account_for_trampoline);

		// patching
		void ApplyPatches(libIRDB::Instruction_t* insn);
		void PatchInstruction(RangeAddress_t addr, libIRDB::Instruction_t* insn);
		void RewritePCRelOffset(RangeAddress_t from_addr,RangeAddress_t to_addr, int insn_length, int offset_pos);
		void ApplyPatch(RangeAddress_t from_addr, RangeAddress_t to_addr);
		void ApplyNopToPatch(RangeAddress_t addr);
		void PatchCall(RangeAddress_t at_addr, RangeAddress_t to_addr);
		void CallToNop(RangeAddress_t at_addr);

		// outputing new .exe
		void FillSection(ELFIO::section* sec, FILE* fexe);
		void OutputBinaryFile(const std::string &name);
		libIRDB::DataScoop_t* FindScoop(const RangeAddress_t &addr);
		void WriteScoop(ELFIO::section* sec, std::FILE* fexe);


		// helpers.
		void ProcessUnpinnedInstruction(const UnresolvedUnpinned_t &uu, const Patch_t &p);
		void InsertNewSegmentIntoExe(std::string old_file, std::string new_file, RangeAddress_t sec_start);
		std::string AddCallbacksToNewSegment(const std::string& tmpname, RangeAddress_t end_of_new_space);
		RangeAddress_t FindCallbackAddress(RangeAddress_t end_of_new_space,RangeAddress_t start_addr, const std::string &callback);
		libIRDB::Instruction_t *FindPinnedInsnAtAddr(RangeAddress_t addr);
		bool ShouldPinImmediately(libIRDB::Instruction_t *upinsn);
		bool IsPinFreeZone(RangeAddress_t addr, int size);

		// routines to deal with a "68 sled"
		int Calc68SledSize(RangeAddress_t addr, size_t sled_overhead=6);
		RangeAddress_t Do68Sled(RangeAddress_t addr);
		void Update68Sled(Sled_t, Sled_t &);
		libIRDB::Instruction_t* Emit68Sled(Sled_t sled);
		libIRDB::Instruction_t* Emit68Sled(RangeAddress_t addr, Sled_t sled, libIRDB::Instruction_t* next_sled);
		/*
		 * The goal here is to simply clear out chain entries
		 * that may be in the way. This will not clear out 
		 * previously added PUSHs.
		 */
		void Clear68SledArea(Sled_t sled);
		void InsertJumpPoints68SledArea(Sled_t &sled);
		// support
		RangeAddress_t extend_section(ELFIO::section *sec,ELFIO::section *next_sec);

		void dump_scoop_map();
		void dump_instruction_map();
 		virtual void RelayoutEhInfo();

	public: 

                virtual Zipr_SDK::MemorySpace_t *GetMemorySpace() { return &memory_space; }
		virtual Zipr_SDK::DollopManager_t *GetDollopManager() { return &m_dollop_mgr; }
                virtual ELFIO::elfio *GetELFIO() { return elfiop; }
                virtual libIRDB::FileIR_t *GetFileIR() { return m_firp; }
                virtual Zipr_SDK::InstructionLocationMap_t *GetLocationMap() { return &final_insn_locations; }
		virtual Zipr_SDK::PlacementQueue_t* GetPlacementQueue() { return &placement_queue; }  
		virtual Zipr_SDK::RangeAddress_t PlaceUnplacedScoops(Zipr_SDK::RangeAddress_t max);
		Stats_t* GetStats() { return m_stats; }


	private:
		Stats_t *m_stats;

		// data for the stuff we're rewriting.
		libIRDB::FileIR_t* m_firp;
		libIRDB::VariantID_t *m_variant_id;
		libIRDB::pqxxDB_t m_pqxx_interface;
		pqxx::largeobject *lo;


		bool m_error;



		// structures necessary for ZIPR algorithm.
		std::set<UnresolvedUnpinned_t> unresolved_unpinned_addrs;
		std::set<UnresolvedPinned_t,pin_sorter_t> unresolved_pinned_addrs; 
		std::multimap<UnresolvedUnpinned_t,Patch_t> patch_list;

		// structures to pinned things.
		std::set<UnresolvedPinned_t> two_byte_pins; 
		std::map<UnresolvedPinned_t,RangeAddress_t> five_byte_pins; 
		std::set<Sled_t> m_sleds;
		std::map<RangeAddress_t,std::pair<libIRDB::Instruction_t*, size_t> > m_InsnSizeAtAddrs; 
		std::map<RangeAddress_t, bool> m_AddrInSled;

		// a manager for all dollops
		ZiprDollopManager_t m_dollop_mgr;

		// final mapping of instruction to address.
		// std::map<libIRDB::Instruction_t*,RangeAddress_t> 
		Zipr_SDK::InstructionLocationMap_t final_insn_locations; 
		std::map<RangeAddress_t,UnresolvedUnpinnedPatch_t> m_PatchAtAddrs; 

		// unpatched callbacks
		std::set<std::pair<Zipr_SDK::DollopEntry_t*,RangeAddress_t> > unpatched_callbacks; 

		std::map<std::string,RangeAddress_t> callback_addrs;

		// way to read elf headers, etc.
		ELFIO::elfio*    elfiop;

		// records where we will insert extra bytes into the program.
		RangeAddress_t start_of_new_space;

		ZiprMemorySpace_t memory_space;
		Zipr_SDK::PlacementQueue_t placement_queue;

		RangeAddress_t bss_needed;
		bool use_stratafier_mode;

		/*
		 * Scoops
		 */
		libIRDB::DataScoopSet_t m_zipr_scoops;

		ZiprPluginManager_t plugman;

		std::map<libIRDB::Instruction_t*,
		std::unique_ptr<std::list<DLFunctionHandle_t>>> plopping_plugins;
		
		// Options
		ZiprOptions_t m_zipr_options;
		ZiprStringOption_t m_output_filename, m_callbacks, m_objcopy;
		ZiprBooleanOption_t m_replop, m_verbose, m_vverbose, m_apply_nop, m_add_sections, m_bss_opts;
		ZiprIntegerOption_t m_variant, m_architecture, m_seed;
		ZiprStringOption_t m_dollop_map_filename;
		ZiprIntegerOption_t m_paddable_minimum_distance;
		std::unique_ptr<ZiprArchitectureHelperBase_t> archhelper;

		std::list<DollopEntry_t *> m_des_to_replop;


                template <class T> static T page_align(const T& in)
		{
			return align_by(in,PAGE_SIZE);
		}
                template <class T> static T align_up_to(const T& in, const T &by)
		{
			return align_by(in+by-1,by);
		}
                template <class T> static T align_by(const T& in, const T &by)
                {
			// assert power of 2.
			assert( (by & (by - 1)) == 0 ); 
                        return in&~(by-1);
                }


};

#endif
