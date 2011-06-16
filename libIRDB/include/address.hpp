//
// An address in a variant.
//
typedef int virtual_offset_t;
class AddressID_t : public BaseObj_t
{
    public:
	AddressID_t(db_id_t myid, db_id_t myfileID, virtual_offset_t voff) : BaseObj_t(NULL), fileID(myfileID), virtual_offset(voff) 
		{ SetBaseID(myid); }

        int GetFileID() const { return fileID; }
        void SetFileID(int thefileID) { fileID=thefileID; }

        virtual_offset_t GetVirtualOffset() { return virtual_offset; }
        void SetVirtualOffset(virtual_offset_t voff) { virtual_offset=voff; }

	void WriteToDB() { assert(0); }

    private:
        db_id_t fileID;          // The ID of the file
        virtual_offset_t virtual_offset;  // the virtual address(offset) into the file. 
                             // May be 0 if this insn doesn't exist
                             // within a file.
};
