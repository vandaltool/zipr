
#include "PNStackLayout.hpp"
#include <cstdlib>
#include <algorithm>
#include <cassert>
#include <sstream>
#include <ctime>

//TODO: debug use only
#include <iostream>

using namespace std;

//TODO: so far it has been assumed that you insert then padd and shuffle
//but if a new element is added after shuffling or padding, what should I do?

static bool CompareRangeBaseOffset(PNRange *a, PNRange *b)
{
    return (a->GetOffset() < b->GetOffset());
}

static bool CompareRangeDisplacedOffset(PNRange *a, PNRange *b)
{
    return ((a->GetOffset()+a->GetDisplacement()) < (b->GetOffset()+b->GetDisplacement()));
}

//TODO: actually make this random
unsigned int PNStackLayout::GetRandomPadding()
{
    //srand(time(NULL));

    int min = 4096;
    return (rand() % 4096)+min;
}

//TODO: a ToString function

//TODO: return value if insert in out args region?

//TODO: use of unsigned ints?? While making it easier here, it is prone to type errors by the user

//TODO: negative offsets?

PNStackLayout::~PNStackLayout()
{
    for(unsigned int i=0;i<mem_objects.size();i++)
    {
	delete mem_objects[i];
    }
}

PNStackLayout::PNStackLayout(const string &layout_name, const string &function_name, unsigned int frame_alloc_size, 
			     unsigned int saved_regs_size, unsigned int out_args_size)
{
    assert(out_args_size <= frame_alloc_size);

    srand(time(NULL));

    pn_layout_name = layout_name;
    isPadded = false;
    isShuffled = false;
    has_out_args = out_args_size > 0;
    this->saved_regs_size = saved_regs_size;
    this->frame_alloc_size = frame_alloc_size;
    this->function_name = function_name;
    this->out_args_size = out_args_size;
    altered_alloc_size = frame_alloc_size;

    //The initial layout is one entry the size of the stack frame.
    //The size is minus the out args size, if greater than 0
    //an out args memory object is pushed below.
    //The offset is following the out args region, if one exists.
    PNRange *frame = new PNRange();
    frame->SetOffset((int)out_args_size);
    frame->SetSize((int)frame_alloc_size-(int)out_args_size);
    
    //If there are out args, then it is automatically pushed as a mem_objects
    //starting at 0.
    if(has_out_args)
    {	
	if(out_args_size != frame_alloc_size)
	{
	    PNRange *out_args = new PNRange();
	    out_args->SetOffset(0);
	    out_args->SetSize((int)out_args_size);
	    mem_objects.push_back(out_args);
	}
	else
	{
	    frame->SetOffset(0);
	    frame->SetSize((int)out_args_size);
	}
    }
    //Push frame last to ensure the mem_objects vector is sorted
    //by offset if there is an out args region
    mem_objects.push_back(frame);
}

void PNStackLayout::InsertESPOffset(int offset)
{
    //No Negative offsets allowed
    if(offset < 0)
    {
	cerr<<"PNStackLayout: InsertESPOffset(): Negative Offset Encountered, Ignoring Insert"<<endl;
	//assert(false);
	return;
    }

    cerr<<"PNStackLayout: Attempting to insert offset "<<offset<<endl;
 

    if(offset >= (int)frame_alloc_size)
    {
	//TODO: throw exception?
	//This can happen when an esp offset is accessing stored registers or function parameters
	cerr<<"PNStackLayout: InsertESPOffset(): Offset is larger than or equal to the allocated stack, Ignoring insert"<<endl;
	return;
    } 

    if(has_out_args)
    {
	//If there are out args, mem_objects[0] is assumed to be the out args region
	//if the new offset being inserted is in this range, the offset is ignored. 
	//The out args region is not divided
	if(offset < (int)mem_objects[0]->GetSize())
	{
	    cerr<<"PNStackLayout: InsertESPOffset(): Offset in out args region, Ignoring insert"<<endl;
	    return;
	}
    }

    //NOTE: make sure to delete this on failure, or else there will be a memory leak
    PNRange *new_range = new PNRange();

    new_range->SetOffset(offset);
	
    int index = GetClosestIndex(offset);

    if(index < 0)
    {
	delete new_range;

	//TODO: don't assert false, ignore, but for now I want to find when this happens
	cerr<<"PNStackLayout: InsertESPOffset(): Could not Find Range to Insert Into, Asserting False for Now"<<endl;
	//TODO: throw exception or ignore?
	assert(false);
    }

    //This should never happen, but just in case, assert false.
    if(index > ((int)mem_objects.size())-1)
	assert(false); //TODO: throw exception

    cerr<<"PNStackLayout: InsertESPOffset(): found offset = "<<
	mem_objects[index]->GetOffset()<<endl;
    //If offset is unique, insert it after the closest
    //range (closest base address with out going over offset)
    if(offset != mem_objects[index]->GetOffset())
    {
	if(index+1 == (int)mem_objects.size())
	    mem_objects.push_back(new_range);
	else
	    mem_objects.insert(mem_objects.begin()+index+1,new_range);
    }
    //else no need to insert, the offset already exists
    else
    {
	cerr<<"PNStackLayout: InsertESPOffset(): Offset already exists, ignoring insert"<<endl;
	delete new_range;
	return;
    }

    //The memory object that was divided has to have its size adjusted
    //based on where the new memory object was inserted (its offset).
    mem_objects[index]->SetSize(mem_objects[index+1]->GetOffset()-mem_objects[index]->GetOffset());
    
    //If the location of the newly inserted range is the end of the vector
    //then the size will be the difference between the frame size and the 
    //offset of the new range
    if((int)mem_objects.size() == index+2)
	mem_objects[index+1]->SetSize(((int)frame_alloc_size)-mem_objects[index+1]->GetOffset());
    //Otherwise it is the difference between the next offset and the current offset
    else
	mem_objects[index+1]->SetSize(mem_objects[index+2]->GetOffset()-mem_objects[index+1]->GetOffset());

   cerr<<"PNStacklayout: Insert Successful, Post Insert Offsets"<<endl;
    for(unsigned int i=0;i<mem_objects.size();i++)
    {
	cerr<<"\tOffset = "<<mem_objects[i]->GetOffset()<<endl;
    }
}

void PNStackLayout::InsertEBPOffset(int offset)
{
    //The size of the saved regs must be taken into consideration when transforming
    //the EBP offset to an esp relative offset

    cerr<<"PNStackLayout: InsertEBPOffset(): Offset="<<offset<<" frame alloc size="<<frame_alloc_size<<" saved regs size="<<saved_regs_size<<endl;
    InsertESPOffset(((int)frame_alloc_size+(int)saved_regs_size) - offset);
}

//Shuffle generates new displacement_offset values for each PNRange which represents
//a logical shuffling, it does not change the ordering of the mem_objects data structure.
void PNStackLayout::Shuffle()
{
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

    //srand(1);
    //srand(time(NULL));

    int random_index;
    int start_index = 0;
    
    //if there are out args, the first element of the passed in vector
    //is considered the out args and will not be shuffled
    if(has_out_args)
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

    cerr<<"PNStackLayout: Shuffle(): "<<ToString()<<endl;
   for(unsigned int i=0;i<mem_objects.size();i++)
    {
	cerr<<"\tOffset = "<<mem_objects[i]->GetOffset()<<" Size = "<<mem_objects[i]->GetSize()<<
	    " Padding = "<<mem_objects[i]->GetPaddingSize()<<" displ = "<<mem_objects[i]->GetDisplacement()<<endl;
    }
}

//TODO: if already padded, remove padding or add to it?, currently I am adding to it
void PNStackLayout::AddPadding()
{
    sort(mem_objects.begin(),mem_objects.end(),CompareRangeDisplacedOffset);
    //counts the additional padding added, does not take into consideration previous padding
    unsigned int total_padding = GetRandomPadding();
    
    //if there is no out args region, add padding below the memory object at esp
    // if(!has_out_args)
    //Only add padding from below the memory object at esp if there are no out args and
    //the layout does not reduce to p1. The reasonf or including the check for a reduction
    //to p1 was added because some times the out args is not found, causing all transforms
    //to fail. For p1, padding from below doesn't add much anyway, so removing this 
    //diversification allows for a transform even if the out args have been incorrectly 
    //identified. 
    if(!has_out_args && !P1Reduction())
	mem_objects[0]->SetDisplacement((int)total_padding+mem_objects[0]->GetDisplacement());
    else
    {
	mem_objects[0]->SetDisplacement(0);
	total_padding = 0;
    }
    unsigned int curpad = GetRandomPadding();
    total_padding += curpad;
    mem_objects[0]->SetPaddingSize(curpad+mem_objects[0]->GetPaddingSize());
    //total_padding += mem_objects[0]->GetPaddingSize();

    for(unsigned int i=1;i<mem_objects.size();i++)
    {
	mem_objects[i]->SetDisplacement(total_padding+mem_objects[i]->GetDisplacement());
	curpad = GetRandomPadding();
	total_padding += curpad;
	mem_objects[i]->SetPaddingSize(curpad+mem_objects[i]->GetPaddingSize());
       
	//total_padding += mem_objects[i]->GetPaddingSize();
    }

    
    
    sort(mem_objects.begin(),mem_objects.end(),CompareRangeBaseOffset);

    //the altered frame size is the size of the old altered frame size plus the additional
    //padding added.
    altered_alloc_size += total_padding;

    isPadded = true; 

    cerr<<"PNStackLayout: AddPadding(): "<<ToString()<<endl;
    for(unsigned int i=0;i<mem_objects.size();i++)
    {
	cerr<<"\tOffset = "<<mem_objects[i]->GetOffset()<<" Size = "<<mem_objects[i]->GetSize()<<
	    " Padding = "<<mem_objects[i]->GetPaddingSize()<<" displ = "<<mem_objects[i]->GetDisplacement()<<endl;
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
    return frame_alloc_size;
}

unsigned int PNStackLayout::GetAlteredAllocSize() const
{
    return altered_alloc_size;
}

unsigned int PNStackLayout::GetSavedRegsSize() const
{
    return saved_regs_size;
}


unsigned int PNStackLayout::GetClosestIndex(int loc) const
{
    int high = ((int)mem_objects.size())-1;
    int low = 0;
    int mid;

    //   cerr<<"PNStackLayout: GetClosestIndex(): Mem objects size = "<<mem_objects.size()<<endl;
    
    while(low <= high)
    {
	mid = (low+high)/2;
/*
	cerr<<"PNStackLayout: GetClosestIndex(): offset = "<<mem_objects[mid]->GetOffset()<<endl; 
	cerr<<"PNStackLayout: GetClosestIndex(): loc = "<<loc<<endl; 
	cerr<<"PNStackLayout: GetClosestIndex(): low = "<<low<<endl; 
	cerr<<"PNStackLayout: GetClosestIndex(): high = "<<high<<endl; 
	cerr<<"PNStackLayout: GetClosestIndex(): size =  "<<mem_objects[mid]->GetSize()<<endl;
	cerr<<"PNStackLayout: GetClosestIndex(): mid =  "<<mid<<endl;
	cerr<<"PNStackLayout: GetClosestIndex(): frame_alloc_size =  "<<frame_alloc_size<<endl;
*/
	if((loc < (mem_objects[mid]->GetOffset()+(int)mem_objects[mid]->GetSize()))  && (loc >= mem_objects[mid]->GetOffset()))
	    return mid;
	else if(loc > mem_objects[mid]->GetOffset())
	    low = mid +1;
	else
	    high = mid -1;
    }
    return -1;
}

PNRange* PNStackLayout::GetClosestRangeEBP(int loc) const
{
    //The size of the saved regs must be taken into consideration when transforming
    //the EBP offset to an esp relative offset
    return GetClosestRangeESP(((int)frame_alloc_size+(int)saved_regs_size) - loc);
}

//Finds the range whose base_offset is the closest to
//loc without going over.
//If no match can be found a negative offset may have been
//passed, the algorithm failed, or the number of mem_objects
//is 0. In this case a null pointer is returned;
PNRange* PNStackLayout::GetClosestRangeESP(int loc) const
{   
    cerr<<"PNstackLayout: GetClosestRangeESP(): Seaching for ESP Offset "<<loc<<endl;

    if(loc >= (int)frame_alloc_size)
    {
	//For now don't do anything if the loc is greater than the frame size
	cerr<<"PNStackLayout: GetClosestRangeESP(): loc >= frame_alloc_size, Returning NULL"<<endl;
	return NULL;
    }

    if(loc < 0)
    {
	cerr<<"PNStackLayout: GetClosestRangeESP(): loc < 0 ("<<loc<<"), Returning NULL"<<endl;
	return NULL;
    }

    int index = GetClosestIndex(loc);

    if(index < 0)
    {
	cerr<<"PNStackLayout: GetClosestRangeESP(): Could Not Find Range, Returning NULL"<<endl;
	//TODO: throw exception?
//	assert(false);
	return NULL;
    }

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
    
    ss<<"Layout = "<<pn_layout_name + "; Function = "<<
	function_name <<"; Frame Alloc Size = "<<frame_alloc_size<<" Altered Frame Size ="<<
	altered_alloc_size<<" Saved Regs Size = "<<saved_regs_size<<"; Out Args Size = "<< 
	out_args_size<<"; Number of Memory Objects = "<< mem_objects.size()<<"; Padded = ";
 
    if(isPadded)
	ss<<"true";
    else
	ss<<"false";

    ss<<"; Shuffled = ";

    if(isShuffled)
	ss<< "true";
    else
	ss<<"false";


    return ss.str();
}

string PNStackLayout::GetLayoutName() const
{
    return pn_layout_name;
}

string PNStackLayout::GetFunctionName() const
{
    return function_name;
}


//TODO: intended to be temporary
vector<PNRange*> PNStackLayout::GetMemoryObjects()
{
    return mem_objects;
}

bool PNStackLayout::CanShuffle() const
{
    return (mem_objects.size() > 2) || (mem_objects.size() == 2 && !has_out_args);
}

bool PNStackLayout::P1Reduction() const
{
    //return !((mem_objects.size() > 2) || (mem_objects.size() == 2 && !has_out_args));
    return mem_objects.size() == 1;
}

unsigned int PNStackLayout::GetOutArgsSize() const
{
    return out_args_size;
}
