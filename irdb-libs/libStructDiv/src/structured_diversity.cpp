

#include <string>
#include <sstream>
#include <iostream>
#include <stdlib.h>
#include <assert.h>

#include <libStructDiv.h>
#include <filebased.h>


using namespace std;
using namespace libStructDiv;

static string get_next_field(string& input, string delim, bool& success)
{
	success=false;

	size_t pos=input.find(delim, 0);

	if(pos==string::npos)
		return string();	// report failure.

	string rest=input.substr(pos+delim.length());
	string beginning=input.substr(0,pos);


	input=rest;     // set output parameter.
	success=true;	// report success.
	return beginning; 	// return the field we parsed out.
}


StructuredDiversity_t* StructuredDiversity_t::factory(string key, string p_ipc_config)
{
	string parse_string=p_ipc_config;
	bool success=true;
	string var_id=get_next_field(parse_string,":", success);
	if(!success) { cout<<"cannot parse variant id from "<<p_ipc_config<<".  Expected format: var_id:tot_vars:url"<<endl;exit(1);}
	string tot_vars=get_next_field(parse_string,":", success);
	if(!success) { cout<<"cannot parse total # of variants from "<<p_ipc_config<<".  Expected format: var_id:tot_vars:url"<<endl;exit(1);}
	string protocol_type=get_next_field(parse_string,"://", success);
	if(!success) { cout<<"cannot protocol from "<<p_ipc_config<<".  Expected format: var_id:tot_vars:protocol://path"<<endl;exit(1);}
	string config=parse_string;


	cout<<"Varid: "<<var_id<<endl;
	cout<<"Total Variants: "<<tot_vars<<endl;
	cout<<"Protocol Type: '"<<protocol_type<<"'"<<endl;
	cout<<"config: "<<config<<endl;

	if(protocol_type=="dir")
	{
		return new FileBased_StructuredDiversity_t(key, strtoul(var_id.c_str(),0,0), strtoul(tot_vars.c_str(),0,0), config);
	}
	else
	{
		cerr<<"Cannot parse '"<<protocol_type<<"' k into a protocol name."<<endl;
		exit(1);
	}

	


}

