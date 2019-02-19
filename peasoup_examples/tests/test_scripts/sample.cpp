#include <stdio.h>
#include <string.h>
#include <stdlib.h>

class base
{
public:
  base() {}
  virtual ~base() {}
  virtual void A() {}
  virtual void B() {}
  virtual void C() {}
};

class derived : public base
{
public:
  derived() {}
  virtual ~derived() {}
  virtual void A();
  virtual void B();
  virtual void C();
};

void derived::A()
{
  puts("A");
}

void derived::B()
{
  puts("B");
}

void derived::C()
{
  puts("C");
}

int IsPasswordOkay(void)
{
	char Password[12];

	gets(Password);
	if (!strcmp(Password, "goodpass"))
		return(1);
	else return(0);
}

int main(int argc, char **argv) 
{
    int PwStatus;

derived d;
d.A();
d.B();
d.C();


	puts("Enter password:");
	PwStatus = IsPasswordOkay();
	if (! PwStatus){
		puts("Access denied");
		exit(-1);
	}
	else puts("Access granted");
   exit(0);
}
