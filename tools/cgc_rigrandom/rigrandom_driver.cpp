#include <stdlib.h>
#include <fstream>
#include <libIRDB-core.hpp>
#include <libgen.h>

#include "rigrandom_instr.hpp"

using namespace std;
using namespace libIRDB;

void usage(char* name)
{
	cerr<<"Usage: " << name << " <variant_id> <random_byte>\n"; 
}

int main(int argc, char **argv)
{
        if (argc != 3)
        {
                usage(argv[0]);
                exit(1);
        }

        string programName(argv[0]);
        int variantID = atoi(argv[1]);

        VariantID_t *pidp=NULL;

        /* setup the interface to the sql server */
        pqxxDB_t pqxx_interface;
        BaseObj_t::SetInterface(&pqxx_interface);

        pidp=new VariantID_t(variantID);
        assert(pidp->IsRegistered()==true);

	cout << argv[0] << " started with 'random' byte " << argv[2][0] << "\n";

        bool one_success = false;
        for(set<File_t*>::iterator it=pidp->GetFiles().begin();
            it!=pidp->GetFiles().end();
                ++it)
        {
                File_t* this_file = *it;
                FileIR_t *firp = new FileIR_t(*pidp, this_file);

		cout << "Transforming " << this_file->GetURL() << endl;

                assert(firp && pidp);

                try
                {
			RigRandom_Instrument rri(firp, argv[2][0]);
			int success = rri.execute();

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
