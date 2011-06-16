// The basic Function of a variant.
class Function_t : public BaseObj_t
{
    public:
	
	Function_t() : BaseObj_t(NULL) {}	// create a new function not in the db 

	Function_t(db_id_t id, std::string name, int size);	// create a function that's already in the DB  

        std::set<Instruction_t*>& GetInstructions() { return my_insns; }

        int GetStackFrameSize() { return stack_frame_size; }
        std::string GetName()	{ return name; }

        void SetStackFrameSize(int size) { stack_frame_size=size; }
        void SetName(std::string newname)	 { name=newname; }

	void WriteToDB();

    private:
        std::set<Instruction_t*> my_insns;
        int stack_frame_size;
        std::string name;
};

