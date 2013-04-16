
#include <all.hpp>
#include <utils.hpp>
#include <stdlib.h>
using namespace libIRDB;
using namespace std;


string AddressID_t::WriteToDB(File_t *fid, db_id_t newid, bool p_withHeader)
{
        assert(fid);
	
	if(GetBaseID()==NOT_IN_DATABASE)
		SetBaseID(newid);

	string q;
	if (p_withHeader)
        q=string("insert into ")+fid->address_table_name + 
			string("(address_id , file_id , vaddress_offset , doip_id)") +
			string(" values ");
	else
		q = ",";
			
	q +=
		string("(") + 
		string("'") + to_string(GetBaseID()) + string("', ") + 
		string("'") + to_string(fileID) + string("', ") + 
		string("'") + to_string(virtual_offset) + string("', ") + 
		string("'") + to_string(GetDoipID()) + string("' ") + 
		string(")");

	return q;
}

