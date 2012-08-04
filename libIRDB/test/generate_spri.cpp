

#include <libIRDB-core.hpp>
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
		fout=&cout;


	VariantID_t *varidp=NULL;
	FileIR_t *firp=NULL;

	/* setup the interface to the sql server */
	pqxxDB_t pqxx_interface;
	BaseObj_t::SetInterface(&pqxx_interface);

	try 
	{

		cerr<<"Looking up variant "<<string(argv[1])<<" from database." << endl;
		varidp=new VariantID_t(atoi(argv[1]));

		assert(varidp->IsRegistered()==true);


                for(set<File_t*>::iterator it=varidp->GetFiles().begin();
                        it!=varidp->GetFiles().end();
                        ++it
                    )
                {
                        File_t* this_file=*it;
                        assert(this_file);
			cerr<<"Reading variant "<<string(argv[1])<<":"<<this_file->GetURL()
			   <<" from database." << endl;

			// read the db  
			firp=new FileIR_t(*varidp,this_file);
			firp->GenerateSPRI(*fout);
			delete firp;
		}

	}
	catch (DatabaseError_t pnide)
	{
		cerr<<"Unexpected database error: "<<pnide<<endl;
		exit(-1);
        }

	cerr<<"Done!"<<endl;

	if(fout!=&cout)
		((ofstream*)fout)->close();
		

	delete varidp;
}


