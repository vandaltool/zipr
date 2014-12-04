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

#ifndef __STACKLAYOUT
#define __STACKLAYOUT
#include "PNRange.hpp"
#include <vector>
#include <string>

class StackLayout
{
protected:
	//frame_alloc_size is the amount subtracted from esp to set up the stack
	//saved regs size is the size in bits of the number of registers pushed after ebp but before the stack alloc instruction

	std::vector<Range> mem_objects;
	unsigned int frame_alloc_size;
	unsigned int saved_regs_size;
	std::string function_name;
	unsigned int out_args_size;
	bool has_out_args;
	bool has_frame_pointer;
	bool is_canary_safe;
	bool is_padding_safe;
	bool is_static_stack;
	bool is_recursive;
	std::string layout_name;

public:

//NOTE: the following todo came from PNStackLayout where the insert functionality used to be, I am not sure what it means
//anymore, but just in case it is useful in the future, here is that original comment.
//TODO: change the code such that only inserts are allowed in the stack alloc region, create two frame sizes, local and total
	StackLayout(const std::string &layout_name, const std::string &function_name, unsigned int frame_alloc_size,
				unsigned int saved_regs_size, bool frame_pointer, unsigned int out_args_size);
	StackLayout(const StackLayout &layout);
	virtual void InsertESPOffset(int offset);
	virtual void InsertEBPOffset(int offset);
	virtual int EBPToESP(int offset);

	virtual unsigned int GetAllocSize();
	virtual unsigned int GetSavedRegsSize();
	virtual unsigned int GetOutArgsRegionSize();
	virtual bool HasFramePointer(){return has_frame_pointer;}
	virtual void SetCanarySafe(bool val){ is_canary_safe = val;}
	//TODO: handle this better. 
	virtual void SetPaddingSafe(bool val){ is_padding_safe = val; is_canary_safe = is_canary_safe && val;}
	virtual void SetStaticStack(bool val){ is_static_stack = val; }
	virtual void SetRecursive(bool val){ is_recursive = val; }
	virtual std::vector<Range> GetRanges() { return mem_objects; }
	//TODO: why is this public
	virtual unsigned int GetClosestIndex(int loc) const;
	friend class PNStackLayout;
};

#endif
