
#include <string>
#include <set>
#include <irdb-core>
#include <transform.hpp>

namespace ElfDep_Tester
{

using namespace libTransform;
using namespace IRDB_SDK;
using namespace std;


typedef set<string> StringSet_t;
class ElfDep_Tester_t : libTransform::Transform
{
	public:
	ElfDep_Tester_t(FileIR_t* firp);
	
	int execute(); 

	private:
	
};

}
