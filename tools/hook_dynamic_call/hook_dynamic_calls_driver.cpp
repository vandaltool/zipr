#include <stdlib.h>
#include <fstream>
#include <libIRDB-core.hpp>
#include <libgen.h>

#include "hook_dynamic_calls.hpp"

using namespace std;
using namespace libIRDB;

void usage(char* name)
{
	cerr<<"Usage: "<<name<<" <variant_id> <use call> [<call> <id>]\n"; 
}

int main(int argc, char **argv)
{
	pqxxDB_t pqxx_interface;
	VariantID_t *pidp=NULL;
	int variantID;
	string programName;
	bool use_call = false;

	string call_name = "";
	int call_id = 0;
	map<string, int> to_hook;

	if(argc < 3)
	{
		usage(argv[0]);
		exit(1);
	}

	programName = string(argv[0]);
	variantID = atoi(argv[1]);
	if (!strcmp(argv[2], "true"))
		use_call = true;

	argv+=3;

	while (*argv) {
		if (call_name.length() == 0)
		{
			call_name = string(*argv);
			cout << "call_name: " << call_name << endl;
		}
		else
		{
			try
			{
				auto finally = [&call_id, &call_name]
				{
					call_id = 0;
					call_name = "";
				};
				Finally f(finally);

				call_id = std::stoi(string(*argv));
				cout << "call_id: " << call_id << endl;
				to_hook.insert(pair<string,int>(call_name, call_id));
			}
			catch (const std::invalid_argument invalid)
			{
				cout << "Invalid call id: " << *argv << endl;
			}
			catch (const std::out_of_range &out_of_range)
			{
				cout << "Invalid call id: " << *argv << endl;
			}
		}
		argv++;
	}

	map<string,int>::iterator to_hook_iterator = to_hook.begin();
	for (; to_hook_iterator != to_hook.end(); to_hook_iterator++)
	{
		string name = to_hook_iterator->first;
		int id = to_hook_iterator->second;
		cout << name << " -> " << id << endl;
	}

	/* setup the interface to the sql server */
	BaseObj_t::SetInterface(&pqxx_interface);

	pidp=new VariantID_t(variantID);
	assert(pidp->IsRegistered()==true);

	cout<<"hook_dynamic_calls.exe started\n";

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
			HookDynamicCalls hookdynamic(firp, use_call);
			hookdynamic.SetToHook(to_hook);

			int success=hookdynamic.execute();

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
