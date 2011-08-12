#include <libIRDB.hpp>
#include <utils.hpp>
#include <stdlib.h>
#include <fstream>

using namespace libIRDB;
using namespace std;

Instruction_t::Instruction_t() :
	BaseObj_t(NULL), 
	data(""),
	callback(""),
	comment("")
{
	SetBaseID(NOT_IN_DATABASE);
	my_address=NULL;
	my_function=NULL;
	orig_address_id=NOT_IN_DATABASE;
	fallthrough=NULL;
	target=NULL;
	indTarg=NULL;
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
	indTarg(my_indTarg)
{
	SetBaseID(id);
	my_address=addr;
	my_function=func;
	orig_address_id=orig_id;
	fallthrough=NULL;
	target=NULL;
}

int Instruction_t::Disassemble(DISASM &disasm){
 
  memset(&disasm, 0, sizeof(DISASM));
  
  disasm.Options = NasmSyntax + PrefixedNumeral;
  disasm.Archi = 32;
  disasm.EIP = (UIntPtr) GetDataBits().c_str();
  disasm.VirtualAddr = GetAddress()->GetVirtualOffset();
  int instr_len = Disasm(&disasm);
  
  //SetDataBits(disasm.EIP);

  return instr_len;  
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

   asmFile<<NASM_BIT_WIDTH<<endl; // define to be 32

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


string Instruction_t::WriteToDB(VariantID_t *vid, db_id_t newid)
{
	assert(vid);
	assert(my_address);

	if(GetBaseID()==NOT_IN_DATABASE)
		SetBaseID(newid);

	db_id_t func_id=NOT_IN_DATABASE;
	if(my_function)
		func_id=my_function->GetBaseID();

	db_id_t ft_id=NOT_IN_DATABASE;
	if(fallthrough)
		ft_id=fallthrough->GetBaseID();

	db_id_t targ_id=NOT_IN_DATABASE;
	if(target)
		targ_id=target->GetBaseID();

	db_id_t indirect_bt_id=NOT_IN_DATABASE;
	if(indTarg)
		indirect_bt_id=indTarg->GetBaseID();

        string q=
		string("insert into ")+vid->instruction_table_name +
                string(" (instruction_id, address_id, parent_function_id, orig_address_id, fallthrough_address_id, target_address_id, data, callback, comment, ind_target_address_id, doip_id) ")+
                string(" VALUES (") +
                string("'") + to_string(GetBaseID())            	+ string("', ") +
                string("'") + to_string(my_address->GetBaseID())   	+ string("', ") +
                string("'") + to_string(func_id)            		+ string("', ") +
                string("'") + to_string(orig_address_id)         	+ string("', ") +
                string("'") + to_string(ft_id)         			+ string("', ") +
                string("'") + to_string(targ_id)         		+ string("', ") +
                string("E'") + pqxx::escape_binary(data) + "'::bytea"   + string(" , ") + // no ticks for this field
											  // also need to append ::bytea
                string("'") + callback                              	+ string("', ") +
                string("'") + comment                              	+ string("', ") +
                string("'") + to_string(indirect_bt_id)                 + string("', ") +
                string("'") + to_string(GetDoipID())            	+ string("') ; ") ;

	return q;
}

