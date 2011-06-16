
#include <libIRDB.hpp>
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

