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

#include <zipr_sdk.h>
#include <unistd.h>
#include <iostream>
#include <cstddef>

#ifndef IMPLEMENTATION_DEBUG
#define IMPLEMENTATION_DEBUG 0
#endif

using namespace Zipr_SDK;
using namespace std;

void ZiprOptionsNamespace_t::PrintNamespace() {
	ZiprOptionsNamespace_t::const_iterator it, it_end = end();
	for (it = begin(); it != it_end; it++) {
		cout << (*it)->Key() << ": " << (*it)->StringValue() << endl;
	}
}

bool ZiprOptionsNamespace_t::RequirementsMet() {
	ZiprOptionsNamespace_t::const_iterator it, it_end = end();
	for (it = begin(); it != it_end; it++) {
		if (!(*it)->RequirementMet())
			return false;
	}
	return true;
}

void ZiprOptionsNamespace_t::AddOption(ZiprOption_t *option) {
	ZiprOption_t *existing_option = OptionByKey(option->Key());
	if (existing_option) {
#if IMPLEMENTATION_DEBUG
		cout << "Found an existing option. Adding an observer." << endl;
#endif
	existing_option->AddObserver(option);
	}
	else {
		insert(option);
	}
}

ZiprOption_t *ZiprOptionsNamespace_t::OptionByKey(string key) {
	ZiprOptionsNamespace_t::const_iterator it, it_end = end();
	for (it = begin(); it != it_end; it++) {
		if ((*it)->Key() == key)
			return *it;
	}
	return NULL;
}

void ZiprOptionsNamespace_t::PrintUsage(int tabs, ostream &out) {
	ZiprOptionsNamespace_t::const_iterator it, it_end = end();
	for (it = begin(); it != it_end; it++) {
		string description = (*it)->Description();
		{ int t = 0; for (; t<tabs; t++) cout << "\t"; }
		out << std::setw(2);
		if (!(*it)->Required())
			out << "[";
		else
			out << "";
		out << "--" + Namespace() << ":" << description;
		if (!(*it)->Required())
			out << " ]";
		out << endl;
	}
}

void ZiprOptions_t::PrintUsage(ostream &out) {
	set<ZiprOptionsNamespace_t*>::const_iterator it, it_end = m_namespaces.end();
	for (it = m_namespaces.begin(); it != it_end; it++)
		(*it)->PrintUsage(1, out);
}	

bool ZiprOptions_t::RequirementsMet() {
	set<ZiprOptionsNamespace_t*>::const_iterator it, it_end = m_namespaces.end();
	for (it = m_namespaces.begin(); it != it_end; it++)
		if (!(*it)->RequirementsMet())
			return false;
	return true;
}

ZiprOptions_t::ZiprOptions_t(int argc, char **argv) {
	int i = 0;
	for (i = 0; i<argc; i++) {
		m_arguments.push_back(string(argv[i]));
	}
}

bool ZiprOptions_t::Parse(ostream &error, ostream &warn) {
	vector<string>::const_iterator it, it_end = m_arguments.end();

	for (it = m_arguments.begin(); it != it_end; it++) {
		string ns, key, argument = *it;
		string::size_type location = 0;
		ZiprOptionsNamespace_t *option_ns;
		ZiprOption_t *option_option;

		if (0 != (location = argument.find_first_of("--"))) {
			warn << "Warning: " << argument << " does not start with --" << endl;
			continue;
		}
#if IMPLEMENTATION_DEBUG
		cout << "location: " << location << endl;
#endif
		argument = argument.substr(location+2, string::npos);
		if (string::npos == (location = argument.find_first_of(":"))) {
			warn << "Warning: " << argument << " going in global namespace." << endl;
			ns = "global";
			location = -1;
		} else {
			ns = argument.substr(0, location);
		}
#if IMPLEMENTATION_DEBUG
		cout << "argument: " << argument << endl;
#endif
		key = argument.substr(location+1, string::npos);
#if IMPLEMENTATION_DEBUG
		cout << "ns: " << ns << endl;
		cout << "key: " << key << endl;
#endif
		if (!(option_ns = Namespace(ns))) {
			error << "Invalid namespace: " << ns << endl;
			return false;
		}
		if (!(option_option = option_ns->OptionByKey(key))) {
			error << "Error: namespace "
			     << ns
					 << " does not accept key "
					 << key << endl;
			return false;
		}
		/*
		 * By default, options need and take values. Some, though,
		 * take values but don't need them. Finally, some neither
		 * take nor need values.
		 */
		if (option_option->NeedsValue()) {
			if ((it + 1) == it_end)
			{
				error << ns << ":" << key << " is missing value." << endl;
				return false;
			}
			option_option->SetValue(*(++it));
		} else if (option_option->TakesValue()) {
			/*
			 * Check to see if the next argument starts with --.
			 * If it does, we consider it the next option
			 * and not the value to the previous option.
			 */
			if (((it+1) != it_end) && 
			    (0 != (location = (*(it+1)).find_first_of("--")))) {
				option_option->SetValue(*(++it));
			} else {
				option_option->Set();
			}
		} else {
			option_option->Set();
		}
	}
	return true;
}

ZiprOptionsNamespace_t *ZiprOptions_t::Namespace(string ns) {
	set<ZiprOptionsNamespace_t*>::const_iterator it, it_end = m_namespaces.end();
	for (it = m_namespaces.begin(); it != it_end; it++) {
		if ((*it)->Namespace() == ns)
			return *it;
	}
	return NULL;
}

void ZiprOptions_t::AddNamespace(ZiprOptionsNamespace_t *ns) {
	if (ns)
		m_namespaces.insert(ns);
}

void ZiprOptionsNamespace_t::MergeNamespace(ZiprOptionsNamespace_t *in) {
	if (!in) return;
	set<ZiprOption_t*>::const_iterator it, it_end = in->end();
	for (it = in->begin(); it != it_end; it++)
		AddOption(*it);
}

void ZiprOptions_t::PrintNamespaces() {
	set<ZiprOptionsNamespace_t*>::const_iterator it, it_end = m_namespaces.end();

	for (it = m_namespaces.begin(); it != it_end; it++) {
		cout << (*it)->Namespace() << endl;
		(*it)->PrintNamespace();
	}
}
