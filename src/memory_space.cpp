/*
 * Copyright (c) 2014 - Zephyr Software LLC
 *
 * This file may be used and modified for non-commercial purposes as long as
 * all copyright, permission, and nonwarranty notices are preserved.
 * Redistribution is prohibited without prior written consent from Zephyr
 * Software.
 *
 * Please contact the authors for restrictions applying to commercial use.
 *
 * THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: Zephyr Software
 * e-mail: jwd@zephyr-software.com
 * URL   : http://www.zephyr-software.com/
 *
 */

#include <zipr_all.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>

using namespace zipr;
using namespace std;

void ZiprMemorySpace_t::SplitFreeRange(RangeAddress_t addr)
{
	RangeSet_t::iterator it=FindFreeRange(addr);
	assert(IsValidRange(it));

	Range_t r=*it;
	if(r.GetStart()==r.GetEnd())
	{
		assert(addr==r.GetEnd());
		free_ranges.erase(it);
	}
	else if(addr==r.GetStart())
	{
		free_ranges.erase(it);
		free_ranges.insert(Range_t(r.GetStart()+1, r.GetEnd()));
	}
	else if(addr==r.GetEnd())
	{
		free_ranges.erase(it);
		free_ranges.insert(Range_t(r.GetStart(), r.GetEnd()-1));
	}
	else // split range 
	{
		free_ranges.erase(it);
		free_ranges.insert(Range_t(r.GetStart(), addr-1));
		free_ranges.insert(Range_t(addr+1, r.GetEnd()));
	}
}

void ZiprMemorySpace_t::MergeFreeRange(RangeAddress_t addr)
{
	/*
	 * Make a new range of one byte.
	 * 
	 * Then, look to see whether or not it
	 * can be merged with another range. 
	 *
	 * If not, add it as a one byte range.
	 */

	Range_t nr(addr, addr);
	bool merged = false;
	RangeSet_t::iterator it=free_ranges.begin();

	for(;it!=free_ranges.end();++it)
	{
		Range_t r=*it;
		if ((addr+1) == r.GetStart()) {
			/*
			 * Make the beginning of this range
			 * one byte smaller!
			 */
			Range_t nnr(addr, r.GetEnd());
			if(m_opts != NULL && m_opts->GetVerbose())
			{
				printf("Expanded range:\n");
				printf("from: %p to %p\n", (void*)r.GetStart(), (void*)r.GetEnd());
				printf("to: %p to %p\n", (void*)nnr.GetStart(), (void*)nnr.GetEnd());
			}
			nr = nnr;
			free_ranges.erase(it);
			break;
		} else if ((addr-1) == r.GetEnd()) {
			/*
			 * Make the end of this range one byte
			 * bigger
			 */
			Range_t nnr(r.GetStart(), addr);
			if(m_opts != NULL && m_opts->GetVerbose())
			{
				printf("Expanded range:\n");
				printf("from: %p to %p\n", (void*)r.GetStart(), (void*)r.GetEnd());
				printf("to: %p to %p\n", (void*)nnr.GetStart(), (void*)nnr.GetEnd());
			}
			nr = nnr;
			free_ranges.erase(it);
			break;
		}
	}

	/*
	 * Correctness: 
	 * Take a pass through and see if there are
	 * free ranges that can now be merged. This
	 * is important because it's possible that
	 * we added the byte to the end of a range
	 * where it could also have gone at the
	 * beginning of another.
	 */
	for(it=free_ranges.begin();it!=free_ranges.end();++it)
	{
		Range_t r = *it;
		if ((
				/*
				 * <--r-->
				 *    <--nr-->
				 */
				((r.GetEnd()+1) >= nr.GetStart() &&
				r.GetEnd() <= nr.GetEnd()) ||
				/*
				 * <--nr-->
				 *     <--r-->
				 */
				((nr.GetEnd()+1) >= r.GetStart() &&
				nr.GetEnd() <= r.GetEnd())
				) &&
				/*
				 * The ranges themselves are not
				 * identical.
				 */
				(r.GetStart() != nr.GetStart() ||
				r.GetEnd() != nr.GetEnd()))
		{
			/*
			 * merge.
			 */
			Range_t merged_range(std::min(r.GetStart(), nr.GetStart()), std::max(r.GetEnd(), nr.GetEnd()));
			if(m_opts != NULL && m_opts->GetVerbose())
			{
				printf("Merged two ranges:\n");
				printf("r:  %p to %p\n", (void*)r.GetStart(), (void*)r.GetEnd());
				printf("nr: %p to %p\n", (void*)nr.GetStart(), (void*)nr.GetEnd());
				printf("to: %p to %p\n", (void*)merged_range.GetStart(), (void*)merged_range.GetEnd());
			}
			free_ranges.erase(it);
			free_ranges.insert(merged_range);
			merged = true;
			break;
		}
	}

	if (!merged)
	{
		free_ranges.insert(nr);
	}
}

void ZiprMemorySpace_t::PrintMemorySpace(std::ostream &out)
{
	for( RangeSet_t::iterator it=free_ranges.begin();
		it!=free_ranges.end();
		++it)
	{
		Range_t r = *it;
		out <<"0x"<<std::hex<<r.GetStart()<<" - 0x"<<std::hex<<r.GetEnd()<<endl;
	}
}

RangeSet_t::iterator ZiprMemorySpace_t::FindFreeRange(RangeAddress_t addr)
{
	RangeSet_t::iterator freer = free_ranges.find(Range_t(addr, addr)); 
	return freer;
}

bool ZiprMemorySpace_t::IsValidRange(RangeSet_t::iterator it)
{
	return it!=free_ranges.end();
}

Range_t ZiprMemorySpace_t::GetFreeRange(int size)
{
	for( RangeSet_t::iterator it=free_ranges.begin();
		it!=free_ranges.end();
		++it)
	{
		Range_t r=*it;
		if(r.GetEnd() - r.GetStart() > size)
			return r;
	}
	assert(0);// assume we find a big enough range.
}

// queries about free areas.
bool ZiprMemorySpace_t::AreBytesFree(RangeAddress_t addr, int num_bytes)
{
	for(int i=0;i<num_bytes;i++)
		if(!IsByteFree(addr+i))
			return false;
	return true;
}

bool ZiprMemorySpace_t::IsByteFree(RangeAddress_t addr)
{
	if (IsValidRange(FindFreeRange(addr)))
		return true;
	return false;
}

void ZiprMemorySpace_t::AddFreeRange(Range_t newRange)
{
	free_ranges.insert(Range_t(newRange.GetStart(), newRange.GetEnd()));
}
int ZiprMemorySpace_t::GetRangeCount()
{
	return free_ranges.size();
}
