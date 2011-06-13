// The basic Function of a program.
class Function_t : public BaseObj_t
{
    public:
        std::set<Instruction_t*>& GetInstructions();

        int GetStackFrameSize();
        std::string GetName();

        void SetStackFrameSize(int);
        void SetName(std::string);
    private:
        std::set<Instruction_t> my_insns;
        int stack_frame_size;
        std::string name;
};

