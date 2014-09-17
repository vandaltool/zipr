#ifndef zipr_options_h
#define zipr_options_h

#include <string>
// #include <libIRDB-core.hpp>

class Options_t 
{
	public:
		Options_t() : m_outname("b.out") { };

		static Options_t* parse_args(int p_argc, char* p_argv[]) 
		{ 
			Options_t *opt=new Options_t;
			assert(opt);
			opt->m_var_id=::atoi(p_argv[1]); 
			return opt;
		};
		
		std::string GetOutputFileName(libIRDB::File_t* p_file) { return m_outname; }
		int GetVariantID() { return m_var_id; }

	private:
		std::string m_outname;
		int m_var_id;
};

#endif
