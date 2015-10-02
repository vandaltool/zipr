#include <iostream>
#include <string>

using namespace std;

extern int foo(int a);

int main(int argc, char* argv[])
{
	// mark argv used to avoid warnings
	(void)argv;


	cout<<"going to foo"<<endl;
	try 
	{
		foo(argc);
	}
	catch(string s)
	{
		cout<<"Caught foo exception:" << s <<endl;
	}
	cout<<"back from foo"<<endl;

	return 0;
}

