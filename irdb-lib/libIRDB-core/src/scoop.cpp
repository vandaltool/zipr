#include "all.hpp"
#include <irdb-util>
#include <sstream>
#include <iomanip>


using namespace std;
using namespace libIRDB;


#define SCOOP_THRESHOLD 990000000	 /* almost 1gb -- leaving a bit of head room for overhead sql syntax overheads */
#define SCOOP_CHUNK_SIZE (10*1024*1024)	 /* 10 mb  */

//#define SCOOP_THRESHOLD 10	 /* almost 1gb -- leaving a bit of head room for overhead sql syntax overheads */
//#define SCOOP_CHUNK_SIZE (128)

string DataScoop_t::WriteToDB(File_t *fid, db_id_t newid)
{
	string q= ""; 
	    
	if(contents.length() < SCOOP_THRESHOLD)
		q+=WriteToDBRange(fid,newid, 0,  contents.length(), fid->scoop_table_name);
	else
	{
		q+=WriteToDBRange(fid,newid, 0,  SCOOP_THRESHOLD, fid->scoop_table_name);
		q+=WriteToDBRange(fid,newid , SCOOP_THRESHOLD,  contents.length(), fid->scoop_table_name+"_part2");
	}

	return q;
}

string DataScoop_t::WriteToDBRange(File_t *fid, db_id_t newid, int start, int end, string table_name)
{

/*
  scoop_id           SERIAL PRIMARY KEY,        -- key
  name               text DEFAULT '',           -- string representation of the type
  type_id            integer,                   -- the type of the data, as an index into the table table.
  start_address_id   integer,                   -- address id for start.
  end_address_id     integer,                   -- address id for end
  permissions        integer                    -- in umask format (bitmask for rwx)

 */

	db_id_t type_id=(getType() ? getType()->getBaseID() : BaseObj_t::NOT_IN_DATABASE);

        ostringstream hex_data;


        string q=string("insert into ")+table_name+
                string(" (scoop_id, name, type_id, start_address_id, end_address_id, data, permissions, relro) ")+
                string(" VALUES (") +
                string("'") + to_string(getBaseID()) + string("', ") +
                string("'") + getName()  + string("', ") +
                string("'") + to_string(type_id) + string("', ") +
                string("'") + to_string(getStart()->getBaseID()) + string("', ") +
                string("'") + to_string(getEnd()->getBaseID()) + string("', ") +
                string("'") + /* empty data field -- for now  + */ string("', ") +
                string("'") + to_string(permissions) + string("', ") + 
                string("'") + to_string(is_relro) + string("'); ") ;
	// add the table row with empty data field.
	dbintr->issueQuery(q);


	// now try to append the data to the field in chunks.

	string query_start="update "+table_name+" set data = data || decode('";
	string query_end="', 'hex') where scoop_id="+ string("'") + to_string(getBaseID()) + string("'; ") ;


        hex_data << query_start << setfill('0') << hex;
        for (auto i = start; i < end; ++i)
	{
                hex_data << setw(2) << (int)(contents[i]&0xff);

		stringstream::pos_type offset = hex_data.tellp();

		if(offset > SCOOP_CHUNK_SIZE)
		{
			// q+=hex_data.str();
			// hex_data.str("");	// reset to empty
			// hex_data.clear();

			// tag the end,
			hex_data << query_end;

			// append this chunk to the db.
			dbintr->issueQuery(hex_data.str());

			// restart 
			hex_data.str("");	// reset to empty
			hex_data.clear();
        		hex_data << query_start << setfill('0') << hex;
		}
	}
	hex_data << query_end;

	// append this chunk to the db.
	dbintr->issueQuery(hex_data.str());

	return "";
}

