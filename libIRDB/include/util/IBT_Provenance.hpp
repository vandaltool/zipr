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
        
        /*Provenance_t getProvenance(const Instruction_t* insn) const
	{
		return ((ProvMap_t) prov_map)[insn];
	}*/

	Provenance_t& operator[] (const Instruction_t* i)  
	{
		return prov_map[i];
	}

	const Provenance_t& operator[] (const Instruction_t* i)  const
	{ 
		ProvMap_t::const_iterator it=prov_map.find(i);
		if (it!= prov_map.end()) 
			return it->second;
		static Provenance_t empty;
		return empty;
	}
 

	protected:
	virtual void Init() {};

	private:

	virtual void AddProvs(const Instruction_t* before, const InstructionSet_t& after);

	ProvMap_t prov_map;
};

#endif

