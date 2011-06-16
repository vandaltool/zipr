
#include <libIRDB.hpp>
using namespace libIRDB;


/*
 * initialize the DB interface
 */
DBinterface_t* BaseObj_t::dbintr=NULL;
const db_id_t BaseObj_t::NOT_IN_DATABASE=-1;

/* 
 * BaseObj_t constructor
 */
BaseObj_t::BaseObj_t(doip_t* _doip) : doip(_doip), base_id(BaseObj_t::NOT_IN_DATABASE) 
{
}


/* 
 * Set the database interface to be used for the base object class.
 */
void BaseObj_t::SetInterface(DBinterface_t *dbi)
{
	dbintr=dbi;
}

