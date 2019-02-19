
#include <iostream>
#include <stdlib.h>
using namespace std;

class Base
{
	public:
		Base() : a(3) { } 

	int a;
};

class Derived : public Base
{
	public:
	Derived() { a=4;} 
};

int bar()
{
	if(getenv("THROW_CHAR")!=NULL)
		throw Base();
	else if(getenv("THROW_INT")!=NULL)
		throw Derived();
	else if(getenv("THROW_FLOAT")!=NULL)
		throw float(3.14);
	else 
		return 0;
}

main()
{

	try	
	{
		int res= bar();
		cout<<"No Throw!"<<endl;
		return res;
	}
	catch(Derived s)
	{
		cout<<"main caught Derived with val=" << s.a << endl;
	}
	catch(Base s)
	{
		cout<<"main caught Base with val=" << s.a << endl;
	}
	catch(...)
	{
		cout<<"main caught unnamed"<<endl;
	}

	return 1;

}

