#include <stdlib.h>
#include <fstream>
#include <libIRDB-core.hpp>
#include <libgen.h>

#include "hook_start.hpp"

using namespace std;
using namespace libIRDB;

void usage(char* name)
{
	cerr<<"Usage: "<<name<<" <variant_id> [callback name]\n"; 
}

int main(int argc, char **argv)
{
	pqxxDB_t pqxx_interface;
	VariantID_t *pidp=NULL;
	int variantID;
	string programName, callback_name;
		
	if(argc < 2)
	{
		usage(argv[0]);
		exit(1);
	}

	programName = string(argv[0]);
	variantID = atoi(argv[1]);

	if (argv[2] != NULL)
		callback_name = string(argv[2]);	

	/* setup the interface to the sql server */
	BaseObj_t::SetInterface(&pqxx_interface);

	pidp=new VariantID_t(variantID);
	assert(pidp->IsRegistered()==true);

	cout<<"hook_start.exe started\n";

	bool one_success = false;
	for(set<File_t*>::iterator it=pidp->GetFiles().begin();
	    it!=pidp->GetFiles().end();
	    ++it)
	{
		File_t* this_file = *it;
		FileIR_t *firp = new FileIR_t(*pidp, this_file);

		cout<<"Transforming "<<this_file->GetURL()<<endl;

		assert(firp && pidp);

		try
		{
			int success = 0;
			HookStart hookstart(firp);

			if (callback_name != "")
				hookstart.CallbackName(callback_name);

			success=hookstart.execute();

			if (success)
			{
				cout<<"Writing changes for "<<this_file->GetURL()<<endl;
				one_success = true;
				firp->WriteToDB();
				delete firp;
			}
			else
			{
				cout<<"Skipping (no changes) "<<this_file->GetURL()<<endl;
			}
		}
		catch (DatabaseError_t pnide)
		{
			cerr << programName << ": Unexpected database error: " << pnide << "file url: " << this_file->GetURL() << endl;
		}
		catch (...)
		{
			cerr << programName << ": Unexpected error file url: " << this_file->GetURL() << endl;
		}
	} // end file iterator

	// if any integer transforms for any files succeeded, we commit
	if (one_success)
	{
		cout<<"Commiting changes...\n";
		pqxx_interface.Commit();
	}

	return 0;
}
