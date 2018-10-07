#ifndef TransformStep_h
#define TransformStep_h

#include <libIRDB-util.hpp>


namespace Transform_SDK
{
	class TransformStep_t
	{
		public:
			// Step names must be unique, allows arguments to
			// be directed to their matching transform steps.
			virtual std::string GetStepName(void) = 0;

			// Allows all steps to parse args before any step takes time to execute
			virtual int ParseArgs(int argc, char* argv[], libIRDB::IRDBObjects_t *irdb_objects)
			{
				return 0; // success
			}
				
			virtual int ExecuteStep(libIRDB::IRDBObjects_t *irdb_objects)
			{
				return 0; // success
			}		
	};
}

extern "C"
Transform_SDK::TransformStep_t* TransformStepFactory(void);    

#endif
