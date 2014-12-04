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


#include <all.hpp>
#include <iostream>
#include <stdlib.h>

using namespace libIRDB;

std::ostream& libIRDB::operator<<(std::ostream& output, const DatabaseError_t& p)
{
	switch(p.GetErrorCode())
	{
		case DatabaseError_t::VariantTableNotRegistered:
			output<<"Error accessing variant_info table";
			break;
		case DatabaseError_t::VariantNotInDatabase:
			output<<"Variant not detected in database";
			break;
		default:
			std::cerr<<"Cannot print database error code, aborting program...\n";
			exit(-100); 
	}

	return output;
}

