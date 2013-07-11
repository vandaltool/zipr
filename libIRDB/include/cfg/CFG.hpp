

class ControlFlowGraph_t
{
	public:
		ControlFlowGraph_t(Function_t* func);

		BasicBlock_t* GetEntry() const { return entry; }
		Function_t* GetFunction() const { return function; }

	protected:
		void Build(Function_t *func);

	private:
		std::set<BasicBlock_t*> blocks;
		BasicBlock_t* entry;
		Function_t* function;

	/* friends */
	public:
		friend std::ostream& operator<<(std::ostream& os, const ControlFlowGraph_t& cfg);
		std::set<BasicBlock_t*>&     GetBlocks()   { return blocks; }
};


std::ostream& operator<<(std::ostream& os, const ControlFlowGraph_t& cfg);


