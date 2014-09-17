/*
 * Zipr is the  ZEST Interface for Program Rewriting 
 * Author: Jason Hiser
 * Copyright: Zephyr Software LLC, 2014
 */

#include <libIRDB-core.hpp>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <assert.h>
#include <sys/mman.h>
#include <ctype.h>


#include <zipr_all.h>


using namespace zipr;
using namespace libIRDB;
using namespace std;

int main(int argc, char* argv[])
{
	Options_t *options=Options_t::parse_args(argc,argv);


        VariantID_t *pidp=NULL;
        FileIR_t * firp=NULL;

        try
        {
                /* setup the interface to the sql server */
                pqxxDB_t pqxx_interface;
                BaseObj_t::SetInterface(&pqxx_interface);

                pidp=new VariantID_t(options->GetVariantID());
        	assert(pidp);

                assert(pidp->IsRegistered()==true);

                cout<<"New Variant, after reading registration, is: "<<*pidp << endl;

                for(set<File_t*>::iterator it=pidp->GetFiles().begin();
                        it!=pidp->GetFiles().end();
                        ++it
                    )
                {
                        File_t* this_file=*it;
                        assert(this_file);

			cout<<"Analyzing file "<<this_file->GetURL()<<endl;
			string this_file_name=options->GetOutputFileName(this_file);

                        // read the db
                        firp=new FileIR_t(*pidp, this_file);
			assert(firp);

                        int elfoid=firp->GetFile()->GetELFOID();
                        pqxx::largeobject lo(elfoid);
                        lo.to_file(pqxx_interface.GetTransaction(),this_file_name.c_str());

			Zipr_t zip(firp,*options);
			zip.CreateBinaryFile(this_file_name);

                        // write the DB back and commit our changes
                        delete firp;
                }

                pqxx_interface.Commit();

        }
        catch (DatabaseError_t pnide)
        {
                cout<<"Unexpected database error: "<<pnide<<endl;
                exit(-1);
        }



        delete pidp;




}
