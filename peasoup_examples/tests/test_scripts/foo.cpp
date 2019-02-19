#include <iostream>
#include <string>

using namespace std;

int foo(int a)
{
	cout<<"In foo with a="<<a<<endl;

	if(a>1)
		throw string("arg>1");

	return a;
}
