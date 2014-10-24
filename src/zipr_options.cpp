#include <zipr_all.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>

using namespace zipr;

void Options_t::print_usage(int p_argc, char *p_argv[])
{
	printf("%s [options]\n", p_argv[0]);
	printf("\t-v variant-id\t--variant variant-id: "
		"Variant ID. Mandatory. Default: none.\n");
	printf("\t-o file\t\t--output file: "
		"Output file name. Optional. Default: b.out.\n");
	printf("\t-z optimization\t--optimize optimization: "
		"Enable an optimization. Repeatable. Optional. \n");
	printf("\t-j path\t\t--objcopy path: "
		"Set the path of objcopy to use. Optional. \n");
	printf("\t-c callback.exe\t\t--path to callbacks file: "
		"Set the path of the file which contains any required callacks.  Missing callbacks elided. \n");
	printf("\t-!\t\t--verbose: "
		"Enable verbose output. \n");
	printf("\t-q\t\t--quiet: "
		"Quiet the verbose output. \n");
}

Options_t* Options_t::parse_args(int p_argc, char* p_argv[])
{
	Options_t *opt=new Options_t;
	opt->SetVerbose(true);
	extern char *optarg;
	extern int optind, opterr, optopt;
	int option = 0;
	char options[] = "!qz:o:v:c:";
	struct option long_options[] = {
		{"verbose",   no_argument,       NULL, '!'}, 
		{"quiet",     no_argument,       NULL, 'q'}, 
		{"optimize",  required_argument, NULL, 'z'},
		{"output",    required_argument, NULL, 'o'},
		{"variant",   required_argument, NULL, 'v'},
		{"callbacks", required_argument, NULL, 'c'},
		{"objcopy",   required_argument, NULL, 'j'},
		{NULL, no_argument, NULL, '\0'},	 // end-of-array marker
	};

	assert(opt);

	while ((option = getopt_long(
		p_argc, 
		p_argv, 
		options, 
		long_options, 
		NULL)) != -1)
	{
		switch (option) 	
		{
			case '!':
			{
				opt->SetVerbose(true);
				break;
			}
			case 'q':
			{
				opt->SetVerbose(false);
				break;
			}
			case 'z':
			{
				if (!strcmp("plopnotjump", ::optarg))
				{
					opt->EnableOptimization(
						Optimizations_t::OptimizationPlopNotJump);
				} 
				else if (!strcmp("fallthroughpinned", ::optarg))
				{
					opt->EnableOptimization(
						Optimizations_t::OptimizationFallthroughPinned);
				}
				else
				{
					printf("Warning: Unrecognized optimization: %s\n", ::optarg);
				}
				break;
			}
			case 'j':
			{
				opt->m_objcopy_path = std::string(::optarg);
			}
			case 'o':
			{
				opt->m_outname = std::string(::optarg);
				break;
			}
			case 'c':
			{
				opt->m_callbackname = std::string(::optarg);
				break;
			}
			case 'v':
			{
				char *valid = NULL;
				long int variant_id = ::strtol(::optarg, 
								&valid, 
								10); 
				if (*valid == '\0') {
					opt->m_var_id = variant_id;
					break;
				}
				printf("Error: Invalid variant id (%s).\n", ::optarg);
				break;
			}
			case '?':
			{
				// getopt_long printed message
				break;
			}
			default:
			{
				printf("Warning: Unrecognized option!\n");
				break;
			}
		}
	}
	return opt;
}
