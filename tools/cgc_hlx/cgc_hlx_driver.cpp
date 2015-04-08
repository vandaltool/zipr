#include <stdlib.h>
#include <fstream>
#include <libIRDB-core.hpp>
#include <libgen.h>
#include <getopt.h>

#include "cgc_hlx.hpp"

using namespace std;
using namespace libIRDB;

#define BINARY_NAME "a.ncexe"
#define SHARED_OBJECTS_DIR "shared_objects"

void usage(char* name)
{
	cerr<<"Usage: "<<name<<" varid=<variant_id> [--do_malloc_padding=<padding_size>] [--do_allocate_padding=<padding_size>]\n"; 
}

int varid=0;
bool enable_malloc_padding = false;
bool enable_allocate_padding = false;
int malloc_padding = 64;
int allocate_padding = 4096;

int parse_args(int p_argc, char* p_argv[])
{
	int option = 0;
	char options[] = "v:m:a:";
	struct option long_options[] = {
		{"varid", required_argument, NULL, 'v'},
		{"do_malloc_padding", required_argument, NULL, 'm'},
		{"do_allocate_padding", required_argument, NULL, 'a'},
		{NULL, no_argument, NULL, '\0'},         // end-of-array marker
	};

	while ((option = getopt_long(
		p_argc,
		p_argv,
		options,
		long_options,
		NULL)) != -1)
	{
		printf("Found option %c\n", option);
		switch (option)
		{
			case 'v':
			{
				varid=atoi(::optarg);	
				cout<<"Transforming variant "<<dec<<varid<<endl;
				break;
			}
			case 'm':
			{
				enable_malloc_padding = true;
				malloc_padding=atoi(::optarg);	
				break;
			}
			case 'a':
			{
				enable_allocate_padding = true;
				allocate_padding=atoi(::optarg);	
				break;
			}
			default:
				return 1;
		}
	}
	return 0;
}


int main(int argc, char **argv)
{
        string programName(argv[0]);
	if(0 != parse_args(argc,argv))
	{
		usage(argv[0]);
		exit(1);
	}

        VariantID_t *pidp=NULL;

        /* setup the interface to the sql server */
        pqxxDB_t pqxx_interface;
        BaseObj_t::SetInterface(&pqxx_interface);

        pidp=new VariantID_t(varid);
        assert(pidp->IsRegistered()==true);

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
			HLX_Instrument hlx(firp);

			if (enable_malloc_padding)
				hlx.enableMallocPadding(malloc_padding);
			if (enable_allocate_padding)
				hlx.enableAllocatePadding(allocate_padding);

			bool success = hlx.execute();

                        if (success)
                        {
				cout << "Padding successful" << endl;
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

        // if any transforms for any files succeeded, we commit
        if (one_success)
	{
		cout<<"Commiting changes...\n";
                pqxx_interface.Commit();
	}

        return 0;
}

