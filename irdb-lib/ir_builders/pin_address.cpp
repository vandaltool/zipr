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
#include <string.h>
#include <cstring>
#include <cstdlib>

using namespace libIRDB;
using namespace std;

/*
 * Prefix-aware (pa) string to unsigned
 * long conversion function.
 */
unsigned long pa_strtoul(const char * const number)
{	
	unsigned long result = 0l;
	char *end = NULL;
	int base = 10;
	int number_len = strlen(number);

	/*
	 * Determine the base.
	 */
	if (number_len >= 2 && number[0] == '0' && 
	   ((number[1] == 'x') || (number[1] == 'X')))
		base = 16;
	else if (number_len >= 1 && number[0] == '0')
		base = 8;

	result = strtoul(number, &end, base);	
	if (end == &number[number_len])
	{
		return result;
	}
	else
	{
		return 0;
	}
}

int main(int argc, char* argv[])
{
	VariantID_t *pidp=NULL;
	FileIR_t * firp=NULL;
	unsigned long address_to_pin = 0;
	bool did_pin = false;

	if(argc != 3)
	{
		cerr<<"Usage: pin_address.exe <id> <address>" << endl;
		exit(-1);
	}

	address_to_pin = pa_strtoul(argv[2]);
	try 
	{
		/* setup the interface to the sql server */
		pqxxDB_t pqxx_interface;
		BaseObj_t::SetInterface(&pqxx_interface);

		pidp=new VariantID_t(atoi(argv[1]));

		assert(pidp->IsRegistered()==true);

		for(set<File_t*>::iterator it=pidp->GetFiles().begin();
			it!=pidp->GetFiles().end();
			++it)
		{
			File_t* this_file=*it;
			assert(this_file);


			// read the db  
			firp=new FileIR_t(*pidp, this_file);
			/*
			 * put the two nested loops here.
			 */
			for(
			  set<Function_t*>::const_iterator itf=firp->GetFunctions().begin();
			  itf!=firp->GetFunctions().end();
			  ++itf
			  )
			{
				Function_t* func=*itf;
				for(
				  set<Instruction_t*>::const_iterator it=func->GetInstructions().begin();
				  it!=func->GetInstructions().end();
				  ++it)
				{
					Instruction_t* insn = *it;
					if (insn->GetAddress()->GetVirtualOffset() == address_to_pin)
					{
						insn->SetIndirectBranchTargetAddress(insn->GetAddress());
						did_pin = true;
					}
				}
			}
			firp->WriteToDB();
			delete firp;

			if (did_pin) {
				cout <<"Pinned 0x" << hex << address_to_pin
				     << " in file " << this_file->GetURL()<<endl;
				break;
			}
		}

		pqxx_interface.Commit();
		if (!did_pin) {
			cerr << "Oops: Could not find an instruction at 0x" << hex
			     << address_to_pin << " to pin." << endl;
		}
	}
	catch (DatabaseError_t pnide)
	{
		cout<<"Unexpected database error: "<<pnide<<endl;
		exit(-1);
	}

	assert(firp && pidp);

	delete pidp;
	return 0;
}
