
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

int Range::GetOffset() const
{
	return offset;
}

unsigned int Range::GetSize() const
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
