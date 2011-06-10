// The basic Function of a program.
class Function_t : public BaseObj_t
{
    public:
        set<Instructions*>& GetInstructions();

        int GetStackFrameSize();
        string GetName();

        void SetStackFrameSize(int);
        void SetName(string);
    private:
        set<Instruction*> my_insns;
        int stack_frame_size;
        string name;
}

