

#include <istream>
#include <fstream>
#include <exeio.h>
#include <exeio_elf.h>
#include <exeio_pe.h>


using namespace EXEIO;
using namespace pe_bliss;
using namespace std;

void exeio::load(char* filename)
{

	ifstream instream(filename); 

	if(!instream)
		assert(0 && "Cannot open file");


	// check for elf magic number
	if( instream.get()=='E' && instream.get()=='L' && instream.get()=='F' )
	{
		backend=new exeio_elf_backend_t();
	}
	// we assume it's ELF or PE.
	else
		backend=new exeio_pe_backend_t();

	backend->load(this, filename);

}
