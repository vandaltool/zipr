#include <zipr_all.h>

using namespace zipr;
using namespace std;

#define INVOKE(a) \
bool __ ## a ## _result = false; \
__ ## a ## _result = a(); \
printf(#a ":"); \
if (__ ## a ## _result) \
{ \
printf(" pass\n"); \
} \
else \
{ \
printf(" fail\n"); \
}

bool TestSplitMemorySpace()
{
	Options_t opts;
	opts.SetVerbose(true);
	MemorySpace_t m(&opts);

	m.AddFreeRange(Range_t(500, 600));

	m.SplitFreeRange(550);

	assert(m.GetRangeCount() == 2);

	return true;
}

bool TestBinarySearchMaxRange()
{
	std::list<Range_t>::iterator foundRange;
	Options_t opts;
	opts.SetVerbose(true);
	MemorySpace_t m(&opts);
	m.AddFreeRange(Range_t(256, (RangeAddress_t)-1));

	m.SplitFreeRange(336);
	m.SplitFreeRange(345);
	m.SplitFreeRange(355);
	m.SplitFreeRange(365);
	m.SplitFreeRange(375);
	m.SplitFreeRange(385);

	m.Sort();

	cout << "Looking for 0x" << std::hex << (RangeAddress_t)-1 << ":" << endl;
	foundRange = m.FindFreeRange(RangeAddress_t((RangeAddress_t)-1));
	assert(m.IsValidRange(foundRange));
	cout << "Found: 0x" << std::hex << (*foundRange).GetStart() << " - 0x" << (*foundRange).GetEnd() << endl;

	return true;
}


bool TestBinarySearch()
{
	std::list<Range_t>::iterator foundRange;
	Options_t opts;
	opts.SetVerbose(true);
	MemorySpace_t m(&opts);
	m.AddFreeRange(Range_t(256, 512));

	m.SplitFreeRange(300);
	m.SplitFreeRange(315);
	m.SplitFreeRange(336);
	m.SplitFreeRange(337);
	m.SplitFreeRange(400);

	m.PrintMemorySpace(cout);
	m.Sort();
	m.PrintMemorySpace(cout);
	
	cout << "Looking for 0x" << std::hex << 258 << ":" << endl;
	foundRange = m.FindFreeRange(RangeAddress_t(258));
	assert(m.IsValidRange(foundRange));
	cout << "Found: 0x" << std::hex << (*foundRange).GetStart() << " - 0x" << (*foundRange).GetEnd() << endl;

	cout << "Looking for 0x" << std::hex << 335 << ":" << endl;
	foundRange = m.FindFreeRange(RangeAddress_t(335));
	assert(m.IsValidRange(foundRange));
	cout << "Found: 0x" << std::hex << (*foundRange).GetStart() << " - 0x" << (*foundRange).GetEnd() << endl;

	cout << "Looking for 0x" << std::hex << 301 << ":" << endl;
	foundRange = m.FindFreeRange(RangeAddress_t(301));
	assert(m.IsValidRange(foundRange));
	cout << "Found: 0x" << std::hex << (*foundRange).GetStart() << " - 0x" << (*foundRange).GetEnd() << endl;
	
	cout << "Looking for 0x" << std::hex << 316 << ":" << endl;
	foundRange = m.FindFreeRange(RangeAddress_t(316));
	assert(m.IsValidRange(foundRange));
	cout << "Found: 0x" << std::hex << (*foundRange).GetStart() << " - 0x" << (*foundRange).GetEnd() << endl;
	
	cout << "Looking for 0x" << std::hex << 338 << ":" << endl;
	foundRange = m.FindFreeRange(RangeAddress_t(338));
	assert(m.IsValidRange(foundRange));
	cout << "Found: 0x" << std::hex << (*foundRange).GetStart() << " - 0x" << (*foundRange).GetEnd() << endl;
	
	cout << "Looking for 0x" << std::hex << 401 << ":" << endl;
	foundRange = m.FindFreeRange(RangeAddress_t(401));
	assert(m.IsValidRange(foundRange));
	cout << "Found: 0x" << std::hex << (*foundRange).GetStart() << " - 0x" << (*foundRange).GetEnd() << endl;
	
	cout << "Looking for 0x" << std::hex << 450 << ":" << endl;
	foundRange = m.FindFreeRange(RangeAddress_t(450));
	assert(m.IsValidRange(foundRange));
	cout << "Found: 0x" << std::hex << (*foundRange).GetStart() << " - 0x" << (*foundRange).GetEnd() << endl;
	
	cout << "Looking for 0x" << std::hex << 512 << ":" << endl;
	foundRange = m.FindFreeRange(RangeAddress_t(512));
	assert(m.IsValidRange(foundRange));
	cout << "Found: 0x" << std::hex << (*foundRange).GetStart() << " - 0x" << (*foundRange).GetEnd() << endl;
	
	cout << "Looking for 0x" << std::hex << 400 << ": (but won't find)" << endl;
	foundRange = m.FindFreeRange(RangeAddress_t(400));
	assert(!m.IsValidRange(foundRange));

	cout << "Looking for 0x" << std::hex << 300 << ": (but won't find)" << endl;
	foundRange = m.FindFreeRange(RangeAddress_t(300));
	assert(!m.IsValidRange(foundRange));
	return true;
}
bool TestSort()
{
	Options_t opts;
	opts.SetVerbose(true);
	MemorySpace_t m(&opts);
	m.AddFreeRange(Range_t(256, 512));

	m.SplitFreeRange(300);
	m.SplitFreeRange(315);
	m.SplitFreeRange(336);
	m.SplitFreeRange(337);
	m.SplitFreeRange(400);

	m.MergeFreeRange(300);
	m.MergeFreeRange(337);
	m.MergeFreeRange(315);

	m.PrintMemorySpace(cout);
	m.Sort();
	m.PrintMemorySpace(cout);

	return true;
}
bool TestMergeFreeRange()
{
	Options_t opts;
	opts.SetVerbose(true);
	MemorySpace_t m(&opts);
	m.AddFreeRange(Range_t(256, 512));

	m.SplitFreeRange(300);
	
	m.SplitFreeRange(315);

	m.SplitFreeRange(336);

	m.SplitFreeRange(337);
	
	m.SplitFreeRange(400);

	cout << "Post Splits at:";
	cout << "0x" << std::hex << 300 << " ";
	cout << "0x" << std::hex << 315 << " ";
	cout << "0x" << std::hex << 336 << " ";
	cout << "0x" << std::hex << 337 << " ";
	cout << "0x" << std::hex << 400 << endl;
	m.PrintMemorySpace(cout);

	m.MergeFreeRange(300);
	
	cout << "Post 0x" << std::hex << 300 << " Merge" << endl;
	m.PrintMemorySpace(cout);
	
	m.MergeFreeRange(315);

	cout << "Post 0x" << std::hex << 315 << " Merge" << endl;
	m.PrintMemorySpace(cout);

	m.MergeFreeRange(337);

	cout << "Post 0x" << std::hex << 337 << " Merge" << endl;
	m.PrintMemorySpace(cout);

	assert(m.GetRangeCount() == 3);
	assert(m.IsByteFree(300));
	assert(m.IsByteFree(315));
	assert(m.IsByteFree(317));
	assert(!m.IsByteFree(336));
	assert(!m.IsByteFree(400));
	return true;
}

int main(int argc, char *argv[])
{
	INVOKE(TestSplitMemorySpace);
	INVOKE(TestMergeFreeRange);
	INVOKE(TestSort);
	INVOKE(TestBinarySearch);
	INVOKE(TestBinarySearchMaxRange);
}
