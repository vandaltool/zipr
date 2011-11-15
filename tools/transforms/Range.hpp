
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
    virtual int GetOffset() const;
    virtual unsigned int GetSize() const;
    virtual void SetOffset(int offset);
    virtual void SetSize(unsigned int size);
    virtual std::string ToString() const;
};

#endif
