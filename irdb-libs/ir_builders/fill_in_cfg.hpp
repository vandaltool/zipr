#ifndef fill_in_cfg_hpp
#define fill_in_cfg_hpp

#include <irdb-core>
#include <stdlib.h>
#include <map>
#include <exeio.h>

namespace PopCFG
{
	using namespace std;
	using namespace IRDB_SDK;
	using namespace EXEIO;

	class PopulateCFG : public TransformStep_t
	{
			using extra_scoop_set_t = set<pair<VirtualOffset_t,VirtualOffset_t> >;
			extra_scoop_set_t extra_scoops;

		public:
			PopulateCFG(DatabaseID_t p_variant_id = 0,
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

			}

			~PopulateCFG(void) override
			{
				// do nothing (this class uses shared IRDB objects that
				// are not managed by this class).
			}
			
			string getStepName(void) const override
			{
				return string("fill_in_cfg");
			}
			int parseArgs(const vector<string> step_args) override;
			int executeStep() override;
		    
		private: // methods
		
			// main workers
			void fill_in_cfg(FileIR_t *);
			void fill_in_scoops(FileIR_t *);
			void detect_scoops_in_code(FileIR_t *firp);
			void ctor_detection(FileIR_t *firp);
			void fill_in_landing_pads(FileIR_t *);
			void rename_start(FileIR_t *firp);

			
			// helpers
			void populate_instruction_map
				(
					map< pair<DatabaseID_t,VirtualOffset_t>, 
					Instruction_t*>&,
					FileIR_t *
				);
			
			void set_fallthrough
				(
					map< pair<DatabaseID_t,VirtualOffset_t>, 
					Instruction_t*>&,
					DecodedInstruction_t *, 
					Instruction_t *, 
					FileIR_t *
				);
			
			void set_target
				(
					map< pair<DatabaseID_t,VirtualOffset_t>, 
					Instruction_t*>&,
					DecodedInstruction_t *, 
					Instruction_t *, 
					FileIR_t *
				);
			
			File_t* find_file(FileIR_t *, DatabaseID_t);
			void add_new_instructions(FileIR_t *);
			bool is_in_relro_segment(const int);
		    
		private: //data
			
			// stats
			int odd_target_count=0;
			int bad_target_count=0;
			int bad_fallthrough_count=0;
			unsigned int failed_target_count=0;

			// check for ctor/dtors at end of .text section?
			using ctor_detection_t = enum ctor_detection { cdt_PE32AUTO, cdt_YES, cdt_NO };
		       
			ctor_detection_t do_ctor_detection=cdt_PE32AUTO; // flexi-default:  -1 means detected pe32+ files and do it, 0 means don't and 1 means do it.

			size_t targets_set=0;
			size_t fallthroughs_set=0;
			size_t scoops_detected=0;
			
			// non-optional
			DatabaseID_t variant_id=BaseObj_t::NOT_IN_DATABASE;        
			bool fix_landing_pads=false;
			
			unique_ptr<exeio> exeiop;
			set< pair<DatabaseID_t,VirtualOffset_t> > missed_instructions;
	};

}
#endif
