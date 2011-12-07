
#include <all.hpp>
#include <pqxx/pqxx>
#include <string>

using namespace libIRDB;
using namespace std;

pqxxDB_t::pqxxDB_t() : DBinterface_t(), txn(conn)
{
	/* no other init needed */
}

void pqxxDB_t::IssueQuery(std::string query)
{
	results=txn.exec(query);
	results_iter=results.begin();
}

void pqxxDB_t::IssueQuery(std::stringstream & query)
{
	results=txn.exec(query);
	results_iter=results.begin();
}

void pqxxDB_t::MoveToNextRow()
{
	assert(!IsDone());
	++results_iter;
}

std::string pqxxDB_t::GetResultColumn(std::string colname)
{
	if(results_iter[colname].is_null())
		return std::string("");

	pqxx::binarystring bin_str(results_iter[colname]);

	return bin_str.str();

//	return results_iter[colname].as<std::string>();
}

bool pqxxDB_t::IsDone()
{
	return results_iter==results.end();
}

void pqxxDB_t::Commit()
{
	txn.commit();
}


