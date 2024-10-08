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

class Stats_t;

class DataScoopByAddressComp_t 
{
        public:
        bool operator()(const IRDB_SDK::DataScoop_t *lhs, const IRDB_SDK::DataScoop_t *rhs) {

                if (!lhs ||
                    !rhs ||
                    !lhs->getStart() ||
                    !rhs->getStart()
                   )
                        throw std::logic_error("Cannot order scoops that have null elements.");

                return std::make_tuple(lhs->getStart()->getVirtualOffset(), lhs) <
                       std::make_tuple(rhs->getStart()->getVirtualOffset(), rhs);
        }
};
using DataScoopByAddressSet_t = std::set<IRDB_SDK::DataScoop_t*, DataScoopByAddressComp_t>;




class pin_sorter_t 
{
	public:
		bool operator() (const UnresolvedPinned_t& p1, const UnresolvedPinned_t& p2)
		{
			assert(p1.getInstruction());
			assert(p2.getInstruction());
			assert(p1.getInstruction()->getIndirectBranchTargetAddress()
			       && p1.getInstruction()->getIndirectBranchTargetAddress()->getVirtualOffset()!=0);
			assert(p2.getInstruction()->getIndirectBranchTargetAddress()
			       && p2.getInstruction()->getIndirectBranchTargetAddress()->getVirtualOffset()!=0);

			return p1.getInstruction()->getIndirectBranchTargetAddress()->getVirtualOffset() < 
				p2.getInstruction()->getIndirectBranchTargetAddress()->getVirtualOffset() ;
		}
};

class ZiprImpl_t : public Zipr_t
{


	public:
		ZiprImpl_t(int argc, char **argv) :
			m_stats(nullptr),
			m_firp(nullptr),
			m_variant_id(nullptr),
			m_pqxx_interface(IRDB_SDK::pqxxDB_t::factory()),
			lo(nullptr),
			m_error(false),
			m_dollop_mgr(this),
			exeiop(new EXEIO::exeio), 
			start_of_new_space(0),
			memory_space(),
			m_zipr_options(argc-1, argv+1)

		{ 
			Init();
 		};
		~ZiprImpl_t();

		void CreateBinaryFile();
		bool Error() {
			return m_error;
		}
		/*
		 * DetermineDollopEntrySize
		 * 
		 * Determine the worst case dollop entry size.
		 * and account for the possibility that a plugin
		 * may be plopping this instruction and want
		 * to do some calculations.
		 */
		size_t determineDollopEntrySize(DollopEntry_t *entry, bool account_for_trampoline) { return DetermineDollopEntrySize(entry,account_for_trampoline); } 
		Zipr_SDK::RangeAddress_t plopDollopEntry(
			DollopEntry_t *de,
			RangeAddress_t override_place = 0,
			RangeAddress_t override_target = 0) { return PlopDollopEntry(de,override_place,override_target); } 

		Zipr_SDK::RangeAddress_t plopDollopEntryWithTarget(
			DollopEntry_t *de,
			RangeAddress_t override_place = 0,
			RangeAddress_t override_target = 0) { return PlopDollopEntryWithTarget(de,override_place,override_target); } 

		Zipr_SDK::RangeAddress_t plopDollopEntryWithCallback(
			DollopEntry_t *de,
			RangeAddress_t override_place = 0) { return PlopDollopEntryWithCallback(de,override_place); } 




		size_t DetermineDollopEntrySize(DollopEntry_t *entry, bool account_for_trampoline);

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

		// ZiprOptionsNamespace_t *registerOptions(ZiprOptionsNamespace_t *ns) { return RegisterOptions(ns); } 
		void registerOptions();

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
		bool AskPluginsAboutPlopping(IRDB_SDK::Instruction_t *);

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
		IRDB_SDK::Instruction_t *FindPatchTargetAtAddr(RangeAddress_t addr);
		void PatchJump(RangeAddress_t at_addr, RangeAddress_t to_addr);
		void AddPatch(const UnresolvedUnpinned_t& uu, const Patch_t& thepatch)
		{
			patch_list.insert(pair<const UnresolvedUnpinned_t,Patch_t>(uu,thepatch));
		}


                virtual Zipr_SDK::MemorySpace_t *getMemorySpace() { return &memory_space; }
                virtual Zipr_SDK::MemorySpace_t *GetMemorySpace() { return &memory_space; }
		virtual Zipr_SDK::DollopManager_t *getDollopManager() { return &m_dollop_mgr; }
                virtual EXEIO::exeio *getEXEIO() { return exeiop; }
                virtual IRDB_SDK::FileIR_t *getFileIR() { return m_firp; }
                virtual Zipr_SDK::InstructionLocationMap_t *getLocationMap() { return &final_insn_locations; }
                virtual Zipr_SDK::InstructionLocationMap_t *GetLocationMap() { return &final_insn_locations; }
		virtual Zipr_SDK::PlacementQueue_t* getPlacementQueue()      { return &placement_queue; }  
		virtual Zipr_SDK::ZiprOptionsManager_t* getOptionsManager()  { return &m_zipr_options; }  
		virtual Zipr_SDK::PlacementQueue_t* GetPlacementQueue() { return &placement_queue; }  
		virtual Zipr_SDK::RangeAddress_t placeUnplacedScoops(Zipr_SDK::RangeAddress_t max) { return PlaceUnplacedScoops(max); } 
		virtual Zipr_SDK::RangeAddress_t PlaceUnplacedScoops(Zipr_SDK::RangeAddress_t max);
		Stats_t* getStats() { return m_stats; }
		ZiprSizerBase_t* getSizer() { return sizer; }
		ZiprPatcherBase_t* GetPatcher() { return patcher; }
		ZiprPinnerBase_t* GetPinner() { return pinner; }


	private:

		void Init();

		Zipr_SDK::RangeAddress_t _PlopDollopEntry(DollopEntry_t*, Zipr_SDK::RangeAddress_t override_address=0);

		/*
		 * FindFreeRanges()
		 *
		 * Input: Nothing
		 * Output: Nothing
		 * Uses: exeio
		 * Effects: memory_space
		 *
		 * Adds available memory ranges to memory space.
		 * It also adds the "large" section.
		 */
		void FindFreeRanges(const std::string &name);

		/* 
		 * Input: A map of section addresses to integers in order of address
		 * Output:  A new scoop with RX perms for each allocatable/executable series-of-segments.
		 * Uses: exeio
		 * Effects: IRDB_SDK IR.
 	         *
		 * Creates a scoop for the executable instructions in the IR.
		 */
		void CreateExecutableScoops(const std::map<RangeAddress_t, int> &ordered_sections);

        
	
		/* 
		 * Input: the IRDB_SDK IR, memory_space
		 * Output:  a revised version of the IR taking memory into account 
		 * Uses: IRDB_SDK IR
		 * Effects: IRDB_SDK IR.
 	         *
		 * Creates a scoop for the executable instructions in the IR.
		 */
		void UpdateScoops();
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
		 * DetermineInsnSize
		 * 
		 * Determine the worst case instruction size
		 * but do not account for the possibility that a plugin
		 * may be plopping this instruction and want
		 * to do some calculations.
		 */
		size_t DetermineInsnSize(IRDB_SDK::Instruction_t*, bool account_for_trampoline);

		// patching
		void ApplyPatches(IRDB_SDK::Instruction_t* insn);
		void PatchInstruction(RangeAddress_t addr, IRDB_SDK::Instruction_t* insn);
		void RewritePCRelOffset(RangeAddress_t from_addr,RangeAddress_t to_addr, int insn_length, int offset_pos);
		void applyPatch(RangeAddress_t from_addr, RangeAddress_t to_addr) { return ApplyPatch(from_addr,to_addr); } 
		void ApplyPatch(RangeAddress_t from_addr, RangeAddress_t to_addr);
		void ApplyNopToPatch(RangeAddress_t addr);
		void PatchCall(RangeAddress_t at_addr, RangeAddress_t to_addr);
		void CallToNop(RangeAddress_t at_addr);

		// outputing new .exe
		void FillSection(EXEIO::section* sec, FILE* fexe);
		void OutputBinaryFile(const std::string &name);
		IRDB_SDK::DataScoop_t* FindScoop(const RangeAddress_t &addr);
		void WriteScoop(EXEIO::section* sec, std::FILE* fexe);


		// helpers.
		void ProcessUnpinnedInstruction(const UnresolvedUnpinned_t &uu, const Patch_t &p);
		void InsertNewSegmentIntoExe(std::string old_file, std::string new_file, RangeAddress_t sec_start);
		std::string AddCallbacksToNewSegment(const std::string& tmpname, RangeAddress_t end_of_new_space);
		RangeAddress_t FindCallbackAddress(RangeAddress_t end_of_new_space,RangeAddress_t start_addr, const std::string &callback);

		// support
		RangeAddress_t extend_section(EXEIO::section *sec,EXEIO::section *next_sec);

		void dump_scoop_map();
		void dump_instruction_map();
 		virtual void RelayoutEhInfo();


		Stats_t *m_stats;

		// data for the stuff we're rewriting.
		IRDB_SDK::FileIR_t* m_firp;
		IRDB_SDK::VariantID_t* m_variant_id;

		std::unique_ptr<IRDB_SDK::FileIR_t> m_firp_p;
		std::unique_ptr<IRDB_SDK::VariantID_t> m_variant_id_p;
		std::unique_ptr<IRDB_SDK::pqxxDB_t> m_pqxx_interface;
		pqxx::largeobject *lo;


		bool m_error;



		// structures necessary for ZIPR algorithm.
		std::multimap<UnresolvedUnpinned_t,Patch_t> patch_list;

		// a manager for all dollops
		ZiprDollopManager_t m_dollop_mgr;

		// final mapping of instruction to address.
		Zipr_SDK::InstructionLocationMap_t final_insn_locations; 
		std::map<RangeAddress_t,UnresolvedUnpinnedPatch_t> m_PatchAtAddrs; 

		// unpatched callbacks
		std::set<std::pair<Zipr_SDK::DollopEntry_t*,RangeAddress_t> > unpatched_callbacks; 

		std::map<std::string,RangeAddress_t> callback_addrs;

		// way to read elf headers, etc.
		EXEIO::exeio*    exeiop;

		// records where we will insert extra bytes into the program.
		RangeAddress_t start_of_new_space;

		ZiprMemorySpace_t memory_space;
		Zipr_SDK::PlacementQueue_t placement_queue;

		RangeAddress_t bss_needed;
		bool use_stratafier_mode;

		/*
		 * Scoops
		 */
		IRDB_SDK::DataScoopSet_t m_zipr_scoops;

		ZiprPluginManager_t plugman;

		std::map<IRDB_SDK::Instruction_t*,
		std::unique_ptr<std::list<DLFunctionHandle_t>>> plopping_plugins;
		
		// Options Manager
		ZiprOptions_t m_zipr_options;

		// Options
		Zipr_SDK::ZiprStringOption_t  *m_output_filename, 
					      *m_callbacks, 
					      *m_objcopy,
					      *m_dollop_map_filename;
		Zipr_SDK::ZiprBooleanOption_t *m_replop, 
					      *m_verbose, 
					      *m_vverbose, 
					      *m_apply_nop, 
					      *m_add_sections, 
					      *m_bss_opts;
		Zipr_SDK::ZiprIntegerOption_t *m_variant, 
					      *m_architecture, 
					      *m_seed,
					      *m_paddable_minimum_distance;



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
		
		// arch specific stuff
		std::unique_ptr<ZiprArchitectureHelperBase_t> archhelper;
		ZiprPinnerBase_t * pinner =nullptr;
		ZiprPatcherBase_t* patcher=nullptr;
		ZiprSizerBase_t  * sizer  =nullptr;


};

#endif
