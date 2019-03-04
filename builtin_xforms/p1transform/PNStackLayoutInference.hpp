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


#ifndef __PNSTACKLAYOUTINFERENCE
#define __PNSTACKLAYOUTINFERENCE
#include <irdb-core>
#include <irdb-cfg>
#include "StackLayout.hpp"
#include "PNStackLayout.hpp"
#include <string>
#include <map>

class PNStackLayoutInference
{

public:	   
	virtual ~PNStackLayoutInference(){}
	virtual PNStackLayout*	GetPNStackLayout(IRDB_SDK::Function_t *func)=0;
	virtual std::string GetInferenceName()const=0;
};

#endif
