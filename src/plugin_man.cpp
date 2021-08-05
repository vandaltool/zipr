/***************************************************************************
 * Copyright (c)  2014  Zephyr Software LLC. All rights reserved.
 *
 * This software is furnished under a license and/or other restrictive
 * terms and may be used and copied only in accordance with such terms
 * and the inclusion of the above copyright notice. This software or
 * any other copies thereof may not be provided or otherwise made
 * available to any other person without the express written consent
 * of an authorized representative of Zephyr Software LCC. Title to,
 * ownership of, and all rights in the software is retained by
 * Zephyr Software LCC.
 *
 * Zephyr Software LLC. Proprietary Information
 *
 * Unless otherwise specified, the information contained in this
 * directory, following this legend, and/or referenced herein is
 * Zephyr Software LLC. (Zephyr) Proprietary Information.
 *
 * CONTACT
 *
 * For technical assistance, contact Zephyr Software LCC. at:
 *
 *
 * Zephyr Software, LLC
 * 2040 Tremont Rd
 * Charlottesville, VA 22911
 *
 * E-mail: jwd@zephyr-software.com
 **************************************************************************/

#include <zipr_all.h>

#include <dlfcn.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <vector>
#include <string>
#include <iostream>


using namespace std;
using namespace Zipr_SDK;
using namespace IRDB_SDK;
using namespace zipr;


#define dispatch_to(func) \
for(DLFunctionHandleSet_t::iterator it=m_handleList.begin(); it!=m_handleList.end();++it) \
{ \
	ZiprPluginInterface_t* zpi=(ZiprPluginInterface_t*)*it; \
	zpi->func(); \
} 

#define dispatch_to_with_var(func, var) \
for(DLFunctionHandleSet_t::iterator it=m_handleList.begin(); it!=m_handleList.end();++it) \
{ \
	ZiprPluginInterface_t* zpi=(ZiprPluginInterface_t*)*it; \
	var=zpi->func(var); \
} 

#define ALLOF(a) begin(a),end(a)



/*
 * Convert a bash PATH-like string into a vector of strings.
 * by default, delimintor is :
 * Returns the vector of strings
 */
const vector<string>& splitVar(const string& toSplit , const char delimiter = ':')
{
    static vector<string> result;
    if( !result.empty() )
        return result;
    if( toSplit.empty() )
        throw runtime_error( "toSplit should not be empty" );

    auto previous = size_t(0);
    auto index = toSplit.find( delimiter );
    while( index != string::npos )
    {
        result.push_back( toSplit.substr(previous, index-previous));
        previous=index+1;
        index = toSplit.find( delimiter, previous );
    }
    result.push_back( toSplit.substr(previous) );

    return result;
}

/*
 * sort_plugins_by_name()
 *
 * Use this function as the comparator for sorting
 * the plugins by the name that they return in their
 * toString() method. Sorting plugins by name is
 * useful when trying to debug problems that depend
 * on specific ordering.
 */
bool sort_plugins_by_name(DLFunctionHandle_t a, DLFunctionHandle_t b)
{
	return a->toString() < b->toString();
}
 
void ZiprPluginManager_t::PinningBegin()
{
	dispatch_to(doPinningBegin);
}

void ZiprPluginManager_t::PinningEnd()
{
	dispatch_to(doPinningEnd);
}

void ZiprPluginManager_t::DollopBegin()
{
	dispatch_to(doDollopBegin);
}

void ZiprPluginManager_t::DollopEnd()
{
	dispatch_to(doDollopEnd);
}
void ZiprPluginManager_t::CallbackLinkingBegin()
{
	dispatch_to(doCallbackLinkingBegin);
}
void ZiprPluginManager_t::CallbackLinkingEnd()
{
	dispatch_to(doCallbackLinkingEnd);
}

RangeAddress_t ZiprPluginManager_t::PlaceScoopsBegin(const RangeAddress_t max_addr)
{
	RangeAddress_t ret=max_addr;
	dispatch_to_with_var(doPlaceScoopsBegin,ret);
	return ret;
}

RangeAddress_t ZiprPluginManager_t::PlaceScoopsEnd(const RangeAddress_t max_addr)
{
	RangeAddress_t ret=max_addr;
	dispatch_to_with_var(doPlaceScoopsEnd,ret);
	return ret;
}


bool ZiprPluginManager_t::DoesPluginAddress(const Zipr_SDK::Dollop_t *dollop, const RangeAddress_t &source, Range_t &place, bool &coalesce, bool &fallthrough_allowed, DLFunctionHandle_t &placer)
{
	DLFunctionHandleSet_t::iterator it=m_handleList.begin();
	for(m_handleList.begin();it!=m_handleList.end();++it)
	{
		ZiprPluginInterface_t* zpi=(ZiprPluginInterface_t*)*it;
		if (Must == zpi->addressDollop(dollop, source, place, coalesce, fallthrough_allowed))
		{
			placer = zpi;
			return true;
		}
	}
	return false;
}

bool ZiprPluginManager_t::DoPluginsPlop(Instruction_t *insn, std::list<DLFunctionHandle_t> &callbacks) 
{
	bool a_plugin_does_plop = false;
	DLFunctionHandleSet_t::iterator it=m_handleList.begin();
	for(m_handleList.begin();it!=m_handleList.end();++it)
	{
		ZiprPluginInterface_t* zpi=(ZiprPluginInterface_t*)*it;
		if (zpi->willPluginPlop(insn))
		{
			callbacks.push_back(zpi);
			a_plugin_does_plop = true;
		}
	}
	return a_plugin_does_plop;
}

bool ZiprPluginManager_t::DoesPluginRetargetCallback(const RangeAddress_t &callback_addr, const Zipr_SDK::DollopEntry_t *callback_entry, RangeAddress_t &target_address, DLFunctionHandle_t &patcher)
{
	DLFunctionHandleSet_t::iterator it=m_handleList.begin();
	for(m_handleList.begin();it!=m_handleList.end();++it)
	{
		ZiprPluginInterface_t* zpi=(ZiprPluginInterface_t*)*it;
		if(Must==zpi->retargetCallback(callback_addr,callback_entry,target_address))
		{
			patcher = zpi;
			return true;
		}
	}
	return false;
}

bool ZiprPluginManager_t::DoesPluginRetargetPin(const RangeAddress_t &patch_addr, const Zipr_SDK::Dollop_t *target_dollop, RangeAddress_t &target_address, DLFunctionHandle_t &patcher) 
{
	for(const auto& zpi : m_handleList)
	{
		if (Must == zpi->retargetPin(patch_addr, target_dollop, target_address))
		{
			patcher = zpi;
			return true;
		}
	}
	return false;
}

void ZiprPluginManager_t::open_plugins
                        (
				Zipr_SDK::Zipr_t* zipr_obj,
				Zipr_SDK::ZiprOptionsManager_t *p_opts
                        )
{
	const auto ziprPluginDirs=splitVar(getenv("ZIPR_PLUGIN_PATH"));
	auto loadedBasenames = set<string>();
	for(const auto dir : ziprPluginDirs) 
	{

		const auto dp = opendir(dir.c_str());
		if(dp == nullptr) 
		{
			cout << "Error(" << errno << ") opening plugins directory: " << dir << endl;
			exit(1);
		}

		auto dirp = (dirent*) nullptr;
		while ((dirp = readdir(dp)) != nullptr) 
		{
			const auto basename = string(dirp->d_name);
			if(loadedBasenames.find(basename) != loadedBasenames.end()) 
			{
				if (m_verbose)
					cout<<"Already loaded "<<basename<<".  Skipping..."<<endl;
				continue;

			}
			loadedBasenames.insert(basename);

			const auto name=dir+'/'+basename;
			const auto zpi=string(".zpi");
			const auto extension=name.substr(name.size() - zpi.length());

			// Automatically skip cwd and pwd entries.
			if(basename == "." || basename == "..")
				continue;

			if (extension!=zpi)
			{
				cout<<"File ("<<name<<") does not have proper extension, skipping."<<endl;
				continue; // try next file
			}

			// test if this library is already loaded, can happen when 
			// an entry in ZIPR_PLUGIN_PATH is duplicated.
			const auto isLoaded = dlopen(name.c_str(), RTLD_LAZY|RTLD_GLOBAL|RTLD_NOLOAD) != nullptr;
			if(isLoaded)
			{
				if (m_verbose)
					cout<<"File ("<<name<<") already loaded."<<endl;
				continue;
			}

			if (m_verbose)
				cout<<"Attempting load of file ("<<name<<")."<<endl;

			// actuall load
			const auto handle=dlopen(name.c_str(), RTLD_LAZY|RTLD_GLOBAL);
			if(!handle)
			{
				cerr<<"Failed to open file ("<<name<<"), error code: "<<dlerror()<<endl;
				exit(1);
			}
			dlerror();

			const auto sym=dlsym(handle,"GetPluginInterface");
			if(!sym)
			{
				cerr<<"Failed to find GetPluginInterface from file ("<<name<<"), error code: "<<dlerror()<<endl;
				exit(1);
			}

			const auto my_GetPluginInterface= (GetPluginInterface_t)sym;
			const auto interface            = (*my_GetPluginInterface)(zipr_obj);

			if(!interface)
			{
				cerr<<"Failed to get interface from file ("<<name<<")"<<endl;
				exit(1);
			}

			// constructors now register options
			// auto global_ns            = p_opts->getNamespace("global");
			// p_opts->addNamespace(interface->registerOptions(global_ns));

			m_handleList.push_back(interface);

		}
		closedir(dp);

	}
	std::sort(ALLOF(m_handleList), sort_plugins_by_name);

    	return;
}
