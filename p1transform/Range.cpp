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


#include "Range.hpp"
#include <sstream>

using namespace std;

Range::Range()
{
	offset = 0;
	size = 0;
}
/*
  Range::Range(int base_offset, unsigned int size)
  {
  this->base_offset = base_offset;
  this->size = size;
  }
*/
Range::Range(const Range &range)
{
	offset = range.offset;
	size = range.size;
}

int Range::getOffset() const
{
	return offset;
}

unsigned int Range::getSize() const
{
	return size;
}

void Range::SetOffset(int offset)
{
	this->offset = offset;
}

void Range::SetSize(unsigned int size)
{
	this->size = size;
}

string Range::ToString() const
{
	stringstream ss;
	ss<<"Offset = "<<offset<<" Size = "<<size;
	return ss.str();
}
