#ifndef fill_in_cfg_hpp
#define fill_in_cfg_hpp

#include <libIRDB-core.hpp>
#include <stdlib.h>
#include <map>
#include <exeio.h>

class PopulateCFG
{
    public:
        PopulateCFG(bool p_fix_landing_pads = true,
                    libIRDB::pqxxDB_t the_pqxx_interface,
                    std::list<libIRDB::FileIR_t *> the_firp_list
            )
            :
            fix_landing_pads(p_fix_landing_pads),
            pqxx_interface(the_pqxx_interface),
            firp_list(the_firp_list)
        {
            odd_target_count = 0;
            bad_target_count = 0;
            bad_fallthrough_count = 0;
            failed_target_count = 0U;
       
            elfiop = NULL;
        }
        static PopulateCFG ParseAndConstruct(int argc, char* argv[], libIRDB::pqxxDB_t, std::list<libIRDB::FileIR_t *>);
        bool execute();
    
    private: // methods
        
        // main workers
        void fill_in_cfg(libIRDB::FileIR_t *);
        void fill_in_scoops(libIRDB::FileIR_t *);
        void fill_in_landing_pads(libIRDB::FileIR_t *);
        
        // helpers
        void populate_instruction_map
	(
		std::map< std::pair<libIRDB::db_id_t,libIRDB::virtual_offset_t>, libIRDB::Instruction_t*>,
		libIRDB::FileIR_t *
	);
        
        void set_fallthrough
	(
                std::map< std::pair<libIRDB::db_id_t,libIRDB::virtual_offset_t>, libIRDB::Instruction_t*>,
                libIRDB::DecodedInstruction_t *, libIRDB::Instruction_t *, libIRDB::FileIR_t *
	);
        
        void set_target
	(
                std::map< std::pair<libIRDB::db_id_t,libIRDB::virtual_offset_t>, libIRDB::Instruction_t*>,
                libIRDB::DecodedInstruction_t *, libIRDB::Instruction_t *, libIRDB::FileIR_t *
	);
        
        static libIRDB::File_t* find_file(libIRDB::FileIR_t *, libIRDB::db_id_t);
        void add_new_instructions(libIRDB::FileIR_t *);
        static bool is_in_relro_segment(const int);
    
    private: //data
        
        // options
        bool fix_landing_pads;
        
        // stats
        int odd_target_count;
        int bad_target_count;
        int bad_fallthrough_count;
        auto failed_target_count;
        
        // non-optional
        libIRDB::pqxxDB_t pqxx_interface;
        std::list<libIRDB::FileIR_t *> firp_list;
        
        EXEIO::exeio *elfiop;
        std::set< std::pair<libIRDB::db_id_t,libIRDB::virtual_offset_t> > missed_instructions;
};

#endif
