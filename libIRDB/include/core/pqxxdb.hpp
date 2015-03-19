/*
 * Copyright (c) 2014 - Zephyr Software LLC
 *
 * This file may be used and modified for non-commercial purposes as long as
 * all copyright, permission, and nonwarranty notices are preserved.
 * Redistribution is prohibited without prior written consent from Zephyr
 * Software.
 *
 * Please contact the authors for restrictions applying to commercial use.
 *
 * THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: Zephyr Software
 * e-mail: jwd@zephyr-software.com
 * URL   : http://www.zephyr-software.com/
 *
 */



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
