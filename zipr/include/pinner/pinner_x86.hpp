#ifndef ZIPR_PINNER_X86
#define ZIPR_PINNER_X86

class ZiprPinnerX86_t : public ZiprPinnerBase_t
{
	public:
		ZiprPinnerX86_t(Zipr_SDK::Zipr_t* p_parent);
		virtual void doPinning() override;

	private:

	
		// methods , remove from zipr_impl
		void AddPinnedInstructions();
		void RecordPinnedInsnAddrs();
		bool ShouldPinImmediately(IRDB_SDK::Instruction_t *upinsn);
		void PreReserve2ByteJumpTargets();
		void InsertJumpPoints68SledArea(Sled_t &sled);
		IRDB_SDK::Instruction_t* Emit68Sled(RangeAddress_t addr, Sled_t sled, IRDB_SDK::Instruction_t* next_sled);
		IRDB_SDK::Instruction_t* Emit68Sled(Sled_t sled);
		void Update68Sled(Sled_t new_sled, Sled_t &existing_sled);
		RangeAddress_t Do68Sled(RangeAddress_t addr);
		void Clear68SledArea(Sled_t sled);
		int Calc68SledSize(RangeAddress_t addr, size_t sled_overhead);
		bool IsPinFreeZone(RangeAddress_t addr, int size);
		void ReservePinnedInstructions();
		void ExpandPinnedInstructions();
		void Fix2BytePinnedInstructions();
		void OptimizePinnedInstructions();
		IRDB_SDK::Instruction_t* FindPinnedInsnAtAddr(RangeAddress_t addr);


		// shortcut 
		IRDB_SDK::Instruction_t* FindPatchTargetAtAddr(RangeAddress_t addr) { return m_parent->FindPatchTargetAtAddr(addr); }
		void PatchJump(RangeAddress_t at_addr, RangeAddress_t to_addr) { return m_parent->PatchJump(at_addr,to_addr); }




		// data
		zipr::ZiprImpl_t* m_parent;
		Zipr_SDK::MemorySpace_t &memory_space;
		Zipr_SDK::DollopManager_t &m_dollop_mgr;
		IRDB_SDK::FileIR_t* m_firp;
		Zipr_SDK::PlacementQueue_t &placement_queue;
		bool m_verbose;
		Stats_t* m_stats;
		Zipr_SDK::InstructionLocationMap_t &final_insn_locations;


		// notes:  move these out of zipr_impl.h
                std::set<UnresolvedPinned_t> two_byte_pins;
                std::map<UnresolvedPinned_t,RangeAddress_t> five_byte_pins;
		std::set<UnresolvedPinned_t,pin_sorter_t> unresolved_pinned_addrs;
                std::map<RangeAddress_t,std::pair<IRDB_SDK::Instruction_t*, size_t> > m_InsnSizeAtAddrs;
                std::map<RangeAddress_t, bool> m_AddrInSled;
		std::set<Sled_t> m_sleds;


};


#endif
