#include "fill_in_cfg.hpp"
#include <assert.h>

using namespace std;
using namespace libIRDB;

int main(int argc, char* argv[])
{
	VariantID_t *pidp=NULL;
	list<FileIR_t *> the_firp_list;

	try 
	{
		/* setup the interface to the sql server */
                pqxxDB_t the_pqxx_interface;
		BaseObj_t::SetInterface(&the_pqxx_interface);

		pidp=new VariantID_t(atoi(argv[1]));

		assert(pidp->IsRegistered()==true);

		cout<<"New Variant, after reading registration, is: "<<*pidp << endl;

                // setup
		for(File_t* it : pidp->GetFiles())
		{
			File_t* this_file=it;
			assert(this_file);
                        
			// read the db  
			FileIR_t* firp=new FileIR_t(*pidp, this_file);
			assert(firp);
                        the_firp_list.push_back(firp);
                }
                
                // fill_in_cfg for all files
                PopulateCFG fill_in_cfg = PopulateCFG::Factory(argc, argv, &the_pqxx_interface, the_firp_list);
                
                bool success = fill_in_cfg.execute();
                if(!success)
                {
                    cout<<"Unexpected error, skipping changes."<<endl;
                    exit(-1);
                }
                
                // cleanup
                for(FileIR_t* the_firp : the_firp_list)
		{
                        assert(the_firp);
                        // write the DB back and commit our changes 
			the_firp->WriteToDB();
			delete the_firp;
		}
		the_pqxx_interface.Commit();
	}
	catch (DatabaseError_t pnide)
	{
		cout<<"Unexpected database error: "<<pnide<<endl;
		exit(-1);
        }

	assert(pidp);

	delete pidp;
	pidp=NULL;
	return 0;
}
