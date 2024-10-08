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

namespace libIRDB
{
using DatabaseErrorType_t = IRDB_SDK::DatabaseErrorType_t;

class DatabaseError_t : virtual public IRDB_SDK::DatabaseError_t
{
	public: 
		virtual ~DatabaseError_t() { }
		DatabaseError_t(DatabaseErrorType_t the_err) : err(the_err) {}
		DatabaseErrorType_t GetErrorCode() const { return err; }
	private:
		DatabaseError_t::DatabaseErrorType_t err;
};

std::ostream& operator<<(std::ostream& output, const DatabaseError_t& p);


// an interface to a database
class DBinterface_t : virtual public IRDB_SDK::DBinterface_t
{
	public:
		DBinterface_t() {};
		virtual ~DBinterface_t() {};
                virtual void issueQuery(std::string query)=0;
                virtual void moveToNextRow()=0;
                virtual std::string getResultColumn(std::string colname)=0;
                virtual bool isDone()=0;
                virtual void commit()=0;

};

}
