

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
	if(argc!=3 && argc!=4)
	{
		cerr<<"Usage: generate_spri.exe <is shared object> <variant id> [<output_file>]"<<endl;
		exit(-1);
	}

	int with_ilr=!atoi(argv[1]);
	assert(with_ilr==!!with_ilr);

	string filename;
	ostream *fout;
	if(argc==4)
		fout=new ofstream(argv[3], ios::out);
	else
		fout=&cout;


	VariantID_t *varidp=NULL;
	FileIR_t *firp=NULL;

	/* setup the interface to the sql server */
	pqxxDB_t pqxx_interface;
	BaseObj_t::SetInterface(&pqxx_interface);

	try 
	{

		cerr<<"Looking up variant "<<string(argv[2])<<" from database." << endl;
		varidp=new VariantID_t(atoi(argv[2]));

		assert(varidp->IsRegistered()==true);


                for(set<File_t*>::iterator it=varidp->GetFiles().begin();
                        it!=varidp->GetFiles().end();
                        ++it
                    )
                {
                        File_t* this_file=*it;
                        assert(this_file);
			cerr<<"Reading variant "<<string(argv[2])<<":"<<this_file->GetURL()
			   <<" from database." << endl;

			// read the db  
			firp=new FileIR_t(*varidp,this_file);
			firp->GenerateSPRI(*fout, with_ilr);
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


