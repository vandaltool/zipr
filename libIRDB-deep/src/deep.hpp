
#include <irdb-core>
#include <irdb-transform>
#include <irdb-deep>
#include <memory>
#include <vector>

#include <stars.h>


namespace libIRDB
{
	using namespace std;

	class StarsDeepAnalysis_t : public IRDB_SDK::DeepAnalysis_t, protected IRDB_SDK::Transform
	{
		private:
			StarsDeepAnalysis_t()                           = delete;
			StarsDeepAnalysis_t(const StarsDeepAnalysis_t& copy) = delete;

		public:

			StarsDeepAnalysis_t(IRDB_SDK::FileIR_t* firp, const vector<string>& options={});

			unique_ptr<IRDB_SDK::DeadRegisterMap_t      > getDeadRegisters()      const ;
			unique_ptr<IRDB_SDK::StaticGlobalStartMap_t > getStaticGlobalRanges() const ;
			unique_ptr<IRDB_SDK::RangeSentinelSet_t     > getRangeSentinels()     const ;

		private:
			STARS::IRDB_Interface_t stars_analysis_engine;

	};
}
