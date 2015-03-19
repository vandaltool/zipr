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


#ifndef __PNRANGE
#define __PNRANGE

#include "Range.hpp"
#include <string>

class PNRange : public Range
{
protected:
	int displacement; //add displacement to offset to get displaced base
	unsigned int padding_size;
public:
	PNRange(const PNRange &range);
	PNRange(const Range &range);
	PNRange();
	virtual unsigned int GetPaddingSize() const;
	virtual int GetDisplacement() const;
	virtual void SetDisplacement(int offset);
	virtual void SetPaddingSize(unsigned int pad_size);
	virtual std::string ToString() const;
	virtual void Reset();

};

#endif
