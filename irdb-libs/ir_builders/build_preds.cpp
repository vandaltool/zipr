/*
 * Copyright (c) 2014 - Zephyr Software LLC
 *
 * This file may be used and modified for non-commercial purposes as long as
 * all copyright, permission, and nonwarranty notices are preserved.
 * Redistribution is prohibited without prior written consent from Zephyr
 * Software.
 *
 * Please contact the authors for restrictions applying to commercial use.
 *
 * THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: Zephyr Software
 * e-mail: jwd@zephyr-software.com
 * URL   : http://www.zephyr-software.com/
 *
 */



#include <libIRDB-core.hpp>
#include <libIRDB-util.hpp>
#include <utils.hpp> // to_string function from libIRDB
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

using namespace libIRDB;
using namespace std;

//
// main routine to build and print a call graph.
//
int main(int argc, char* argv[])
{
	if(argc!=2)
	{
		cerr<<"Usage: "<<argv[0]<<" <variant id> "<<endl;
		exit(-1);
	}

	string filename;
	ostream *fout;
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

		// A callgraph 
		InstructionPredecessors_t *ip=new InstructionPredecessors_t;

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
	
			// ILR only for the main file.
			ip->AddFile(firp);
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

	return 0;
}


