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


#include "PNStackLayout.hpp"
#include <cstdlib>
#include <algorithm>
#include <cassert>
#include <sstream>
#include <ctime>
#include "globals.h"
#include <libIRDB-core.hpp>

//TODO: debug use only
#include <iostream>

using namespace std;

//TODO: so far it has been assumed that you insert then padd and shuffle
//but if a new element is added after shuffling or padding, what should I do?

//TODO: we do not handle negative offsets relative to esp. These offsets will not be inserted
//into the layout, however if this case is encountered, offsets are changed here to avoid
//complications. This is primarily a problem with EBP offsets extending beyond the stack
//pointer. I have not observed this so far, but if this does happen, upon requesting a new
//offset the amount passed the stack pointer is calculated, and added to the new frame
//size to give a new EBP relative offset. ESP relative offsets require to adjustments

static bool CompareRangeBaseOffset(PNRange *a, PNRange *b)
{
	return (a->GetOffset() < b->GetOffset());
}

static bool CompareRangeDisplacedOffset(PNRange *a, PNRange *b)
{
	return ((a->GetOffset()+a->GetDisplacement()) < (b->GetOffset()+b->GetDisplacement()));
}


unsigned int PNStackLayout::GetRandomPadding(unsigned int obj_size)
{
	int min,max;

	min = pn_options->getMinStackPadding();
	max = pn_options->getMaxStackPadding();

	if(stack_layout.is_recursive)
	{
		min = pn_options->getRecursiveMinStackPadding();
		max = pn_options->getRecursiveMaxStackPadding();
	}


	int pad = (rand() % (max+1-min)) + min;
	if(isaligned)
	{
//TODO: if previously padded, and not aligned, this will not gurantee alignment, so 
//I should probably pass the memory object itself, so I can use all that information.
		pad = pad + obj_size;
		pad = pad - (pad % ALIGNMENT_BYTE_SIZE);
		pad = pad - obj_size;
	}

	//Finally, add padding equivalent to the original stack frame size
	//this helps protect against very large overflows/underflows.
	//align the original stack frame if not aligned, adding more bytes if necessary
	//TODO: should this be scaled down if the func is recursive?



	if(pn_options->getShouldDoubleFrameSize() && obj_size < pn_options->getDoubleThreshold())
	{
		//if the original frame size is not aligned, then add as many bytes as necessary to align it
		//for example, if 3 bytes over alignment, and the alignment stride is 8, then add 8 - 3, or 5 bytes. 
		pad += (ALIGNMENT_BYTE_SIZE - (stack_layout.frame_alloc_size % ALIGNMENT_BYTE_SIZE));
		pad += stack_layout.frame_alloc_size; 
	}

	return pad;
}

//TODO: a ToString function

//TODO: return value if insert in out args region?

//TODO: use of unsigned ints?? While making it easier here, it is prone to type errors by the user

//TODO: negative offsets?


PNStackLayout::PNStackLayout(StackLayout stack_layout) : stack_layout(stack_layout)
{
	ALIGNMENT_BYTE_SIZE=libIRDB::FileIR_t::GetArchitectureBitWidth()/sizeof(int);
	//PNTransformDriver sets up the seed, I need a better way of handling this
	//but for now assume it has been properly seeded. 
	//srand(time(NULL));

	this->stack_layout = stack_layout;
	isPadded = false;
	isShuffled = false;
	altered_alloc_size = stack_layout.frame_alloc_size;

	for(unsigned int i=0;i<stack_layout.mem_objects.size();i++)
	{
		PNRange *pn_obj = new PNRange(stack_layout.mem_objects[i]);
		mem_objects.push_back(pn_obj);
	}

	base_id = 0;
	entry_id = 0;
}

PNStackLayout::PNStackLayout(const PNStackLayout &stack_layout): stack_layout(stack_layout.stack_layout)
{
	ALIGNMENT_BYTE_SIZE=libIRDB::FileIR_t::GetArchitectureBitWidth()/sizeof(int);
	pn_layout_name = stack_layout.pn_layout_name;
	isPadded = stack_layout.isPadded;
	isShuffled = stack_layout.isShuffled;
	isaligned = stack_layout.isaligned;
	this->stack_layout = stack_layout.stack_layout;
	altered_alloc_size = stack_layout.altered_alloc_size;

	for(unsigned int i=0;i<stack_layout.mem_objects.size();i++)
	{
		PNRange *pn_obj = new PNRange(*stack_layout.mem_objects[i]);
		mem_objects.push_back(pn_obj);
	}

	base_id = 0;
	entry_id = 0;
}

PNStackLayout::~PNStackLayout()
{
	for(unsigned int i=0;i<mem_objects.size();i++)
	{
		delete mem_objects[i];
	}
}

//Shuffle generates new displacement_offset values for each PNRange which represents
//a logical shuffling, it does not change the ordering of the mem_objects data structure.
void PNStackLayout::Shuffle()
{
	if(!IsShuffleSafe())
		return;

	//TODO: this function can be optimized to not randomize the mem_objects vector then resort

	int lowest_address = mem_objects[0]->GetDisplacement() + mem_objects[0]->GetOffset();

	//Find the lowest displacement offset 
	//This is in case the layout has been padded before shuffling. 
	//In this case, the element randomly selected to be the first element
	//will be placed at the lowest displaced offset, rather than at 0. If 0 
	//were used this would effectively remove any padding between 0 and the
	//first memory object. 
	for(unsigned int i=1;i<mem_objects.size();i++)
	{
		int next_offset = mem_objects[i]->GetDisplacement() + mem_objects[i]->GetOffset();
		if(lowest_address > next_offset)
			lowest_address = next_offset;
	}

	int random_index;
	int start_index = 0;
	
	//if there are out args, the first element of the passed in vector
	//is considered the out args and will not be shuffled
	if(stack_layout.has_out_args)
	{
		start_index = 1;
	}

	//Shuffle all regions except the lowest region if there are out args
	for(int i=start_index;i<((int)mem_objects.size())-1;i++)
	{
		//TODO: There may be a bias here
		random_index = i + (rand() % (mem_objects.size() - i));

		assert(random_index>=i && random_index<(int)mem_objects.size());

		PNRange *swap = mem_objects[i];
		mem_objects[i] = mem_objects[random_index];
		mem_objects[random_index] = swap;
	}

	//If there aren't any out args, then the element at the lowest displaced
	//address (displacement + offset) is considered the base address, 
	//if not previously padded this should be zero, in which case the
	//new displacment will be 0.
	//No check is needed for out args since the lowest_address should be 0
	//if there are out args
	mem_objects[0]->SetDisplacement(lowest_address-mem_objects[0]->GetOffset());

	//generate new base addresses for each region
	for(unsigned int i=1;i<mem_objects.size();i++)
	{
		//Displacement = New Location  - original location
		//Displacement + offset = new address
		mem_objects[i]->SetDisplacement((mem_objects[i-1]->GetDisplacement()+mem_objects[i-1]->GetOffset() +
										 mem_objects[i-1]->GetSize() + mem_objects[i-1]->GetPaddingSize()) -
										mem_objects[i]->GetOffset());
	}

	//At this bound the mem_objects data structure has been randomized, sort by original base offset
	sort(mem_objects.begin(),mem_objects.end(), CompareRangeBaseOffset);

	isShuffled = true;

	if(verbose_log)
	{
		cerr<<"PNStackLayout: Shuffle(): "<<ToString()<<endl;
		for(unsigned int i=0;i<mem_objects.size();i++)
		{
			cerr<<"\tOffset = "<<mem_objects[i]->GetOffset()<<" Size = "<<mem_objects[i]->GetSize()<<
				" Padding = "<<mem_objects[i]->GetPaddingSize()<<" displ = "<<mem_objects[i]->GetDisplacement()<<endl;
		}
	}
}

PNStackLayout PNStackLayout::GetCanaryLayout() const
{
	PNStackLayout new_layout = *(this);

	new_layout.AddCanaryPadding();

	return new_layout;
}

//Adds padding for canaries between objects, or if padding
//exists, does nothing.
void PNStackLayout::AddCanaryPadding()
{
	// if(!IsPaddingSafe())
	// 	return;
//TODO: I should throw an exception, but for now I will just assert false if
//the layout is not padding safe but canary padding is requested. 
	if(!IsPaddingSafe())
		assert(false);


//TODO: I need to check the padding for each variable, but for now, I will assume
//if the layout is padded, it is padded enough for canaries
	if(IsPadded())
		return;

	unsigned int size = 8;

// Twitcher adds another guard
#ifdef TWITCHER_GUARD
        size += 8;
#endif

	sort(mem_objects.begin(),mem_objects.end(),CompareRangeDisplacedOffset);
	//counts the additional padding added, does not take into consideration previous padding
	unsigned int total_padding =0;
	

	unsigned int curpad = size;
	total_padding += curpad;
	mem_objects[0]->SetPaddingSize(curpad+mem_objects[0]->GetPaddingSize());

	for(unsigned int i=1;i<mem_objects.size();i++)
	{
		mem_objects[i]->SetDisplacement(total_padding+mem_objects[i]->GetDisplacement());
		curpad = size;
		total_padding += curpad;
		mem_objects[i]->SetPaddingSize(curpad+mem_objects[i]->GetPaddingSize());
	}
	
	sort(mem_objects.begin(),mem_objects.end(),CompareRangeBaseOffset);

	//the altered frame size is the size of the old altered frame size plus the additional
	//padding added.
	altered_alloc_size += total_padding;

	isPadded = true; 

	if(verbose_log)
	{
		cerr<<"PNStackLayout: AddPadding(): "<<ToString()<<endl;
		for(unsigned int i=0;i<mem_objects.size();i++)
		{
			cerr<<"\tOffset = "<<mem_objects[i]->GetOffset()<<" Size = "<<mem_objects[i]->GetSize()<<
				" Padding = "<<mem_objects[i]->GetPaddingSize()<<" displ = "<<mem_objects[i]->GetDisplacement()<<endl;
		}
	}	 
}


/* 
 * roundUp - round number up to the nearest multiple .  multiple must be power of 2.
 */
int roundUp(int numToRound, int multiple) 
{
	assert((multiple & (multiple-1))==0);
   	return (numToRound + multiple - 1) & ~(multiple - 1);
}

/*
  Add padding between variables in the stack frame. If there are no out args, 
  padding is added below the lowest variable, but only if the stack frame
  is not a p1 reduction. Hypotheses of the out args region are sometimes
  incorrect. If the out args region is not identified, adding padding
  below the lowest variable will result in all transforms incorrectly
  transforming the stack. To avoid this, if the most conservative layout is found
  (p1), no padding is added below the lowest variable. This way at least p1 
  can transform these stack frames.

  Previous padding is added to, not removed. Padding may be added before or after
  shuffling.
*/
void PNStackLayout::AddRandomPadding(bool isaligned)
{
	if(!IsPaddingSafe())
		return;

	this->isaligned = isaligned;

	if(verbose_log)
		cerr<<"ALIGNMENT IS "<<isaligned<<endl;

	sort(mem_objects.begin(),mem_objects.end(),CompareRangeDisplacedOffset);
	//counts the additional padding added, does not take into consideration previous padding
	unsigned int total_padding = GetRandomPadding(0);
	
	//if there is no out args region, add padding below the memory object at esp
	// if(!has_out_args)
	//Only add padding from below the memory object at esp if there are no out args and
	//the layout does not reduce to p1. The reasonf or including the check for a reduction
	//to p1 was added because some times the out args is not found, causing all transforms
	//to fail. For p1, padding from below doesn't add much anyway, so removing this 
	//diversification allows for a transform even if the out args have been incorrectly 
	//identified. 
	if(!stack_layout.has_out_args && mem_objects.size() != 1)
		mem_objects[0]->SetDisplacement((int)total_padding+mem_objects[0]->GetDisplacement());
	else
	{
		mem_objects[0]->SetDisplacement(0);
		total_padding = 0;
	}

	unsigned int curpad = GetRandomPadding(mem_objects[0]->GetSize());
	total_padding += curpad;
	mem_objects[0]->SetPaddingSize(curpad+mem_objects[0]->GetPaddingSize());

	for(unsigned int i=1;i<mem_objects.size();i++)
	{
		mem_objects[i]->SetDisplacement(total_padding+mem_objects[i]->GetDisplacement());
		curpad = GetRandomPadding(mem_objects[i]->GetSize());
		total_padding += curpad;
		mem_objects[i]->SetPaddingSize(curpad+mem_objects[i]->GetPaddingSize());
	}

	
	sort(mem_objects.begin(),mem_objects.end(),CompareRangeBaseOffset);

	int last=mem_objects.size()-1;
	int last_pad=mem_objects[last]->GetPaddingSize();
	if(verbose_log)
	{
		cerr<<"Last object size=0x"<<hex<<last_pad<<endl;
		cerr<<"altered_aloc_size=0x"<<hex<<altered_alloc_size<<endl;
		cerr<<"total_padding=0x"<<hex<<total_padding<<endl;
	}
	
	int new_total_padding_size=roundUp(total_padding,ALIGNMENT_BYTE_SIZE);
	mem_objects[last]->SetPaddingSize(mem_objects[last]->GetPaddingSize()+new_total_padding_size-total_padding);
	total_padding=new_total_padding_size;

	//the altered frame size is the size of the old altered frame size plus the additional
	//padding added.
	altered_alloc_size += total_padding;

	isPadded = true; 

	if(verbose_log)
	{
		cerr<<"PNStackLayout: AddPadding(): "<<ToString()<<endl;
		for(unsigned int i=0;i<mem_objects.size();i++)
		{
			cerr<<"\tOffset = "<<mem_objects[i]->GetOffset()<<" Size = "<<mem_objects[i]->GetSize()<<
				" Padding = "<<mem_objects[i]->GetPaddingSize()<<" displ = "<<mem_objects[i]->GetDisplacement()<<endl;
		}
	}	  
}

//Basically this is the same proces as the random padding function, but the padding
//is determined by not by random, but by the location of each variable. 
void PNStackLayout::AddDMZPadding()
{
	sort(mem_objects.begin(),mem_objects.end(),CompareRangeDisplacedOffset);
	//counts the additional padding added, does not take into consideration previous padding
	unsigned int total_padding = stack_layout.frame_alloc_size;
	
	//if there is no out args region, add padding below the memory object at esp
	// if(!has_out_args)
	//Only add padding from below the memory object at esp if there are no out args and
	//the layout does not reduce to p1. The reasonf or including the check for a reduction
	//to p1 was added because some times the out args is not found, causing all transforms
	//to fail. For p1, padding from below doesn't add much anyway, so removing this 
	//diversification allows for a transform even if the out args have been incorrectly 
	//identified. 
	if(!stack_layout.has_out_args && mem_objects.size() == 1)
		mem_objects[0]->SetDisplacement((int)total_padding+mem_objects[0]->GetDisplacement());
	else
	{
		mem_objects[0]->SetDisplacement(0);
		total_padding = 0;
	}

	unsigned int curpad = (stack_layout.frame_alloc_size-mem_objects[0]->GetOffset())-mem_objects[0]->GetSize();
	total_padding += curpad;
	mem_objects[0]->SetPaddingSize(curpad+mem_objects[0]->GetPaddingSize());

	for(unsigned int i=1;i<mem_objects.size();i++)
	{
		mem_objects[i]->SetDisplacement(total_padding+mem_objects[i]->GetDisplacement());
		curpad = (stack_layout.frame_alloc_size-mem_objects[i]->GetOffset())-mem_objects[i]->GetSize();
		total_padding += curpad;
		mem_objects[i]->SetPaddingSize(curpad+mem_objects[i]->GetPaddingSize());
	}
	
	sort(mem_objects.begin(),mem_objects.end(),CompareRangeBaseOffset);

	//the altered frame size is the size of the old altered frame size plus the additional
	//padding added.
	altered_alloc_size += total_padding;

	isPadded = true; 


	if(verbose_log)
	{
		cerr<<"PNStackLayout: AddDMZPadding(): "<<ToString()<<endl;
		for(unsigned int i=0;i<mem_objects.size();i++)
		{
			cerr<<"\tOffset = "<<mem_objects[i]->GetOffset()<<" Size = "<<mem_objects[i]->GetSize()<<
				" Padding = "<<mem_objects[i]->GetPaddingSize()<<" displ = "<<mem_objects[i]->GetDisplacement()<<endl;
		}
	}		
}


bool PNStackLayout::IsPadded() const
{
	return isPadded;
}

bool PNStackLayout::IsShuffled() const
{
	return isShuffled;
}


unsigned int PNStackLayout::GetOriginalAllocSize() const
{
	return stack_layout.frame_alloc_size;
}

unsigned int PNStackLayout::GetAlteredAllocSize() const
{
	return altered_alloc_size;
}

unsigned int PNStackLayout::GetSavedRegsSize() const
{
	return stack_layout.saved_regs_size;
}

PNRange* PNStackLayout::GetClosestRangeEBP(int loc) const
{
	//The size of the saved regs must be taken into consideration when transforming
	//the EBP offset to an esp relative offset
	return GetClosestRangeESP(((int)stack_layout.frame_alloc_size+(int)stack_layout.saved_regs_size) - loc);
}

//Finds the range whose base_offset is the closest to
//loc without going over.
//If no match can be found a negative offset may have been
//passed, the algorithm failed, or the number of mem_objects
//is 0. In this case a null pointer is returned;
PNRange* PNStackLayout::GetClosestRangeESP(int loc) const
{
	if(verbose_log)
		cerr<<"PNstackLayout: GetClosestRangeESP(): Seaching for ESP Offset "<<loc<<endl;

	if(loc >= (int)stack_layout.frame_alloc_size)
	{
		if(verbose_log)
		{
			//For now don't do anything if the loc is greater than the frame size
			cerr<<"PNStackLayout: GetClosestRangeESP(): loc >= frame_alloc_size, Returning NULL"<<endl;
		}
		return NULL;
	}

	if(loc < 0)
	{
		if(verbose_log)
			cerr<<"PNStackLayout: GetClosestRangeESP(): loc < 0 ("<<loc<<"), Returning NULL"<<endl;
		return NULL;
	}

	int index = stack_layout.GetClosestIndex(loc);

	if(index < 0)
	{
		if(verbose_log)
			cerr<<"PNStackLayout: GetClosestRangeESP(): Could Not Find Range, Returning NULL"<<endl;
		return NULL;
	}

	if(verbose_log)
		cerr<<"PNStackLayout: GetClosestRangeESP(): Found range "<<mem_objects[index]->ToString()<<endl;

	return mem_objects[index];
}

unsigned int PNStackLayout::GetNumberOfMemoryObjects() const
{
	return mem_objects.size();
}

string PNStackLayout::ToString() const
{
	stringstream ss;
	
	ss<<"Layout = "<<stack_layout.layout_name + "; Function = "<<
		stack_layout.function_name <<"; Frame Alloc Size = "<<stack_layout.frame_alloc_size<<" Altered Frame Size ="<<
		altered_alloc_size<<" Saved Regs Size = "<<stack_layout.saved_regs_size<<"; Out Args Size = "<< 
		stack_layout.out_args_size<<"; Number of Memory Objects = "<< mem_objects.size()<<"; Padded = ";
 
	if(isPadded)
		ss<<"true";
	else
		ss<<"false";

	ss<<"; Shuffled = ";

	if(isShuffled)
		ss<< "true";
	else
		ss<<"false";

	ss<<"; canary_safe = ";
	if(this->IsCanarySafe())
		ss<<"true";
	else
		ss<<"false";

	return ss.str();
}


string PNStackLayout::GetLayoutName() const
{
	return stack_layout.layout_name;
}

string PNStackLayout::GetFunctionName() const
{
	return stack_layout.function_name;
}


//A frame can be shuffled if there are two or more variables, not including
//the out arguments region. 
bool PNStackLayout::IsShuffleSafe() const
{
	return (mem_objects.size() > 2) || (mem_objects.size() == 2 && !stack_layout.has_out_args);
}

bool PNStackLayout::IsP1() const
{
	//for now this is actually identical (although negated) to the logic used in IsShuffleSafe()
	//however, this might chagne in the future, so I am copying the logic
	//from IsShuffleSafe
	//For example, I would like to detect if an offset is inferred or actually
	//encountered. Padding between the out args and the first variable
	//can give the false impression of boundaries. 
	return !((mem_objects.size() > 2) || (mem_objects.size() == 2 && !stack_layout.has_out_args));
}


unsigned int PNStackLayout::GetOutArgsSize() const
{
	return stack_layout.out_args_size;
}


int PNStackLayout::GetNewOffsetESP(int esp_offset) const
{
	int new_offset = esp_offset;

	//If the esp relative address goes above or equal to the frame pointer, esp
	//could be accessing incoming args or the saved registers
	//(such as fomit-frame-pointer). In this case we simply add
	//to the offset the size of the altered frame.

	//TODO: converting stack_layout.frame_alloc_size to int
	//check if the size is greater than max int. This should
	//never occur though. 
	if(esp_offset >= (int)stack_layout.frame_alloc_size)
	{
		if(verbose_log)
			cerr<<"PNStackLayout: GetNewOffsetESP: Offset greater than or equal to frame size, adjusting based on new frame size"<<endl;
		//Get the number of bytes beyond the stack frame
		new_offset = esp_offset-stack_layout.frame_alloc_size;
		//add those bytes to the altered stack size
		new_offset += altered_alloc_size;	
	}
	else
	{
		PNRange *closest = GetClosestRangeESP(esp_offset);

		//TODO: I assume no negative ESP offsets, at this point if the esp offset is not found
		//then we have an issue in the program, so assert(false) to uncover the problem. 
		if(closest == NULL)
		{
			assert(false);
		}
		if(verbose_log)
			cerr<<"PNStackLayout: GetNewOffsetESP: closest displacement = "<<closest->GetDisplacement()<<endl;

		new_offset = closest->GetDisplacement() + esp_offset;
	}

	return new_offset;	
}

int PNStackLayout::GetNewOffsetEBP(int ebp_offset) const
{

	//If the function doesn't use a frame pointer, then do not alter any ebp relative instructions.
	if(!HasFramePointer())
	{
		return ebp_offset;
	}

	//In the event that an ebp relative offset extends beyond the stack pointer
	//determine the distance beyond the stack pointer, and return 
	//the sum of this distance with the new frame size and saved register region size
	if(ebp_offset > (int)GetOriginalAllocSize()+(int)GetSavedRegsSize())
	{
		if(verbose_log)
			cerr<<"PNStackLayout: GetNewOffsetEBP: ebp offset extends passed stack pointer, maintaining relative position to esp"<<endl;
		return ebp_offset-(int)GetOriginalAllocSize()+(int)altered_alloc_size;
	}

	PNRange *closest = GetClosestRangeEBP(ebp_offset);

	//If no range can be found, since the above check finds
	//offsets passed the stack pointer, it is assumed the ebp offset
	//refers to a saved register (the space between the frame pointer
	//and where local variables start). 
	//Often ebp relative addresses access saved registeres, if this
	//happens, there is no need to do a conversion, as no padding
	//is added between EBP and saved registers. The untouched offset
	//is returned. 
	if(closest == NULL)
	{
		return ebp_offset;
	}

	if(verbose_log)
		cerr<<"PNStackLayout: GetNewOffsetEBP: closest displacement = "<<closest->GetDisplacement()<<endl;

	int new_offset = ((int)GetOriginalAllocSize() + (int)GetSavedRegsSize() - ebp_offset);
	new_offset += closest->GetDisplacement();
	new_offset = ((int)altered_alloc_size + (int)GetSavedRegsSize()) - new_offset;

	return new_offset;
}

void PNStackLayout::ResetLayout()
{
	isPadded = false;
	isShuffled = false;
	for(unsigned int i=0;i<mem_objects.size();i++)
	{
		mem_objects[i]->Reset();
	}
}

string PNStackLayout::ToMapEntry() const
{
	stringstream ss;
	
	ss << "" << stack_layout.layout_name << ";" << stack_layout.function_name << ";" << stack_layout.frame_alloc_size << ";" <<
		altered_alloc_size << ";" << stack_layout.saved_regs_size << ";" << stack_layout.out_args_size << ";" << mem_objects.size() << ";";
 
	if(isPadded)
		ss << "true";
	else
		ss << "false";

	ss << ";";

	if(isShuffled)
		ss << "true";
	else
		ss << "false";

	ss << ";";
	if(this->IsCanarySafe())
		ss << "true";
	else
		ss << "false";

	ss << ";";

	if(canaries.size() > 0) 
		ss << std::hex << canaries[0].canary_val;
	else
		ss << 0;

	ss << ";";
	/*asj5b - add func id*/
	ss << std::hex << base_id;
	ss << ";";
	ss << std::hex << entry_id;

        /* add canary offset */
	ss << ";";
	if(canaries.size() > 0) 
		ss << std::dec << canaries[0].floating_offset;
	else
		ss << 0;

	return ss.str();
}
