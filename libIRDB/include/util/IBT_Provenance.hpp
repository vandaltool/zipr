#ifndef IBT_Provenance_h
#define IBT_Provenance_h

#include "Provenance.hpp"

class IBTProvenance_t
{
	private:
	typedef std::map<const Instruction_t*, Provenance_t> ProvMap_t;

	public:
	IBTProvenance_t(const FileIR_t* f=NULL) {Init(); if(f) AddFile(f);}
        virtual ~IBTProvenance_t() {;}
	virtual void AddFile(const FileIR_t* );
        
        Provenance_t getProvenance(const Instruction_t* insn) const
	{
		return prov_map.at(insn);
	} 

	protected:
	virtual void Init() {};

	private:

	virtual void AddProvs(const Instruction_t* before, const InstructionSet_t& after);

	ProvMap_t prov_map;
};

#endif

