//
// An address in a program.
//
class AddressID_t : public BaseObj_t
{
    public:
        int GetFileID();
        int GetVirtualOffset();
        void SetFileID(int);
        void SetVirtualOffset(int);

    private:
        int fileID;          // The ID of the file
        int virtual_offset;  // the virtual address(offset) into the file. 
                             // May be 0 if this insn doesn't exist
                             // within a file.
}
