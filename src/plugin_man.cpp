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

/*
 * sort_plugins_by_name()
 *
 * Use this function as the comparator for sorting
 * the plugins by the name that they return in their
 * ToString() method. Sorting plugins by name is
 * useful when trying to debug problems that depend
 * on specific ordering.
 */
bool sort_plugins_by_name(DLFunctionHandle_t a, DLFunctionHandle_t b)
{
	return a->ToString() < b->ToString();
}
 
void ZiprPluginManager_t::PinningBegin()
{
	dispatch_to(PinningBegin);
}

void ZiprPluginManager_t::PinningEnd()
{
	dispatch_to(PinningEnd);
}

void ZiprPluginManager_t::DollopBegin()
{
	dispatch_to(DollopBegin);
}

void ZiprPluginManager_t::DollopEnd()
{
	dispatch_to(DollopEnd);
}
void ZiprPluginManager_t::CallbackLinkingBegin()
{
	dispatch_to(CallbackLinkingBegin);
}
void ZiprPluginManager_t::CallbackLinkingEnd()
{
	dispatch_to(CallbackLinkingEnd);
}

RangeAddress_t ZiprPluginManager_t::PlaceScoopsBegin(const RangeAddress_t max_addr)
{
	RangeAddress_t ret=max_addr;
	dispatch_to_with_var(PlaceScoopsBegin,ret);
	return ret;
}

RangeAddress_t ZiprPluginManager_t::PlaceScoopsEnd(const RangeAddress_t max_addr)
{
	RangeAddress_t ret=max_addr;
	dispatch_to_with_var(PlaceScoopsEnd,ret);
	return ret;
}


bool ZiprPluginManager_t::DoesPluginAddress(const Dollop_t *dollop, const RangeAddress_t &source, Range_t &place, bool &coalesce, bool &fallthrough_allowed, DLFunctionHandle_t &placer)
{
	DLFunctionHandleSet_t::iterator it=m_handleList.begin();
	for(m_handleList.begin();it!=m_handleList.end();++it)
	{
		ZiprPluginInterface_t* zpi=(ZiprPluginInterface_t*)*it;
		if (Must == zpi->AddressDollop(dollop, source, place, coalesce, fallthrough_allowed))
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
		if (zpi->WillPluginPlop(insn))
		{
			callbacks.push_back(zpi);
			a_plugin_does_plop = true;
		}
	}
	return a_plugin_does_plop;
}

bool ZiprPluginManager_t::DoesPluginRetargetCallback(const RangeAddress_t &callback_addr, const DollopEntry_t *callback_entry, RangeAddress_t &target_address, DLFunctionHandle_t &patcher)
{
	DLFunctionHandleSet_t::iterator it=m_handleList.begin();
	for(m_handleList.begin();it!=m_handleList.end();++it)
	{
		ZiprPluginInterface_t* zpi=(ZiprPluginInterface_t*)*it;
		if(Must==zpi->RetargetCallback(callback_addr,callback_entry,target_address))
		{
			patcher = zpi;
			return true;
		}
	}
	return false;
}

bool ZiprPluginManager_t::DoesPluginRetargetPin(const RangeAddress_t &patch_addr, const Dollop_t *target_dollop, RangeAddress_t &target_address, DLFunctionHandle_t &patcher) 
{
	DLFunctionHandleSet_t::iterator it=m_handleList.begin();
	for(m_handleList.begin();it!=m_handleList.end();++it)
	{
		ZiprPluginInterface_t* zpi=(ZiprPluginInterface_t*)*it;
		if (Must == zpi->RetargetPin(patch_addr, target_dollop, target_address))
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
				Zipr_SDK::ZiprOptions_t *p_opts
                        )
{
	char* zinst=getenv("ZIPR_INSTALL");

	if(!zinst)
	{
		cerr<<"Cannot fid $ZIPR_INSTALL environment variable.  Please set properly."<<endl;
	}

	string dir=string(zinst)+"/plugins/";

    	DIR *dp;
    	struct dirent *dirp;
    	if((dp  = opendir(dir.c_str())) == nullptr) 
	{
        	cout << "Error(" << errno << ") opening plugins directory: " << dir << endl;
		exit(1);
    	}

    	while ((dirp = readdir(dp)) != nullptr) 
	{
		string basename = string(dirp->d_name);
		string name=dir+basename;
		string zpi(".zpi");
		string extension=name.substr(name.size() - zpi.length());

		// Automatically skip cwd and pwd entries.
		if(basename == "." || basename == "..")
			continue;

		if (extension!=zpi)
		{
			cout<<"File ("<<name<<") does not have proper extension, skipping."<<endl;
			continue; // try next file
		}
		if (m_verbose)
			cout<<"Attempting load of file ("<<name<<")."<<endl;

		void* handle=dlopen(name.c_str(), RTLD_LAZY|RTLD_GLOBAL);
		if(!handle)
		{
			cerr<<"Failed to open file ("<<name<<"), error code: "<<dlerror()<<endl;
			exit(1);
		}
		dlerror();

		void* sym=dlsym(handle,"GetPluginInterface");
		if(!sym)
		{
			cerr<<"Failed to find GetPluginInterface from file ("<<name<<"), error code: "<<dlerror()<<endl;
			exit(1);
		}

		ZiprOptionsNamespace_t *global_ns = p_opts->Namespace("global");
		GetPluginInterface_t my_GetPluginInterface=(GetPluginInterface_t)sym;
		Zipr_SDK::ZiprPluginInterface_t *interface=(*my_GetPluginInterface)(zipr_obj);

		if(!interface)
		{
			cerr<<"Failed to get interface from file ("<<name<<")"<<endl;
			exit(1);
		}
		p_opts->AddNamespace(interface->RegisterOptions(global_ns));

		m_handleList.push_back(interface);
		
    	}
    	closedir(dp);

			std::sort(m_handleList.begin(), m_handleList.end(), sort_plugins_by_name);

    	return;
}
