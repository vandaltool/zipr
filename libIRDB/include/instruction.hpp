
class Function_t; // forward decls.
class Address_t;

#define MAX_INSN_SIZE 32	// x86 really declares this as 16, but we'll allow 
				// for bigger instructions, maybe from other machines?

// The basic instruction of a program.
class Instruction_t : public BaseObj_t
{
    public:

        Address_t* GetAddress();
        Function_t* GetFunction();
        AddressID_t GetOriginalAddressID();
        Instruction_t* &GetFallthrough();
        Instruction_t* GetDataBits();

        void SetAddress(Address_t* );
        void SetFunction(Function_t*);
        void SetOriginalAddressID(AddressID_t );
        void SetFallthrough(Instruction_t* );
        void SetDataBits(Instruction_t* );       

    private:
        Address_t *my_address;
        Function_t *my_function;
        AddressID_t orig_address_id;        // const, should not change.
        Instruction_t* fallthrough;
        Instruction_t* target;
        char data[MAX_INSN_SIZE];
};
