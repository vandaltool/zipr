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
	ZiprOptions_t options = ZiprOptions_t(argc-1, argv+1);
	ZiprIntegerOption_t variant_id("variant");
	ZiprOptionsNamespace_t global_options(string("global"));

	VariantID_t *pidp=NULL;
	FileIR_t * firp=NULL;

	variant_id.SetDescription("Variant ID");
	variant_id.SetRequired(true);
	options.AddNamespace(&global_options);
	global_options.AddOption(&variant_id);

	options.Parse();
	if (!variant_id.RequirementMet()) {
		cout << variant_id.Description() << endl;
		return 1;
	}

	try
	{
		/* setup the interface to the sql server */
		pqxxDB_t pqxx_interface;
		BaseObj_t::SetInterface(&pqxx_interface);

		pidp=new VariantID_t(variant_id);
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
			// only do a.ncexe for now.
			if(this_file->GetURL().find("a.ncexe")==string::npos)
				continue;

			// read the db
			firp=new FileIR_t(*pidp, this_file);
			assert(firp);
			ZiprImpl_t zip(firp, &options);

			if (!options.Parse() || !options.RequirementsMet())
			{
				options.PrintUsage(cout);
				exit(1);
			}
			options.PrintNamespaces();

			string this_file_name=options.Namespace("zipr")->
				OptionByKey("output")->
				StringValue();


			int elfoid=firp->GetFile()->GetELFOID();
			pqxx::largeobject lo(elfoid);
			lo.to_file(pqxx_interface.GetTransaction(),this_file_name.c_str());

			cout << "Calling CreateBinaryFile() with " << this_file_name << endl;
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
