#ifndef Xeon_relocs_hpp
#define Xeon_relocs_hpp

#include <libIRDB-core.hpp>
#include "Rewrite_Utility.hpp"
#include <dlfcn.h>
#include <zipr_sdk.h>

using namespace ELFIO;
using namespace Zipr_SDK;
using namespace libIRDB;

class Xeon_relocs : public ZiprPluginInterface_t
{
    public:
        // The plugin constructor
        Xeon_relocs(MemorySpace_t *p_ms,
		              elfio *p_elfio,
		              FileIR_t *p_firp,
		              InstructionLocationMap_t *p_fil,
		              Zipr_t *p_zipr) :
                        firp(p_firp),
			m_memory_space(p_ms), 
			m_elfio(p_elfio),
			final_insn_locations(p_fil),
			m_zipr(p_zipr),
			m_verbose("verbose"),
			m_on(false)
		{
			if(p_firp==NULL)
				return;
			// optimization
			// find out if slow path insns are even needed!
			if(p_firp!=NULL)
			{
                            for( auto it=firp->GetInstructions().begin(); it!=firp->GetInstructions().end(); ++it)
				{
					const auto insn=*it;
					if(FindRelocation(insn, "insert_dynlib_check"))
					{
						m_on=true;
						break;
					}	
				}
			}
			if(m_on)
				cout<<"Xeon_relocs plugin: Turning plugin on."<<endl;
			

		}
        
        virtual void PinningEnd()
			{ 
				if(!m_on) return; 
				cout<<"Xeon_relocs: Inserting bounds check."<<endl;
				AddDynLibCheckCode(); 
			}
        virtual void CallbackLinkingEnd()
			{
				if(!m_on) return; 
				cout<<"Xeon_relocs: Updating bounds check"  <<endl;
				UpdateAddrRanges(); 
			}
	virtual ZiprOptionsNamespace_t *RegisterOptions(ZiprOptionsNamespace_t *);
        virtual bool WillPluginPlop(Instruction_t*);
                virtual RangeAddress_t PlopDollopEntry(DollopEntry_t *,
                        RangeAddress_t &,
                        RangeAddress_t &,
			size_t,
			bool &);

		size_t DollopEntryOpeningSize(DollopEntry_t*);
		size_t DollopEntryClosingSize(DollopEntry_t*);
                virtual string ToString() { return "Xeon_relocs"; }
        
    private: //methods
        bool IsExeNonceRelocation(Relocation_t& reloc);
        Relocation_t* FindExeNonceRelocation(Instruction_t* insn);
        Relocation_t* FindRelocation(Instruction_t* insn, std::string type);
        Instruction_t* GenerateOutOfRangeHandler(void);
        void UpdateAddrRanges();
        void AddDynLibCheckCode();
        Instruction_t* GetDynLibCheckCode(std::string reg, Instruction_t* fall_through);
        
    private: //data
        FileIR_t* firp;
        MemorySpace_t *m_memory_space;	
	elfio *m_elfio;
	InstructionLocationMap_t *final_insn_locations;
	Zipr_t *m_zipr;
        ZiprBooleanOption_t m_verbose;
	bool m_on;
        // bounds check updates
	InstructionSet_t max_addr_update;
        InstructionSet_t min_addr_update;
};

#endif

