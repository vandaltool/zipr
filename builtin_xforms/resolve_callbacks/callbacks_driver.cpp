#include <stdlib.h>
#include <fstream>
#include <irdb-core>
#include <libgen.h>

#include "callbacks.hpp"

using namespace std;
using namespace IRDB_SDK;

void usage(char* name)
{
	cerr<<"Usage: "<<name<<" <variant_id> <callbacks file name>\n"; 
}

int main(int argc, char **argv)
{
	auto pqxx_interface=pqxxDB_t::factory();
	int variantID;
	string programName, callbackFileName;


	if(argc != 3)
	{
		usage(argv[0]);
		exit(1);
	}

	programName = string(argv[0]);
	variantID = atoi(argv[1]);
	callbackFileName = string(argv[2]);

	/* setup the interface to the sql server */
	BaseObj_t::setInterface(pqxx_interface.get());

	auto pidp=VariantID_t::factory(variantID);
	assert(pidp->isRegistered()==true);

	cout<<"callbacks.exe started\n";

	bool one_success = false;
	for(set<File_t*>::iterator it=pidp->getFiles().begin();
	    it!=pidp->getFiles().end();
	    ++it)
	{
		File_t* this_file = *it;
		auto firp = FileIR_t::factory(pidp.get(), this_file);

		cout<<"Transforming "<<this_file->getURL()<<endl;

		assert(firp && pidp);

		try
		{
			Callbacks callbacks(firp.get());
			callbacks.SetCallbackFile(callbackFileName);

			int success=callbacks.execute();

			if (success)
			{
				cout<<"Writing changes for "<<this_file->getURL()<<endl;
				one_success = true;
				firp->writeToDB();
			}
			else
			{
				cout<<"Skipping (no changes) "<<this_file->getURL()<<endl;
			}
		}
		catch (DatabaseError_t pnide)
		{
			cerr << programName << ": Unexpected database error: " << pnide << "file url: " << this_file->getURL() << endl;
		}
		catch (...)
		{
			cerr << programName << ": Unexpected error file url: " << this_file->getURL() << endl;
		}
	} // end file iterator

	// if any integer transforms for any files succeeded, we commit
	if (one_success)
	{
		cout<<"Commiting changes...\n";
		pqxx_interface->commit();
	}

	return 0;
}
