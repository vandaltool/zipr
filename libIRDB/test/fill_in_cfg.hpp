#ifndef fill_in_cfg_hpp
#define fill_in_cfg_hpp

#include <libIRDB-core.hpp>
#include <libIRDB-util.hpp>
#include <stdlib.h>
#include <map>
#include <exeio.h>
#include "transform_step.h"

class PopulateCFG : public Transform_SDK::TransformStep_t
{
    public:
        PopulateCFG(libIRDB::pqxxDB_t* p_pqxx_interface = NULL,
		    libIRDB::db_id_t p_variant_id = 0,
                    bool p_fix_landing_pads = true
            )
            :
	    pqxx_interface(p_pqxx_interface),
            variant_id(p_variant_id),
            fix_landing_pads(p_fix_landing_pads)
        {
            odd_target_count = 0;
            bad_target_count = 0;
            bad_fallthrough_count = 0;
            failed_target_count = 0U;
       
            elfiop = NULL;
        }
	
	std::string GetStepName(void)
	{
		return std::string("fill_in_cfg");
	}
        int ParseArgs(int argc, char* argv[]);
	int ExecuteStep(libIRDB::IRDBObjects_t*);
    
    private: // methods
        
        // main workers
        void fill_in_cfg(libIRDB::FileIR_t *);
        void fill_in_scoops(libIRDB::FileIR_t *);
        void fill_in_landing_pads(libIRDB::FileIR_t *);
        
        // helpers
        void populate_instruction_map
	(
		std::map< std::pair<libIRDB::db_id_t,libIRDB::virtual_offset_t>, libIRDB::Instruction_t*>&,
		libIRDB::FileIR_t *
	);
        
        void set_fallthrough
	(
                std::map< std::pair<libIRDB::db_id_t,libIRDB::virtual_offset_t>, libIRDB::Instruction_t*>&,
                libIRDB::DecodedInstruction_t *, libIRDB::Instruction_t *, libIRDB::FileIR_t *
	);
        
        void set_target
	(
                std::map< std::pair<libIRDB::db_id_t,libIRDB::virtual_offset_t>, libIRDB::Instruction_t*>&,
                libIRDB::DecodedInstruction_t *, libIRDB::Instruction_t *, libIRDB::FileIR_t *
	);
        
        libIRDB::File_t* find_file(libIRDB::FileIR_t *, libIRDB::db_id_t);
        void add_new_instructions(libIRDB::FileIR_t *);
        bool is_in_relro_segment(const int);
    
    private: //data
        
        // stats
        int odd_target_count;
        int bad_target_count;
        int bad_fallthrough_count;
        unsigned int failed_target_count;
        
        // non-optional
        libIRDB::pqxxDB_t* pqxx_interface;
	libIRDB::db_id_t variant_id;        
        bool fix_landing_pads;
        
        EXEIO::exeio *elfiop;
        std::set< std::pair<libIRDB::db_id_t,libIRDB::virtual_offset_t> > missed_instructions;
};

#endif
