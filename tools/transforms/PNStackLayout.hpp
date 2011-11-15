#ifndef __PNSTACKLAYOUT
#define __PNSTACKLAYOUT
#include "PNRange.hpp"
#include <string>
#include <vector>
#include <exception>

class PNStackLayout
{
protected:
    std::vector<PNRange*> mem_objects;
    unsigned int frame_alloc_size;
    unsigned int saved_regs_size;
    unsigned int altered_alloc_size;
    std::string pn_layout_name;
    std::string function_name;
    bool isPadded;
    bool isShuffled;
    bool has_out_args;
    unsigned int out_args_size;

    virtual unsigned int GetClosestIndex(int loc) const;
    virtual unsigned int GetRandomPadding();

public:
    //frame_alloc_size is the amount subtracted from esp to set up the stack
    //saved regs size is the size in bits of the number of registers pushed after ebp but before the stack alloc instruction

//TODO: change the code such that only inserts are allowed in the stack alloc region, create two frame sizes, local and total
    PNStackLayout(const std::string &layout_name, const std::string &function_name, 
		  unsigned int frame_alloc_size, unsigned int saved_regs_size, unsigned int out_args_size);
    virtual ~PNStackLayout();
    virtual void InsertESPOffset(int offset);
    virtual void InsertEBPOffset(int offset);
    virtual void Shuffle();
    virtual void AddPadding();
    virtual bool IsPadded() const;
    virtual bool IsShuffled() const;
    virtual bool CanShuffle() const;
    virtual bool P1Reduction() const;
    virtual unsigned int GetNumberOfMemoryObjects() const;
    virtual PNRange* GetClosestRangeEBP(int loc) const;
    virtual PNRange* GetClosestRangeESP(int loc) const;
    virtual unsigned int GetOriginalAllocSize() const;
    virtual unsigned int GetAlteredAllocSize() const;
    virtual unsigned int GetSavedRegsSize() const;
    virtual std::string ToString() const;
    virtual std::string GetLayoutName() const;
    virtual std::string GetFunctionName() const;
    virtual unsigned int GetOutArgsSize() const;

    //TODO: intended to be temporary
    virtual std::vector<PNRange*> GetMemoryObjects();
};

#endif
