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


#define CURRENT_SCHEMA 2

class VariantID_t;

std::ostream& operator<<(std::ostream& out, const libIRDB::VariantID_t& pid);

class VariantID_t : public BaseObj_t
{
    public:
        VariantID_t();        		// create a Variant ID not in the database 
        VariantID_t(db_id_t pid);       // read from the DB
	~VariantID_t();     // Deletes the File_t objects -- beware dangling File_t* in FileIR_t objects!  

        bool IsRegistered();               
        bool Register();    // accesses DB

        VariantID_t* Clone(bool deep=true);       // accesses DB

	void WriteToDB();

	void DropFromDB();

        std::set<File_t*>&    GetFiles() { return files; }

	std::string GetName() { return name; }
	void SetName(std::string newname) { name=newname;}

	File_t* GetMainFile() const;

	friend std::ostream& libIRDB::operator<<(std::ostream& out, const VariantID_t& pid);
	friend class FileIR_T;
	friend class Function_t;
	friend class AddressID_t;
	friend class Instruction_t;

	db_id_t GetOriginalVariantID() const { return orig_pid;}
	
	void CloneFiles(std::set<File_t*>& files);
	File_t* CloneFile(File_t* fptr);

    private:
        schema_version_t schema_ver;
        db_id_t orig_pid;       // matches pid if this is an "original"
                                // Variant and not a cloned variant.

        std::string name;

	void CreateTables();	// create the address, function and instruction tables 

        std::set<File_t*> files;

        void  ReadFilesFromDB();




};

