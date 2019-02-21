/*
 * Copyright (c) 2014, 2015 - University of Virginia 
 *
 * This file may be used and modified for non-commercial purposes as long as 
 * all copyright, permission, and nonwarranty notices are preserved.  
 * Redistribution is prohibited without prior written consent from the University 
 * of Virginia.
 *
 * Please contact the authors for restrictions applying to commercial use.
 *
 * THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: University of Virginia
 * e-mail: jwd@virginia.com
 * URL   : http://www.cs.virginia.edu/
 *
 */

#include <stdlib.h>
#include <fstream>
#include <irdb-core>
#include <libgen.h>
#include <iomanip>
#include <algorithm>


using namespace std;
using namespace IRDB_SDK;

void usage(char* name)
{
	cerr<<"Usage: "<<name<<" <variant_id>\n"; 
}


int main(int argc, char **argv)
{
        if(argc != 2)
        {
                usage(argv[0]);
                exit(1);
        }

        auto programName=string(argv[0]);
        auto variantID = atoi(argv[1]);

        /* setup the interface to the sql server */
        auto pqxx_interface=pqxxDB_t::factory();
        BaseObj_t::setInterface(pqxx_interface.get());

        auto pidp=VariantID_t::factory(variantID);
        assert(pidp->isRegistered()==true);



        auto one_success = false;

	auto this_file = pidp->getMainFile();
	try
	{
		auto firp = FileIR_t::factory(pidp.get(), this_file);

		cout<<"file: "<<this_file->getURL()<<endl;
		cout<<setw(9)<<"ID"<<" "
		    <<setw(10)<<"Addr."<<" "
		    <<setw(30)<<left << "Comment"<<" "
		    <<setw(9)<<right<<"EhpID"<<" "
		    <<setw(15)<<"FDE prog sz"<<endl;

		assert(firp && pidp);

		auto total_fde_instructions = (size_t)0;
		for(auto insn : firp->getInstructions())
		{
			cout<<hex<<setw(9)<<insn->getBaseID()<<" "<<hex<<setw(10)<<insn->getAddress()->getVirtualOffset();
			cout<<" "<<setw(30)<<left << insn->getComment() << right;

			const auto ehp=insn->getEhProgram();
			if(ehp==nullptr)
			{
				cout<<"No EHP";
			}
			else
			{
				auto ehpID=ehp->getBaseID();
				cout<<setw(9)<<hex<<ehpID<<" ";

				auto fdeprog_size=ehp->getFDEProgram().size();
				cout<<setw(15)<<hex<<fdeprog_size<<" ";

				total_fde_instructions+=fdeprog_size;
			}
			cout<<endl;
		}
		cout<<"Total FDE Program instructions " << dec << total_fde_instructions << endl;



	}
	catch (DatabaseError_t pnide)
	{
		cerr << programName << ": Unexpected database error: " << pnide << "file url: " << this_file->getURL() << endl;
	}
	catch (...)
	{
		cerr << programName << ": Unexpected error file url: " << this_file->getURL() << endl;
	}

        // if any integer transforms for any files succeeded, we commit
        if (one_success)
	{
		cout<<"Commiting changes...\n";
                pqxx_interface->commit();
	}

        return 0;
}

