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

#include <zipr-sdk>
#include <zipr_options.h>
#include <unistd.h>
#include <iostream>
#include <cstddef>
#include <iomanip>

#ifndef IMPLEMENTATION_DEBUG
#define IMPLEMENTATION_DEBUG 0
#endif

using namespace zipr;
using namespace std;

void ZiprOptionsNamespace_t::printNamespace() {
	ZiprOptionsNamespace_t::const_iterator it, it_end = end();
	for (it = begin(); it != it_end; it++) {
		cout << (*it)->getKey() << ": " << (*it)->getStringValue() << endl;
	}
}

bool ZiprOptionsNamespace_t::areRequirementsMet() const {
	ZiprOptionsNamespace_t::const_iterator it, it_end = end();
	for (it = begin(); it != it_end; it++) {
		if (!(*it)->areRequirementMet())
			return false;
	}
	return true;
}

void ZiprOptionsNamespace_t::addOption(ZiprOption_t *option) {
	ZiprOption_t *existing_option = optionByKey(option->getKey());
	if (existing_option) {
#if IMPLEMENTATION_DEBUG
		cout << "Found an existing option. Adding an observer." << endl;
#endif
	existing_option->addObserver(option);
	}
	else {
		insert(option);
	}
}

ZiprOption_t *ZiprOptionsNamespace_t::optionByKey(const string& key) {
	ZiprOptionsNamespace_t::const_iterator it, it_end = end();
	for (it = begin(); it != it_end; it++) {
		if ((*it)->getKey() == key)
			return *it;
	}
	return nullptr;
}

void ZiprOptionsNamespace_t::printUsage(int tabs, ostream &out) {
	ZiprOptionsNamespace_t::const_iterator it, it_end = end();
	for (it = begin(); it != it_end; it++) {
		string description = (*it)->getDescription();
		{ int t = 0; for (; t<tabs; t++) cout << "\t"; }
		out << std::setw(2);
		if (!(*it)->isRequired())
			out << "[";
		else
			out << "";
		out << "--" + getNamespace() << ":" << description;
		if (!(*it)->isRequired())
			out << " ]";
		out << endl;
	}
}

void ZiprOptions_t::printUsage(ostream &out) 
{
	set<ZiprOptionsNamespace_t*>::const_iterator it, it_end = m_namespaces.end();
	for (it = m_namespaces.begin(); it != it_end; it++)
		(*it)->printUsage(1, out);
}	

bool ZiprOptions_t::areRequirementsMet() const 
{
	set<ZiprOptionsNamespace_t*>::const_iterator it, it_end = m_namespaces.end();
	for (it = m_namespaces.begin(); it != it_end; it++)
		if (!(*it)->areRequirementsMet())
			return false;
	return true;
}

ZiprOptions_t::ZiprOptions_t(int argc, char **argv) 
{
	int i = 0;
	for (i = 0; i<argc; i++) {
		m_arguments.push_back(string(argv[i]));
	}
}

bool ZiprOptions_t::parse(ostream *error, ostream *warn) 
{
	bool success = true;

	auto it_end = m_arguments.end();
	for (auto it = m_arguments.begin(); it != it_end; it++) {
		string ns, key, argument = *it;
		string::size_type location = 0;
		bool next_is_option_value = false;

		if (0 != (location = argument.find_first_of("--"))) {
			if (warn)
				*warn << "Warning: " << argument << " does not start with --" << endl;
			continue;
		}
#if IMPLEMENTATION_DEBUG
		cout << "location: " << location << endl;
#endif
		argument = argument.substr(location+2, string::npos);
		if (string::npos == (location = argument.find_first_of(":"))) {
			if (warn)
				*warn << "Warning: " << argument << " going in global namespace."<<endl;
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
		auto option_ns = dynamic_cast<zipr::ZiprOptionsNamespace_t*>(getNamespace(ns));
		if (!option_ns) 
		{
			if (error)
				*error << "Invalid namespace: " << ns << endl;
			success = false;
			continue;
		}
		auto option_option = option_ns->optionByKey(key);
		if (!option_option) 
		{
			if (error)
				*error << "Error: namespace "
				       << ns
				       << " does not accept key "
				       << key << endl;
			success = false;
			continue;
		}
		/*
		 * By default, options need and take values. Some, though,
		 * take values but don't need them. Finally, some neither
		 * take nor need values.
		 */
		if (((it+1) != it_end) && (0 != (location = (*(it+1)).find_first_of("--")))) 
		{
			next_is_option_value = true;
		}
		if (option_option->getNeedsValue()) 
		{
			if ((it + 1) == it_end || !next_is_option_value)
			{
				if (error)
					*error << ns << ":" << key << " is missing value." << endl;
				success = false;
				continue;
				//return false;
			}
			option_option->setValue(*(++it));
		} 
		else if (option_option->getTakesValue()) 
		{
			/*
			 * Check to see if the next argument starts with --.
			 * If it does, we consider it the next option
			 * and not the value to the previous option.
			 */
			if (next_is_option_value) 
			{
				option_option->setValue(*(++it));
			} 
			else 
			{
				option_option->setOption();
			}
		} 
		else 
		{
			option_option->setOption();
		}
	}
	return success;
}

Zipr_SDK::ZiprOptionsNamespace_t *ZiprOptions_t::getNamespace(const string& ns) 
{
	auto it_end = m_namespaces.end();
	for (auto it = m_namespaces.begin(); it != it_end; it++) 
	{
		if ((*it)->getNamespace() == ns)
			return *it;
	}

	auto ret=new zipr::ZiprOptionsNamespace_t(ns);
	m_namespaces.insert(ret);
	return ret;
}

void ZiprOptions_t::addNamespace(ZiprOptionsNamespace_t *ns) 
{
	if (ns)
		m_namespaces.insert(ns);
}

void ZiprOptionsNamespace_t::mergeNamespace(ZiprOptionsNamespace_t *in) 
{
	if (!in) return;
	auto it_end = in->end();
	for (auto it = in->begin(); it != it_end; it++)
		addOption(*it);
}

void ZiprOptions_t::printNamespaces() 
{
	auto  it_end = m_namespaces.end();
	for (auto it = m_namespaces.begin(); it != it_end; it++) 
	{
		cout << (*it)->getNamespace() << endl;
		(*it)->printNamespace();
	}
}


#if 0
Zipr_SDK::ZiprOptionsNamespace_t* ZiprOptions_t::getNamespace(const string& name)
{
}


Zipr_SDK::ZiprStringOption_t*  zipr::ZiprOptionsNamespace_t::getStringOption (const string& name, const string &description, const string& default_value)    
{
}

Zipr_SDK::ZiprIntegerOption_t* zipr::ZiprOptionsNamespace_t::getIntegerOption(const string& name, const string &description, const size_t& default_value)
{
}

Zipr_SDK::ZiprBooleanOption_t* zipr::ZiprOptionsNamespace_t::getBooleanOption(const string& name, const string &description, const bool  & default_value)
{
}

Zipr_SDK::ZiprDoubleOption_t*  zipr::ZiprOptionsNamespace_t::getDoubleOption (const string& name, const string &description, const double& default_value)
{

}

#endif
