
#include <zipr_all.h>
#include <irdb-core>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <map>
#include <assert.h>
#include <sys/mman.h>
#include <ctype.h>
#include <iostream>   
#include <string>     
#include <fstream>
#include <elf.h>


using namespace IRDB_SDK;
using namespace std;
using namespace zipr;
using namespace EXEIO;


static inline uintptr_t page_round_down(uintptr_t x)
{
        return x & (~(PAGE_SIZE-1));
}
static inline uintptr_t page_round_up(uintptr_t x)
{
        return  ( (((uintptr_t)(x)) + PAGE_SIZE-1)  & (~(PAGE_SIZE-1)) );
}



void PeWriter::Write(const EXEIO::exeio *exeiop, const string &out_file, const string &infile)
{

	assert(0); // to do 

}

