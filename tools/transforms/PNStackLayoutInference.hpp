
#ifndef __PNSTACKLAYOUTINFERENCE
#define __PNSTACKLAYOUTINFERENCE
#include <libIRDB-core.hpp>
#include <libIRDB-cfg.hpp>
#include "StackLayout.hpp"
#include "PNStackLayout.hpp"
#include <string>
#include <map>

class PNStackLayoutInference
{

public:	   
	virtual ~PNStackLayoutInference(){}
	virtual PNStackLayout*	GetPNStackLayout(libIRDB::Function_t *func)=0;
	virtual std::string GetInferenceName()const=0;
};

#endif
