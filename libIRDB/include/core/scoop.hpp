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

#ifndef libirdb_data_scoop_hpp
#define libirdb_data_scoop_hpp

class DataScoop_t : public BaseObj_t
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
				std::string p_contents)
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
			SetBaseID(id);
		}

		virtual ~DataScoop_t() { /* management of addresses and types is explicit */ } 

		const std::string GetName() const { return name; }
		const std::string& GetContents() const { return contents; }
		std::string &GetContents() { return contents; }
		libIRDB::AddressID_t* GetStart() const { return start; }
		libIRDB::AddressID_t* GetEnd() const { return end; }
		libIRDB::Type_t* GetType() const { return type; }
		libIRDB::virtual_offset_t GetSize() { assert(start && end); return end->GetVirtualOffset() - start->GetVirtualOffset() + 1 ; }
		bool isReadable() const  { return (permissions & permissions_r) == permissions_r; }
		bool isWriteable() const { return (permissions & permissions_w) == permissions_w; };
		bool isExecuteable() const { return (permissions & permissions_x) == permissions_x; };
		bool isRelRo() const { return is_relro; };
		int  getRawPerms() const { return permissions; }
		void  setRawPerms(int newperms) { permissions=newperms; }

		void SetName(const std::string &n) { name=n; }
		void SetContents(const std::string &n) { contents=n; }
		void SetStart( libIRDB::AddressID_t* addr) { assert(addr); start=addr; }
		void SetEnd( libIRDB::AddressID_t* addr ) { assert(addr); end=addr; }
		void SetType( libIRDB::Type_t*  t) { type=t; }

		void SetReadable() { permissions |= permissions_r; }
		void SetWriteable() { permissions |= permissions_w; }
		void SetExecuteable() { permissions |= permissions_x; }
		void SetRelRo() { is_relro = true; }

		void ClearReadable() { permissions &= ~permissions_r; }
		void ClearWriteable() { permissions &= ~permissions_w; }
		void ClearExecuteable() { permissions &= ~permissions_x; }
		void ClearRelRo() { is_relro=false; }

                std::string WriteToDB(File_t *fid, db_id_t newid);
		std::string WriteToDBRange(File_t *fid, db_id_t newid, int start, int end, std::string table_name);


	private:
		const static int permissions_r=4;
		const static int permissions_w=2;
		const static int permissions_x=1;

		std::string name;
		libIRDB::AddressID_t* start;
		libIRDB::AddressID_t* end;
		libIRDB::Type_t* type;
		int permissions;
		bool is_relro;
		std::string contents;


};

typedef std::set<DataScoop_t*> DataScoopSet_t;

#endif
