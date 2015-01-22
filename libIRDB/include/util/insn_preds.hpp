#ifndef insn_preds_h
#define insn_preds_h


class InstructionPredecessors_t
{
	private:
	typedef std::map<const Instruction_t*, InstructionSet_t> PredMap_t;

	public:
	InstructionPredecessors_t(const FileIR_t* f=NULL) {Init(); if(f) AddFile(f);}
	virtual void AddFile(const FileIR_t* );

	const InstructionSet_t& operator[] (const Instruction_t* i)  const
		{ 
			PredMap_t::const_iterator it=pred_map.find(i);
			if (it!= pred_map.end()) 
				return it->second;
			static InstructionSet_t empty;
			return empty;
		}

	protected:
	virtual void Init() {};

	private:

	virtual void AddPred(const Instruction_t* before, const Instruction_t* after);

	PredMap_t pred_map;
};

#endif
