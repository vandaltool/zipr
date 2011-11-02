//
// An address in a variant.
//
typedef int virtual_offset_t;
class AddressID_t : public BaseObj_t
{
    public:
	AddressID_t() : BaseObj_t(NULL), fileID(NOT_IN_DATABASE), virtual_offset(0) 
		{ SetBaseID(NOT_IN_DATABASE); }
	AddressID_t(db_id_t myid, db_id_t myfileID, virtual_offset_t voff) : BaseObj_t(NULL), fileID(myfileID), virtual_offset(voff) 
		{ SetBaseID(myid); }

	AddressID_t& operator=(const AddressID_t &rhs) {
		if (this != &rhs)
		{
			this->fileID = rhs.fileID;	
			this->virtual_offset = rhs.virtual_offset;	
		}

		return *this;
 	}

        db_id_t GetFileID() const { return fileID; }
        void SetFileID(db_id_t thefileID) { fileID=thefileID; }

        virtual_offset_t GetVirtualOffset() { return virtual_offset; }
        void SetVirtualOffset(virtual_offset_t voff) { virtual_offset=voff; }

	void WriteToDB() { assert(0); }
        std::string WriteToDB(VariantID_t *vid, db_id_t newid);


    private:
        db_id_t fileID;          // The ID of the file
        virtual_offset_t virtual_offset;  // the virtual address(offset) into the file. 
                             // May be 0 if this insn doesn't exist
                             // within a file.

	void Register(VariantID_t *vid);

};
