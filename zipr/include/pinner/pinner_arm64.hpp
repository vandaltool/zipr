#ifndef ZIPR_PINNER_ARM64
#define ZIPR_PINNER_ARM64

class ZiprPinnerARM64_t  : public ZiprPinnerBase_t
{
	public:
                ZiprPinnerARM64_t(Zipr_SDK::Zipr_t* p_parent);
                virtual void doPinning() override;

        private:
		zipr::ZiprImpl_t* m_parent;
		IRDB_SDK::FileIR_t* m_firp;
		Zipr_SDK::MemorySpace_t &memory_space;
                Zipr_SDK::DollopManager_t &m_dollop_mgr;
		Zipr_SDK::PlacementQueue_t &placement_queue;



};


#endif
