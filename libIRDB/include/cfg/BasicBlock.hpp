

class BasicBlock_t
{

	public:
		BasicBlock_t();

		bool GetIsExitBlock() { return is_exit_block; }
		void SetIsExitBlock(bool is_exit) { is_exit_block=is_exit; }

		std::vector<Instruction_t*>& GetInstructions() { return instructions; }
		std::set<BasicBlock_t*>&     GetPredecessors() { return predecessors; }
		std::set<BasicBlock_t*>&     GetSuccessors()   { return successors; }
		std::set<BasicBlock_t*>&     GetIndirectTargets() { return indirect_targets; }

		BasicBlock_t* GetFallthrough();
		BasicBlock_t* GetTarget();
		bool EndsInBranch();
		bool EndsInIndirectBranch();
		bool EndsInConditionalBranch();
		Instruction_t* GetBranchInstruction();

		void BuildBlock(Function_t* func,
                		Instruction_t* insn,
                		const std::map<Instruction_t*,BasicBlock_t*> &insn2block_map
        			);


	private:

		std::vector<Instruction_t*> instructions;
		std::set<BasicBlock_t*> predecessors;
		std::set<BasicBlock_t*> successors;
		std::set<BasicBlock_t*> indirect_targets;
		bool is_exit_block;

	friend std::ostream& operator<<(std::ostream& os, const BasicBlock_t& block);
};

std::ostream& operator<<(std::ostream& os, const BasicBlock_t& block);

