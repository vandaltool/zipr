// The basic instruction of a program.
class Instruction_t : public BaseObj_t
{
    public:

        Address* GetAddress();
        Function* GetFunction();
        AddressID GetOriginalAddressID();
        Instruction* &GetFallthrough();
        Instruction* GetDataBits();

        void SetAddress(Address* );
        void SetFunction(Function*);
        void SetOriginalAddressID(AddressID );
        void SetFallthrough(Instruction* );
        void SetDataBits(Instruction* );       

    private:
        Address *my_address;
        Function* my_function;
        AddressID orig_address_id;        // const, should not change.
        Instruction* fallthrough;
        Instruction* target;
        char data[MAX_INSN_SIZE];
}
