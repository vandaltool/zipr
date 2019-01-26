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
#include <zipr_sdk.h>

using namespace zipr;
using namespace std;
using namespace Zipr_SDK;

#define INVOKE(a) \
bool __ ## a ## _result = false; \
printf("Invoking " #a ":\n"); \
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

class DollopMockup {
	private:
		int m_size;
	public:
		DollopMockup() : m_size(0) {};
		void Size(int size) { m_size = size; }
		int Size() const { return m_size; }
};

bool TestRangeSpeed() {
	ZiprMemorySpace_t m;
	DollopMockup *d = new DollopMockup();

	d->Size(50);

	for (int i = 0;
	     i<25;
			 i++) {
		m.AddFreeRange(Range_t(100*i, (100*(i+1))-1));
	}

	//m.PrintMemorySpace(cout);
	const auto timeStart=clock();	
	auto i=0;
	for (i = 0; (clock()-timeStart)/CLOCKS_PER_SEC < 10; i++)
	{
		volatile RangeAddress_t found_start = 0;
		auto placement = m.getFreeRange(d->Size());

		found_start = placement.getStart();
		found_start++;
	}
	cout<<"In 10 seconds, executed "<<dec<<i<<" iterations of getFreeRange()"<<endl;

	return true;
}

int main(int argc, char *argv[])
{
	INVOKE(TestRangeSpeed);
	return 0;
}
