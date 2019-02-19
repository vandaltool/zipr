
#include <libStructDiv.h>

#include <iostream>
#include <stdlib.h>
#include <libgen.h>


using namespace std;
using namespace libStructDiv;



template<class t> void print_res(int round, StructuredDiversity_t* p, const vector<t> &res)
{
	for(int i=0;i<res.size();i++)
		cout<<"round="<<round<<" myid="<<p->GetVariantID()<<":res["<<i<<"]="<<res[i]<<endl;
}

int main(int argc, char* argv[])
{
	if(argc!=2)
	{
		cerr<<"Usage: "<<argv[0]<<" varId:totVars:url"<<endl;
		exit(1);
	}

	StructuredDiversity_t* p=StructuredDiversity_t::factory(basename(argv[0]), argv[1]);

	int sign=1;
	if(p->GetVariantID()==1)
		sign=-sign;

	for(int i=1;i<10;i++)
	{

		if( i&1 ) 
		{
			const vector<double> &res=p->Barrier<double>((3.14+i)*sign);
			print_res(i,p,res);
		}
		else
		{
			const vector<int> &res=p->Barrier<int>(i *sign );	
			print_res(i,p,res);
		}

	}


	delete p;

	return 0;
}
