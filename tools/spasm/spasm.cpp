#include "spasm.h"
#include <vector>
#include <regex.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <map>
#include <cstdlib>
#include <cerrno>
#include <climits>
#include <cstring>
#include <assert.h>

#include "ben_lib.h"
#include "beaengine/BeaEngine.h"

#ifdef SPASM64
#define NASM_BIT_WIDTH "BITS 64"
#else
#define NASM_BIT_WIDTH "BITS 32"
#endif

using namespace std;

static string regularAddressRegex = "0x[[:xdigit:]]+";
static string offsetAddressRegex = "[a-zA-Z0-9\\._-]+[[:blank:]]*[+][[:blank:]]*0x[[:xdigit:]]+|[a-zA-Z0-9\\._]+[[:blank:]]*[+][[:blank:]]*[[:xdigit:]]+";

static string allAddressRegex = regularAddressRegex + "|" + offsetAddressRegex;
 
static string commentOnlyRegex = "^[[:blank:]]*(;|#).*$";
static string entryRedirectRegex = "^[[:blank:]]*("+allAddressRegex + ")[[:blank:]]+(->)[[:blank:]]+([.]|[a-zA-Z0-9_]*|" + allAddressRegex + ")[[:blank:]]*((;|#).*)?$";
static string otherRedirectRegex = "^[[:blank:]]*([.]|[a-zA-Z][a-zA-Z0-9_]*)[[:blank:]]+(->)[[:blank:]]+(("+ allAddressRegex + ")|[a-zA-Z][a-zA-Z0-9_]*)[[:blank:]]*((;|#).*)?$";
static string insertRedirectRegex = "^[[:blank:]]*([.]|[a-zA-Z][a-zA-Z0-9_]*)[[:blank:]]+([-][|])[[:blank:]]+("+allAddressRegex + ")[[:blank:]]*((;|#).*)?$";
static string instructionRegex = "^[[:blank:]]*([.]|[a-zA-Z][a-zA-Z0-9_]*)[[:blank:]]+([*][*])[[:blank:]]+.*$";
static string callbackRegex = "^[[:blank:]]*([.]|[a-zA-Z][a-zA-Z0-9_]*)[[:blank:]]+([(][)])[[:blank:]]+.*$";
static string relocRegex = "^[[:blank:]]*([.]|[a-zA-Z][a-zA-Z0-9_]*)[[:blank:]]+([r][l])[[:blank:]]+.*$";

static regex_t coPattern, erPattern, orPattern, irPattern, insPattern, cbPattern, rlPattern;


//TODO: if I am getting rid of the requirement for 0x address prefixes, make sure comments reflec this

typedef struct spasmline {
	string address;
	string op;
	string rhs;	 //represents "right hand side"
	string comment;
	bool commentOnly;
	unsigned int lineNum;
} spasmline_t;

typedef struct bin_instruction {
	string hex_str;
	unsigned int size;
	//char array is not by convention null terminated.
	unsigned char raw_bin[50];
} bin_instruction_t;
		


static unsigned int const ORG_PC = 0xff000000;
//padding is added to the ORG_PC for the first vpc
//the padding amount is [0-PC_PADDING_MAX), i.e., not inclusive of PC_PADDING_MAX
static unsigned int const PC_PADDING_MAX = 8001;
static unsigned int vpc = ORG_PC; 
static map<string,string> symMap; 
static map<string,string> callbackMap; 

static ifstream sl_stream;
static unsigned int sl_line_cnt=0;
 
static void initSpasmLines(const string &inputFile);
static bool getNextSpasmLine(spasmline_t &spasm_line);
static void resetSpasmLines();

static void assemble(const string &assemblyFile);

static unsigned int bin_index =0;
static unsigned int bin_fsize=0;
static unsigned char *memblock = NULL;
static unsigned int assem_cnt =0;

static void initBin(const string &binFile);
static bool getNextBin(bin_instruction_t &bin);

static void printSPRI(const string &symbolFilename, const string &outFile);


//static vector<spasmline_t> getSpasmLines(const string &inputFile);
//static vector<string> getAssembly(const vector<spasmline_t> &lines);
//static void assemble(const vector<string> &assembly, const string &assemblyFile);
static void resolveSymbols(const string &mapFile);
//static vector<bin_instruction_t> parseBin(const string &binFile);
//static vector<string> getSPRI(const vector<bin_instruction_t> &bin, const vector<spasmline_t> &spasmlines, const string &symbolFilename);
//static void printVector(const string &outputFile, const vector<string> &lines);
static int getSymbolAddress(const string &symbolFilename, const string &symbol) throw(exception);

//
// @todo: need to cache results
//
static string getCallbackAddress(const string &symbolFilename, const string &symbol) throw(exception)
{
	char buf[30];
	int diff=getSymbolAddress(symbolFilename, symbol)
		- getSymbolAddress(symbolFilename, "strata_init");
	sprintf(buf,"%x", diff);
	string s(buf);
	return s;
}


static int getSymbolAddress(const string &symbolFilename, const string &symbol) throw(exception)
{

	string symbolFullName = symbolFilename + "+" + symbol;
	map<string,string>::iterator callbackMapIterator;
	if(callbackMap.find(symbolFullName) != callbackMap.end())
	{
		return strtol(callbackMap[symbolFullName].c_str(),NULL,16);
	}

// nm -a stratafier.o.exe | egrep " integer_overflow_detector$" | cut -f1 -d' '
	string command = "nm -a " + symbolFilename + " | egrep \" " + symbol + "$\" | cut -f1 -d' '";
	char* address = new char[128];

	FILE *fp = popen(command.c_str(), "r");

	fscanf(fp,"%s", address);
	string addressString = string(address);

	//TODO: throw exception if address is not found. 
	//for now assert the address string isn't empty
	assert(!addressString.empty());

	pclose(fp);
	delete [] address; 

	callbackMap[symbolFullName] = addressString;

	return strtol(addressString.c_str(),NULL,16);
}

//void a2bspri(const string &input, const string &output, const string &symbolFilename) throw(exception)
void a2bspri(const vector<string> &input, const string &symbolFilename) throw(exception)
{
	srand(time(0));

	vpc += rand()%PC_PADDING_MAX;

	cout<<"VPC init loc: "<<hex<<vpc<<endl;

	for(unsigned int i=0;i<input.size();i++)
	{
		symMap.clear();
	
		initSpasmLines(input[i]);

		assemble(string(input[i]+".asm"));

		initBin(string(input[i]+".asm.bin"));
	
		resolveSymbols(input[i]+".asm.map");

		string output = input[i];
		size_t pos = output.find(".aspri");
		if(pos != string::npos)
			output = output.substr(0,pos);
	
		output += ".bspri";
	

		resetSpasmLines();
		cout<<"Printing spri to file "<<output<<"...";

		printSPRI(symbolFilename,output);

		cout<<"Done!"<<endl;

	}


//TODO: cleanup, I don't currently clean up anything on exit or exception
	//clean memblock, close streams
/*
  regfree(&erPattern);
  regfree(&irPattern);
  regfree(&orPattern);
  regfree(&coPattern);
  regfree(&insPattern);
  regfree(&rlPattern);
*/

}


static void initSpasmLines(const string &inputFile)
{

	sl_line_cnt = 0;
 
#define COMPILE_REGEX(pattern,the_string)								\
	if (regcomp(&pattern, the_string.c_str(), REG_EXTENDED) != 0)		\
	{																	\
		throw SpasmException("ERROR: program bug, regex compilation failure for " #the_string  " in getSpasmLines"); \
	} 

	COMPILE_REGEX(rlPattern,relocRegex);
	COMPILE_REGEX(coPattern,commentOnlyRegex);
	COMPILE_REGEX(erPattern,entryRedirectRegex);
	COMPILE_REGEX(orPattern,otherRedirectRegex);
	COMPILE_REGEX(irPattern,insertRedirectRegex);
	COMPILE_REGEX(insPattern,instructionRegex);
	COMPILE_REGEX(coPattern,commentOnlyRegex);
	COMPILE_REGEX(cbPattern,callbackRegex);


	sl_stream.open(inputFile.c_str());
	 
	if(!sl_stream.is_open())
	{
		throw SpasmException("ERROR: input file " + inputFile + " could not be opened.");			   
	}

}
static void resetSpasmLines()
{
	sl_line_cnt = 0;
	sl_stream.seekg(0,ios::beg);
	sl_stream.clear();
}

static bool getNextSpasmLine(spasmline_t &spasmline)
{

	assert(sl_stream.is_open());
	 
	if(!sl_stream.good())
		return false;

	sl_line_cnt++;

	string line;
	getline(sl_stream,line);
	vector<string> tokens;

	spasmline.address = "";
	spasmline.op = "";
	spasmline.rhs = "";
	spasmline.comment = "";
	spasmline.commentOnly = false;
	spasmline.lineNum = sl_line_cnt;

	regmatch_t pmatch[5];

	trim(line);

	if(line.length() == 0)
		return getNextSpasmLine(spasmline);

	//comment only line check
	if(regexec(&coPattern, line.c_str(), 0, NULL, 0)==0)
	{
		spasmline.commentOnly = true;
		//The comment is the entire line
		spasmline.comment = line;
	}		
	else if(regexec(&erPattern,line.c_str(),5,pmatch,0)==0 || 
			regexec(&orPattern,line.c_str(),5,pmatch,0)==0 ||
			regexec(&irPattern,line.c_str(),5,pmatch,0)==0 || 
			regexec(&insPattern,line.c_str(),5,pmatch,0)==0 || 
			regexec(&cbPattern, line.c_str(),5,pmatch,0)==0 ||
			regexec(&rlPattern, line.c_str(),5,pmatch,0)==0)
	{
		int mlen = pmatch[1].rm_eo - pmatch[1].rm_so;
		spasmline.address = line.substr(pmatch[1].rm_so,mlen);

		mlen = pmatch[2].rm_eo - pmatch[2].rm_so;
		spasmline.op = line.substr(pmatch[2].rm_so,mlen);

		spasmline.rhs = line.substr(pmatch[2].rm_eo);

		//There may be an inline comment, search rhs for ';'and split rhs accordingly
		for(unsigned int i=0;i<spasmline.rhs.length();i++)
		{
			if(spasmline.rhs[i] == ';' || spasmline.rhs[i] == '#')
			{
				spasmline.comment = spasmline.rhs.substr(i);
				//yea I am changing part of the guard in a loop, but I am breaking immediately
				spasmline.rhs = spasmline.rhs.substr(0,i);

				break;
			}
		}

	}
	else
	{
		//TODO: close stream on failure?
		stringstream ss;
		ss<<sl_line_cnt;
		throw SpasmException("ERROR: improperly formatted spasm line at " + ss.str());
	}

	trim(spasmline.comment);
	trim(spasmline.rhs);
	trim(spasmline.op);
	trim(spasmline.address);
  
	return true;
}



//initSpasmLines must be called before assembly.
//Assembly in the spasm lines are placed in the given file. 
//The assembly file is then
//assembled into a raw binary file using the nasm assembler. The produced
//raw binary file is assemblyFile+".bin".
//In addition to the raw binary file, a nasm produced symbol map file is
//generated with the name assemblyFile+".map".
//
//TODO: it is currently assumed the assemblyFile string will have a .asm
//postfix, perhaps a check should be done as I don't think nasm will accept
//other extensions.
//
//[in]	assemblyFile	the file that will hold nasm assembly
//static void assemble(const vector<string> &assembly, const string &assemblyFile)
static void assemble(const string &assemblyFile)
{
	assem_cnt = 0;

	//remove any preexisting assembly or nasm generated files
	string command = "rm -f " + assemblyFile;
	system(command.c_str());
	command = "rm -f "+assemblyFile+".bin";
	system(command.c_str());
	command = "rm -f "+assemblyFile+".map";
	system(command.c_str());


	ofstream asmFile;
	asmFile.open(assemblyFile.c_str());
	if(!asmFile.is_open())
	{
		throw SpasmException("ERROR: Could not create a prelim assembly file for writing");					
	}
	 
	asmFile<<NASM_BIT_WIDTH<<endl;
	asmFile<<"ORG 0x"<<hex<<vpc<<endl;
	asmFile<<"[map symbols "<<assemblyFile<<".map]"<<endl;

	spasmline_t sline;

	while(getNextSpasmLine(sline))
	{
		// skip comments and relocations */
		if(sline.commentOnly || (sline.op.compare("rl")==0))
			continue;

		string assemblyLine = "";
			
		string lineAddr = sline.address;
		string lineOp = sline.op;
		string lineRH = sline.rhs;
			

		//if lineAddr has a plus in it, if so it is an address
		//optimally I would do all these checks with a regex, but
		//hindsight is 20/20

		//If not '.' or an offset address (<base> + <offset>)
		//then the address is a label
		//TODO: I really need to use regex for all checks like this
		if(lineAddr.find("+") == string::npos && lineAddr[0] != '.' && lineAddr[0] != '0')
		{
			if(symMap.find(lineAddr) != symMap.end())
			{
				stringstream ss;
				ss << sline.lineNum;
				cout<<sline.op<<endl;
				throw SpasmException("ERROR: multiple symbolic destination detected for symbol "+lineAddr+ " on line " + ss.str());
			}

			symMap[lineAddr] = "";
			assemblyLine = lineAddr + ": ";
		}
	
		if(lineOp.compare("->")==0)
		{
			//Check if label or .
			//non-entry point redirections require one byte of space. This space is reserved with nop
			if(lineAddr.find("+") == string::npos && lineAddr[0] != '0')
			{
				lineRH = "nop";
			}
			//else this is an entry redirect which takes up no space
			else 
				continue;
		} 
		else if(lineOp.compare("-|")==0)
		{
			//terminating redirects require one byte of space which is reserved with nop
			lineRH = "nop";
		}
		else if(lineOp.compare("()")==0)
		{
			// this is a callback
/*
  assemblyLine = "; ";
  assemblyLine += lineAddr;
  assemblyLine += " () ";
  assemblyLine += " needToResolveAddressFor: ";
*/
			string callback = lineRH;
			lineRH = "nop";
			lineRH += " ;";
			lineRH += callback;
		}

		assemblyLine += lineRH;

		asmFile<<assemblyLine<<endl;
		assem_cnt++;
	}

	asmFile.close();

//TODO: check if system fails, make a func call to handle system
	command = "nasm -O1 -w-number-overflow " + assemblyFile + " -o "+assemblyFile+".bin";
	cout<<"Running nasm ("<<command<<")...";
	system(command.c_str());
	cout<<"Done!"<<endl;

	
	//see if the file was created
	ifstream filetest;
	filetest.open(string(assemblyFile+".bin").c_str());

	if(!filetest.is_open())
	{
		throw SpasmException("Nasm failed to assemble, review error output and " + assemblyFile);
	} 
	filetest.close(); 

}

static void resolveSymbols(const string &mapFile)
{
	ifstream mapFileStream;
	mapFileStream.open(mapFile.c_str());
	
	//If the map file doesn't exist, NASM must have failed since even an empty map
	//is produced if no symbols are present
	if(!mapFileStream.is_open())
	{
		throw SpasmException("ERROR: Nasm map file "+mapFile +" does not exist. Indicates a Nasm failure.");		   
	}  

	cout<<"Resolving Symbols .... ";

	string line;
	vector<string> tokens;
	while(mapFileStream.good())
	{
		tokens.clear();
		getline(mapFileStream,line);
		trim(line);

		if(line.empty())
			continue;

		tokenize(tokens,line);		
		
		if(tokens.size() != 3)
			continue;

		//Assume we are in a symbol table entry if there are three tokens on the line
		//and the first two tokens are hex numbers
		//The first token represents the physical address, the second the virtual address
		//and the third is the symbol.
		char *endptr;
		char *tok_c_str = const_cast<char*>(tokens[0].c_str());
		long long addrval;
		addrval = strtoll(tok_c_str,&endptr,16); 

		if((errno == ERANGE && (addrval == LLONG_MAX || addrval == LLONG_MIN))
		   || ((errno != 0 && addrval == 0) || endptr == tok_c_str))
		{
			continue;
		}

		tok_c_str = const_cast<char*>(tokens[1].c_str());
		addrval = strtoll(tok_c_str,&endptr,16); 

		if((errno == ERANGE && (addrval == LLONG_MAX || addrval == LLONG_MIN))
		   || ((errno != 0 && addrval == 0) || endptr == tok_c_str))
		{
			continue;
		}

		if(symMap.find(tokens[2]) != symMap.end())
		{
			symMap[tokens[2]] = tokens[1];
//			  cout<<"SYMBOL RESOLVED: symbol "<<tokens[2]<<" to address "<<tokens[1]<<endl;
		}
	} 
	cout<<"Done!"<<endl;

	mapFileStream.close();
}

static void initBin(const string &binFile)
{
	ifstream binreader;
	binreader.open(binFile.c_str(),ifstream::in|ifstream::binary);

	if(!binreader.is_open())
	{
		throw SpasmException("ERROR: Nasm bin file "+binFile +" does not exist. Indicates a Nasm failure.");
	}

	binreader.seekg(0,ios::end);

	bin_fsize = binreader.tellg();
   
	binreader.seekg(0,ios::beg);

	memblock = new unsigned char[bin_fsize];

	binreader.read((char*)memblock,bin_fsize);
	binreader.close();

}

static bool hasNextBin()
{
	return bin_index < bin_fsize;
}

static bool getNextBin(bin_instruction_t &bin)
{
	DISASM disasm;
	memset(&disasm, 0, sizeof(DISASM));

	disasm.Options = NasmSyntax +  PrefixedNumeral;
	disasm.Archi = 32;

	if(bin_index >= bin_fsize)
		return false;

	disasm.EIP = (int) &memblock[bin_index];
	int instr_len = Disasm(&disasm);

	bin.size = (unsigned int) instr_len;

	char tempstr[50];
	sprintf(tempstr, "%x",bin.size);
	bin.hex_str = string(tempstr);
	for(unsigned int i=0;i<bin.size;i++)
	{
		bin.raw_bin[i] = memblock[bin_index+i];
		sprintf(tempstr,"%02x",(int)memblock[bin_index+i]);
		bin.hex_str += " " + string(tempstr);
	}

	bin_index += bin.size;
	
	return true;
}


//It is assumed the initSpasmLines and initBin has been called at some point before function entry
static void printSPRI(const string &symbolFilename, const string &outFileName)
{
	unsigned int pop_bin_cnt=0;
	ofstream outFile;
	outFile.open(outFileName.c_str());
	if(!outFile.is_open())
	{
		throw SpasmException("ERROR: could not write to the output file " + outFileName);
	}  

	resetSpasmLines();

	spasmline_t sline;

	while(getNextSpasmLine(sline))
	{
		int incSize = 0;

		string comments = "";
		if(!sline.comment.empty())
		{
			comments = "#";
			comments += sline.comment.substr(1);
		}

		if(sline.commentOnly)
		{
			//The first character is a comment symbol, replace with a spri comment symbol
			//and push the comment
			outFile<<comments<<endl;
			continue;
		}

		stringstream ss;
		ss<<hex<<vpc;

		string vpcstr = ss.str();
		ss.str("");

		string address = sline.address;
		string op = sline.op;
		string rhs = sline.rhs;

		string spriline = "";

		//No non-symbols are allowed as addresses in spasm except for entry points
		//which require no memory space, so push the instruction alone
		//no symbols are allowed on the rhs for these spasm instructions, therefore
		//there is no need to resolve any symbols
		//The assumption is that spasm instruction lines (** ops), do not have
		//actual addresses on the left hand side. 
		// if (is address and not a relocaion) 
		if((address.find("+") != string::npos || address[0] == '0'))
		{
			outFile<<endl;//ensures a space separates spri entry points
			//remove 0x of the address (not necessary but makes all addresses uniform)
			//rhs is replaced with current vpc
			//spriline = address.substr(2)+" "+op+" ";

			spriline = address + " " + op + " ";

			//rhs has a dot symbol
			if(rhs[0] == '.')
				spriline += vpcstr+" ";
			else if(op.compare("rl") == 0 ) 
				spriline += rhs;
			else if(rhs.find("+") != string::npos || rhs[0] == '0')
				spriline += rhs;
			//rhs is a user defined symbol, and must be resolved
			else
			{
				stringstream ss;
				ss <<sline.lineNum;
				assert(op.compare("->")==0 || op.compare("-|")==0);
				if (symMap.find(rhs) == symMap.end())
					throw SpasmException("ERROR: unresolved symbol " + rhs + " for symbol defined on aspri line " + ss.str()); 
		
				spriline += symMap[rhs]+" ";
			}

			spriline += comments;

			outFile<<spriline<<endl;
			continue;
		}

		//If the address is a symbol, replace with resolved symbol address
		//a symbol is not '.' or a <base> + <offset> pattern. At this point
		//we have already weeded out all instructions that use a non-symbolic
		//address so we only check for '.'.
		if(address[0] != '.')
		{
			if(symMap.find(address) == symMap.end())
			{
				stringstream ss;
				ss <<sline.lineNum;

				throw SpasmException("ERROR: unresolved symbol " + address + " for symbol defined on aspri line " + ss.str()); 
			}

			if(comments.empty())
				comments = "#";
			else
				comments += " ; ";

			comments += "src addr = <" + address + ">";

			spriline = symMap[address]+" ";
		}
		//else if '.' use vpc
		else
		{			 
			spriline = string(vpcstr+" ");
		}
		
		spriline += op+" ";

		// handle relocations
		if(op.compare("rl") == 0)
		{
			spriline += rhs;
			spriline += "\t"+comments;
			outFile<<spriline<<endl;
			continue;
		}

		//grabing bin can only happen here since the above "rl" check does not use any assembly
		//checking after results in buffer overrun of bin. It is assumed from this point on
		//that all operators require binary in the bin, whether or not it is actually used 
		//to generate the spri for its corresponding spri line. 

		bin_instruction_t binLine;
		pop_bin_cnt++;
		assert(getNextBin(binLine));

		// handle callback handlers
		if (op.compare("()") == 0)
		{
			incSize = 1;
			string callbackAddress = getCallbackAddress(symbolFilename, rhs);
			if (callbackAddress.empty())
				throw SpasmException(string("ERROR: could not resolve address for callback handler: " + rhs + " in symbol file: " + symbolFilename));		
			spriline += callbackAddress;
		}
		//terminating and non-terminating redirects may have symbols on the right hand side
		//resolve them.
		else if(op.compare("->") == 0 ||  op.compare("-|") ==0)
		{
			//If the current disassembled instruction is not nop, then something is out of sync
			stringstream ss;
			ss <<sline.lineNum;
			if(binLine.hex_str.compare("1 90") !=0)
				throw SpasmException(string("ERROR: Bug detected in getSPRI, bin out of sync with spasm lines. ") +
									 "Expected a place holder nop (1 90) for a SPRI redirect, but found " + binLine.hex_str +". " +
									 "Sync error occurs on line " + ss.str() + " of the SPASM input file");

			//non-entry point redirects require one byte of memory
			incSize = 1;

			if(rhs.find("+") != string::npos || rhs[0] == '0')
				spriline += rhs;
			//else the rhs must be a label
			else 
			{
				if(symMap.find(rhs) == symMap.end())
				{
					stringstream ss;
					ss <<sline.lineNum;
					throw SpasmException("ERROR: unresolved symbol " + rhs + " for symbol referenced on aspri line " + ss.str()); 
				}

				if(comments.empty())
					comments = "#";
				else
					comments += " ; ";
				
				comments += "dest addr = <" + rhs + ">";

				spriline += symMap[rhs]+" ";
			}		
		}
		else
		{
			assert(op.compare("**")==0);
			//Add a comment indicating the assembly used for this instruction
			if(comments.empty())
				comments = "#";
			else
				comments += " ; ";

			comments +=rhs;

			incSize = binLine.size;

			spriline += binLine.hex_str + " ";
		}

		spriline += "\t"+comments;

		outFile<<spriline<<endl;

		vpc += incSize;
	}

	//At this point all binary instructions should have been covered
	//double sanity checks, just in case
	assert(pop_bin_cnt == assem_cnt);
	assert(!hasNextBin());

	outFile.close();
 
}

