#include <stdlib.h>
#include <fstream>
#include <libIRDB-core.hpp>
#include <libgen.h>

#include "fix_canaries.hpp"

using namespace std;
using namespace libIRDB;

void usage(const char* name)
{
	cerr<<"Usage: "<<name<<" <variant_id> [-v] [-c <callback>]\n"; 
}

int main(int argc, char **argv)
{
	pqxxDB_t pqxx_interface;
	VariantID_t *pidp=NULL;
	int variantID;
	string programName;
	bool verbose = false;
	string callbackName;
	bool parsed_parameters = true;

	programName = string(argv[0]);
	if(argc < 2)
	{
		parsed_parameters = false;
	} else {
		variantID = atoi(argv[1]);
		argv+=2;
	}

	while (parsed_parameters && *argv != NULL) {
		if (!strcmp("-v", *argv)) {
			verbose = true;
		} else if (!strcmp("-c", *argv)) {
			argv++;
			if (*argv == NULL) {
				cerr << "-c requires a parameter." << endl;
			} else {
				callbackName = string(*argv);
			}
		}
		argv++;
	}

	if (!parsed_parameters) {
		usage(programName.c_str());
		exit(1);
	}

	if (verbose) {
		cout << "verbose:  " << verbose << endl;
		cout << "callback: " << callbackName << endl;
		cout << "variant:  " << variantID << endl;
	}

	/* setup the interface to the sql server */
	BaseObj_t::SetInterface(&pqxx_interface);

	pidp=new VariantID_t(variantID);
	assert(pidp->IsRegistered()==true);

	cout<<"fix_canaries_driver.exe started\n";

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
			FixCanaries fixcanaries(firp);

			if (verbose) {
				fixcanaries.set_verbose(verbose);
			}
			if (callbackName.length() != 0) {
				fixcanaries.set_callback(callbackName);
			}

			int success=fixcanaries.execute();

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
