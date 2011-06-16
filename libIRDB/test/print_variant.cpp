

#include <libIRDB.hpp>
#include <iostream>
#include <stdlib.h>

using namespace libIRDB;
using namespace std;

main(int argc, char* argv[])
{

	if(argc!=2)
	{
		cerr<<"Usage: simple (<pid>)"<<endl;
		exit(-1);
	}


	/* setup the interface to the sql server */
	pqxxDB_t pqxx_interface;
	BaseObj_t::SetInterface(&pqxx_interface);


	VariantID_t *pidp=NULL;
	try 
	{
		pidp=new VariantID_t(atoi(argv[1]));
	}
	catch (DatabaseError_t pnide)
	{
		if(pnide.GetErrorCode()==DatabaseError_t::VariantNotInDatabase)
		{
			cout<<"Variant "<< argv[1]<< " not found in db"<<endl;
			exit(-2);
		}
       		else
		{
			cout<<"Unexpected database error: "<<pnide<<endl;
			exit(-1);
		}
        }

	cout<<"Variant "<< argv[1]<< " found in db: "<<*pidp << endl;

	delete pidp;
}
