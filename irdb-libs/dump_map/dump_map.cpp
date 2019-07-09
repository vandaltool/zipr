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


void dump_icfs(Instruction_t* insn)
{
	if(insn->getIBTargets()==NULL)	
		return;	
	
	cout<<"\tComplete: "<<boolalpha<<insn->getIBTargets()->isComplete()<<endl;
	cout<<"\tModComplete: "<<boolalpha<<insn->getIBTargets()->isModuleComplete()<<endl;
	cout<<"\tTargets: "<<endl;
	for_each(insn->getIBTargets()->begin(), insn->getIBTargets()->end(), [&](const Instruction_t* targ)
	{
		const auto d=DecodedInstruction_t::factory(targ);
		cout<<"\t"<<targ->getBaseID()<<":"<<d->getDisassembly()<<endl;
	});
}

int main(int argc, char **argv)
{
        if(argc != 2)
        {
                usage(argv[0]);
                exit(1);
        }

	auto dump_icfs_flag=(DatabaseID_t)BaseObj_t::NOT_IN_DATABASE; 
	auto dump_icfs_str=getenv("DUMP_ICFS");
	if(dump_icfs_str)
		dump_icfs_flag=(DatabaseID_t)strtoull(dump_icfs_str,NULL,0);
		

        string programName(argv[0]);
        int variantID = atoi(argv[1]);

        /* setup the interface to the sql server */
        auto pqxx_interface=pqxxDB_t::factory();
        BaseObj_t::setInterface(pqxx_interface.get());

        auto pidp=VariantID_t::factory(variantID);
        assert(pidp->isRegistered()==true);



        bool one_success = false;
        for(set<File_t*>::iterator it=pidp->getFiles().begin();
            it!=pidp->getFiles().end();
                ++it)
        {
                File_t* this_file = *it;
                try
                {
                	auto firp = FileIR_t::factory(pidp.get(), this_file);
	
			cout<<"file: "<<this_file->getURL()<<endl;
			cout<<setw(9)<<"ID"<<" "
			    <<setw(10)<<"Addr."<<" "
			    <<setw(10)<<"IBTA"<<" "
			    <<setw(9)<<"FT ID"<<" "
			    <<setw(9)<<"TargID"<<" "
			    <<setw(9)<<"Func"<<" "
			    <<"Disassembly"<< endl;

                	assert(firp && pidp);

			for(auto it=firp->getInstructions().begin(); it!=firp->getInstructions().end(); ++it)
			{
				Instruction_t* insn=*it;
				assert(insn);
				cout<<hex<<setw(9)<<insn->getBaseID()<<" "<<hex<<setw(10)<<insn->getAddress()->getVirtualOffset();
				
				cout<<" "<<hex<<setw(10)<<
					(insn->getIndirectBranchTargetAddress() ? 
						insn->getIndirectBranchTargetAddress()->getVirtualOffset() :
						0
					)
				    <<" ";
				cout<<hex<<setw(9)<<(insn->getFallthrough()  ? insn->getFallthrough()->getBaseID() : -1) << " ";
				cout<<hex<<setw(9)<<(insn->getTarget()  ? insn->getTarget()->getBaseID() : -1) << " ";
				if(insn->getFunction() && insn->getFunction()->getEntryPoint())
					cout<<hex<<setw(9)<<insn->getFunction()->getEntryPoint()->getBaseID();
				else
					cout<<setw(9)<<"NoFunc";
					
				const auto d=DecodedInstruction_t::factory(insn);
				cout<<" "<<d->getDisassembly()<<"("<<insn->getComment()<<")"<<endl;
	
				const auto is_dump_all  = dump_icfs_str && dump_icfs_flag==1;
				const auto is_dump_this = dump_icfs_flag == insn->getBaseID();
				if(is_dump_all || is_dump_this)
					dump_icfs(insn);
			}



                }
                catch (DatabaseError_t pnide)
                {
                        cerr << programName << ": Unexpected database error: " << pnide << "file url: " << this_file->getURL() << endl;
                }
                catch (...)
                {
                        cerr << programName << ": Unexpected error file url: " << this_file->getURL() << endl;
                }
        } // end file iterator

        // if any integer transforms for any files succeeded, we commit
        if (one_success)
	{
		cout<<"Commiting changes...\n";
                pqxx_interface->commit();
	}

        return 0;
}

