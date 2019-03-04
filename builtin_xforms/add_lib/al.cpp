
#include "al.hpp"
#include <irdb-elfdep>

using namespace IRDB_SDK;
using namespace std;
using namespace AddLib;


AddLib_t::AddLib_t(FileIR_t* firp, const StringSet_t &p_prepended, const StringSet_t &p_appended)
	: Transform (firp), prepended(p_prepended), appended(p_appended)
{
}

	
int AddLib_t::execute()
{

	auto ed=ElfDependencies_t::factory(getFileIR());

	for(const auto &p : prepended)
		ed->prependLibraryDepedencies(p);

	for(const auto &a : appended)
		ed->appendLibraryDepedencies(a);

	return 0;
}
	
