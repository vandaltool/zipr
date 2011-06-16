
class Function_t; // forward decls.

#define MAX_INSN_SIZE 32	// x86 really declares this as 16, but we'll allow 
				// for bigger instructions, maybe from other machines?

// The basic instruction of a variant.
class Instruction_t : public BaseObj_t
{
    public:

	Instruction_t(db_id_t id, AddressID_t *addr, Function_t *func, db_id_t orig_id, 
		std::string data, std::string comment, db_id_t doip_id);

        AddressID_t* GetAddress() {assert(0);}
        Function_t* GetFunction() {assert(0);}
        AddressID_t GetOriginalAddressID() {assert(0);}
        Instruction_t* &GetFallthrough() {assert(0);}
        Instruction_t* GetDataBits() {assert(0);}

        void SetAddress(AddressID_t* ) {assert(0); }
        void SetFunction(Function_t*) {assert(0); }
        void SetOriginalAddressID(AddressID_t ) {assert(0); }
        void SetFallthrough(Instruction_t* i) {fallthrough=i;}
        void SetTarget(Instruction_t* i) {target=i; }
        void SetDataBits(std::string orig ) {assert(0); }       

	void WriteToDB() { assert(0); }

    private:
        AddressID_t *my_address;
        Function_t *my_function;
        db_id_t 	orig_address_id;        // const, should not change.
        Instruction_t* fallthrough;
        Instruction_t* target;
        std::string data;
};
