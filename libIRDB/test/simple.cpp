

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

	ProgramID_t pid(atoi(argv[1]));
	if(!pid.IsRegistered())
	{
		cerr << "Program ID "<< argv[1] << " is not registered in the DB" << endl;
		exit(-2);
	}

}
