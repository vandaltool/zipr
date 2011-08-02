
class Function_t; // forward decls.

#define MAX_INSN_SIZE 32	// x86 really declares this as 16, but we'll allow 
				// for bigger instructions, maybe from other machines?

// The basic instruction of a variant.
class Instruction_t : public BaseObj_t
{
    public:

	Instruction_t();

	Instruction_t(db_id_t id, AddressID_t *addr, Function_t *func, db_id_t orig_id, 
		std::string data, std::string callback, std::string comment, AddressID_t *my_indTarg, db_id_t doip_id);

        AddressID_t* GetAddress() { return my_address; }
        Function_t* GetFunction() { return my_function; }
        db_id_t GetOriginalAddressID() { return orig_address_id; }
        Instruction_t* GetFallthrough() { return fallthrough; }
        Instruction_t* GetTarget() { return target; }
        std::string GetDataBits()  { return data; }
        std::string GetCallback()  { return callback; }
        std::string GetComment()   { return comment; }

        void SetAddress(AddressID_t* newaddr)  { my_address=newaddr; }
        void SetFunction(Function_t* func   )  { my_function=func;}
        void SetOriginalAddressID(db_id_t origid) { orig_address_id=origid; /* you shouldn't do this, unless you know what you're doing! */}
        void SetFallthrough(Instruction_t* i)  {fallthrough=i;}
        void SetTarget(Instruction_t* i)       {target=i; }
        void SetDataBits(std::string orig )    { data=orig;}
        void SetCallback(std::string orig )    { callback=orig;}
        void SetComment(std::string orig )     { comment=orig;}

	AddressID_t* GetIndirectBranchTargetAddress()	       { return indTarg; }
	void SetIndirectBranchTargetAddress(AddressID_t* myIndTarg) { indTarg=myIndTarg; }

	void WriteToDB() { assert(0); }
        std::string WriteToDB(VariantID_t *vid, db_id_t newid);


    private:
        AddressID_t *my_address;
        Function_t *my_function;
        db_id_t 	orig_address_id;        // const, should not change.
        Instruction_t* fallthrough;
        Instruction_t* target;
        std::string data;
        std::string callback;    // name of callback handler (if any)
        std::string comment;
	AddressID_t* indTarg;
};
