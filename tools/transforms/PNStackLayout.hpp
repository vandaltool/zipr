#ifndef __PNSTACKLAYOUT
#define __PNSTACKLAYOUT
#include "PNRange.hpp"
#include "StackLayout.hpp"
#include <string>
#include <vector>
#include <exception>

const int MIN_PADDING = 4096;
const int MAX_PADDING = MIN_PADDING*2;
const int RECURSIVE_MIN_PADDING = 64;
const int RECURSIVE_MAX_PADDING = RECURSIVE_MAX_PADDING*2;
const int ALIGNMENT_BYTE_SIZE = 8;

class PNStackLayout
{
protected:
    std::string pn_layout_name;
    bool isPadded;
    bool isShuffled;
    bool isaligned;//should only be used internally, since I don't know if the raw stack is aligned.
    //TODO: Storing pointers here but I did away with pointers in StackLayout since I was worried about
    //possible memory leaks. I need pointers to return null, but I can achieve this functionality
    //through aliasing. Consider removing the pointers for consistency. 
    std::vector<PNRange*> mem_objects;

    //NOTE: I chose composition over inheritence here, as I wanted to make it impossible to change the
    //layout after a PNStackLayout has been created. The issue was if a new boundary was added
    //after the layout had been padded/shuffled/etc, it would cause problems. This is avoided by
    //this structure, but it is important to make a deap copy of the passed in stack_layout, which
    //is achieved through the copy constructor for StackLayout. 
    StackLayout stack_layout;
    unsigned int altered_alloc_size;

    
    virtual void AddCanaryPadding();

public:
    PNStackLayout(StackLayout stack_layout);
    PNStackLayout(const PNStackLayout &stack_layout);

    virtual ~PNStackLayout();

    virtual void Shuffle();
    //TODO: the max min and alignment size should also be optionally specified. 
    virtual void AddRandomPadding(bool isaligned=false);
    //Adds on to previous padding and does not change any previous displacement
    //changes. If you wish to only add DMZ padding, you must clear any previous
    //padding. 
    //TODO: should I worry about alignment here? 

    //DMZ padding is defined as the space between the original variable location
    //and the end of the local variable region. The theory is, if the variable
    //boundaries are correct, then this region should be untouched from any 
    //memory access. 
    virtual void AddDMZPadding();
    virtual bool IsPadded() const;
    virtual bool IsShuffled() const;
    virtual bool CanShuffle() const;
    //Remove any transformations, leaves the PNStackLayout as if the object had just been constructed.
    virtual void ResetLayout();
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
    virtual bool HasFramePointer() const;
    virtual int GetNewOffsetESP(int ebp_offset) const;
    virtual int GetNewOffsetEBP(int ebp_offset) const;
    virtual PNStackLayout GetCanaryLayout() const;
    virtual std::vector<PNRange*> GetRanges() {return mem_objects;}
    virtual bool IsCanarySafe() const {return stack_layout.is_canary_safe;}
    virtual bool IsPaddingSafe()const {return stack_layout.is_padding_safe;}
    virtual bool IsStaticStack()const {return stack_layout.is_static_stack;}
    //memory leak?
    //virtual StackLayout* GetStackLayout() { return new StackLayout(stack_layout); }

    virtual StackLayout GetStackLayout() { return StackLayout(stack_layout); }

    //This previously was a protected func, moved out for TNE,
    //to support dynamic array padding, the name is a bit confusing. 
    virtual unsigned int GetRandomPadding(unsigned int obj_size=0);
};

#endif
