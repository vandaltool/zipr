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
#include <libIRDB-core.hpp>
#include <libgen.h>
#include <iomanip>
#include <algorithm>


using namespace std;
using namespace libIRDB;

void usage(char* name)
{
	cerr<<"Usage: "<<name<<" <variant_id>\n"; 
}


void dump_icfs(Instruction_t* insn)
{
	if(insn->GetIBTargets()==NULL)	
		return;	
	
	cout<<"\tComplete: "<<boolalpha<<insn->GetIBTargets()->IsComplete()<<endl;
	cout<<"\tModComplete: "<<boolalpha<<insn->GetIBTargets()->IsModuleComplete()<<endl;
	cout<<"\tTargets: "<<endl;
	for_each(insn->GetIBTargets()->begin(), insn->GetIBTargets()->end(), [&](const Instruction_t* targ)
	{
		const auto d=DecodedInstruction_t(targ);
		cout<<"\t"<<targ->GetBaseID()<<":"<<d.getDisassembly()<<endl;
	});
}

int main(int argc, char **argv)
{
        if(argc != 2)
        {
                usage(argv[0]);
                exit(1);
        }

	auto dump_icfs_flag=(db_id_t)BaseObj_t::NOT_IN_DATABASE; 
	auto dump_icfs_str=getenv("DUMP_ICFS");
	if(dump_icfs_str)
		dump_icfs_flag=(db_id_t)strtoull(dump_icfs_str,NULL,0);
		

        string programName(argv[0]);
        int variantID = atoi(argv[1]);

        VariantID_t *pidp=NULL;

        /* setup the interface to the sql server */
        pqxxDB_t pqxx_interface;
        BaseObj_t::SetInterface(&pqxx_interface);

        pidp=new VariantID_t(variantID);
        assert(pidp->IsRegistered()==true);



        bool one_success = false;
        for(set<File_t*>::iterator it=pidp->GetFiles().begin();
            it!=pidp->GetFiles().end();
                ++it)
        {
                File_t* this_file = *it;
                try
                {
                	FileIR_t *firp = new FileIR_t(*pidp, this_file);
	
			cout<<"file: "<<this_file->GetURL()<<endl;
			cout<<setw(9)<<"ID"<<" "
			    <<setw(10)<<"Addr."<<" "
			    <<setw(10)<<"IBTA"<<" "
			    <<setw(9)<<"FT ID"<<" "
			    <<setw(9)<<"TargID"<<" "
			    <<setw(9)<<"Func"<<" "
			    <<"Disassembly"<< endl;

                	assert(firp && pidp);

			for(InstructionSet_t::iterator it=firp->GetInstructions().begin(); it!=firp->GetInstructions().end(); ++it)
			{
				Instruction_t* insn=*it;
				assert(insn);
				cout<<hex<<setw(9)<<insn->GetBaseID()<<" "<<hex<<setw(10)<<insn->GetAddress()->GetVirtualOffset();
				
				cout<<" "<<hex<<setw(10)<<
					(insn->GetIndirectBranchTargetAddress() ? 
						insn->GetIndirectBranchTargetAddress()->GetVirtualOffset() :
						0
					)
				    <<" ";
				cout<<hex<<setw(9)<<(insn->GetFallthrough()  ? insn->GetFallthrough()->GetBaseID() : -1) << " ";
				cout<<hex<<setw(9)<<(insn->GetTarget()  ? insn->GetTarget()->GetBaseID() : -1) << " ";
				if(insn->GetFunction() && insn->GetFunction()->GetEntryPoint())
					cout<<hex<<setw(9)<<insn->GetFunction()->GetEntryPoint()->GetBaseID();
				else
					cout<<setw(9)<<"NoFunc";
					
				const auto d=DecodedInstruction_t(insn);
				cout<<" "<<d.getDisassembly()<<"("<<insn->GetComment()<<")"<<endl;
	
				if(dump_icfs_flag == insn->GetBaseID())
					dump_icfs(insn);
			}



                }
                catch (DatabaseError_t pnide)
                {
                        cerr << programName << ": Unexpected database error: " << pnide << "file url: " << this_file->GetURL() << endl;
                }
                catch (...)
                {
                        cerr << programName << ": Unexpected error file url: " << this_file->GetURL() << endl;
                }
        } // end file iterator

        // if any integer transforms for any files succeeded, we commit
        if (one_success)
	{
		cout<<"Commiting changes...\n";
                pqxx_interface.Commit();
	}

        return 0;
}

