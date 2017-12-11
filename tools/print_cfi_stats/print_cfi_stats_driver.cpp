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


using namespace std;
using namespace libIRDB;

void usage(char* name)
{
	cerr<<"Usage: "<<name<<" <variant_id>\n"; 
}


void print_cfi_stats(FileIR_t* firp)
{
	InstructionSet_t total_call_targets, total_jmp_targets, total_ret_targets, total_ib_targets;
	int calls=0, jmps=0, rets=0, ibs=0;
	int act_call_targs=0, act_jmp_targs=0, act_ret_targs=0, act_ib_targs=0;
	int insn_count=0;
	int insn_byte_count=0;


	for(
		InstructionSet_t::iterator it=firp->GetInstructions().begin();
		it!=firp->GetInstructions().end();
		++it
	   )
	{
		Instruction_t* insn=*it;
		assert(insn);

		// count total insns
		insn_count++;
		insn_byte_count+=insn->GetDataBits().size();

		ICFS_t* icfs_set= insn->GetIBTargets();

		// skip if not ib.
		if(!icfs_set) continue;

		string dis=insn->getDisassembly();

		// record IB stats.
		ibs++;
		act_ib_targs+=icfs_set->size();
		total_ib_targets.insert(icfs_set->begin(), icfs_set->end());


		if(dis.find("call")!=string::npos)
		{
			calls++;
			act_call_targs+=icfs_set->size();
			total_call_targets.insert(icfs_set->begin(), icfs_set->end());
		}
		else if(dis.find("jmp")!=string::npos)
		{
			jmps++;
			act_jmp_targs+=icfs_set->size();
			total_jmp_targets.insert(icfs_set->begin(), icfs_set->end());
		}
		else if(dis.find("ret")!=string::npos)
		{
			rets++;
			act_ret_targs+=icfs_set->size();
			total_ret_targets.insert(icfs_set->begin(), icfs_set->end());
		}
	}

	float act_targs_per_ib=(float)act_ib_targs/(float)ibs;
	float act_targs_per_call=(float)act_call_targs/(float)calls;
	float act_targs_per_jmp=(float)act_jmp_targs/(float)jmps;
	float act_targs_per_ret=(float)act_ret_targs/(float)rets;

	cout<<"# ATTRIBUTE cfi::actual_targs_per_ib_no_cfi="<<insn_byte_count<<endl;
//	cout<<"# ATTRIBUTE cfi::actual_targs_per_call_no_cfi="<<insn_byte_count<<endl;
	cout<<"# ATTRIBUTE cfi::actual_targs_per_jmp_no_cfi="<<insn_byte_count<<endl;
	cout<<"# ATTRIBUTE cfi::ctual_targs_per_ret_no_cfi="<<insn_byte_count<<endl;

	cout<<"# ATTRIBUTE cfi::ossible_targs_per_ib_basic_cfi="<<total_ib_targets.size()<<endl;
//	cout<<"# ATTRIBUTE cfi::possible_targs_per_call_basic_cfi="<<total_call_targets.size()<<endl;
	cout<<"# ATTRIBUTE cfi::possible_targs_per_jmp_basic_cfi="<<total_jmp_targets.size()<<endl;
	cout<<"# ATTRIBUTE cfi::possible_targs_per_ret_basic_cfi="<<total_ret_targets.size()<<endl;

	cout<<"# ATTRIBUTE cfi::actual_targs_per_ib_refined_cfi="<<act_targs_per_ib<<endl;
//	cout<<"# ATTRIBUTE cfi::actual_targs_per_call_refined_cfi="<<act_targs_per_call<<endl;
	cout<<"# ATTRIBUTE cfi::actual_targs_per_jmp_refined_cfi="<<act_targs_per_jmp<<endl;
	cout<<"# ATTRIBUTE cfi::actual_targs_per_ret_refined_cfi="<<act_targs_per_ret<<endl;


	float basic_cfi_ib_percent_reduction=1-( (float)total_ib_targets.size() / insn_byte_count);
	float basic_cfi_call_percent_reduction=1-( (float)total_call_targets.size() / insn_byte_count);
	float basic_cfi_jmp_percent_reduction=1-( (float)total_jmp_targets.size() / insn_byte_count);
	float basic_cfi_ret_percent_reduction=1-( (float)total_ret_targets.size() / insn_byte_count);

	cout<<"# ATTRIBUTE cfi::basic_cfi_ib_percent_reduction="<<basic_cfi_ib_percent_reduction<<endl;
	cout<<"# ATTRIBUTE cfi::basic_cfi_call_percent_reduction="<<basic_cfi_call_percent_reduction<<endl;
	cout<<"# ATTRIBUTE cfi::basic_cfi_jmp_percent_reduction="<<basic_cfi_jmp_percent_reduction<<endl;
	cout<<"# ATTRIBUTE cfi::basic_cfi_ret_percent_reduction="<<basic_cfi_ret_percent_reduction<<endl;

	float refined_cfi_ib_percent_reduction=1-( act_targs_per_ib / total_ib_targets.size()  );
	float refined_cfi_call_percent_reduction=1-( act_targs_per_call / total_call_targets.size()  );
	float refined_cfi_jmp_percent_reduction=1-( act_targs_per_jmp / total_jmp_targets.size()  );
	float refined_cfi_ret_percent_reduction=1-( act_targs_per_ret / total_ret_targets.size()  );

	cout<<"# ATTRIBUTE cfi::refined_cfi_ib_percent_reduction_over_basic="<<refined_cfi_ib_percent_reduction<<endl;
	cout<<"# ATTRIBUTE cfi::refined_cfi_call_percent_reduction_over_basic="<<refined_cfi_call_percent_reduction<<endl;
	cout<<"# ATTRIBUTE cfi::refined_cfi_jmp_percent_reduction_over_basic="<<refined_cfi_jmp_percent_reduction<<endl;
	cout<<"# ATTRIBUTE cfi::refined_cfi_ret_percent_reduction_over_basic="<<refined_cfi_ret_percent_reduction<<endl;

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

        VariantID_t *pidp=NULL;

        /* setup the interface to the sql server */
        pqxxDB_t pqxx_interface;
        BaseObj_t::SetInterface(&pqxx_interface);

        pidp=new VariantID_t(variantID);
        assert(pidp->IsRegistered()==true);

	cout<<"ret_shadow_stack.exe started\n";

        bool one_success = false;
        for(set<File_t*>::iterator it=pidp->GetFiles().begin();
            it!=pidp->GetFiles().end();
                ++it)
        {
		File_t* this_file = *it;
                try
                {


			FileIR_t *firp = new FileIR_t(*pidp, this_file);

			cout<<"Transforming "<<this_file->GetURL()<<endl;

			assert(firp && pidp);


			print_cfi_stats(firp);
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

