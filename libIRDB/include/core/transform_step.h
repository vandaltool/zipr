#ifndef TransformStep_h
#define TransformStep_h


namespace Transform_SDK
{
	class TransformStep_t
	{
		public:
			// Step names must be unique, allows arguments to
			// be directed to their matching transform steps.
			virtual std::string getStepName(void) const = 0;

			// Allows all steps to parse args before any step takes time to execute
			virtual int parseArgs(int argc, const char* const argv[])
			{
				return 0; // success
			}
				
			virtual int executeStep(libIRDB::IRDBObjects_t *const irdb_objects)
			{
				return 0; // success
			}
			
			virtual ~TransformStep_t(void)
			{
				// do nothing 
			}		
	};
}

extern "C"
std::shared_ptr<Transform_SDK::TransformStep_t> GetTransformStep(void);    

#endif
