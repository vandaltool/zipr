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


//TODO: for now we can't trust the DB for the frame size,
//in the future the constructor should be passed the size as found
//by the database

#ifndef __P1INFERENCE
#define __P1INFERENCE

#include "PNStackLayoutInference.hpp"
#include "OffsetInference.hpp"

class P1Inference : public PNStackLayoutInference
{
protected:
	OffsetInference *offset_inference;
public:
	P1Inference(OffsetInference *offset_inference);
	virtual PNStackLayout* GetPNStackLayout(libIRDB::Function_t *func);
	virtual std::string GetInferenceName() const;
};

#endif
