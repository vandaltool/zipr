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

