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


#ifndef __RANGE
#define __RANGE

#include <string>

class Range
{
protected:
	int offset;
	unsigned int size;
public:
	Range();
	// Range(int base_offset, unsigned int size);
	Range(const Range &range);
	virtual int getOffset() const;
	virtual unsigned int getSize() const;
	virtual void SetOffset(int offset);
	virtual void SetSize(unsigned int size);
	virtual std::string ToString() const;
};

#endif
