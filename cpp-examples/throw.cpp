
#include <iostream>
#include <string>
using namespace std;

void bar()
{
	throw string("Print this string");
}

void foo()
{
	string s=" shoult not print ";
	bar();
	cout<<s<<endl;

}
main()
{

	try	
	{
		foo();
	}
	catch(string s)
	{
		cout<<"Threw string s:" << s << endl;
	}

}

