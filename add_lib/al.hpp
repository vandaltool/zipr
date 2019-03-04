
#include <string>
#include <set>
#include <irdb-core>
#include <irdb-transform>

namespace AddLib
{
using namespace IRDB_SDK;
using namespace std;


typedef set<string> StringSet_t;
class AddLib_t : Transform
{
	public:
	AddLib_t(FileIR_t* firp, const StringSet_t &prepended, const StringSet_t &appended);
	
	int execute(); 

	private:
		StringSet_t prepended;
		StringSet_t appended;
	
};

}
