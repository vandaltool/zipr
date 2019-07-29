
#include <irdb-core>
#include <irdb-transform>
#include <irdb-deep>
#include <memory>
#include <vector>

#include <stars.h>


namespace libIRDB
{
	using namespace std;

	class StarsDeepAnalysis_t : public IRDB_SDK::DeepAnalysis_t, protected IRDB_SDK::Transform_t
	{
		private:
			StarsDeepAnalysis_t()                           = delete;
			StarsDeepAnalysis_t(const StarsDeepAnalysis_t& copy) = delete;

		public:

			StarsDeepAnalysis_t(IRDB_SDK::FileIR_t* firp, const vector<string>& options={});

			virtual unique_ptr<IRDB_SDK::FunctionSet_t         > getLeafFunctions()      const override;
			virtual unique_ptr<IRDB_SDK::DeadRegisterMap_t     > getDeadRegisters()      const override;
			virtual unique_ptr<IRDB_SDK::StaticGlobalStartMap_t> getStaticGlobalRanges() const override;
			virtual unique_ptr<IRDB_SDK::RangeSentinelSet_t    > getRangeSentinels()     const override;

			virtual unique_ptr<IRDB_SDK::LoopNest_t> getLoops(IRDB_SDK::Function_t* f)           const override;
                        virtual unique_ptr<IRDB_SDK::LoopNest_t> getLoops(IRDB_SDK::ControlFlowGraph_t* cfg) const override;


		private:
			STARS::IRDB_Interface_t stars_analysis_engine;

	};
}
