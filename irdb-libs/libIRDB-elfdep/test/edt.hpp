
#include <string>
#include <set>
#include <irdb-core>
#include <irdb-transform>

namespace ElfDep_Tester
{

using namespace IRDB_SDK;
using namespace std;


typedef set<string> StringSet_t;
class ElfDep_Tester_t : Transform_t
{
	public:
	ElfDep_Tester_t(FileIR_t* firp);
	
	int execute(); 

	private:
	
};

}
