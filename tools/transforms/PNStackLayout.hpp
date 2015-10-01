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

#ifndef __PNSTACKLAYOUT
#define __PNSTACKLAYOUT
#include "PNRange.hpp"
#include "StackLayout.hpp"
#include <string>
#include <vector>
#include <exception>
#include <assert.h>
#include "globals.h"

//NOTE: padding adds a value between max and min, plus the frame size
//I believe this was done to protect against a very large buffer 
//overflow, but I'm not sure I have the example to demonstrate the
//effectiveness. If I had to guess, I think the example is if the
//attacker is trying to exceed the frame size, and the vulnerable buffer
//is at the bottom of the stack, but moved by us to the top, the 
//overflow might exceed our padding and corrupt other stack frames.  
//const int MIN_PADDING = 64;
//const int MAX_PADDING = 64;
//const int MAX_PADDING = MIN_PADDING*2;
//const int RECURSIVE_MIN_PADDING = 32;
//const int RECURSIVE_MAX_PADDING = RECURSIVE_MAX_PADDING*2;


class PNStackLayout
{
protected:
	int ALIGNMENT_BYTE_SIZE;
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
	virtual int GetNewOffsetESP(int ebp_offset) const;
	virtual int GetNewOffsetEBP(int ebp_offset) const;
	virtual PNStackLayout GetCanaryLayout() const;
	virtual std::vector<PNRange*> GetRanges() {return mem_objects;}
	virtual bool IsCanarySafe() const 
	{
		assert(pn_options);
		return 
			stack_layout.is_canary_safe && 		// detected as safe.
			pn_options->getDoCanaries() && 		// and we're allowed to do canaries.
			pn_options->shouldCanaryFunction(stack_layout.GetFunctionName());	// and we're allowed to do canaries on this function
	}
	virtual bool IsPaddingSafe()const {return stack_layout.is_padding_safe;}
	virtual bool IsShuffleSafe() const ;
	virtual bool IsP1() const;
	virtual bool IsStaticStack()const {return stack_layout.is_static_stack;}
	virtual bool HasFramePointer()const{return stack_layout.has_frame_pointer;}
	virtual bool DoShuffleValidate()const {return !IsCanarySafe() && IsShuffleSafe();} 
	//memory leak?
	//virtual StackLayout* GetStackLayout() { return new StackLayout(stack_layout); }

	virtual StackLayout GetStackLayout() { return StackLayout(stack_layout); }

	//This previously was a protected func, moved out for TNE,
	//to support dynamic array padding, the name is a bit confusing. 
	virtual unsigned int GetRandomPadding(unsigned int obj_size=0);
};

#endif
