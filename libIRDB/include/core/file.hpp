/*
 * Copyright (c) 2014-2015 - Zephyr Software LLC
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

// An ELF file as represented by the DB
class File_t : public BaseObj_t
{
    public:
        // create new item.
        File_t( const db_id_t &file_id, const db_id_t &orig_fid, const std::string &url, 
		const std::string &hash, const std::string &arch, const int &elfoid, 
		const std::string &atn, const std::string &ftn, const std::string &itn, 
		const std::string &icfs, const std::string &icfs_map, 
		const std::string &rtn, const std::string &typ, const std::string &scoop, 
		const std::string &ehpgms, const std::string &ehcss, 
		const db_id_t &doipid);

        File_t(db_id_t file_id) : BaseObj_t(NULL) { (void)file_id; assert(0);}          // read from DB       
        void WriteToDB() { assert(0); }   // writes to DB ID is not -1.

        std::string GetAddressTableName() const { return address_table_name; }
        std::string GetFunctionTableName() const { return function_table_name; }
        std::string GetInstructionTableName() const { return instruction_table_name; }
        std::string GetICFSTableName() const { return icfs_table_name; }
        std::string GetICFSMapTableName() const { return icfs_map_table_name; }
        std::string GetRelocationsTableName() const { return relocs_table_name; }
        std::string GetTypesTableName() const { return types_table_name; }
        std::string GetScoopTableName() const { return scoop_table_name; }
        std::string GetEhProgramTableName() const { return ehpgm_table_name; }
        std::string GetEhCallSiteTableName() const { return ehcss_table_name; }
        std::string GetURL() const { return url; }

	void CreateTables();

	int GetELFOID() const { return elfoid; };

        friend class FileIR_t;
        friend class Function_t;
        friend class AddressID_t;
        friend class Instruction_t;
        friend class VariantID_t;
        friend class Relocation_t;
        friend class Type_t;
        friend class BasicType_t;
        friend class PointerType_t;
        friend class AggregateType_t;
        friend class FuncType_t;
        friend class ICFS_t;
	friend class DataScoop_t;
//        friend class IBTargets;

    private:
	db_id_t orig_fid;
        std::string url;
        std::string hash;
        std::string arch;       
        std::string address_table_name;
        std::string function_table_name;
        std::string instruction_table_name;
        std::string icfs_table_name;
        std::string icfs_map_table_name;
        std::string relocs_table_name;
        std::string types_table_name;
        std::string scoop_table_name;
        std::string ehpgm_table_name;
        std::string ehcss_table_name;
	int elfoid;
};
