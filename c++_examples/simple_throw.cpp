
#include <iostream>
#include <stdlib.h>
using namespace std;

int bar()
{
	if(getenv("THROW_INT")!=NULL)
		throw int(3);
	else if(getenv("THROW_float")!=NULL)
		throw float(3.14);
	else 
		return 0;
}

int foo()
{
	return bar();

}
main()
{

	try	
	{
		return foo();
	}
	catch(int s)
	{
		cout<<"caught int:" << s << endl;
	}
	catch(...)
	{
		cout<<"caught unnamed"<<endl;
	}

	return 1;

}

