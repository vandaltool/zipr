char temp;
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

#include <iostream>
#include <map>
#include <time.h>
#include <string.h>
#include "rewriter.h"
#include <pqxx/pqxx>
#include <stdlib.h>
#include "MEDS_AnnotationParser.hpp"
#include "MEDS_FuncPrototypeAnnotation.hpp"
#include "libIRDB-core.hpp"

using namespace std;
using namespace pqxx;
using namespace libIRDB;
using namespace MEDS_Annotation;

#include <sstream>

string functionTable;
string addressTable;
string instructionTable;
string typesTable;

static const int STRIDE = 50;

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
  txn.exec("SET client_encoding='LATIN1';");

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
  txn.exec("SET client_encoding='LATIN1';");
  // for each instruction:
  //    (1) get address, insert into address table
  //    (2) populate instruction table

  int count = 0;

  for (int i = 0; i < instructions.size(); i += STRIDE )
  {
    char buf[128];

    string query = "INSERT INTO " + addressTable;
    query += " (address_id, file_id, vaddress_offset) VALUES ";

    string query2 = "INSERT INTO " + instructionTable;
    query2 += " (instruction_id,address_id, ind_target_address_id, parent_function_id, orig_address_id, data, comment) VALUES ";

    for (int j = i; j < i + STRIDE; ++j)
    {
      if (j >= instructions.size()) break;
      wahoo::Instruction *instruction = instructions[j];
      app_iaddr_t   addr = instruction->getAddress();

      int ind_target_address=instruction->getIBTAddress();

      address_to_instructionid_map[addr]=j;

      int address_id = next_address_id++;

      // insert into address table
      if (j != i) query += ",";
      query += "(";
      query += txn.quote(address_id) + ",";
      query += txn.quote(fileID) + ",";
      sprintf(buf,"%lld", (long long)addr);
      query += txn.quote(string(buf));
      query += ")";

      if(ind_target_address!=0)
      {
        query += ",";
	ind_target_address= next_address_id++;
        query += "(";
        query += txn.quote(ind_target_address) + ",";
        query += txn.quote(fileID) + ",";
        sprintf(buf,"%lld", (long long)addr);
        query += txn.quote(string(buf));
        query += ")";
      }
      else 
	ind_target_address=-1;

      int parent_function_id = -1;
      if (instruction->getFunction())
      {
        parent_function_id = instruction->getFunction()->getFunctionID();
      }
      int orig_address_id = address_id;
      string asmData = instruction->getAsm();

      if (j != i ) query2 += ",";
      query2 += "(";
      query2 += txn.quote(my_to_string(j)) + ",";
      query2 += txn.quote(address_id) + ","; // the address id
      query2 += txn.quote(ind_target_address) + ","; // the IBT address id
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
    }

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
  txn.exec("SET client_encoding='LATIN1';");

  // bulk insert of function information into the DB
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
      int functionFrameSize =  0; // FIXME -- this isn't right.

      int function_id = j;
      f->setFunctionID(function_id);

      int outArgsRegionSize = f->getOutArgsRegionSize();
      bool useFP = f->getUseFramePointer();

      if (j != i) query += ",";
      query += "(";
      query += txn.quote(function_id) + ",";
      query += txn.quote(functionName) + ",";
      query += txn.quote(functionFrameSize) + ",";
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
  txn.exec("SET client_encoding='LATIN1';");

  // bulk insert of function information into the DB
  int count = 0;
  string query;
  for (int i = 0; i < functions.size(); i += STRIDE )
  {  
    query="";
    for (int j = i; j < i + STRIDE; ++j)
    {
        if (j >= functions.size()) break;
      	wahoo::Function *f = functions[j];
      	string functionName = f->getName();
      	app_iaddr_t functionAddress = f->getAddress();
      	int functionSize = f->getSize();
      	int function_id = f->getFunctionID();
      	int outArgsRegionSize = f->getOutArgsRegionSize();
      	bool useFP = f->getUseFramePointer();
	int insnid=-1; 	// NOT_IN_DATABASE

/*
	if(functionAddress!=0)	
	{
		assert(address_to_instructionid_map.find(functionAddress)!=address_to_instructionid_map.end());
		insnid=address_to_instructionid_map[functionAddress];
	}
    	query += "update " + functionTable;
	query += " set entry_point_id = " + txn.quote(my_to_string(insnid));
    	query += " where function_id = " + txn.quote(my_to_string(function_id));
	query += ";";
*/

	// if a function has a valid address, but the address isn't in the table...
	if(functionAddress!=0 && 
		address_to_instructionid_map.find(functionAddress)==address_to_instructionid_map.end())
	{
		// remove the function from the list of valid functions.
		query+="delete from "+functionTable;
		query+=" where function_id = " + txn.quote(my_to_string(function_id));
		query += ";";
		
	}
	else
	{
		if(functionAddress!=0)	
			insnid=address_to_instructionid_map[functionAddress];
    		query += "update " + functionTable;
		query += " set entry_point_id = " + txn.quote(my_to_string(insnid));
    		query += " where function_id = " + txn.quote(my_to_string(function_id));
		query += ";";
	}

    }
    txn.exec(query);
  }

  txn.commit(); // must commit o/w everything will be rolled back
}

/*
	typedef enum IRDB_Type {
		T_UNKNOWN = 0, T_NUMERIC = 1, T_POINTER = 2, 
		T_INT = 10, T_CHAR = 11, T_FLOAT = 12, T_DOUBLE = 13,
		T_VARIADIC = 20, T_TYPEDEF = 21, T_SUBTYPE = 22, 
		T_FUNC = 100, T_AGGREGATE = 101
	} IRDB_Type;

// MUST MATCH typedef in type.hpp in libIRDB-core!!!
*/

static int getNewTypeId()
{
	// start at 5000 so we don't clash with predefined type ids
	static int next_type_id=5000; 
	return next_type_id++;
}

void populate_predefined_types()
{
	connection conn;
	work txn(conn);
	string q = "SET client_encoding='LATIN1';";
	q += "INSERT into " + typesTable + " (type_id, type, name, ref_type_id, ref_type_id2) values ('0', '0','unknown','-1','-1');";
	q += "INSERT into " + typesTable + " (type_id, type, name, ref_type_id, ref_type_id2) values ('1', '1','numeric','-1','-1');";
	q += "INSERT into " + typesTable + " (type_id, type, name, ref_type_id, ref_type_id2) values ('2', '2','pointer','0','-1');";
	txn.exec(q);
	txn.commit(); 
}

void update_function_prototype(vector<wahoo::Function*> functions, char* annotFile)
{
	populate_predefined_types();

	MEDS_Annotations_t annotations;

	ifstream annotationif(annotFile, ifstream::in);
	if (annotationif.is_open())
	{
		MEDS_AnnotationParser annotationParser(annotationif);
		annotations = annotationParser.getAnnotations();
	}
	else
	{
		cerr << "warning: cannot open: " << annotFile << endl;
	}

	cerr << "annotations size: " << annotations.size() << endl;
	cerr << "functions size: " << functions.size() << endl;

	connection conn;
	work txn(conn);
	txn.exec("SET client_encoding='LATIN1';");

	for (int i = 0; i < functions.size(); i += STRIDE)
	{  
		string q = "";
		for (int j = i; j < i + STRIDE; ++j)
		{
			if (j >= functions.size()) break;
			wahoo::Function *f = functions[j];
      			int function_id = f->getFunctionID();
			app_iaddr_t functionAddress = f->getAddress();
			VirtualOffset vo(functionAddress);

			MEDS_FuncPrototypeAnnotation* fn_prototype_annot = NULL; 
			MEDS_FuncPrototypeAnnotation* fn_returntotype_annot = NULL; 
			
			std::vector<MEDS_Arg> *args = NULL;
			MEDS_Arg *returnArg = NULL;

			if (annotations.count(vo) > 0)
			{
				std::pair<MEDS_Annotations_t::iterator,MEDS_Annotations_t::iterator> ret; 
				ret = annotations.equal_range(vo);
				MEDS_FuncPrototypeAnnotation* p_annotation; 
				for ( MEDS_Annotations_t::iterator it = ret.first; it != ret.second; ++it)
				{    
					MEDS_AnnotationBase *base_type=(it->second);
					p_annotation = dynamic_cast<MEDS_FuncPrototypeAnnotation*>(base_type);
					if(p_annotation == NULL || !p_annotation->isValid()) 
						continue;

					if (p_annotation->getArgs())
						args = p_annotation->getArgs();

					if (p_annotation->getReturnArg())
						returnArg = p_annotation->getReturnArg();
				}    
			}

			// we should have 2 valid annotations per function
			// one for the args, one for the return type
			if (args)
			{
				// (1) define new aggregate type
				// (2) define new return type
				// (3) define new function type (combo (1) + (2))
				int aggregate_type_id = getNewTypeId();
				int func_type_id = getNewTypeId();
				int basic_type_id = T_UNKNOWN;

				for (int i = 0; i < args->size(); ++i)
				{
					if ((*args)[i].isNumericType())
						basic_type_id = T_NUMERIC;
					else if ((*args)[i].isPointerType())
						basic_type_id = T_POINTER;

					q += "INSERT into " + typesTable + " (type_id, type, name, ref_type_id, pos) VALUES (";
					q += txn.quote(my_to_string(aggregate_type_id)) + ",";
					q += txn.quote(my_to_string(T_AGGREGATE)) + ",";

					q += txn.quote(string(f->getName()) + "_arg") + ",";
					q += txn.quote(my_to_string(basic_type_id)) + ",";
					q += txn.quote(my_to_string(i)) + ");";
				}

				int return_type_id = T_UNKNOWN;
				if (returnArg) 
				{
					if (returnArg->isNumericType())
						return_type_id = T_NUMERIC;
					else if (returnArg->isPointerType())
						return_type_id = T_POINTER;
					else
						return_type_id = T_UNKNOWN;
				}

				// new function type id (ok to have duplicate prototypes)
				// ref_type_id is the return type id
				// ref_type_id2 is the type id for the aggregated type 
				//     that describes the function arguments
				q += "INSERT into " + typesTable + " (type_id, type, name, ref_type_id, ref_type_id2) VALUES (";

				q += txn.quote(my_to_string(func_type_id)) + ",";
				q += txn.quote(my_to_string(T_FUNC)) + ",";
				q += txn.quote(string(f->getName()) + "_func") + ",";
				q += txn.quote(my_to_string(return_type_id)) + ",";
				q += txn.quote(my_to_string(aggregate_type_id)) + ");";

				// update the type id in the function table
				q += "UPDATE " + functionTable;
				q += " SET type_id = " + txn.quote(my_to_string(func_type_id));
				q += " where function_id = " + txn.quote(my_to_string(function_id)) + "; ";
			} // update function prototype
		} // strided

		if (q.size() > 0)
			txn.exec(q);
	} // outer loop

 	txn.commit(); // must commit o/w everything will be rolled back
}

int main(int argc, char **argv)
{
  	if (argc != 10)
  	{
    		cerr << "usage: " << argv[0] << " <annotations file> <info annotation file> <file id> <func tab name> <insn tab name> <addr tab name> <types tab name> <elf file> <STARSxref file>" << endl;
    		return 1;
  	}

  	char *annotFile = argv[1];
  	char *infoAnnotFile = argv[2];
  	char *fid=argv[3];
  	char *myFunctionTable=argv[4];
  	char *myInstructionTable=argv[5];
  	char *myAddressTable=argv[6];
  	char *myTypesTable=argv[7];
  	char *elfFile=argv[8];
  	char *starsXrefFile=argv[9];

	cout<<"Annotation file: "<< annotFile<<endl;
	cout<<"Info annotation file: "<< infoAnnotFile<<endl;
	cout<<"File ID: "<< fid<<endl;
	cout<<"FTN:: "<< myFunctionTable<<endl;
	cout<<"ITN: "<< myInstructionTable<<endl;
	cout<<"ATN: "<< myAddressTable<<endl;
	cout<<"TYP: "<< myTypesTable<<endl;

	// set global vars for importing.
	functionTable=myFunctionTable;
	addressTable=myAddressTable;
	instructionTable=myInstructionTable;
	typesTable=myTypesTable;

  	Rewriter *rewriter = new Rewriter(elfFile, annotFile, starsXrefFile);

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

	// add function prototype information to the IRDB
	update_function_prototype(functions, infoAnnotFile);
	exit(0);
}
