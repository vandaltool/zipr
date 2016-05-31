#include "all.hpp"
#include <utils.hpp>
#include <sstream>
#include <iomanip>


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

        ostringstream hex_data;


        string q=string("insert into ")+fid->scoop_table_name +
                string(" (scoop_id, name, type_id, start_address_id, end_address_id, data, permissions, relro) ")+
                string(" VALUES (") +
                string("'") + to_string(GetBaseID()) + string("', ") +
                string("'") + GetName()  + string("', ") +
                string("'") + to_string(type_id) + string("', ") +
                string("'") + to_string(GetStart()->GetBaseID()) + string("', ") +
                string("'") + to_string(GetEnd()->GetBaseID()) + string("', ") +
                string("decode('");

        hex_data << setfill('0') << hex;
        for (size_t i = 0; i < contents.length(); ++i)
	{
                hex_data << setw(2) << (int)(contents[i]&0xff);
		q+=hex_data.str();
		hex_data.str("");	// reset to empty
		hex_data.clear();
	}
		

	q+=     string("', 'hex'), ") +
                string("'") + to_string(permissions) + string("', ") + 
                string("'") + to_string(is_relro) + string("'); ") ;

	return q;
}

