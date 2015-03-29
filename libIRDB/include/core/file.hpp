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
        File_t(db_id_t file_id, db_id_t orig_fid, std::string url, std::string hash, std::string arch, int elfoid, 
		std::string atn, std::string ftn, std::string itn, std::string icfs, std::string icfs_map, std::string rtn, std::string typ, db_id_t doipid);

        File_t(db_id_t file_id) : BaseObj_t(NULL) { assert(0);}          // read from DB       
        void WriteToDB() { assert(0); }   // writes to DB ID is not -1.

        std::string GetAddressTableName() { return address_table_name; }
        std::string GetFunctionTableName() { return function_table_name; }
        std::string GetInstructionTableName() { return instruction_table_name; }
// xxx        std::string GetIBTargetsTableName() { return ibtargets_table_name; }
        std::string GetICFSTableName() { return icfs_table_name; }
        std::string GetICFSMapTableName() { return icfs_map_table_name; }
        std::string GetRelocationsTableName() { return relocs_table_name; }
        std::string GetTypesTableName() { return types_table_name; }
        std::string GetURL() { return url; }

	void CreateTables();

	int GetELFOID() { return elfoid; };

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
	int elfoid;
};
