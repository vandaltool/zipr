#include <iostream>
#include <map>
#include <time.h>
#include <string.h>
#include "rewriter.h"
#include <pqxx/pqxx>

using namespace std;
using namespace pqxx;

// extract the file id from the md5 hash and the program name
int get_file_id(char *progName, char *md5hash)
{
  connection conn;
  work txn(conn);

  string query = "SELECT file_id FROM file_info WHERE hash=";
  query += txn.quote(string(md5hash));
  query += " AND url LIKE";
  query += txn.quote(string("%") + string(progName) + string("%"));

  result r = txn.exec(query);

  for (result::const_iterator row = r.begin(); row != r.end(); ++row)
  {
    return row["file_id"].as<int>();
  }
  
  return -1; // error
}

// insert addresses & instructions into DB
void insert_instructions(string programName, int fileID, vector<wahoo::Instruction*> instructions, vector<wahoo::Function*> functions)
{
  cerr << "Inserting instructions in the DB";
  connection conn;
  work txn(conn);
  // for each instruction:
  //    (1) get address, insert into address table
  //    (2) populate instruction table

  const int STRIDE = 40;
  int count = 0;

  for (int i = 0; i < instructions.size(); i += STRIDE)
  {
    char buf[128];
    string addressTable = programName + "_" + "address";
    string query = "INSERT INTO " + addressTable;
    query += " (address_id, file_id, vaddress_offset) VALUES ";

    string instructionTable = programName + "_" + "instruction";
    string query2 = "INSERT INTO " + instructionTable;
    query2 += " (address_id, parent_function_id, file_id, orig_address_id, data, asm) VALUES ";

    for (int j = i; j < i + STRIDE; ++j)
    {
      if (j >= instructions.size()) break;
      count++;

      wahoo::Instruction *instruction = instructions[j];
      app_iaddr_t   addr = instruction->getAddress();

      // insert into address table
      if (j != i) query += ",";
      query += "(";
      query += txn.quote(j) + ",";
      query += txn.quote(fileID) + ",";
      sprintf(buf,"0x%08X", addr);
      query += txn.quote(string(buf));
      query += ")";

      // insert into instruction table
      if (j != i) query2 += ",";

      int address_id = j;
      int parent_function_id = -1;
      if (instruction->getFunction())
      {
        parent_function_id = instruction->getFunction()->getFunctionID();
      }
      int orig_address_id = address_id;
      string asmData = instruction->getAsm();

      query2 += "(";
      query2 += txn.quote(address_id) + ","; // j is the address id
      query2 += txn.quote(parent_function_id) + ","; 
      query2 += txn.quote(fileID) + ","; 
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
    }

//   cerr << "Query: " << query << endl; 
//   cerr << "Query2: " << query2 << endl; 

    txn.exec(query);
    txn.exec(query2); 
  }

  cerr << "Committing all instructions - this may take a while";
  txn.commit();
  cerr << "Done inserting instructions in the DB";
}

int main(int argc, char **argv)
{
  if (argc < 5)
  {
    cerr << "usage: " << argv[0] << " <programName> <elfFile> <md5ElfFile> <annotationFile> " << endl;
    return 1;
  }

  char *programName = argv[1];
  char *elfFile = argv[2];
  char *md5hash = argv[3];
  char *annotFile = argv[4];

  cout << "program name:" << programName << endl;
  cout << "elf file:" << elfFile << endl;
  cout << "hash-md5:" << md5hash << endl;
  cout << "annotation file:" << annotFile << endl;

  connection conn;
  work txn(conn);

  Rewriter *rewriter = new Rewriter(elfFile, annotFile, "spri.out");

  int fileID = get_file_id(programName, md5hash);
  if (fileID < 0)
  {
    cerr << argv[0] << ": Error retrieving file id for: " << programName << endl;
    return 1;
  }
  else
    cout << "File id is: " << fileID << endl;

  // get functions & instructions from MEDS
  vector<wahoo::Function*> functions = rewriter->getAllFunctions();
  vector<wahoo::Instruction*> instructions = rewriter->getAllInstructions();

  cout << "Number of functions: " << functions.size() << endl;
  cout << "Number of instructions: " << instructions.size() << endl;

  // bulk insert of function information into the DB
  const int STRIDE = 25;
  int count = 0;
  for (int i = 0; i < functions.size(); i += STRIDE)
  {  
    string functionTable = string(programName) + "_" + "function";
    string query = "INSERT INTO " + functionTable;
    query += " (function_id, file_id, name, stack_frame_size) VALUES ";

    for (int j = i; j < i + STRIDE; ++j)
    {
      if (j >= functions.size()) break;
    
      wahoo::Function *f = functions[j];
      string functionName = f->getName();
      app_iaddr_t functionAddress = f->getAddress();
      int functionSize = f->getSize();
      int function_id = j;

      cout << functionName << " size: " << functionSize << endl;
      if (j != i) query += ",";
      query += "(";
      query += txn.quote(function_id) + ",";
      query += txn.quote(fileID) + ",";
      query += txn.quote(functionName) + ",";
      query += txn.quote(functionSize) + ")";

      f->setFunctionID(function_id);
    }

    txn.exec(query);
  }

  txn.commit(); // must commit o/w everything will be rolled back

  insert_instructions(programName, fileID, instructions, functions);
}

