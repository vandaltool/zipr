#include "StackLayout.hpp"
#include <cassert>
#include <iostream>

using namespace std;

StackLayout::StackLayout(const std::string &layout_name, const std::string &function_name, unsigned int frame_alloc_size,
						 unsigned int saved_regs_size, bool frame_pointer,unsigned int out_args_size)
{
	assert(out_args_size <= frame_alloc_size);

	this->layout_name = layout_name;
	this->function_name = function_name;
	this->frame_alloc_size = frame_alloc_size;
	this->saved_regs_size = saved_regs_size;
	this->has_frame_pointer = frame_pointer;
	this->out_args_size = out_args_size;
	has_out_args = out_args_size > 0;
	is_canary_safe = true;
	is_padding_safe = true;
	//TODO: is static stack hacked in for TNE, needs a redesign 
	is_static_stack = true;//innocent 'til proven guilty.
	is_recursive = false; //innocent 'til proven guilty

	//The initial layout is one entry the size of the stack frame.
	//The size is minus the out args size, if greater than 0
	//an out args memory object is pushed below.
	//The offset is following the out args region, if one exists.
	PNRange frame;
	frame.SetOffset((int)out_args_size);
	frame.SetSize((int)frame_alloc_size-(int)out_args_size);
	
	//If there are out args, then it is automatically pushed as a mem_objects
	//starting at 0.
	if(has_out_args)
	{	
		if(out_args_size != frame_alloc_size)
		{
			PNRange out_args;
			out_args.SetOffset(0);
			out_args.SetSize((int)out_args_size);
			mem_objects.push_back(out_args);
		}
		else
		{
			frame.SetOffset(0);
			frame.SetSize((int)out_args_size);
		}
	}

	//Push frame last to ensure the mem_objects vector is sorted
	//by offset if there is an out args region
	mem_objects.push_back(frame);
}

StackLayout::StackLayout(const StackLayout &layout)
{
	this->layout_name = layout.layout_name;
	this->function_name = layout.function_name;
	this->frame_alloc_size = layout.frame_alloc_size;
	this->saved_regs_size = layout.saved_regs_size;
	this->out_args_size = layout.out_args_size;
	this->has_frame_pointer = layout.has_frame_pointer;
	has_out_args = layout.out_args_size > 0;
	is_canary_safe = layout.is_canary_safe;
	is_padding_safe = layout.is_padding_safe;
	is_static_stack = layout.is_static_stack;
	is_recursive = layout.is_recursive;

	for(unsigned int i=0;i<layout.mem_objects.size();i++)
	{
		this->mem_objects.push_back(layout.mem_objects[i]);
	}  
}

void StackLayout::InsertESPOffset(int offset)
{
	//No Negative offsets allowed
	if(offset < 0)
	{
		cerr<<"StackLayout: InsertESPOffset(): Negative Offset Encountered, Ignoring Insert"<<endl;
		//assert(false);
		return;
	}

	cerr<<"StackLayout: Attempting to insert offset "<<offset<<endl;
 

	if(offset >= (int)frame_alloc_size)
	{
		//This can happen when an esp offset is accessing stored registers or function parameters
		cerr<<"StackLayout: InsertESPOffset(): Offset is larger than or equal to the allocated stack, Ignoring insert"<<endl;
		return;
	} 

	if(has_out_args)
	{
		//If there are out args, mem_objects[0] is assumed to be the out args region
		//if the new offset being inserted is in this range, the offset is ignored. 
		//The out args region is not divided
		if(offset < (int)mem_objects[0].GetSize())
		{
			cerr<<"StackLayout: InsertESPOffset(): Offset in out args region, Ignoring insert"<<endl;
			return;
		}
	}

	PNRange new_range;

	new_range.SetOffset(offset);
	
	int index = GetClosestIndex(offset);

	if(index < 0)
	{
		//TODO: don't assert false, ignore, but for now I want to find when this happens
		cerr<<"StackLayout: InsertESPOffset(): Could not Find Range to Insert Into, Asserting False for Now"<<endl;
		//TODO: throw exception or ignore?
		assert(false);
	}

	//This should never happen, but just in case, assert false.
	if(index > ((int)mem_objects.size())-1)
		assert(false); //TODO: throw exception

	cerr<<"PNStackLayout: InsertESPOffset(): found offset = "<<
		mem_objects[index].GetOffset()<<endl;
	//If offset is unique, insert it after the closest
	//range (closest base address with out going over offset)
	if(offset != mem_objects[index].GetOffset())
	{
		if(index+1 == (int)mem_objects.size())
			mem_objects.push_back(new_range);
		else
			mem_objects.insert(mem_objects.begin()+index+1,new_range);
	}
	//else no need to insert, the offset already exists
	else
	{
		cerr<<"StackLayout: InsertESPOffset(): Offset already exists, ignoring insert"<<endl;
		return;
	}

	//The memory object that was divided has to have its size adjusted
	//based on where the new memory object was inserted (its offset).
	mem_objects[index].SetSize(mem_objects[index+1].GetOffset()-mem_objects[index].GetOffset());
	
	//If the location of the newly inserted range is the end of the vector
	//then the size will be the difference between the frame size and the 
	//offset of the new range
	if((int)mem_objects.size() == index+2)
		mem_objects[index+1].SetSize(((int)frame_alloc_size)-mem_objects[index+1].GetOffset());
	//Otherwise it is the difference between the next offset and the current offset
	else
		mem_objects[index+1].SetSize(mem_objects[index+2].GetOffset()-mem_objects[index+1].GetOffset());

	cerr<<"Stacklayout: Insert Successful, Post Insert Offsets"<<endl;
	for(unsigned int i=0;i<mem_objects.size();i++)
	{
		cerr<<"\tOffset = "<<mem_objects[i].GetOffset()<<endl;
	}
}

void StackLayout::InsertEBPOffset(int offset)
{
	//The size of the saved regs must be taken into consideration when transforming
	//the EBP offset to an esp relative offset
	cerr<<"StackLayout: InsertEBPOffset(): Offset="<<offset<<" frame alloc size="<<frame_alloc_size<<" saved regs size="<<saved_regs_size<<endl;
	int esp_conversion = EBPToESP(offset);//((int)frame_alloc_size+(int)saved_regs_size) - offset;

	//It is possible to have ebp offsets that extend beyond the stack pointer. I haven't seen it
	//but still. If this occurs, ignore the insert. We currently do not handle this case
	if(esp_conversion >= 0)
		InsertESPOffset(esp_conversion);
	else
	{
		cerr<<"PNStackLayout: InsertEBPOffset: Coverted EBP offset to negative ESP offset, ignoring insert"<<endl;
	}
}

unsigned int StackLayout::GetClosestIndex(int loc) const
{
	int high = ((int)mem_objects.size())-1;
	int low = 0;
	int mid;
	
	while(low <= high)
	{
		mid = (low+high)/2;
		if((loc < (mem_objects[mid].GetOffset()+(int)mem_objects[mid].GetSize()))  && (loc >= mem_objects[mid].GetOffset()))
			return mid;
		else if(loc > mem_objects[mid].GetOffset())
			low = mid +1;
		else
			high = mid -1;
	}
	return -1;
}


unsigned int StackLayout::GetAllocSize()
{
	return frame_alloc_size;
}

unsigned int StackLayout::GetSavedRegsSize()
{
	return saved_regs_size;
}

unsigned int StackLayout::GetOutArgsRegionSize()
{
	return out_args_size;
}

//TODO: maybe this should be an unsigned int, since I am assuming it is
//positive. 
int StackLayout::EBPToESP(int offset)
{
	return ((int)frame_alloc_size+(int)saved_regs_size) - offset;
}
