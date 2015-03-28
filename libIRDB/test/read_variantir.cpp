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
#include <iostream>
#include <stdlib.h>

using namespace libIRDB;
using namespace std;

main(int argc, char* argv[])
{

	if(argc!=2)
	{
		cerr<<"Usage: read_variantir <id>"<<endl;
		exit(-1);
	}


	VariantID_t *pidp=NULL;
	try 
	{
		/* setup the interface to the sql server */
		pqxxDB_t pqxx_interface;
		BaseObj_t::SetInterface(&pqxx_interface);

		pidp=new VariantID_t(atoi(argv[1]));

		assert(pidp->IsRegistered()==true);

		cout<<"New Variant, after reading registration, is: "<<*pidp << endl;
		for(set<File_t*>::iterator it=pidp->GetFiles().begin();
                        it!=pidp->GetFiles().end();
                        ++it
			)
		{
			File_t* this_file=*it;
			assert(this_file);

			cout<<"Analyzing file "<<this_file->GetURL()<<endl;

			// read the db  
			FileIR_t* firp=new FileIR_t(*pidp, this_file);
			assert(firp);

			for(
				set<Instruction_t*>::const_iterator it=firp->GetInstructions().begin();
				it!=firp->GetInstructions().end(); 
				++it
				)
			{
				Instruction_t* insn=*it;
				cout<<"Found insn at addr:" << std::hex << insn->GetAddress()->GetVirtualOffset() << " " << insn->getDisassembly() << endl;
				ICFS_t* ibtargets = insn->GetIBTargets();
				if (!ibtargets) continue;

				ICFS_t::iterator ibtargets_it;

				if (ibtargets->size() > 0)
					cout<<"   indirect branch targets: ";

				int count;
				for (count = 0, ibtargets_it = ibtargets->begin(); ibtargets_it != ibtargets->end(); ++ibtargets_it, ++count)
				{
					Instruction_t* insn = *ibtargets_it;
					assert(insn);
					cout<< std::hex << insn->GetAddress()->GetVirtualOffset() << " ";
					if (count >= 10) {
						cout << "...";
						break;
					}
				}
				if (ibtargets->size() > 0)
					cout << dec << endl;
			}

			for(ICFSSet_t::const_iterator it=firp->GetAllICFS().begin();
				it != firp->GetAllICFS().end();
				++it)
			{
				ICFS_t *icfs = *it;
				cout << "icfs set id: " << icfs->GetBaseID() << "  #ibtargets: " << icfs->size() << endl;
			}
			delete firp;
		}
		delete pidp;
	}
	catch (DatabaseError_t pnide)
	{
		cout<<"Unexpected database error: "<<pnide<<endl;
		exit(-1);
	}

}
