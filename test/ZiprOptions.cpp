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
#include <zipr_sdk.h>

using namespace zipr;
using namespace std;

#define INVOKE(a) \
bool __ ## a ## _result = false; \
__ ## a ## _result = a(); \
printf(#a ":"); \
if (__ ## a ## _result) \
{ \
printf(" pass\n"); \
} \
else \
{ \
printf(" fail\n"); \
}

int main(int argc, char *argv[])
{
	ZiprOptions_t options(argc-1, argv+1);

	ZiprOptionsNamespace_t global_ns("global");
	ZiprOptionsNamespace_t local_ns("local");

	cout << "Constructing ZiprStringOption_t('a')." << endl;
	ZiprStringOption_t global_a_option("a");
	cout << "Constructing ZiprStringOption_t('a')." << endl;
	ZiprStringOption_t global_shadow_a_option("a");
	cout << "Constructing ZiprBooleanOption_t('b', true)." << endl;
	ZiprBooleanOption_t local_b_option("b", true);
	cout << "Constructing ZiprBooleanOption_t('b')." << endl;
	ZiprBooleanOption_t local_shadow_b_option("b");
	cout << "Constructing ZiprBooleanOption_t('c', false)." << endl;
	ZiprBooleanOption_t local_c_option("c", false);
	cout << "Constructing ZiprStringOption_t('c', false)." << endl;
	ZiprStringOption_t local_shadow_c_option("c");
	cout << "Constructing ZiprIntegerOption_t('d', '55')." << endl;
	ZiprIntegerOption_t local_d_option("d", "55");
	cout << "Constructing ZiprIntegerOption_t('e')." << endl;
	ZiprIntegerOption_t local_e_option("e");
	cout << "Constructing ZiprIntegerOption_t('e')." << endl;
	ZiprIntegerOption_t local_shadow_e_option("e");
	cout << "Constructing ZiprDoubleOption_t('f')." << endl;
	ZiprDoubleOption_t local_f_option("f");
	cout << "Constructing ZiprDoubleOption_t('g', 2.4)." << endl;
	ZiprDoubleOption_t local_g_option("g", 2.4);

	local_b_option.SetRequired(true);
	local_b_option.SetDescription("Set the B option.");
	local_d_option.SetRequired(true);

	global_ns.AddOption(&global_a_option);
	global_ns.AddOption(&global_shadow_a_option);
	local_ns.AddOption(&local_b_option);
	local_ns.AddOption(&local_shadow_b_option);
	local_ns.AddOption(&local_c_option);
	//local_ns.AddOption(&local_shadow_c_option);
	local_ns.AddOption(&local_d_option);
	local_ns.AddOption(&local_e_option);
	local_ns.AddOption(&local_shadow_e_option);
	local_ns.AddOption(&local_f_option);
	local_ns.AddOption(&local_g_option);

	options.AddNamespace(&global_ns);
	options.AddNamespace(&local_ns);

	options.Parse(&cout);

	if (!options.RequirementsMet())
	{
		cout << "Usage: " << endl;
		options.PrintUsage(cout);
	}

	options.PrintNamespaces();
	cout << "global_shadow_a_option: " << global_shadow_a_option.Value() << endl;
	cout << "local_shadow_b_option: " << local_shadow_b_option.StringValue() << endl;
	if (global_a_option == "avl") {
		cout << "global_a_option is avl (" << global_a_option.substr(0,1) << ")." << endl;
	} else {
		cout << "global_a_option is NOT avl." << endl;
	}

	if (local_b_option) {
		cout << "local_b_option is true." << endl;
	} else {
		cout << "local_b_option is false." << endl;
	}
	if (local_shadow_b_option) {
		cout << "local_shadow_b_option is true." << endl;
	} else {
		cout << "local_shadow_b_option is false." << endl;
	}
	if (!local_c_option) {
		cout << "local_c_option is false." << endl;
	} else {
		cout << "local_c_option is true." << endl;
	}
	if (local_d_option == 55) {
		cout << "local_d_option is " << local_d_option.Value() << endl;
	}
	cout << "local_e_option: " << ((int)local_e_option) << endl;
	if (local_shadow_e_option == local_e_option) {
		cout << "local_shadow_e_option == local_e_option" << endl;
	}
	if (local_g_option == 2.5) {
		cout << "local_g_option is 2.5!" << endl;
	}
	if (local_g_option != 2.5) {
		cout << "local_g_option is NOT 2.5!" << endl;
	}
	if (local_g_option) {
		cout << "local_g_option is!" << endl;
	}
	if (!local_g_option) {
		cout << "local_g_option is NOT!" << endl;
	}
}
