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
using namespace libIRDB;
using namespace zipr;


#define dispatch_to(func) \
for(DLFunctionHandleSet_t::iterator it=m_handleList.begin(); it!=m_handleList.end();++it) \
{ \
	ZiprPluginInterface_t* zpi=(ZiprPluginInterface_t*)*it; \
	zpi->func(); \
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

bool ZiprPluginManager_t::DoesPluginPlop(Instruction_t *insn, DLFunctionHandle_t &callback) 
{
	DLFunctionHandleSet_t::iterator it=m_handleList.begin();
	for(m_handleList.begin();it!=m_handleList.end();++it)
	{
		ZiprPluginInterface_t* zpi=(ZiprPluginInterface_t*)*it;
		if (zpi->WillPluginPlop(insn))
		{
			callback = zpi;
			return true;
		}
	}
	return false;
}

void ZiprPluginManager_t::open_plugins
                        (
                         Zipr_SDK::MemorySpace_t *p_ms,
                         ELFIO::elfio *p_elfio,
                         libIRDB::FileIR_t *p_firp,
                         Zipr_SDK::ZiprOptions_t *p_opts,
                         Zipr_SDK::InstructionLocationMap_t *p_fil
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
    	if((dp  = opendir(dir.c_str())) == NULL) 
	{
        	cout << "Error(" << errno << ") opening plugins directory: " << dir << endl;
		exit(1);
    	}

    	while ((dirp = readdir(dp)) != NULL) 
	{
        	string name=dir+string(dirp->d_name);
		string zpi(".zpi");

		string extension=name.substr(name.size() - zpi.length());
		if(extension!=zpi)
		{
			cout<<"File ("<<name<<") does not have proper extension, skipping."<<endl;
			continue; // try next file
		}
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
		GetPluginInterface_t GetPluginInterface=(GetPluginInterface_t)sym;
		Zipr_SDK::ZiprPluginInterface_t *interface=(*GetPluginInterface)(p_ms,p_elfio,p_firp,p_fil);


		if(!interface)
		{
			cerr<<"Failed to get interface from file ("<<name<<")"<<endl;
			exit(1);
		}
		p_opts->AddNamespace(interface->RegisterOptions(global_ns));

		m_handleList.insert(interface);
		
    	}
    	closedir(dp);
    	return;
}
