

#include <libIRDB.hpp>
#include <utils.hpp> // to_string function from libIRDB
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

using namespace libIRDB;
using namespace std;

//
// main routine to generate spri rules for a variant.
//
main(int argc, char* argv[])
{
	if(argc!=2 && argc!=3)
	{
		cerr<<"Usage: generate_spri.exe <variant id> [<output_file>]"<<endl;
		exit(-1);
	}

	string filename;
	ostream *fout;
	if(argc==3)
		fout=new ofstream(argv[2], ios::out);
	else
		fout=&cerr;


	VariantID_t *varidp=NULL;
	VariantIR_t *varirp=NULL;

	/* setup the interface to the sql server */
	pqxxDB_t pqxx_interface;
	BaseObj_t::SetInterface(&pqxx_interface);

	try 
	{

		cout<<"Looking up variant "<<string(argv[1])<<" from database." << endl;
		varidp=new VariantID_t(atoi(argv[1]));

		assert(varidp->IsRegistered()==true);

		// read the db  
		cout<<"Reading variant "<<string(argv[1])<<" from database." << endl;
		varirp=new VariantIR_t(*varidp);

		cout<<"Reading variant "<<varidp->GetOriginalVariantID()<<" from database." << endl;

		varirp->generate_spri(*fout);

	}
	catch (DatabaseError_t pnide)
	{
		cout<<"Unexpected database error: "<<pnide<<endl;
		exit(-1);
        }

	cout<<"Done!"<<endl;

	if(fout!=&cerr)
		((ofstream*)fout)->close();
		

	delete varidp;
	delete varirp;
}


