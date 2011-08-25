

#include <libIRDB-core.hpp>
#include <iostream>
#include <stdlib.h>

using namespace libIRDB;
using namespace std;

main(int argc, char* argv[])
{

	if(argc!=1)
	{
		cerr<<"Usage: list_tests)"<<endl;
		exit(-1);
	}


	/* setup the interface to the sql server */
	pqxxDB_t pqxx_interface;
	BaseObj_t::SetInterface(&pqxx_interface);

	for(int i=1; true ; i++)
	{
		try 
		{
			/* try to load the program ID */
			VariantID_t pid(i);
			cout<<pid<<endl;
		}
		catch (DatabaseError_t pnide)
		{
			if(pnide.GetErrorCode()==DatabaseError_t::VariantNotInDatabase)
				break;
			else
			{
				cout<<"Unexpected database error: "<<pnide<<endl;
				exit(-1);
			}
		};
	}

}
