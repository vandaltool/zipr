
#include <iostream>
#include <stdlib.h>
using namespace std;

int bar()
{
	if(getenv("THROW_CHAR")!=NULL)
		throw char(4);
	else if(getenv("THROW_INT")!=NULL)
		throw int(3);
	else if(getenv("THROW_FLOAT")!=NULL)
		throw float(3.14);
	else 
		return 0;
}

int foo()
{
	int ret=0;
	try 
	{
		ret=bar();
	}
	catch(char c)
	{
		cout<<"foo caught char:" << +c << endl;
		return c;
	}
	cout<<"No throw!"<<endl;
	return ret;

}
main()
{

	try	
	{
		return foo();
	}
	catch(int s)
	{
		cout<<"main caught int:" << s << endl;
	}
	catch(...)
	{
		cout<<"main caught unnamed"<<endl;
	}

	return 1;

}

