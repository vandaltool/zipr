// An ELF file as represented by the DB
class Relocation_t : public BaseObj_t
{
    public:

        // create new item.
        Relocation_t() : BaseObj_t(NULL), offset(0)  {}	// new reloc w/no data

	// a reloc read from the DB 
        Relocation_t(db_id_t reloc_id, int _offset, std::string _type) :
		BaseObj_t(NULL), offset(_offset), type(_type) { SetBaseID(reloc_id); }

        Relocation_t(db_id_t reloc_id) : BaseObj_t(NULL) { assert(0);}          // read from DB       
        void WriteToDB() { assert(0); }   // writes to DB ID is not -1.
        std::string WriteToDB(File_t* fid, Instruction_t* insn);    // writes to DB, ID is not -1.

	void SetOffset(int off) { offset=off;}
	int GetOffset() { return offset; }
	void SetType(std::string ty) { type=ty;}
	std::string GetType() { return type; }

        friend class FileIR_t;
        friend class Function_t;
        friend class AddressID_t;
        friend class Instruction_t;
        friend class VariantID_t;

    private:
	int offset;		// how far into the instruction the relocation should be applied.
	std::string type;	// a string that describes the relocation type.  
				// possible values:  32-bit, pcrel
};
