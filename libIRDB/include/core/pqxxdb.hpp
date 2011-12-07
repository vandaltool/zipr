

class pqxxDB_t : public DBinterface_t
{
	public:
		pqxxDB_t();
		void IssueQuery(std::string query);
		void IssueQuery(std::stringstream & query);
		void MoveToNextRow();
		std::string GetResultColumn(std::string colname);
		bool IsDone();
		void Commit();

		pqxx::connection& GetConnection() { return conn; }
		pqxx::work& GetTransaction() { return txn; }

	private:
		pqxx::connection conn;
		pqxx::work txn;
		pqxx::result results;
		pqxx::result::const_iterator results_iter;

};
