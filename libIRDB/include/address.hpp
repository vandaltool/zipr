//
// An address in a program.
//
typedef int virtual_offset_t;
class AddressID_t : public BaseObj_t
{
    public:
        int GetFileID();
        void SetFileID(int);

        virtual_offset_t GetVirtualOffset();
        void SetVirtualOffset(virtual_offset_t);

	void WriteToDB();

    private:
        int fileID;          // The ID of the file
        virtual_offset_t virtual_offset;  // the virtual address(offset) into the file. 
                             // May be 0 if this insn doesn't exist
                             // within a file.
};
