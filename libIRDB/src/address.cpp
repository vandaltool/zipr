
#include <libIRDB.hpp>
#include <utils.hpp>
#include <stdlib.h>
using namespace libIRDB;
using namespace std;


string AddressID_t::WriteToDB(VariantID_t *vid, db_id_t newid)
{
        assert(vid);
	
	if(GetBaseID()==NOT_IN_DATABASE)
		SetBaseID(newid);

        string q=string("insert into ")+vid->address_table_name +
		string("(address_id , file_id , vaddress_offset , doip_id)") +
		string(" values ") +
		string("(") + 
		string("'") + to_string(GetBaseID()) + string("', ") + 
		string("'") + to_string(fileID) + string("', ") + 
		string("'") + to_string(virtual_offset) + string("', ") + 
		string("'") + to_string(GetDoipID()) + string("' ") + 
		string(");");

	return q;
}

