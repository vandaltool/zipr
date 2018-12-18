/*
 * Copyright (c) 2014-2015 - Zephyr Software LLC
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

#include <stdlib.h>
#include <fstream>
#include <libIRDB-core.hpp>
#include <libgen.h>

#include "scfi_instr.hpp"

using namespace std;
using namespace libIRDB;


#define BINARY_NAME "a.ncexe"
#define SHARED_OBJECTS_DIR "shared_objects"


void usage(char* name)
{
	cerr<<" Usage: "<<name<<" <variant_id>  \n"
"               [--nonce-size [1|2|4|8]]  \n"
"		[--exe-nonce-size [1|2|4|8]]  \n"
"		[--color|--no-color]  \n"
"		[--color-exe-nonces|--no-color-exe-nonces]  \n"         
"		[--protect-jumps|--no-protect-jumps]  \n"
"		[--protect-calls|--no-protect-calls]  \n"
"		[--protect-rets|--no-protect-rets] \n"
"		[--protect-safefn|--no-protect-safefn]  \n"
"		[ --common-slow-path | --no-common-slow-path ] \n"
"		[ --multimodule | --no-multimodule ] \n"
"		[--exe-nonce-for-call|--no-exe-nonce-for-call]  \n"
" \n"
"default: --no-color --no-color-exe-nonces --no-protect-jumps --protect-calls --protect-rets --protect-safefn --common-slow-path --no-multimodule --no-exe-nonce-for-call --nonce-size 1 --exe-nonce-size 4\n"; 
}

int main(int argc, char **argv)
{
        if(argc < 2)
        {
                usage(argv[0]);
                exit(1);
        }

#if 0
// CFI step is asserting with error message if a call is found.
	if(!getenv("FIX_CALLS_FIX_ALL_CALLS"))
	{
		cerr<<"FIX_CALLS_FIX_ALL_CALLS should be set."<<endl;
                exit(1);
	}
#endif

	bool do_coloring=false;
        bool do_color_exe_nonces=false;
	bool do_common_slow_path=true;
	bool do_jumps=false; // do_jumps=true will cause failures b/c registers stay live over ind jumps
	bool do_calls=true;
	bool do_rets=true;
	bool do_safefn=true;
	bool do_multimodule=false;
	bool do_exe_nonce_for_call=false;
        int nonce_size=1;
        int exe_nonce_size=4;
	for(int  i=2;i<argc;i++)
	{
                if(string(argv[i])=="--nonce-size" && 
                        string(argv[i+1]).length()==1 && isdigit(string(argv[i+1])[0]))
                {
                    nonce_size = stoi(string(argv[i+1]));
                    if(!(nonce_size == 1 || nonce_size == 2 || nonce_size == 4 || nonce_size == 8))
                    {
                        cerr<<"Unknown option: "<< argv[i] << endl;
			usage(argv[0]);
			exit(1);
                    }
                    ++i; // Skip over size argument
                }
                else if(string(argv[i])=="--exe-nonce-size" && 
                        string(argv[i+1]).length()==1 && isdigit(string(argv[i+1])[0]))
                {
                    exe_nonce_size = stoi(string(argv[i+1]));
                    if(!(exe_nonce_size == 1 || exe_nonce_size == 2 || exe_nonce_size == 4 || exe_nonce_size == 8))
                    {
                        cerr<<"Unknown option: "<< argv[i] << endl;
			usage(argv[0]);
			exit(1);
                    }
                    ++i; // Skip over size argument
                }
		else if(string(argv[i])=="--color")
		{
			cout<<"Using coloring..."<<endl;
			do_coloring=true;
		}
		else if(string(argv[i])=="--no-color")
		{
			cout<<"Not using coloring..."<<endl;
			do_coloring=false;
		}
                else if(string(argv[i])=="--color-exe-nonces")
		{
			cout<<"Using coloring for exe nonces..."<<endl;
			do_color_exe_nonces=true;
		}
		else if(string(argv[i])=="--no-color-exe-nonces")
		{
			cout<<"Not using coloring for exe nonces..."<<endl;
			do_color_exe_nonces=false;
		}
		else if(string(argv[i])=="--protect-calls")
		{
			cout<<"protecting calls..."<<endl;
			do_calls=true;
		}
		else if(string(argv[i])=="--no-protect-calls")
		{
			cout<<"Not protecting calls..."<<endl;
			do_calls=false;
		}
		else if(string(argv[i])=="--protect-jumps")
		{
			cout<<"protecting jumps..."<<endl;
			do_jumps=true;
		}
		else if(string(argv[i])=="--no-protect-jumps")
		{
			cout<<"Not protecting jumps..."<<endl;
			do_jumps=false;
		}
		else if(string(argv[i])=="--protect-rets")
		{
			cout<<"protecting returns..."<<endl;
			do_rets=true;
		}
		else if(string(argv[i])=="--no-protect-rets")
		{
			cout<<"Not protecting returns..."<<endl;
			do_rets=false;
		}
		else if(string(argv[i])=="--protect-safefn")
		{
			cout<<"protecting safe functions..."<<endl;
			do_safefn=true;
		}
		else if(string(argv[i])=="--no-protect-safefn")
		{
			cout<<"Not protecting safe functions..."<<endl;
			do_safefn=false;
		}
		else if(string(argv[i])=="--no-multimodule")
		{
			cout<<"Not adding multimodule support..."<<endl;
			do_multimodule=false;
		}
		else if(string(argv[i])=="--multimodule")
		{
			cout<<"Adding multimodule support ..."<<endl;
			do_multimodule=true;
		}
		else if(string(argv[i])=="--no-exe-nonce-for-call")
		{
			cout<<"Not adding exe-nonce-for-call support..."<<endl;
			do_exe_nonce_for_call=false;
		}
		else if(string(argv[i])=="--exe-nonce-for-call")
		{
			cout<<"Adding exe-nonce-for-call support ..."<<endl;
			do_exe_nonce_for_call=true;
		}
		else if(string(argv[i])=="--common-slow-path")
		{
			cout<<"Using common slow path..."<<endl;
			do_common_slow_path=true;
		}
		else if(string(argv[i])=="--no-common-slow-path")
		{
			cout<<"Not using common slow path..."<<endl;
			do_common_slow_path=false;
		}
		else
		{
			cerr<<"Unknown option: "<< argv[i] << endl;
			usage(argv[0]);
			exit(1);
		}
	}

        string programName(argv[0]);
        int variantID = atoi(argv[1]);

        VariantID_t *pidp=NULL;

        /* setup the interface to the sql server */
        pqxxDB_t pqxx_interface;
        BaseObj_t::SetInterface(&pqxx_interface);

        pidp=new VariantID_t(variantID);
        assert(pidp->IsRegistered()==true);

	cout<<"selective_cfi.exe started\n";

        bool one_success = false;
	bool seen_failures = false;
        for(set<File_t*>::iterator it=pidp->GetFiles().begin();
            it!=pidp->GetFiles().end();
                ++it)
        {
                File_t* this_file = *it;
                FileIR_t *firp = new FileIR_t(*pidp, this_file);

		cout<<"Transforming "<<this_file->GetURL()<<endl;

                assert(firp && pidp);

                try
                {
			SCFI_Instrument scfii(firp, nonce_size, exe_nonce_size, do_coloring, do_color_exe_nonces, do_common_slow_path, do_jumps, do_calls, do_rets, do_safefn, do_multimodule, do_exe_nonce_for_call);


			int success=scfii.execute();

                        if (success)
                        {
				cout<<"Writing changes for "<<this_file->GetURL()<<endl;
                                one_success = true;
                                firp->WriteToDB();
                                delete firp;
                        }
			else
			{
				seen_failures = true;
				cout<<"Skipping (no changes) "<<this_file->GetURL()<<endl;
			}
                }
                catch (DatabaseError_t pnide)
                {
			seen_failures = true;
                        cerr << programName << ": Unexpected database error: " << pnide << "file url: " << this_file->GetURL() << endl;
                }
                catch (...)
                {
			seen_failures = true;
                        cerr << programName << ": Unexpected error file url: " << this_file->GetURL() << endl;
                }
        } // end file iterator

        // if any integer transforms for any files succeeded, we commit
        if (one_success)
	{
		cout<<"Commiting changes...\n";
                pqxx_interface.Commit();
	}

	if(seen_failures)
	{
		return 1;
	}
	else
	{
        	return 0;
	}
}

