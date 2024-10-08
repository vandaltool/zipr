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


//
// An address in a variant.
//
using virtual_offset_t = IRDB_SDK::VirtualOffset_t;
class AddressID_t : virtual public IRDB_SDK::AddressID_t, public BaseObj_t
{
    public:
	virtual ~AddressID_t() {}
	AddressID_t() : BaseObj_t(NULL), fileID(NOT_IN_DATABASE), virtual_offset(0) 
		{ setBaseID(NOT_IN_DATABASE); }
	AddressID_t(db_id_t myid, db_id_t myfileID, virtual_offset_t voff) : BaseObj_t(NULL), fileID(myfileID), virtual_offset(voff) 
		{ setBaseID(myid); }

	AddressID_t& operator=(const AddressID_t &rhs) {
		if (this != &rhs)
		{
			this->fileID = rhs.fileID;	
			this->virtual_offset = rhs.virtual_offset;	
		}

		return *this;
 	}

        db_id_t getFileID() const { return fileID; }
        void setFileID(db_id_t thefileID) { fileID=thefileID; }

        virtual_offset_t getVirtualOffset() const { return virtual_offset; }
        void setVirtualOffset(virtual_offset_t voff) { virtual_offset=voff; }

	std::vector<std::string> WriteToDB(File_t *vid, db_id_t newid, bool p_withHeader);

	bool operator<(const AddressID_t& cmp) const 
		{ return fileID < cmp.fileID || (fileID == cmp.fileID && virtual_offset < cmp.virtual_offset);} 

	bool operator!=(const AddressID_t& cmp) const 
		{ return fileID != cmp.fileID || virtual_offset != cmp.virtual_offset; }  


    private:
        db_id_t fileID;          // The ID of the file
        virtual_offset_t virtual_offset;  // the virtual address(offset) into the file. 
                             // May be 0 if this insn doesn't exist
                             // within a file.

	void Register(VariantID_t *vid);

};
}
