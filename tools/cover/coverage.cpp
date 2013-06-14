#include "coverage.h"
#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <limits.h>
#include <string>
#include <fstream>

using namespace std;
using namespace libIRDB;

void trim(string& str)
{
  string::size_type pos = str.find_last_not_of(' ');
  if(pos != string::npos) {
    str.erase(pos + 1);
    pos = str.find_first_not_of(' ');
    if(pos != string::npos) str.erase(0, pos);
  }
  else str.erase(str.begin(), str.end());
}

enum STR2NUM_ERROR { SUCCESS, OVERFLOW, UNDERFLOW, INCONVERTIBLE };

//TODO: what if the string represents a negative number? Currently
//the number will be translated into an unsigned int. I could make this
//and incovertible situation. 
STR2NUM_ERROR str2uint (unsigned int &i, char const *s, int base=0)
{
	char *end;
	unsigned long  l;
	errno = 0;
	l = strtoul(s, &end, base);
	if ((errno == ERANGE && l == ULONG_MAX) || l > UINT_MAX) {
		return OVERFLOW;
	}
	if (*s == '\0' || *end != '\0') {
		return INCONVERTIBLE;
	}
	i = l;

	return SUCCESS;
}


void coverage::parse_coverage_file(ifstream &coverage_file)
{
	string line;
	while(coverage_file.is_open() && std::getline(coverage_file,line))
	{
		trim(line);

		if(line.empty())
				continue;

		string file,addr;

		istringstream iss(line);

		getline(iss,file,'+');
		assert(!file.empty());
		getline(iss,addr,'+');

		//if addr is empty, assume the entry for file
		//is the addr
		trim(addr);
		if(addr.empty())
		{
			addr = file;
			file = "a.ncexe";
		}


		if(coverage_map.find(file) == coverage_map.end())
		{
			file_coverage fc;
			fc.file = file;
			coverage_map[file]=fc;
		}			

		
		unsigned int uint_addr;
		assert(str2uint(uint_addr,addr.c_str())==SUCCESS);

		coverage_map[file].coverage[uint_addr]=uint_addr;
	}
}

file_coverage* coverage::find_file_coverage(string url)
{
	for(map<string, file_coverage >::iterator it=coverage_map.begin();
		it!=coverage_map.end(); ++it)
	{
		string key = it->first;

		if(key.empty())
			continue;

		if(url.find(key)!=string::npos)
		{
			return &(it->second);
		}
	}

	return NULL;
}

void coverage::print_coverage_for_file(file_coverage *fc, FileIR_t *fileirp, ofstream &out_file)
{
	for(
		set<Function_t*>::const_iterator func_it=fileirp->GetFunctions().begin();
		func_it!=fileirp->GetFunctions().end();
		++func_it
		)
	{
		Function_t *func = *func_it;
		if(func==NULL)
			continue;

		unsigned int total_ins = func->GetInstructions().size();
		unsigned int covered_ins = 0;

		for(
			set<Instruction_t*>::const_iterator it=func->GetInstructions().begin();
			it!=func->GetInstructions().end();
			++it
			)
		{
			Instruction_t* instr = *it;

			if(instr==NULL || instr->GetAddress()==NULL)
				continue;

			unsigned int addr = (unsigned int) instr->GetAddress()->GetVirtualOffset();
			if(fc->coverage.find(addr)!=fc->coverage.end())
				covered_ins++;
		}

		double cov_percent = ((double)covered_ins)/((double)total_ins);

		string func_name = func->GetName();
		if(func_name.length() > 1 && func_name[0] != '.' && cov_percent >= 0.5)
			majority_coverage++;

		out_file<<fc->file<<"+"<<func->GetName()<<" "<<cov_percent<<" "<<covered_ins<<" "<<total_ins<<endl;

	}
}

void coverage::print_function_coverage_file(libIRDB::VariantID_t *vidp,std::ofstream &out_file)
{
	assert(vidp);

	majority_coverage = 0;

	for(set<File_t*>::iterator it=vidp->GetFiles().begin();
		it!=vidp->GetFiles().end();
		++it
		)
	{
		File_t* this_file=*it;
		assert(this_file);

		// read the db  
		FileIR_t *fileirp=new FileIR_t(*vidp,this_file);
		assert(fileirp);

		//NULL fc should only happen if the coverage file is empty. 
		//TODO: add extra sanity checks to make sure that null only happens under these conditions. 
		file_coverage* fc = find_file_coverage(fileirp->GetFile()->GetURL());

		if(fc!=NULL)
			print_coverage_for_file(fc, fileirp, out_file);

		delete fileirp;
	}

	cout<<"Coverage Summary: Non-PLT Functions with 50% coverage or greater: "<<majority_coverage<<endl;
}
