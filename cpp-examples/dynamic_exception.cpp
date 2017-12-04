#include <iostream>
#include <exception>
#include <cstdlib>

using namespace std;
 
class X {};
class Y {};
class Z : public X {};
class W {};
 
void f() throw(X, Y) 
{
    	const auto env=getenv("THROW_TYPE");
	if(env==NULL)
		return;
    	const int n = atoi(env);
    	if (n==0) throw X(); // OK
    	if (n==1) throw Z(); // also OK
    	if (n==2) throw Y(); // also OK
    	throw W(); // will call std::unexpected()
}
 
int main() {
	std::set_unexpected([]{
		std::cout << "That was unexpected" << std::endl; // flush needed
		std::abort();
	});

	try{
  		f();
		cout<<"No catch"<<endl;
	}
	catch(X x)
	{
		cout<<"Caught X (or maybe Z)"<<endl;
	}
	catch(Y Y)
	{
		cout<<"Caught Y"<<endl;
	}
}
