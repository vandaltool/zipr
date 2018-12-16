#ifndef fill_in_cfg_hpp
#define fill_in_cfg_hpp

#include <libIRDB-core.hpp>
#include <stdlib.h>
#include <map>
#include <exeio.h>

class PopulateCFG : public libIRDB::Transform_SDK::TransformStep_t
{
    public:
        PopulateCFG(libIRDB::db_id_t p_variant_id = 0,
                    bool p_fix_landing_pads = true
            )
            :
            variant_id(p_variant_id),
            fix_landing_pads(p_fix_landing_pads)
        {
		odd_target_count = 0;
		bad_target_count = 0;
		bad_fallthrough_count = 0;
		failed_target_count = 0U;

		targets_set=0;
		fallthroughs_set=0;
		scoops_detected=0;

		elfiop = std::unique_ptr<EXEIO::exeio>(nullptr);
        }

	~PopulateCFG(void) override
	{
		// do nothing (this class uses shared IRDB objects that
		// are not managed by this class).
	}
	
	std::string getStepName(void) const override
	{
		return std::string("fill_in_cfg");
	}
        int parseArgs(const std::vector<std::string> step_args) override;
	int executeStep(libIRDB::IRDBObjects_t *const) override;
    
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

	size_t targets_set=0;
	size_t fallthroughs_set=0;
	size_t scoops_detected=0;
        
        // non-optional
	libIRDB::db_id_t variant_id;        
        bool fix_landing_pads;
        
        std::unique_ptr<EXEIO::exeio> elfiop;
        std::set< std::pair<libIRDB::db_id_t,libIRDB::virtual_offset_t> > missed_instructions;
};

#endif
