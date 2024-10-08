

#include <string>
#include <structured_diversity.h>
#include <filebased.h>
#include <fstream>
#include <iostream>
#include <stdlib.h>



#if defined(__linux__)
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

using namespace std;
using namespace libStructDiv;


template <class T>
void ignore_result(T /* res */ ) {}


template<typename T> std::string toString(const T& value)
{
    std::ostringstream oss;
    oss << value;
    return oss.str();
}


FileBased_StructuredDiversity_t::FileBased_StructuredDiversity_t(string key, int varid, int total_variants, string p_config)
		: StructuredDiversity_t(key, varid, total_variants), m_barrier_count(0)
{

	assert(varid>=0 && varid<total_variants);

#if defined(__linux__)
	char* uname = getenv("USER");
	if(!uname) {
		uname = getenv("USERNAME");
	}
	if(!uname) {
		uname = getlogin();
	}
	// assume p_config, which previously started with "dir://" already has the protocol stripped.
	m_shared_dir=p_config;

	// check path exists.
	struct stat info = {};
	if( stat(m_shared_dir.c_str(), &info ) != 0 )
	{
		// dir doesn't exist, make it.
		if(mkdir(m_shared_dir.c_str(),0755)!=0)
		{
			// OK if this fails, because maybe another variant did it since we last checked.
		}	
	}

	// but now, _we_ have tried to make the dir, and so it should be there, or we give up.
	if( stat(m_shared_dir.c_str(), &info ) != 0 )
	{
		// at this point, we tried to make it and couldn't.
		// and no one else did either.  Thus, it's an error.
		perror("FileBased_StructuredDiversity_t::FileBased_StructuredDiversity_t");
		cerr<<"Cannot create dir:"<<m_shared_dir<<endl;
		exit(1);
	}

	if(!S_ISDIR(info.st_mode))
	{
		cerr<<"FileBased_StructuredDiversity_t::FileBased_StructuredDiversity_t: "
		    <<m_shared_dir<<" is not a directory."<<endl;
		exit(1);
		
	}

	
	// dir exists,
	// remove any old Barriers file for my variant..
	string base_filespec=m_shared_dir+"/Barriers_"+uname+"_"+GetKey()+"_*_"+toString(GetVariantID())+".*";
	string rm_cmd="/bin/rm -f "+base_filespec;
	int res=system(rm_cmd.c_str());
	if(res!=0)
	{
		perror("FileBased_StructuredDiversity_t::FileBased_StructuredDiversity_t");
		cerr<<"Cannot remove files, cmd="<<rm_cmd<<endl;
		exit(1);
	}
		
	cout<<"Initing shared path: "<<m_shared_dir<<endl<<"Contents:"<<endl;
	string ls_cmd="ls "+m_shared_dir;
	ignore_result(system(ls_cmd.c_str()));
#else
	assert(0); // filesystem sharing not implement on non-linux platforms yet
#endif

}


static bool fexists(const char *filename) {
  ifstream ifile(filename);
  return (!!ifile);
}

vector<string> FileBased_StructuredDiversity_t::DoBarrier(string value)
{
	char* uname = getenv("USER");
	if(!uname) {
		uname = getenv("USERNAME");
	}
#if defined(__linux__)
	if(!uname) {
		uname = getlogin();
	}
#endif
	assert(uname);
	string base_filename=m_shared_dir+"/Barriers_"+uname+"_"+GetKey()+"_"+toString(m_barrier_count)+"_"+toString(GetVariantID());
	string data_filename=base_filename+".data";
	string done_filename=base_filename+".done";

	vector<string> vres;

	cout<<"Writing shared (base) filename: "<<base_filename<<endl;


	ofstream data_file(data_filename.c_str()); 
	if(!data_file) 
		assert(0);
	data_file<<value;
	data_file.close();

	ofstream done_file(done_filename.c_str()); 
	if(!done_file) 
		assert(0);
	done_file<<1;
	done_file.close();

	for(int i=0;i<GetNumberOfVariants();i++)
	{
		string var_base_filename=m_shared_dir+"/Barriers_"+uname+"_"+GetKey()+"_"+toString(m_barrier_count)+"_"+toString(i);
		string var_data_filename=var_base_filename+".data";
		string var_done_filename=var_base_filename+".done";

		cout<<"Waiting for shared (base) filename: "<<var_base_filename<<endl;

		while(!fexists(var_done_filename.c_str()))
			sleep(1);

		ifstream infile(var_data_filename.c_str());
		string res;
		infile>>res;
		infile.close();
		vres.push_back(res);
	}

	// next time, we'll use different file names.
	m_barrier_count++;

	return vres;
}


