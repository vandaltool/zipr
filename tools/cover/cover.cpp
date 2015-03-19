/*
 * Copyright (c) 2013, 2014 - University of Virginia 
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

#include <iostream>
#include "coverage.h"
#include <cstdlib>
#include <fstream>

using namespace std;
using namespace libIRDB;

void usage(string prog_name)
{
	cerr << "usage: "<<prog_name<<" <variant_id> <coverage_file> <output_file>" << endl;
}


int main(int argc, char **argv)
{

	if(argc != 4)
	{
		usage(string(argv[0]));
		return -1;
	}

	int variant_id = atoi(argv[1]);
	string coverage_file_name = string(argv[2]);
	string output_file_name = string(argv[3]);

	coverage prog_coverage;

	ifstream coverage_file;
	coverage_file.open(coverage_file_name.c_str());

	if(!coverage_file.is_open())
	{
		cerr<<"Coverage Error: Could not open coverage file: "<<coverage_file_name<<endl;
		return -1;
	}

	prog_coverage.parse_coverage_file(coverage_file);
	coverage_file.close();

	ofstream output_file;
	output_file.open(output_file_name.c_str(),ofstream::out);

	if(!output_file.is_open())
	{
		cerr<<"Coverage Error: Could not open output file: "<<output_file_name<<endl;
		return -1;
	}

	pqxxDB_t pqxx_interface;
	BaseObj_t::SetInterface(&pqxx_interface);
	
	VariantID_t *vidp;
	try
	{
		vidp = new VariantID_t(variant_id);
		assert(vidp->IsRegistered());
	}
	catch (DatabaseError_t pnide)
	{
		cout<<"Unexpected database error: "<<pnide<<endl;
		return -1;	
	}
	
	prog_coverage.print_function_coverage_file(vidp,output_file);

	output_file.close();
}

