#ifndef zipr_h
#define zipr_h


class Zipr_t
{
	public:
		Zipr_t(libIRDB::FileIR_t* p_firp, const Options_t &p_opts)
			: m_firp(p_firp), m_opts(p_opts) { };

		void CreateBinaryFile(const std::string &name);

	protected:

		// data for the stuff we're rewriting.
		libIRDB::FileIR_t* m_firp;
		const Options_t& m_opts;

		// phases of rewriting.
		void FindFreeRanges(const std::string &name);
		void AddPinnedInstructions();
		void ResolvePinnedInstructions();
		void ReservePinnedInstructions();
		void ExpandPinnedInstructions();
		void Fix2BytePinnedInstructions();
		void OptimizePinnedInstructions();
		void PlopTheUnpinnedInstructions();

		// range operatations
		void SplitFreeRange(RangeAddress_t addr);
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





		// helpers.
		void ProcessUnpinnedInstruction(const UnresolvedUnpinned_t &uu, const Patch_t &p);


	private:
		// structures necessary for ZIPR algorithm.
		std::set<UnresolvedUnpinned_t> unresolved_unpinned_addrs;
		std::list<Range_t> free_ranges;   // keep ordered
		std::set<UnresolvedPinned_t> unresolved_pinned_addrs; 
		std::list<Range_t> pinned_ranges; // keep ordered
		std::multimap<UnresolvedUnpinned_t,Patch_t> patch_list;

		// map of where bytes will actually go.
		std::map<RangeAddress_t,char> byte_map;

		std::set<UnresolvedPinned_t> two_byte_pins; 
		std::map<UnresolvedPinned_t,RangeAddress_t> five_byte_pins; 

		std::map<libIRDB::Instruction_t*,RangeAddress_t> final_insn_locations; 


};

#endif
