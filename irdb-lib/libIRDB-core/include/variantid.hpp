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

#define CURRENT_SCHEMA 2

using FileSet_t = IRDB_SDK::FileSet_t;

class VariantID_t : public BaseObj_t, virtual public IRDB_SDK::VariantID_t
{
    public:
        VariantID_t();        		// create a Variant ID not in the database 
        VariantID_t(db_id_t pid);       // read from the DB
	~VariantID_t();     // Deletes the File_t objects -- beware dangling File_t* in FileIR_t objects!  

        bool isRegistered() const;               
        bool registerID();    // accesses DB

	IRDB_SDK::VariantID_t* clone(bool deep=true);       // accesses DB

	void WriteToDB();

	void DropFromDB();

	const FileSet_t&    getFiles() const { return files; }

	std::string getName() const { return name; }
	void setName(std::string newname) { name=newname;}

	IRDB_SDK::File_t* getMainFile() const;


	db_id_t getOriginalVariantID() const { return orig_pid;}
	
	void CloneFiles(FileSet_t& files);
	File_t* CloneFile(File_t* fptr);

    private:
        schema_version_t schema_ver;
        db_id_t orig_pid;       // matches pid if this is an "original"
                                // Variant and not a cloned variant.

        std::string name;

	void CreateTables();	// create the address, function and instruction tables 

        FileSet_t files;

        void  ReadFilesFromDB();

};




}

