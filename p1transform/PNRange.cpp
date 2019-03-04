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


#include "PNRange.hpp"
#include <sstream>

using namespace std;

/*
  PNRange::PNRange(int displacement_offset, unsigned int padding_size, const Range &range) : Range(range)
  {
  this->displacement_offset = displacement_offset;
  this->padding_size = padding_size;

  new_size = size + padding_size;
  }
*/

PNRange::PNRange(const PNRange &range) : Range(range) 
{ 
	padding_size = range.padding_size;
	displacement = range.displacement;
}

PNRange::PNRange(const Range &range) : Range(range)
{
	padding_size = 0;
	displacement = 0;
}


PNRange::PNRange() : Range()
{
	displacement = 0;
	padding_size = 0;
}
unsigned int PNRange::GetPaddingSize() const
{
	return padding_size;
}
 
int PNRange::GetDisplacement() const
{
	return displacement;
}

void PNRange::SetDisplacement(int offset)
{
	displacement = offset;
}

void PNRange::SetPaddingSize(unsigned int pad_size)
{
	padding_size = pad_size;
}

void PNRange::Reset()
{
	padding_size = 0;
	displacement = 0;
}

string PNRange::ToString() const
{
	stringstream ss;

	ss<<Range::ToString()<<" Padding = "<<padding_size<<" Displacement = "<<displacement;

	return ss.str();
}


