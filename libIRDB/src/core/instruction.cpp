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
#include <stdlib.h> 
#include <fstream>
#include <sstream>
#include <iomanip>
#include <bea_deprecated.hpp>

#undef EIP

using namespace libIRDB;
using namespace std;

Instruction_t::Instruction_t() :
	BaseObj_t(NULL), 
	data(""),
	callback(""),
	comment(""),
	my_address(NULL),
	my_function(NULL),
	orig_address_id(NOT_IN_DATABASE),
	fallthrough(NULL),
	target(NULL),
	indTarg(NULL),
	icfs(NULL),
	eh_pgm(NULL),
	eh_cs(NULL)
{
	SetBaseID(NOT_IN_DATABASE);
}

Instruction_t::Instruction_t(db_id_t id, 
		AddressID_t *addr, 
		Function_t *func, 
		db_id_t orig_id, 
                std::string thedata, 
		std::string my_callback, 
		std::string my_comment, 
		AddressID_t *my_indTarg, 
		db_id_t doip_id) :

	BaseObj_t(NULL), 
	data(thedata),
	callback(my_callback),
	comment(my_comment),
	indTarg(my_indTarg),
	my_address(addr),
	my_function(func),
	orig_address_id(orig_id),
	fallthrough(NULL),
	target(NULL),
	icfs(NULL),
	eh_pgm(NULL),
	eh_cs(NULL)
{
	SetBaseID(id);
}

/*
int Instruction_t::Disassemble(DISASM &disasm) const
{
 
  	memset(&disasm, 0, sizeof(DISASM));
  
  	disasm.Options = NasmSyntax + PrefixedNumeral;
	disasm.Archi = FileIR_t::GetArchitectureBitWidth();
  	disasm.EIP = (UIntPtr) GetDataBits().c_str();
  	disasm.VirtualAddr = GetAddress()->GetVirtualOffset();
  	int instr_len = Disasm(&disasm);
 	 
  	return instr_len;  
}
*/

std::string Instruction_t::getDisassembly() const
{
  	DISASM disasm;
  	Disassemble(this,disasm);
  	return std::string(disasm.CompleteInstr);
}

// 
// Given an instruction in assembly, returns the raw bits in a string
// On error, return the empty string
//
bool Instruction_t::Assemble(string assembly)
{
   const string assemblyFile = "tmp.asm"; 
   const string binaryOutputFile = "tmp.bin";

   //remove any preexisting assembly or nasm generated files
   string command = "rm -f " + assemblyFile;
   system(command.c_str());
   command = "rm -f "+assemblyFile+".bin";
   system(command.c_str());

   ofstream asmFile;
   asmFile.open(assemblyFile.c_str());
   if(!asmFile.is_open())
   {
     return false;
   }

   asmFile<<"BITS "<<std::dec<<FileIR_t::GetArchitectureBitWidth()<<endl; 

   asmFile<<assembly<<endl;
   asmFile.close();

   command = "nasm " + assemblyFile + " -o "+ binaryOutputFile;
   system(command.c_str());

    ifstream binreader;
    unsigned int filesize;
    binreader.open(binaryOutputFile.c_str(),ifstream::in|ifstream::binary);

    if(!binreader.is_open())
    {
      return false;
    }

    binreader.seekg(0,ios::end);

    filesize = binreader.tellg();

    binreader.seekg(0,ios::beg);

    if (filesize == 0) return false;

    unsigned char *memblock = new unsigned char[filesize];

    binreader.read((char*)memblock,filesize);
    binreader.close();

    string rawBits;
    rawBits.resize(filesize);
    for (int i = 0; i < filesize; ++i)
      rawBits[i] = memblock[i];

    // should erase those 2 files here

    this->SetDataBits(rawBits);
    return true;
}


vector<string> Instruction_t::WriteToDB(File_t *fid, db_id_t newid)
{
	assert(fid);
	assert(my_address);

	if(GetBaseID()==NOT_IN_DATABASE)
		SetBaseID(newid);

	auto func_id=NOT_IN_DATABASE;
	if(my_function) func_id=my_function->GetBaseID();

	auto ft_id=NOT_IN_DATABASE;
	if(fallthrough) ft_id=fallthrough->GetBaseID();

	auto targ_id=NOT_IN_DATABASE;
	if(target) targ_id=target->GetBaseID();

	auto icfs_id=NOT_IN_DATABASE;
	if (icfs) icfs_id=icfs->GetBaseID();

	auto indirect_bt_id=NOT_IN_DATABASE;
	if(indTarg) indirect_bt_id=indTarg->GetBaseID();

	auto eh_pgm_id=NOT_IN_DATABASE;
	if(eh_pgm) eh_pgm_id=eh_pgm->GetBaseID();

	auto eh_css_id=NOT_IN_DATABASE;
	if(eh_cs) eh_css_id=eh_cs->GetBaseID();

	ostringstream hex_data;
	hex_data << setfill('0') << hex;;
	for (size_t i = 0; i < data.length(); ++i)
		hex_data << setw(2) << (int)(data[i]&0xff);


	return {
		to_string(GetBaseID()),
                to_string(my_address->GetBaseID()),
                to_string(func_id),
                to_string(orig_address_id),
                to_string(ft_id),
                to_string(targ_id),
                to_string(icfs_id),
                to_string(eh_pgm_id),
                to_string(eh_css_id),
                hex_data.str(),
                callback,
                comment,
                to_string(indirect_bt_id),
                to_string(GetDoipID()) };
}


/* return true if this instructino exits the function -- true if there's no function, because each instruction is it's own function? */
bool Instruction_t::IsFunctionExit() const 
{ 
	if(!my_function) 
		return true;  

	/* if there's a target that's outside this function */
	Instruction_t *target=GetTarget();
	if(target && !is_in_set(my_function->GetInstructions(),target))
		return true;

	/* if there's a fallthrough that's outside this function */
	Instruction_t *ft=GetFallthrough();
	if(fallthrough && !is_in_set(my_function->GetInstructions(),ft))
		return true;

	/* some instructions have no next-isntructions defined in the db, and we call them function exits */
	if(!target && !fallthrough)
		return true;

	return false;
}


/*
bool Instruction_t::SetsStackPointer(ARGTYPE* arg)
{
	if((arg->AccessMode & WRITE ) == 0)
		return false;
	int access_type=arg->ArgType;

	if(access_type==REGISTER_TYPE + GENERAL_REG +REG4)
		return true;
	return false;
	
}

bool Instruction_t::SetsStackPointer(DISASM* disasm)
{
	if(strstr(disasm->Instruction.Mnemonic, "push")!=NULL)
		return true;
	if(strstr(disasm->Instruction.Mnemonic, "pop")!=NULL)
		return true;
	if(strstr(disasm->Instruction.Mnemonic, "call")!=NULL)
		return true;
	if(disasm->Instruction.ImplicitModifiedRegs==REGISTER_TYPE+GENERAL_REG+REG4)
		return true;

	if(SetsStackPointer(&disasm->Argument1)) return true;
	if(SetsStackPointer(&disasm->Argument2)) return true;
	if(SetsStackPointer(&disasm->Argument3)) return true;

	return false;

}
*/
