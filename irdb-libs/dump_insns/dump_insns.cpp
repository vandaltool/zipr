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

                	assert(firp && pidp);

			for(auto insn : firp->getInstructions())
			{
				assert(insn);
				const auto p_d=DecodedInstruction_t::factory(insn);
				const auto &d=*p_d;
				const auto &operands=d.getOperands();

				cout<<" "<<d.getDisassembly()<<endl;
				int op_count=0;
				for(const auto p_op : operands)
				{
					const auto &op=*p_op;
					auto readWriteString= string();
					if(op.isRead()) readWriteString += "READ ";
					if(op.isWritten()) readWriteString += "WRITE ";

					cout<<"\t"<<"operand["<<op_count<<"]="<<op.getString() << boolalpha 
					    <<" rw="     <<readWriteString
					    <<" isGPReg="<<op.isGeneralPurposeRegister()
					    <<" isMem="  <<op.isMemory()
					    <<" isImm="  <<op.isConstant() 
					    <<" isPcrel="<<op.isPcrel() 
					    << endl;
	
					op_count++;			
				}
				
	
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

