

#include <libIRDB-core.hpp>
#include <iostream>
#include <fstream>

#include <stdlib.h>

using namespace libIRDB;
using namespace std;

main(int argc, char* argv[])
{

	if(argc!=3)
	{
		cerr<<"Usage: clone <vid> <output.id>"<<endl;
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

		db_id_t newpid_id=newpidp->GetBaseID();
		ofstream f;
		f.open(argv[2]);
		if(!f.is_open())
		{
			cerr<<"Cannot open output file"<<endl;
			exit(-1);
		}
		f<<std::dec<<newpid_id<<endl;
		f.close();

	}
	catch (DatabaseError_t pnide)
	{
		cout<<"Unexpected database error: "<<pnide<<endl;
		exit(-1);
        }


	delete newpidp;
	delete pidp;


	exit(0);
}
