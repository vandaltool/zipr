/*
 * Copyright (c) 2014, 2015 - University of Virginia 
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

#ifndef _LIBTRANSFORM_KILL_DEADS_H
#define _LIBTRANSFORM_KILL_DEADS_H

#include <libIRDB-core.hpp>
#include "../../libtransform/include/transform.hpp"
#include "../../libMEDSannotation/include/VirtualOffset.hpp"

class KillDeads : public libTransform::Transform
{
	public:
		KillDeads(libIRDB::FileIR_t *p_variantIR, libIRDB::pqxxDB_t &p_dbinterface); 
		int execute();

	private:
		libIRDB::pqxxDB_t &dbinterface;
};
#endif
