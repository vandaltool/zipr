

#include <exeio.h>
#include <exeio_elf.h>


using namespace EXEIO;
using namespace std;

void exeio::load(char* filename)
{
	backend=new exeio_elf_backend_t();
	backend->load(this, filename);

}
