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

#include "kill_deads.hpp"

#include <assert.h>
#include <stars.h>

using namespace libTransform;
using namespace libIRDB;
using namespace STARS;
using namespace std;

KillDeads::KillDeads(FileIR_t *p_variantIR, pqxxDB_t& p_dbinterface) 
	: 
		Transform(NULL, p_variantIR, NULL),
		dbinterface(p_dbinterface)
{
	
}

int KillDeads::execute()
{
	IRDB_Interface_t stars_analysis_engine(dbinterface);
	stars_analysis_engine.do_STARS(getFileIR());

}
