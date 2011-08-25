
#include <all.hpp>
#include <utils.hpp>
#include <stdlib.h>
using namespace libIRDB;


File_t::File_t(db_id_t myfile_id, std::string myurl, std::string myhash, std::string myarch, int myoid, db_id_t mydoipid) :
	BaseObj_t(NULL), url(myurl), hash(myhash), arch(myarch), elfoid(myoid)
{
	SetBaseID(myfile_id);
}

