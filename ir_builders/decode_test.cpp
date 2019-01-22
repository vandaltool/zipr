#include <libIRDB-core.hpp>

using namespace std;
using namespace libIRDB;

int main()
{

	FileIR_t::SetArchitecture(64,admtX86_64);

	for(auto i=0U; i<10*1000*1000; i++)
	{
		const auto d=DecodedInstruction_t(1024, "\x90", 1);
	}
	return 0;
}
