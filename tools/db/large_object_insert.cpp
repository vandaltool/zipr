

#include <string>
#include <stdlib.h>
#include <set>
#include <assert.h>
#include <string>
#include <iostream>
#include <pqxx/pqxx>


int main(int argc, char *argv[])
{
	if(argc!=2)
	{
		std::cerr<<"Usage: "<<argv[0]<<" <filename>"<<std::endl;
		exit(-1);
	}

	pqxx::connection conn;
	pqxx::work txn(conn);

	pqxx::largeobject file(txn,std::string(argv[1]));

	pqxx::oid myoid=file.id();

	txn.commit();

	return myoid;
}
