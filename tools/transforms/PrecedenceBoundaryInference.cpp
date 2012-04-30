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
	if((loc < (ranges[mid].GetOffset()+(int)ranges[mid].GetSize()))  && (loc >= ranges[mid].GetOffset()))
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
    for(int i=0;i<ranges.size();i++)
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

    //insert all the precedence boundaries
    for(int i=0;i<espranges.size();i++)
    {
	precedence_layout.InsertESPOffset(espranges[i].GetOffset());
	precedence_layout.InsertESPOffset(espranges[i].GetOffset()+espranges[i].GetSize());
    }

    for(int i=0;i<pnranges.size();i++)
    {
	//look for the offset and the offset+size for each pnrange
	//if not found, -1 is returned, and in this case we want to insert
	//the offset into the inference. Otherwise, the location exists inside
	//one of the precedence boundaries, and therefore we should not insert. 
	if(GetClosestIndex(espranges,pnranges[i].GetOffset()) < 0)
	    precedence_layout.InsertESPOffset(pnranges[i].GetOffset());

	if(GetClosestIndex(espranges,pnranges[i].GetOffset()+pnranges[i].GetSize()) < 0)
	    precedence_layout.InsertESPOffset(pnranges[i].GetOffset()+pnranges[i].GetSize());
    }

    return new PNStackLayout(precedence_layout);
}

string PrecedenceBoundaryInference::GetInferenceName() const
{
    return string("Precedence Boundary Inference using "+(base_inference == NULL? "NULL" : base_inference->GetInferenceName()));
}
