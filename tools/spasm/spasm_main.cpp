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

#include "spasm.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <vector>
#include <assert.h>

using namespace std;

bool fexists(string filename)
{
	ifstream ifile(filename.c_str());
	return ifile;
}

void usage()
{
    cerr<<"SPASM usage:\n-s <symbol file> <input files> <exe>"<<endl;
    exit(1);
}


///Utility SPASM's main
int main(int argc, char *argv[])
{
    	string input, output, elf;

    	if(argc == 5)
    	{
    		elf = string(argv[4]);
		if(!fexists(elf))
		{
			cerr<<"Symbol file "<<elf<<" does not exist.  SPASM will not be able to process callbacks properly."<<endl;
			assert(false);
		}
    	}
    	else if(argc == 6) 
    	{
    		elf = string(argv[4]);
		if(!fexists(elf))
		{
    			elf = string(argv[5]);
			if(!fexists(elf))
			{
				cerr<<"Symbol files ("<<argv[4] << " and " << argv[6] << 
					") do not exist.  SPASM will not be able to process callbacks properly."<<endl;
				assert(false);
			}
		}
		
    }
    else 
    {
        cerr<<"SPASM Usage:\n<input file> <output file> <symbol file> [<symbol file>] \n"<<endl;
        exit(1);
    }

    input = string(argv[1]);
    output = string(argv[2]);
    string exe = string(argv[3]);
	cout<<"Input:"<<input<<endl;
	cout<<"Output:"<<output<<endl;
	cout<<"Symbols:"<<elf<<endl;

	vector<string> input_list;
	input_list.push_back(input);
    try
    {
		a2bspri(input_list,output,exe,elf);
    }
    catch (SpasmException err)
    {
        cerr<<err.what()<<endl;
        exit(1);
    }

	return 0;
}
