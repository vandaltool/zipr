#include <iostream>
#include <exeio.h>

using namespace std;
using namespace EXEIO;


int main(int argc, char* argv[])
{
	if(argc!=2)
	{
		cout<<"Usage: "<<argv[0]<<": <exe>"<<endl;
	}

	exeio *exep=new exeio;

	exep->load(argv[1]);

	EXEIO::dump::header(cout,*exep);
	EXEIO::dump::section_headers(cout,*exep);

	return 0;
}
