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
}

Options_t* Options_t::parse_args(int p_argc, char* p_argv[])
{
	extern char *optarg;
	extern int optind, opterr, optopt;
	int option = 0;
	char options[] = "o:v:z:";
	struct option long_options[] = {
		{"output", 1, 0, 'o'},
		{"variant", 1, 0, 'v'},
		{"optimize", 0, 0, 'z'},
		{0, 0, 0, 0},
	};

	Options_t *opt=new Options_t;
	assert(opt);

	while ((option = getopt_long(
		p_argc, 
		p_argv, 
		options, 
		long_options, 
		NULL)) != -1)
	{
		switch (option) {
			case 'z':
			{
				if (!strcmp("plopnotjump", ::optarg))
				{
					opt->EnableOptimization(
						Optimizations_t::OptimizationPlopNotJump);
				}
				else
				{
					printf("Warning: Unrecognized optimization: %s\n", ::optarg);
				}
				break;
			}
			case 'o':
			{
				opt->m_outname = std::string(::optarg);
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
			default:
			{
				printf("Warning: Unrecognized option!\n");
				break;
			}
		}
	}
	return opt;
}
