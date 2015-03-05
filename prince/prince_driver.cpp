#include <stdlib.h>
#include <fstream>
#include <libIRDB-core.hpp>
#include <libgen.h>

using namespace std;
using namespace libIRDB;

void usage(char* name)
{
	cerr<<"Usage: "<<name<<" <variant_id> <target_cinderella_executable> <function_name>\n";
}

uintptr_t findAddress(FileIR_t* firp, string functionName)
{
	FunctionSet_t functions = firp->GetFunctions();
	for (FunctionSet_t::iterator it = functions.begin(); it != functions.end(); ++it)
	{
		Function_t *fn = *it;	
		if (fn && fn->GetEntryPoint() && fn->GetEntryPoint()->GetAddress() &&
			fn->GetEntryPoint()->GetAddress()->GetVirtualOffset() > 0)
			return fn->GetEntryPoint()->GetAddress()->GetVirtualOffset();
	}
	return 0;
}

extern int test_prince(string targetName, string functionName, uintptr_t address);

int main(int argc, char **argv)
{
	if(argc != 4)
  	{
		usage(argv[0]);
		exit(1);
	}

	string programName(argv[0]);
	string cinderellaExecutable(argv[1]);
	int variantID = atoi(argv[2]);
	string functionName(argv[3]);

	VariantID_t *pidp=NULL;

	/* setup the interface to the sql server */
	pqxxDB_t pqxx_interface;
	BaseObj_t::SetInterface(&pqxx_interface);

	pidp=new VariantID_t(variantID);
	assert(pidp->IsRegistered()==true);

	for(set<File_t*>::iterator it=pidp->GetFiles().begin(); 
		it!=pidp->GetFiles().end(); 
		++it)
	{
		File_t* this_file = *it;
		try
		{
			FileIR_t *firp = new FileIR_t(*pidp, this_file);
			assert(firp && pidp);

			uintptr_t address = findAddress(firp, functionName);
			if (address > 0)
				return test_prince(cinderellaExecutable, functionName, address);
			else
				return 1;
		}
		catch (DatabaseError_t pnide)
		{
			cerr << programName << ": Unexpected database error: " << pnide << "file url: " << this_file->GetURL() << endl;
		}
		catch (...)
		{
			cerr << programName << ": Unexpected error file url: " << this_file->GetURL() << endl;
		}
	} 

	return 1; // error
}

