/*
 * Copyright (c) 2016 - Zephyr Software LLC
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

class DataScoopByAddressComp_t;

class DataScoop_t : public BaseObj_t, virtual public IRDB_SDK::DataScoop_t
{

	public:
		DataScoop_t()
			:
				BaseObj_t(NULL),
				name(),
				start(NULL),
				end(NULL),
				type(NULL),
				permissions(0),
				is_relro(false),
				contents()
		{}
	
		DataScoop_t( 	libIRDB::db_id_t id,
				std::string p_name,
				libIRDB::AddressID_t* p_start,
				libIRDB::AddressID_t* p_end,
				libIRDB::Type_t* p_type,
				int p_permissions, 
				bool p_is_relro, 
				const std::string &p_contents)
			:
				BaseObj_t(NULL),
				name(p_name),
				start(p_start),
				end(p_end),
				type(p_type),
				permissions(p_permissions),
				is_relro(p_is_relro),
				contents(p_contents)
		{
			assert(start && end);
			setBaseID(id);
		}

		virtual ~DataScoop_t() { /* management of addresses and types is explicit */ } 

		const std::string getName() const { return name; }
		const std::string& getContents() const { return contents; }
		std::string &getContents() { return contents; }
		IRDB_SDK::AddressID_t* getStart() const { return start; }
		IRDB_SDK::AddressID_t* getEnd() const { return end; }
		IRDB_SDK::Type_t* getType() const { return type; }
		IRDB_SDK::VirtualOffset_t getSize() const { assert(start && end); return end->getVirtualOffset() - start->getVirtualOffset() + 1 ; }
		bool isReadable() const  { return (permissions & permissions_r) == permissions_r; }
		bool isWriteable() const { return (permissions & permissions_w) == permissions_w; };
		bool isExecuteable() const { return (permissions & permissions_x) == permissions_x; };
		bool isRelRo() const { return is_relro; };
		uint8_t  getRawPerms() const { return permissions; }
		void  setRawPerms(int newperms) { permissions=newperms; }

		void setName(const std::string &n) { name=n; }
		void setContents(const std::string &n) { contents=n; }
		void setStart( IRDB_SDK::AddressID_t* addr) { assert(addr); start=dynamic_cast<AddressID_t*>(addr); assert(start); }
		void setEnd  ( IRDB_SDK::AddressID_t* addr) { assert(addr); end  =dynamic_cast<AddressID_t*>(addr); assert(end  ); }
		void setType( IRDB_SDK::Type_t*  t) { type=dynamic_cast<Type_t*>(t); }

		void setReadable() { permissions |= permissions_r; }
		void setWriteable() { permissions |= permissions_w; }
		void setExecuteable() { permissions |= permissions_x; }
		void setRelRo() { is_relro = true; }

		void clearReadable() { permissions &= ~permissions_r; }
		void clearWriteable() { permissions &= ~permissions_w; }
		void clearExecuteable() { permissions &= ~permissions_x; }
		void clearRelRo() { is_relro=false; }

                std::string WriteToDB(File_t *fid, db_id_t newid);
		std::string WriteToDBRange(File_t *fid, db_id_t newid, int start, int end, std::string table_name);

	friend DataScoopByAddressComp_t;

	private:
		const static int permissions_r=4;
		const static int permissions_w=2;
		const static int permissions_x=1;

		std::string name;
		AddressID_t* start;
		AddressID_t* end;
		Type_t* type;
		int permissions;
		bool is_relro;
		std::string contents;


};

class DataScoopByAddressComp_t {
	public:
	bool operator()(const DataScoop_t *lhs, const DataScoop_t *rhs) {

		if (!lhs ||
		    !rhs ||
		    !lhs->start ||
		    !rhs->start
		   )
			throw std::logic_error("Cannot order scoops that have null elements.");

		return std::make_tuple(lhs->start->getVirtualOffset(), lhs) <
		       std::make_tuple(rhs->start->getVirtualOffset(), rhs);
	}
};

using DataScoopSet_t          = IRDB_SDK::DataScoopSet_t;
using DataScoopByAddressSet_t = std::set<DataScoop_t*, DataScoopByAddressComp_t>;

}
