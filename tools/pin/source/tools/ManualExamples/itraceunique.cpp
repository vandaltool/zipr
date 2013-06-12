/*BEGIN_LEGAL 
  Intel Open Source License 

  Copyright (c) 2002-2011 Intel Corporation. All rights reserved.
 
  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met:

  Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.  Redistributions
  in binary form must reproduce the above copyright notice, this list of
  conditions and the following disclaimer in the documentation and/or
  other materials provided with the distribution.  Neither the name of
  the Intel Corporation nor the names of its contributors may be used to
  endorse or promote products derived from this software without
  specific prior written permission.
 
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL OR
  ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  END_LEGAL */
#include <stdio.h>
#include <set>

#include <string>
#include <fstream>
#include <iostream>
#include <cerrno>
#include <sstream>
#include <cstdlib>
#include <libgen.h>
#include <cstring>

#include "pin.H"

FILE * trace;

using namespace std;

set<string> instructions;
//set<VOID*>instructions;

// This function is called before every instruction is executed
// and prints the IP
VOID printip(void  *ip) 
{ 
	PIN_LockClient();
	IMG img = IMG_FindByAddress((ADDRINT)ip);
	PIN_UnlockClient();

	if(!IMG_Valid(img))
		return;

	stringstream ss;
	
	IMG_TYPE img_type = IMG_Type(img);

	if(img_type==IMG_TYPE_STATIC || img_type==IMG_TYPE_SHARED)
		ss<<"a.ncexe+0x"<<hex<<(ADDRINT)ip;
	else
		ss<<IMG_Name(img)<<"+0x"<<hex<<(ADDRINT)ip-IMG_LowAddress(img);

	string instruction_key = ss.str();
    if(instructions.count(instruction_key) > 0)
    	return;

//	fprintf(trace, "%s+0x%x\n", IMG_Name(img).c_str(),(ADDRINT)ip-IMG_LowAddress(img)); 
	fprintf(trace, "%s\n", instruction_key.c_str()); 
	instructions.insert(instruction_key);
}

// Pin calls this function every time a new instruction is encountered
VOID Instruction(INS ins, VOID *v)
{
	PIN_LockClient();
	IMG img = IMG_FindByAddress(INS_Address(ins));
	PIN_UnlockClient();

	if(!IMG_Valid(img))
		return;
	
	string img_name=IMG_Name(img);
	char *tmp=new char[img_name.length()+1];
	strcpy(tmp,img_name.c_str());
	string img_dir = string(dirname(tmp));
	delete [] tmp;

	if(img_dir.compare("/lib") == 0 ||
	   img_dir.compare("/lib/tls/i686/cmov")==0 ||
	   img_dir.compare("/usr/lib")==0||
	   img_dir.compare("/lib/i686/cmov")==0||
	   img_dir.compare("/lib/i386-linux-gnu")==0||
	   img_dir.compare("/usr/local/lib")==0||
	   img_dir.compare("/usr/lib/i386-linux-gnu")==0||
	   img_dir.compare("/opt/stonesoup/dependencies")==0)
		return;
    
	// fprintf(trace, "0x%08x\n", INS_Address(ins)); 
    // Insert a call to printip before every instruction, and pass it the IP
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)printip, IARG_INST_PTR, IARG_END);
}

// This function is called when the application exits
VOID Fini(INT32 code, VOID *v)
{
    fclose(trace);
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
    PIN_ERROR("This Pintool prints the IPs of every instruction executed\n" 
              + KNOB_BASE::StringKnobSummary() + "\n");
    return -1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char * argv[])
{
    // Initialize pin
	PIN_InitSymbols();
    if (PIN_Init(argc, argv)) return Usage();

	string coverage_file = string(getenv("COVERAGE_RESULTS_FILE"));
	if(coverage_file.empty())
	{
		PIN_ERROR("Could not find COVERAGE_RESULTS_FILE environment variable.");
		return -1;
	}

    trace = fopen(coverage_file.c_str(), "a");

    // Register Instruction to be called to instrument instructions
    INS_AddInstrumentFunction(Instruction, 0);

    // Register Fini to be called when the application exits
    PIN_AddFiniFunction(Fini, 0);
    
    // Start the program, never returns
    PIN_StartProgram();
    
    return 0;
}
