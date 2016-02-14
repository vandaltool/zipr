#include "all.hpp"
#include <utils.hpp>

using namespace std;
using namespace libIRDB;



string DataScoop_t::WriteToDB(File_t *fid, db_id_t newid)
{

/*
  scoop_id           SERIAL PRIMARY KEY,        -- key
  name               text DEFAULT '',           -- string representation of the type
  type_id            integer,                   -- the type of the data, as an index into the table table.
  start_address_id   integer,                   -- address id for start.
  end_address_id     integer,                   -- address id for end
  permissions        integer                    -- in umask format (bitmask for rwx)

 */

	db_id_t type_id=(GetType() ? GetType()->GetBaseID() : BaseObj_t::NOT_IN_DATABASE);

        string q=string("insert into ")+fid->scoop_table_name +
                string(" (scoop_id, name, type_id, start_address_id, end_address_id, permissions) ")+
                string(" VALUES (") +
                string("'") + to_string(GetBaseID()) + string("', ") +
                string("'") + GetName()  + string("', ") +
                string("'") + to_string(type_id) + string("', ") +
                string("'") + to_string(GetStart()->GetBaseID()) + string("', ") +
                string("'") + to_string(GetEnd()->GetBaseID()) + string("', ") +
                string("'") + to_string(permissions) + string("'); ") ;

	return q;
}

