

#include <istream>
#include <fstream>
#include <exeio.h>
#include <exeio_elf.h>
#include <exeio_pe.h>


using namespace EXEIO;
using namespace std;
#ifndef SOLARIS
using namespace pe_bliss;
#endif

void exeio::load(char* filename)
{

	ifstream instream(filename); 

	if(!instream)
		assert(0 && "Cannot open file");

	int c0=instream.get();
	int c1=instream.get();
	int c2=instream.get();
	int c3=instream.get();

	// check for elf magic number
	if(c0 == '\177' && c1=='E' && c2=='L' && c3=='F')
	{
		backend=new exeio_elf_backend_t;
	}
	// check for CGC magic number
	else if(c0 == '\177' && c1=='C' && c2=='G' && c3=='C')
	{
		backend=new exeio_elf_backend_t;
	}
	else
#ifndef SOLARIS
		// we assume it's PE.
		backend=new exeio_pe_backend_t;
#else
		// don't build win support on solaris.
		assert(0);
#endif

	backend->load(this, filename);

}
