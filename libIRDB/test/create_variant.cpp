

#include <libIRDB-core.hpp>
#include <iostream>
#include <stdlib.h>

using namespace libIRDB;
using namespace std;

main(int argc, char* argv[])
{

	if(argc!=2)
	{
		cerr<<"Usage: create_variant <name>"<<endl;
		exit(-1);
	}


	/* setup the interface to the sql server */
	pqxxDB_t pqxx_interface;
	BaseObj_t::SetInterface(&pqxx_interface);


	VariantID_t *pidp=NULL;
	try 
	{
		pidp=new VariantID_t();

		assert(pidp->IsRegistered()==false);

		pidp->SetName(argv[1]);

		cout<<"New Variant, before registration, is: "<<*pidp << endl;

		pidp->Register();

		assert(pidp->IsRegistered()==true);

		cout<<"New Variant, after registration, is: "<<*pidp << endl;

		pidp->WriteToDB();

		cout<<"New Variant, after writing to DB, is: "<<*pidp << endl;

		// commit the changes to the db if all went well 
		pqxx_interface.Commit();

	}
	catch (DatabaseError_t pnide)
	{
		cout<<"Unexpected database error: "<<pnide<<endl;
		exit(-1);
        }

	delete pidp;
}
