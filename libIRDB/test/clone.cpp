

#include <libIRDB.hpp>
#include <iostream>
#include <stdlib.h>

using namespace libIRDB;
using namespace std;

main(int argc, char* argv[])
{

	if(argc!=2)
	{
		cerr<<"Usage: clone <vid>"<<endl;
		exit(-1);
	}


	/* setup the interface to the sql server */
	pqxxDB_t pqxx_interface;
	BaseObj_t::SetInterface(&pqxx_interface);


	VariantID_t *pidp=NULL;
	VariantID_t *newpidp=NULL;
	try 
	{
		pidp=new VariantID_t(atoi(argv[1]));

		assert(pidp->IsRegistered()==true);

		newpidp=pidp->Clone();

		assert(newpidp->IsRegistered()==true);

		cout<<"Cloned Variant is: "<<*newpidp << endl;

		// commit the changes to the db if all went well 
		pqxx_interface.Commit();

	}
	catch (DatabaseError_t pnide)
	{
		cout<<"Unexpected database error: "<<pnide<<endl;
		exit(-1);
        }

	db_id_t newpid_id=newpidp->GetBaseID();

	delete pidp;
	delete newpidp;

	exit(newpid_id);
}
