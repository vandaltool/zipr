#include "OffsetInference.hpp"
#include "PrecedenceBoundaryInference.hpp"
#include "General_Utility.hpp"
#include "beaengine/BeaEngine.h"
#include <cassert>
#include <iostream>
#include <cstdlib>
#include <set>
#include <fstream>
#include <algorithm>

using namespace std;
using namespace libIRDB;


static bool CompareRangeBaseOffset(Range a, Range b)
{
	return (a.GetOffset() < b.GetOffset());
}

//TODO: I am basically repeating this code from StackLayout.cpp for a slighly different purpose, I should abstract
//out a binary search utility.
//assuming the vector is sorted
static unsigned int GetClosestIndex(vector<Range> ranges, int loc)
{

	int high = ((int)ranges.size())-1;
	int low = 0;
	int mid;
	
	while(low <= high)
	{
		mid = (low+high)/2;
		if((loc < (ranges[mid].GetOffset()+(int)ranges[mid].GetSize()))	 && (loc >= ranges[mid].GetOffset()))
			return mid;
		else if(loc > ranges[mid].GetOffset())
			low = mid +1;
		else
			high = mid -1;
	}
	return -1;
}



PNStackLayout* PrecedenceBoundaryInference::GetPNStackLayout(libIRDB::Function_t *func)
{
	if(!base_inference || !pbgen)
		return NULL;

	vector<Range> ranges = pbgen->GetBoundaries(func);
	
	cerr<<"PrecedenceBoundaryInference: ranges found = "<<ranges.size()<<endl;
	if(ranges.size() == 0)
		return NULL;

	PNStackLayout *base_layout = base_inference->GetPNStackLayout(func);

	if(!base_layout || !base_layout->IsStaticStack())
		return NULL;

	StackLayout precedence_layout(GetInferenceName(),base_layout->GetFunctionName(),base_layout->GetOriginalAllocSize(),
								  base_layout->GetSavedRegsSize(), base_layout->HasFramePointer(), base_layout->GetOutArgsSize());

	//cerr<<"PrecedenceLayoutInference: inf name="<<GetInferenceName()<<" func name= "<<base_layout->GetFunctionName()<<" stack alloc size="<<base_layout->GetOriginalAllocSize()<<" saved regs size="<<base_layout->GetSavedRegsSize()<<" has frame pointer="<< base_layout->HasFramePointer()<<" out args size="<< base_layout->GetOutArgsSize()<<endl;

	
	StackLayout slayout = base_layout->GetStackLayout();
	vector<Range> pnranges= slayout.GetRanges();

	vector<Range> espranges;

	//TODO: for now I need to transform offsets to esp to keep everything consistent
	for(unsigned int i=0;i<ranges.size();i++)
	{
		Range cur;
		cur.SetOffset(ranges[i].GetOffset());
		cur.SetSize(ranges[i].GetSize());

		if(ranges[i].GetOffset() < 0)
		{
			cur.SetOffset(slayout.EBPToESP(-1*ranges[i].GetOffset()));
		}
		espranges.push_back(cur);
	}

	sort(espranges.begin(),espranges.end(),CompareRangeBaseOffset);


	//get transitive closure of offsets
	//TODO: should this be done in the generator? Requires knowing layout.
	vector<Range> closure_ranges;

	Range cur;
	cur.SetOffset(espranges[0].GetOffset());
	cur.SetSize(espranges[0].GetSize());
   
	for(unsigned int i=1;i<espranges.size();i++)
	{
		int next_offset = espranges[i].GetOffset();
		unsigned int next_size = espranges[i].GetSize();

		//if the next element's offset falls in the current range's boundary
		//and its size exceeds the current range's boundary, then expand the
		//current range to include this range.

		//TODO: casting cur.GetSize() to an int in two places below. 
		//make sure size isn't greater than int max, although I don't
		//think this will happen and I don't know what to do if I see
		//it occur. 
		if((next_offset >= cur.GetOffset()) && 
		   (next_offset < ((int)cur.GetSize()+cur.GetOffset())) &&
		   ((next_offset+next_size) > (cur.GetOffset()+cur.GetSize())))
		{
			cur.SetSize(next_size + (next_offset-cur.GetOffset()));
		}
		else if(next_offset > (cur.GetOffset() + (int)cur.GetSize()))
		{
			closure_ranges.push_back(cur);
			cur.SetOffset(next_offset);
			cur.SetSize(next_size);
		}
		//else the next bound is completely inside the cur, so ignore. 
	}

	closure_ranges.push_back(cur);

	//insert all the precedence boundaries
	for(unsigned int i=0;i<closure_ranges.size();i++)
	{
		precedence_layout.InsertESPOffset(closure_ranges[i].GetOffset());
		precedence_layout.InsertESPOffset(closure_ranges[i].GetOffset()+closure_ranges[i].GetSize());
	}

	for(unsigned int i=0;i<pnranges.size();i++)
	{
		//look for the offset and the offset+size for each pnrange
		//if not found, -1 is returned, and in this case we want to insert
		//the offset into the inference. Otherwise, the location exists inside
		//one of the precedence boundaries, and therefore we should not insert. 
		if(GetClosestIndex(closure_ranges,pnranges[i].GetOffset()) < 0)
			precedence_layout.InsertESPOffset(pnranges[i].GetOffset());

		if(GetClosestIndex(closure_ranges,pnranges[i].GetOffset()+pnranges[i].GetSize()) < 0)
			precedence_layout.InsertESPOffset(pnranges[i].GetOffset()+pnranges[i].GetSize());
	}

	//Since I am using annotations about calls to find offsets, there better
	//be an out arguments region, if not, consider the loweset object the out
	//args region and produce a new precedence_layout
	//TODO: in the future I may want the option in stack layout to set
	//the out args region after the fact. 
	if(precedence_layout.GetOutArgsRegionSize() == 0)
	{
		vector<Range> inserted_ranges = precedence_layout.GetRanges();

		sort(inserted_ranges.begin(),inserted_ranges.end(),CompareRangeBaseOffset);

		unsigned int args_size =  inserted_ranges[0].GetOffset()+inserted_ranges[0].GetSize();

		StackLayout revised_playout(GetInferenceName(),base_layout->GetFunctionName(),base_layout->GetOriginalAllocSize(),
									base_layout->GetSavedRegsSize(), base_layout->HasFramePointer(), args_size);

		for(unsigned int i=1;i<inserted_ranges.size();i++)
		{
			revised_playout.InsertESPOffset(inserted_ranges[i].GetOffset()+inserted_ranges[i].GetSize());
		}

//I don't know why I marked these as not canary safe, but since I validate
//everything now, I'm removing this restriction.
//		revised_playout.SetCanarySafe(false);

		return new PNStackLayout(revised_playout);
	}

//I don't know why I marked these as not canary safe, but since I validate
//everything now, I'm removing this restriction.
//	precedence_layout.SetCanarySafe(false);
	
	return new PNStackLayout(precedence_layout);
}

string PrecedenceBoundaryInference::GetInferenceName() const
{
	return string("Precedence Boundary Inference using "+(base_inference == NULL? "NULL" : base_inference->GetInferenceName()));
}
