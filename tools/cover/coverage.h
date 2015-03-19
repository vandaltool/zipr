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

#ifndef __PEASOUP_COVERAGE
#define __PEASOUP_COVERAGE

#include <map>
#include <libIRDB-core.hpp>
#include <iostream>
#include <string>

struct file_coverage
{
	std::string file;
	std::map<unsigned int,unsigned int> coverage;
};

class coverage
{
private:
	std::map<std::string,file_coverage> coverage_map;

	file_coverage* find_file_coverage(std::string url);
	void print_coverage_for_file(file_coverage *fc, libIRDB::FileIR_t *fileirp, std::ofstream &out_file);
public:
	void parse_coverage_file(std::ifstream &coverage_file);
	void print_function_coverage_file(libIRDB::VariantID_t *vidp,std::ofstream &out_file);
};

#endif
