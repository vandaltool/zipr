
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
