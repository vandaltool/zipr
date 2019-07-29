
#include <loops.hpp>

using namespace libIRDB;
using namespace std;

IRDB_SDK::LoopSet_t LoopNest_t::getAllLoops()   const 
{
	auto ret=IRDB_SDK::LoopSet_t();
	for(const auto &p : loops)
	{
		ret.insert(p.second.get());
	}
	return ret;
}

