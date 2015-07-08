/***************************************************************************
 * Copyright (c)  2014  Zephyr Software LLC. All rights reserved.
 *
 * This software is furnished under a license and/or other restrictive
 * terms and may be used and copied only in accordance with such terms
 * and the inclusion of the above copyright notice. This software or
 * any other copies thereof may not be provided or otherwise made
 * available to any other person without the express written consent
 * of an authorized representative of Zephyr Software LLC.  Title to,
 * ownership of, and all rights in the software is retained by
 * Zephyr Software LCC.
 *
 * Zephyr Software LLC. Proprietary Information
 *
 * Unless otherwise specified, the information contained in this
 * directory, following this legend, and/or referenced herein is
 * Zephyr Software LLC. (Zephyr) Proprietary Information. 
 *
 * CONTACT
 *
 * For technical assistance, contact Zephyr Software LCC. at:
 *      
 *
 * Zephyr Software, LLC
 * 2040 Tremont Rd
 * Charlottesville, VA 22911
 *
 * E-mail: jwd@zephyr-software.com
 **************************************************************************/

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
	ZiprOptions_t *options=ZiprOptions_t::parse_args(argc,argv);


        VariantID_t *pidp=NULL;
        FileIR_t * firp=NULL;

	if (options->GetVariantID() == -1)
	{
		ZiprOptions_t::print_usage(argc, argv);
		return 1;
	}

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

			// only do a.ncexe for now.
			if(this_file->GetURL().find("a.ncexe")==string::npos)
				continue;


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
