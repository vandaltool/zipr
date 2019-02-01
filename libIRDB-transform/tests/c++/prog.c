#include <iostream>

using namespace std;

class A {
  public:
    A(char *p) { m_string = string(p); }
    A(string s) { m_string = s; }
	std::string getString() { return m_string; }
	~A() { m_string = ""; }

  private:
    std::string m_string;
};

class B : public A {
  public:
     B(string s) : A(s) { m_x = 1;} ;
	 void C() { throw std::exception(); }
  protected:
     int m_x;
};

main()
{
	A a("hello");
	std::string world("world");

	B b("world");

	cerr << a.getString() << endl;
	cout << b.getString() << endl;

	try {
		b.C();
	} catch (...)
	{
		cerr << "exception caught" << endl;
	}
}
