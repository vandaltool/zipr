#include <iostream>
#include <map>
#include <time.h>
#include <string.h>
#include "rewriter.h"
#include <pqxx/pqxx>
#include <stdlib.h>

using namespace std;
using namespace pqxx;

#include <sstream>

string functionTable;
string addressTable;
string instructionTable;


template <class T>
inline std::string my_to_string (const T& t)
{
        std::stringstream ss;
        ss << t;
        return ss.str();
}


int next_address_id=0;

map<app_iaddr_t,int> address_to_instructionid_map;


// extract the file id from the md5 hash and the program name
int get_file_id(char *progName, char *md5hash)
{
  connection conn;
  work txn(conn);

  string query = "SELECT file_id FROM file_info WHERE hash=";
  query += txn.quote(string(md5hash));
  query += " AND url LIKE";
	/* the plus 7 here is to drop "psprog_" from the name */
  query += txn.quote(string("%") + string(progName+7) + string("%"));

  result r = txn.exec(query);

  for (result::const_iterator row = r.begin(); row != r.end(); ++row)
  {
    return row["file_id"].as<int>();
  }
  
  return -1; // error
}


// insert addresses & instructions into DB
void insert_instructions(int fileID, vector<wahoo::Instruction*> instructions, vector<wahoo::Function*> functions)
{
  cerr << "Inserting instructions in the DB"<<endl;
  connection conn;
  work txn(conn);
  // for each instruction:
  //    (1) get address, insert into address table
  //    (2) populate instruction table

  int count = 0;

  for (int i = 0; i < instructions.size(); i ++ )
  {
    char buf[128];
    string query = "INSERT INTO " + addressTable;
    query += " (address_id, file_id, vaddress_offset) VALUES ";

    string query2 = "INSERT INTO " + instructionTable;
    query2 += " (instruction_id,address_id, parent_function_id, orig_address_id, data, comment) VALUES ";

      wahoo::Instruction *instruction = instructions[i];
      app_iaddr_t   addr = instruction->getAddress();

      address_to_instructionid_map[addr]=i;

      int address_id = next_address_id++;

      // insert into address table
      query += "(";
      query += txn.quote(address_id) + ",";
      query += txn.quote(fileID) + ",";
      sprintf(buf,"%d", addr);
      query += txn.quote(string(buf));
      query += ")";


      int parent_function_id = -1;
      if (instruction->getFunction())
      {
        parent_function_id = instruction->getFunction()->getFunctionID();
      }
      int orig_address_id = address_id;
      string asmData = instruction->getAsm();

      query2 += "(";
      query2 += txn.quote(my_to_string(i)) + ",";
      query2 += txn.quote(address_id) + ","; // i is the address id
      query2 += txn.quote(parent_function_id) + ","; 
      query2 += txn.quote(orig_address_id) + ","; 

      // encode instruction binary data information
      // using the Postgre bytea syntax (octal representation of binary data)
      unsigned char *data = (unsigned char*) instruction->getData();
      buf[0] = '\0';
      if (data)
      {
        sprintf(buf,"E'");
        for (int k = 0; k < instruction->getSize(); ++k)
        {
          unsigned char c = data[k];
          sprintf(&buf[strlen(buf)],"\\\\%03o", c); // octal encoding
        }
        sprintf(&buf[strlen(buf)],"'::bytea");
      }
      else
      {
        sprintf(buf,"''"); 
      }
 
      query2 += string(buf) + ","; 
      query2 += txn.quote(asmData); 
      query2 += ")";

//   cerr << "Query: " << query << endl; 
//   cerr << "Query2: " << query2 << endl; 

    txn.exec(query);
    txn.exec(query2); 
  }

  cerr << "Committing all instructions - this may take a while"<<endl;
  txn.commit();
  cerr << "Done inserting instructions in the DB"<<endl;
}


void insert_functions(int fileID, const vector<wahoo::Function*> &functions  )
{
  connection conn;
  work txn(conn);

  // bulk insert of function information into the DB
  const int STRIDE = 25;
  int count = 0;
  for (int i = 0; i < functions.size(); i += STRIDE)
  {  
    string query = "INSERT INTO " + functionTable;
    query += " (function_id, name, stack_frame_size, out_args_region_size, use_frame_pointer) VALUES ";


    for (int j = i; j < i + STRIDE; ++j)
    {
      if (j >= functions.size()) break;
      wahoo::Function *f = functions[j];
      string functionName = f->getName();
      app_iaddr_t functionAddress = f->getAddress();
      int functionSize = f->getSize();

      int function_id = j;
      f->setFunctionID(function_id);

      int outArgsRegionSize = f->getOutArgsRegionSize();
      bool useFP = f->getUseFramePointer();

      if (j != i) query += ",";
      query += "(";
      query += txn.quote(function_id) + ",";
      query += txn.quote(functionName) + ",";
      query += txn.quote(functionSize) + ",";
      query += txn.quote(outArgsRegionSize) + ",";
      query += txn.quote(useFP) + ")";

    }

    txn.exec(query);
  }

  txn.commit(); // must commit o/w everything will be rolled back
}

void update_functions(int fileID, const vector<wahoo::Function*> &functions  )
{
  connection conn;
  work txn(conn);

  // bulk insert of function information into the DB
  int count = 0;
  string query;
  for (int i = 0; i < functions.size(); i++ )
  {  
      	wahoo::Function *f = functions[i];
      	string functionName = f->getName();
      	app_iaddr_t functionAddress = f->getAddress();
      	int functionSize = f->getSize();
      	int function_id = f->getFunctionID();
      	int outArgsRegionSize = f->getOutArgsRegionSize();
      	bool useFP = f->getUseFramePointer();
	int insnid=address_to_instructionid_map[functionAddress];

    	query += "update " + functionTable;
	query += " set entry_point_id = " + txn.quote(my_to_string(insnid));
    	query += " where function_id = " + txn.quote(my_to_string(function_id));
	query += ";";


  }

  txn.exec(query);
  txn.commit(); // must commit o/w everything will be rolled back
}



int main(int argc, char **argv)
{
  	if (argc != 7)
  	{
    		cerr << "usage: " << argv[0] << " <annotations file> <file id> <func tab name> <insn tab name> <addr tab name> <elf file>" << endl;
    		return 1;
  	}

  	char *annotFile = argv[1];
  	char *fid=argv[2];
  	char *myFunctionTable=argv[3];
  	char *myInstructionTable=argv[4];
  	char *myAddressTable=argv[5];
  	char *elfFile=argv[6];

	cout<<"Annotation file: "<< annotFile<<endl;
	cout<<"File ID: "<< fid<<endl;
	cout<<"FTN:: "<< myFunctionTable<<endl;
	cout<<"ITN: "<< myInstructionTable<<endl;
	cout<<"ATN: "<< myAddressTable<<endl;

	// set global vars for importing.
	functionTable=myFunctionTable;
	addressTable=myAddressTable;
	instructionTable=myInstructionTable;


  	Rewriter *rewriter = new Rewriter(elfFile, annotFile);

  	int fileID = atoi(fid);
	if(fileID<=0)
	{
		cerr << "Bad fileID: " << fid <<endl;
		exit(1);
	}

  	// get functions & instructions from MEDS
  	vector<wahoo::Function*> functions = rewriter->getAllFunctions();
  	vector<wahoo::Instruction*> instructions = rewriter->getAllInstructions();

  	cerr << "Number of functions: " << functions.size() << endl;
  	cerr << "Number of instructions: " << instructions.size() << endl;


  	insert_functions(fileID, functions);
  	insert_instructions(fileID, instructions, functions);
  	update_functions(fileID, functions);

	exit(0);
}
