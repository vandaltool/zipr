// The basic Function of a variant.
class Function_t : public BaseObj_t
{
    public:
	
	Function_t() : BaseObj_t(NULL) {}	// create a new function not in the db 

	// create a function that's already in the DB  
	Function_t(db_id_t id, std::string name, int size, int oa_size, bool use_fp, Instruction_t *entry);	

        std::set<Instruction_t*>& GetInstructions() { return my_insns; }

        int GetStackFrameSize() { return stack_frame_size; }
        std::string GetName()	{ return name; }
        int GetOutArgsRegionSize() {return out_args_region_size; }

        void SetStackFrameSize(int size) { stack_frame_size=size; }
        void SetName(std::string newname)	 { name=newname; }
        void SetOutArgsRegionSize(int oa_size) {out_args_region_size=oa_size;}

	void SetEntryPoint(Instruction_t *insn) {entry_point=insn;}
	Instruction_t* GetEntryPoint() { return entry_point;}

	void WriteToDB();		// we need the variant ID to write into a program.
	std::string WriteToDB(VariantID_t *vid, db_id_t newid);

        bool GetUseFramePointer() { return use_fp; }
        void SetUseFramePointer(bool useFP) { use_fp = useFP; }


    private:
	Instruction_t *entry_point;
        std::set<Instruction_t*> my_insns;
        int stack_frame_size;
        std::string name;
        int out_args_region_size;
        bool use_fp;
};

