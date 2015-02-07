/*
 * Copyright (c) 2014 - Zephyr Software LLC
 *
 * This file may be used and modified for non-commercial purposes as long as
 * all copyright, permission, and nonwarranty notices are preserved.
 * Redistribution is prohibited without prior written consent from Zephyr
 * Software.
 *
 * Please contact the authors for restrictions applying to commercial use.
 *
 * THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: Zephyr Software
 * e-mail: jwd@zephyr-software.com
 * URL   : http://www.zephyr-software.com/
 *
 */

#include <zipr_all.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>

using namespace zipr;

void ZiprOptions_t::print_usage(int p_argc, char *p_argv[])
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
	printf("\t-m [32|64]\t--architecture [32|64]: "
		"Override default system architecture detection.\n");
	printf("\t-!\t\t--verbose: "
		"Enable verbose output. \n");
	printf("\t-q\t\t--quiet: "
		"Quiet the verbose output. \n");
}

ZiprOptions_t* ZiprOptions_t::parse_args(int p_argc, char* p_argv[])
{
	ZiprOptions_t *opt=new ZiprOptions_t;
	opt->SetVerbose(true);
	extern char *optarg;
	extern int optind, opterr, optopt;
	int option = 0;
	char options[] = "!qz:o:v:c:j:m:";
	struct option long_options[] = {
		{"verbose",     no_argument,       NULL, '!'},
		{"quiet",       no_argument,       NULL, 'q'},
		{"optimize",    required_argument, NULL, 'z'},
		{"output",      required_argument, NULL, 'o'},
		{"variant",     required_argument, NULL, 'v'},
		{"callbacks",   required_argument, NULL, 'c'},
		{"objcopy",     required_argument, NULL, 'j'},
		{"architecture",required_argument, NULL, 'm'},
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
		printf("Found option %c\n", option);
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
				printf("Found option -j %s\n", ::optarg);
				opt->m_objcopy_path = std::string(::optarg);
				break;
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
			case 'm':
			{
				char *valid = NULL;
				long int architecture = ::strtol(::optarg,
								&valid,
								10);
				if (*valid == '\0' && (architecture == 32 || architecture == 64)) {
					opt->m_architecture = architecture;
					break;
				}
				printf("Error: Invalid architecture (%s); Must be 32 or 64. "
					"Will use detection to determine architecture. \n", ::optarg);
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

int ZiprOptions_t::GetArchitecture() {
	/*
	 * If the user specified an architecture, return it.
	 * Otherwise, return the one detected.
	 */
	if (m_architecture != -1)
		return m_architecture;
	return libIRDB::FileIR_t::GetArchitectureBitWidth();
}
