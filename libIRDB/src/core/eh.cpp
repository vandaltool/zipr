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

#include <all.hpp>
#include <utils.hpp>
#include <tuple>
#include <functional>
#include <assert.h>
#include <sstream>
#include <iostream>
#include <iomanip>

using namespace libIRDB;
using namespace std;

bool libIRDB::operator<(const EhProgram_t&a, const EhProgram_t&b)
{
        return  tie(a.cie_program,a.fde_program,a.code_alignment_factor,a.data_alignment_factor,a.return_register, a.ptrsize, a.GetRelocations())
                <
                tie(b.cie_program,b.fde_program,b.code_alignment_factor,b.data_alignment_factor,b.return_register, b.ptrsize, b.GetRelocations());
}

namespace std
{
template<>
class hash<EhProgramListing_t> 
{
	public:
		std::size_t operator()(const EhProgramListing_t& s) const
		{
			auto sub_hasher=std::hash<string>();
			auto res=std::size_t(0);
			for(const auto& i: s)
			{
				res ^= sub_hasher(i);
			}
			return res;
		}
	
};
}


void libIRDB::EhProgram_t::print() const
{
	std::hash<EhProgramListing_t> ehpgmlist_hash;
	auto cie_hash=ehpgmlist_hash(cie_program);
	auto fde_hash=ehpgmlist_hash(fde_program);
	cout<<dec<<"CAF: "<<code_alignment_factor<<" DAF: "<<data_alignment_factor<<" ptrsize="<< +ptrsize<<" cie_hash: "<<hex<<cie_hash<<" fde_hash: "<<fde_hash<<endl;
}

std::string EhProgram_t::WriteToDB(File_t* fid)    // writes to DB, ID is not -1.
{
	auto str_to_encoded_string=[](const string& data) -> string
	{
		ostringstream hex_data;
		hex_data << setfill('0') << hex;
		for (size_t i = 0; i < data.length(); ++i)
			hex_data << setw(2) << (int)(data[i]&0xff);
		return hex_data.str();
	};

	auto vec_to_encoded_string=[&](const vector<string>& in) -> string
	{
		string q="";
		for(const auto& i : in)
		{
			if(q!="") 	
				q+=",";
			q+=str_to_encoded_string(i);	
		}
		return q;
	};

	string encoded_cie_program=vec_to_encoded_string(cie_program);
	string encoded_fde_program=vec_to_encoded_string(fde_program);

	string q;
	q ="insert into " + fid->GetEhProgramTableName();
	q+="(eh_pgm_id,caf,daf,return_register,ptrsize,cie_program,fde_program) "+
		string(" VALUES (") +
		string("'") + to_string(GetBaseID())          + string("', ") +
		string("'") + to_string(+code_alignment_factor)               + string("', ") +
		string("'") + to_string(+data_alignment_factor)               + string("', ") +
		string("'") + to_string(+return_register)               + string("', ") +
		string("'") + to_string(+ptrsize)               + string("', ") +
		string("'") + encoded_cie_program               + string("', ") +
		string("'") + encoded_fde_program               + string("') ; ");
	return q;

}

std::string EhCallSite_t::WriteToDB(File_t* fid)    // writes to DB, ID is not -1.
{
	const auto vec_to_string=[](const vector<int> &v)
	{
		stringstream s;
		for(const auto &e : v)
		{
			s<<dec<<e<<" ";
		}
		
		return s.str();
	};
	string q;

	

	auto landing_pad_id=BaseObj_t::NOT_IN_DATABASE;
	if(landing_pad != NULL)
		landing_pad_id=landing_pad->GetBaseID();

	const auto ttov_str=vec_to_string(ttov);

	q ="insert into " + fid->GetEhCallSiteTableName();
	q+="(ehcs_id,tt_encoding,ttov,lp_insn_id) "+
		string(" VALUES (") +
		string("'") + to_string(GetBaseID())         + string("', ") +
		string("'") + to_string(+tt_encoding)        + string("', ") +
		string("'") + ttov_str        + string("', ") +
		string("'") + to_string(landing_pad_id)      + string("') ;");
	return q;
}

bool EhCallSite_t::GetHasCleanup() const 
{
	const auto ttov_it=find(ttov.begin(), ttov.end(), 0);
	return ttov_it!=ttov.end();
}

void EhCallSite_t::SetHasCleanup(bool p_has_cleanup) 
{
	if(p_has_cleanup)
	{
		if(!GetHasCleanup())
			ttov.push_back(0);
	}
	else
	{
		if(GetHasCleanup())
		{
			const auto ttov_it=find(ttov.begin(), ttov.end(), 0);
			ttov.erase(ttov_it);
		}
	}
}

