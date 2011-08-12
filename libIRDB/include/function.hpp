// The basic Function of a variant.
class Function_t : public BaseObj_t
{
    public:
	
	Function_t() : BaseObj_t(NULL) {}	// create a new function not in the db 

	Function_t(db_id_t id, std::string name, int size, int oa_size, bool use_fp, File_t *file);	// create a function that's already in the DB  

        std::set<Instruction_t*>& GetInstructions() { return my_insns; }

        int GetStackFrameSize() { return stack_frame_size; }
        std::string GetName()	{ return name; }
        int GetOutArgsRegionSize() {return out_args_region_size; }

        void SetStackFrameSize(int size) { stack_frame_size=size; }
        void SetName(std::string newname)	 { name=newname; }
        void SetOutArgsRegionSize(int oa_size) {out_args_region_size=oa_size;}

	void SetFile(File_t* file) {my_file=file;}
	File_t* GetFile() { return my_file;}

	void WriteToDB();		// we need the variant ID to write into a program.
	std::string WriteToDB(VariantID_t *vid, db_id_t newid);

        bool GetUseFramePointer() { return use_fp; }
        void SetUseFramePointer(bool useFP) { use_fp = useFP; }


    private:
	File_t *my_file;
        std::set<Instruction_t*> my_insns;
        int stack_frame_size;
        std::string name;
        int out_args_region_size;
        bool use_fp;
};

