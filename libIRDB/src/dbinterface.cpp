
#include <libIRDB.hpp>
#include <iostream>
#include <stdlib.h>

using namespace libIRDB;

std::ostream& libIRDB::operator<<(std::ostream& output, const DatabaseError_t& p)
{
	switch(p.GetErrorCode())
	{
		case DatabaseError_t::ProgramNotInDatabase:
			output<<"Program not detected in database";
			break;
		default:
			std::cerr<<"Cannot print database error code, aborting program...\n";
			exit(-100); 
	}

	return output;
}

