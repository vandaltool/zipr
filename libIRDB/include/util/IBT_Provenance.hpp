#ifndef IBT_Provenance_h
#define IBT_Provenance_h

#include "Provenance.hpp"

class IBTProvenance_t
{
	private:

	// types 
	using InsnProvMap_t =  std::map<const Instruction_t*, Provenance_t>;
	using ICFSProvMap_t =  std::map<const ICFS_t*, Provenance_t>;

	// data
	InsnProvMap_t prov_map;
	static Provenance_t empty;

	// methods
	void Init() {};
	void AddProvs(const Provenance_t& p, const InstructionSet_t& after) ;

	public:
	IBTProvenance_t(const FileIR_t* f=NULL) {Init(); if(f) AddFile(f);}
	virtual ~IBTProvenance_t() {} 	// default destructor not OK for some reason?
	void AddFile(const FileIR_t* );
        

	const Provenance_t& operator[] (const Instruction_t* i)  const
	{ 
		const auto it=prov_map.find(i);
		if (it!= prov_map.end()) 
			return it->second;
		return empty;
	}
 

};

#endif

