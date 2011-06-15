
#include <libIRDB.hpp>
#include <pqxx/pqxx>
#include <string>

using namespace libIRDB;

pqxxDB_t::pqxxDB_t() : DBinterface_t(), conn(), txn(conn)
{
	/* no other init needed */
}

void pqxxDB_t::IssueQuery(std::string query)
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
	return results_iter[colname].as<std::string>();
}
bool pqxxDB_t::IsDone()
{
	return results_iter==results.end();
}

void pqxxDB_t::Commit()
{
	txn.commit();
}


