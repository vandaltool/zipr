#ifndef IBT_Provenance_h
#define IBT_Provenance_h

#include <bitset>

class IBTProvenance_t
{
	private:
        enum class IB_Type { IndJmp = 0, IndCall = 1, Ret = 2 };
	typedef std::map<const Instruction_t*, std::bitset<3>> ProvMap_t;

	public:
	IBTProvenance_t(const FileIR_t* f=NULL) {Init(); if(f) AddFile(f);}
        virtual ~IBTProvenance_t() {;}
	virtual void AddFile(const FileIR_t* );
        
        bool IsInsnRetTarg(const Instruction_t* i) const
        {
            return (prov_map.at(i)).test((size_t) IB_Type::Ret);
        }

	bool IsInsnIndJmpTarg(const Instruction_t* i) const
        {
            return (prov_map.at(i)).test((size_t) IB_Type::IndJmp);
        }
        
        bool IsInsnIndCallTarg(const Instruction_t* i) const
        {
            return (prov_map.at(i)).test((size_t) IB_Type::IndCall);
        }

	protected:
	virtual void Init() {};

	private:

	virtual void AddProvs(const Instruction_t* before, const InstructionSet_t& after);

	ProvMap_t prov_map;
};

#endif

