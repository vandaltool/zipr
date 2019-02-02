#ifndef IBT_Provenance_h
#define IBT_Provenance_h

#include "Provenance.hpp"

class IBTProvenance_t : public IRDB_SDK::IBTProvenance_t
{
	private:

	// types 
	using InsnProvMap_t =  std::map<const IRDB_SDK::Instruction_t*, Provenance_t>;

	// data
	InsnProvMap_t prov_map;
	static Provenance_t empty;

	// methods
	void Init() {};
	void AddProvs(const Provenance_t& p, const InstructionSet_t& after) ;

	public:
	IBTProvenance_t(const IRDB_SDK::FileIR_t* f=NULL) {Init(); if(f) addFile(f);}
	virtual ~IBTProvenance_t() {} 	// default destructor not OK for some reason?
	void addFile(const IRDB_SDK::FileIR_t* );
        
	const Provenance_t& getProvenance (const IRDB_SDK::Instruction_t* i)  const
	{
		return (*this)[i];
	}

	const Provenance_t& operator[] (const IRDB_SDK::Instruction_t* i)  const
	{ 
		const auto it=prov_map.find(i);
		if (it!= prov_map.end()) 
			return it->second;
		return empty;
	}
 

};

#endif

