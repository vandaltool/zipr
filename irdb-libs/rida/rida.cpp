#include <iostream>
#include <sstream>
#include <assert.h>
#include <set>
#include <algorithm>
#include <getopt.h>
#include <ehp.hpp>
#include <exeio.h>
#include <string>
#include "capstone/capstone.h"
#include <fstream>
#include <elf.h>
#include <functional>


using namespace std;
using namespace EHP;
using namespace EXEIO;

#define ALLOF(a) begin(a),end(a)

void usage(int argc, char* argv[])
{
	cout<<"Usage: "<<argv[0]<<" input.exe output.annot>"<<endl;
	exit(1);
}



class CreateFunctions_t
{
	private:
		shared_ptr<const EHFrameParser_t> ehp;
		using Address_t = uint64_t;
		class Range_t : public pair<Address_t,Address_t>
		{
			public:
				Range_t(const Address_t &a, const Address_t &b) : pair<Address_t,Address_t>(a,b) { } 
				bool contains(const Address_t &c) const { return first <= c && c<second; }
		};
		using RangeSet_t = set<Range_t>;
		set < RangeSet_t > sccs;
		map<RangeSet_t,string> funcNames;
		bool verbose;
		exeio_t exeio;
		csh cshandle;
		ofstream outfile;
		execlass_t file_class;
		MachineType_t machine_type;
		friend ostream& operator<<(ostream& os, const CreateFunctions_t::RangeSet_t& rs);
	public:
		CreateFunctions_t(const string &input_pgm, const string &output_annot, const bool p_verbose)
			: 
			verbose(p_verbose),
			exeio(input_pgm),
			cshandle(),
			file_class(exeio.get_class()),
			machine_type(exeio.getMachineType())
		{
			outfile.open(output_annot.c_str(), ofstream::out);
			if(!outfile.is_open())
			{
				cerr<<"Cannot open "<<output_annot<<endl;
				exit(1);
			}
			ehp = EHFrameParser_t::factory(input_pgm);
			if(verbose)
				ehp->print();

			if(file_class!=ELF64 && file_class != ELF32)
			{
				cerr<<"Rida can only process ELF files."<<endl;
				exit(1);
			}

			const auto cs_mode= 
				machine_type==mtAarch64 ? CS_MODE_LITTLE_ENDIAN :
				file_class==ELF64 ? CS_MODE_64 : 
				file_class==ELF32 ? CS_MODE_32 : 
				throw std::runtime_error("Cannot handle ELF class");

			const auto my_cs_arch = 
				machine_type == mtX86_64  ?  CS_ARCH_X86 : 
				machine_type == mtI386    ?  CS_ARCH_X86 :
				machine_type == mtAarch64 ?  CS_ARCH_ARM64 : 
				throw std::runtime_error("Cannot handle architecture");

			if (cs_open(my_cs_arch, cs_mode , &cshandle) != CS_ERR_OK)
			{
				cerr<<"Cannot initialize capstone"<<endl;
				exit(1);
			}
		}
		virtual ~CreateFunctions_t()
		{
			cs_close(&cshandle);
		}


		void calculate()
		{


			ehframeToSccs();
			addSectionToSccs(".init");
			addSectionToSccs(".fini");
	
			if(file_class==ELF64)
			{
				class Extracter64
				{
					public:
						Elf64_Xword  elf_r_sym (Elf64_Xword a) { return ELF64_R_SYM (a); }
						Elf64_Xword  elf_r_type(Elf64_Xword a) { return ELF64_R_TYPE(a); }
						unsigned char  elf_st_bind(unsigned char a) { return ELF64_ST_BIND(a); }
						unsigned char  elf_st_type(unsigned char a) { return ELF64_ST_TYPE(a); }
				};
				pltSplit<Elf64_Sym, Elf64_Rela, Elf64_Rel, Extracter64>(".plt", ".plt.got");
				nameFunctions<Elf64_Sym, Extracter64>();
			}
			else
			{
				class Extracter32
				{
					public:
						Elf32_Word  elf_r_sym (Elf32_Word a) { return ELF32_R_SYM (a); }
						Elf32_Word  elf_r_type(Elf32_Word a) { return ELF32_R_TYPE(a); }
						unsigned char  elf_st_bind(unsigned char a) { return ELF32_ST_BIND(a); }
						unsigned char  elf_st_type(unsigned char a) { return ELF32_ST_TYPE(a); }
				};
				pltSplit<Elf32_Sym, Elf32_Rela, Elf32_Rel, Extracter32>(".plt", ".plt.got");
				nameFunctions<Elf32_Sym, Extracter32>();
			}

		}

		template<class T_Sym, class T_Extracter>
		void nameFunctions()
		{
			// do symbol names.
			parseSyms<T_Sym, T_Extracter>(".dynsym", ".dynstr");
			parseSyms<T_Sym, T_Extracter>(".symtab", ".strtab");

			auto namedFunctions=0U;
			auto unnamedFunctions=0U;
			auto functions=0U;

			// set default names 
			for(const auto &func: sccs)
			{
				assert(func.begin() != func.end());
				const auto first_range=*(func.begin());
				const auto startAddr=first_range.first;
				std::stringstream ss;
				ss << "sub_" << hex << startAddr;
				const auto name = ss.str();

				functions++;
				if(funcNames[func]=="")	// destructive test OK, next line sets if empty.
				{
					unnamedFunctions++;
					funcNames[func]=name;
				}
				else
				{
					namedFunctions++;
				}
					
			}

			cout<<"#ATTRIBUTE functions="<<dec<<functions<<endl;
			cout<<"#ATTRIBUTE named_functions="<<dec<<namedFunctions<<endl;
			cout<<"#ATTRIBUTE uunamed_functions="<<dec<<unnamedFunctions<<endl;
	
		}

		template<class T_Sym, class T_Extracter>
		void parseSyms(const string& secName, const string & stringSecName)
		{
			const auto sec=exeio.sections[secName];
			if(!sec) return;	// err check

			const auto stringSec=exeio.sections[stringSecName];
			if(!stringSec) return; // err check

			const auto data=sec->get_data();
			const auto stringData=stringSec->get_data();

			for(auto i=0U; i+sizeof(T_Sym) <= (size_t)sec->get_size(); i+=sizeof(T_Sym))
			{
				const auto sym=reinterpret_cast<const T_Sym *>(data+i);
				const auto value=sym->st_value;
				if(value==0) 
					continue;

				// works for both ELF64 and ELF32, macros defined the same.
				const auto type=T_Extracter().elf_st_type(sym->st_info);
				if(type!=STT_FUNC) 
					continue;


				// functions with non-zero address at this point.
				const auto name_offset=sym->st_name;
	
				// sanity check string length
				if(name_offset < 0U || name_offset > (size_t)stringSec->get_size())
					continue;

				// get the name
				const auto name=string(stringData+name_offset);
		

				// find a function 
				auto func_it=find_if(ALLOF(sccs), [&](const RangeSet_t& s) 
					{ 
						return s.begin() -> first == value;
					});
				if(func_it!=sccs.end())
				{
					cout<<"Setting function at "<<hex<<value<<" to name "<<name<<endl;
					funcNames[*func_it]=name;
				}

			}
		}

		void ehframeToSccs()
		{
			const auto fdes=ehp->getFDEs();
			for(const auto fde : *fdes) 
				//sccs.insert({ RangeSet_t({fde->getStartAddress(), fde->getEndAddress()})});
				sccs.insert(RangeSet_t({Range_t(fde->getStartAddress(),fde->getEndAddress())}));

			cout<<hex;
			if(getenv("SELF_VALIDATE"))
				assert(fdes->size()>0);

			for(const auto fde : *fdes)
			{
				if(verbose)
					cout<<"Found FDE at : " << fde->getStartAddress() << "-"<<fde->getEndAddress()<<endl;
				auto pair=Range_t(fde->getStartAddress(), fde->getEndAddress());
				const auto lsda=fde->getLSDA();
				assert(lsda);
				const auto callsites=lsda->getCallSites();
				assert(callsites);

				for(const auto cs : *callsites)
				{
					if(verbose)
						cout<<"\tCall site (0x"<<cs->getCallSiteAddress()<<"-"<<cs->getCallSiteEndAddress()
						    <<") with landing pad=0x"<<cs->getLandingPadAddress()<<endl;
					if(cs->getLandingPadAddress()==0x0)
						continue;
					auto set1_it=find_if(ALLOF(sccs), [&](const RangeSet_t& s) { return s.find(pair) != s.end(); } );
					assert(set1_it!=sccs.end());

					auto set2_it=find_if(ALLOF(sccs), [&](const RangeSet_t& s) 
						{ 
							return find_if(ALLOF(s), [&](const Range_t& r) { return r.contains(cs->getLandingPadAddress()); }) != s.end(); 
						});
					assert(set2_it!=sccs.end());
					auto set1=*set1_it;
					auto set2=*set2_it;
					if(set1!=set2)
					{
						sccs.erase(set1);
						sccs.erase(set2);
						auto set3=RangeSet_t();
						if(verbose)
							cout<<"\tMerging: set1="<< hex<< set1 << " and set2="<<set2<<dec<<endl;
						set_union(ALLOF(set1), ALLOF(set2), inserter(set3, set3.begin()));
						sccs.insert(set3);
					}
				}
			}


		}

		void addSectionToSccs(const string &sec_name)
		{
			const auto sec=exeio.sections[sec_name];
			if(sec==nullptr)
				return;
			const auto range=Range_t(sec->get_address(), sec->get_address()+sec->get_size());
			const auto ranges=RangeSet_t({range});
			sccs.insert(ranges);
		}

		template<class T_Sym, class T_Rela, class T_Rel, class T_Extracter>
		void pltSplit(const string &pltSecName, const string &endSecName)
		{
			const auto dynsymSec=exeio.sections[".dynsym"];
			const auto dynstrSec=exeio.sections[".dynstr"];
			const auto relapltSec=exeio.sections[".rela.plt"];
			const auto relpltSec=exeio.sections[".rel.plt"];
			const auto relSec=relapltSec  ? relapltSec : relpltSec;
			const auto relSecEntrySize=relapltSec  ? sizeof(T_Rela) : sizeof(T_Rel);
			

			const auto addRange=[&](const Address_t s, size_t len)
			{
				if(verbose)
					cout<<"Adding PLT function "<<s<<" "<<len<<endl;
				sccs.insert(RangeSet_t({Range_t({s,s+len})}));
			};

			const auto addName=[&](const Address_t addr, uint64_t symIndex)
			{
				if(!dynsymSec) return;
				if(!dynstrSec) return;
				if(!relSec) return;
			
				// get the data out of the plt section.
				const auto relData=relSec->get_data();
				if(symIndex*relSecEntrySize >= (size_t)relSec->get_size()) return;
				const auto relDataAsSymPtr=reinterpret_cast<const T_Rel *>(relData + symIndex*relSecEntrySize);
				const auto &relEntry=*relDataAsSymPtr;

				// calculate index into dynsym, section.
				const auto dynsymIndex=T_Extracter().elf_r_sym(relEntry.r_info);
				const auto dynsymData=dynsymSec->get_data();
				const auto dynstrData=dynstrSec->get_data();

				cout<<dec<<"At entry "<<symIndex<<", reloc entry has dynsym index "<<dynsymIndex<<endl;

				// the index into the .dynsym section for the relocation.
				const auto dynsymDataAsSymPtr=reinterpret_cast<const T_Sym *>(dynsymData);
				if(dynsymIndex*sizeof(T_Sym) >= (size_t)dynsymSec->get_size()) return;

				// get a reference to the dynsym entry.
				const auto &dynsymEntry=dynsymDataAsSymPtr[dynsymIndex];
				// extra where in the string table the name is.
				const auto name_offset=dynsymEntry.st_name;
				
				// sanity check string length
				if(name_offset < 0U || name_offset > (size_t)dynstrSec->get_size())
					return;

				const auto applyName=[&](const string& part, const Address_t myAddr)
					{
						// get the name
						const auto name=string(dynstrData+name_offset)+part+"@plt";

						// find a function 
						auto func_it=find_if(ALLOF(sccs), [&](const RangeSet_t& s) 
							{ 
								return s.begin() -> first == myAddr;
							});
						if(func_it!=sccs.end())
						{
							cout<<"Setting function at "<<hex<<myAddr<<" to name "<<name<<endl;
							funcNames[*func_it]=name;
						}
					};

				applyName("part1", addr);
				applyName("part2", addr+6);
			};

			const auto pltSec=exeio.sections[pltSecName];
			if(pltSec==NULL) return;

			const auto startAddr=pltSec->get_address();
			const auto endAddr=pltSec->get_address()+pltSec->get_size();

			if(verbose)
				cout<<"Found plt function range is "<<hex<<startAddr<<"-"<<endAddr<<endl;

			const auto pltRange_it=find_if(ALLOF(sccs), [&](const RangeSet_t& s) 
				{ 
					return find_if(ALLOF(s), [&](const Range_t& r) { return r.contains(startAddr); }) != s.end(); 
				});
			// erase startAddr if found.
			if(pltRange_it!=sccs.end())
				sccs.erase(pltRange_it);	// invalidates all iterators

			auto dynsymEntryIndex=0;

			const auto handle_x86_plt=[&]()
			{
				const auto plt_skip=16;
				const auto plt_header_size=12;
				const auto plt_entry_size=16;
				const auto plt_entry_size_first_part=6;

				addRange(startAddr,plt_header_size);
				for(auto i=startAddr+plt_skip; i<endAddr; i+=plt_skip) 
				{
					addRange(i,plt_entry_size_first_part);
					addRange(i+6,plt_entry_size-plt_entry_size_first_part);
					addName(i,dynsymEntryIndex++);
				}
			};
			const auto handle_arm_plt=[&]()
			{
				const auto plt_entry_size=16;
				const auto plt_header_size=8*4;

				addRange(startAddr,plt_header_size);
				for(auto i=startAddr+plt_header_size; i<endAddr; i+=plt_entry_size) 
				{
					addRange(i,plt_entry_size);
					addName(i,dynsymEntryIndex++);
				}
			};

			switch(machine_type)
			{
				case mtX86_64:
				case mtI386: 
					handle_x86_plt();
					break;
				case mtAarch64:
					handle_arm_plt();
					break;
				default:
					assert(0);

			};
			cout<<"#ATTRIBUTE plt_entries="<<dec<<dynsymEntryIndex<<endl;


			// deal with gotPlt Section.
			const auto gotPltSec=exeio.sections[endSecName];
			if(gotPltSec==NULL)
				return;


			// both 32- and 64-bit, entries are 6 bytes, with 2 bytes of padding.
			const auto gotPltEntrySize=8;
			const auto gotPltRangeSize=6;
			const auto gotPltStartAddr=gotPltSec->get_address();

			const auto gotPltRange_it=find_if(ALLOF(sccs), [&](const RangeSet_t& s) 
				{ 
					return find_if(ALLOF(s), [&](const Range_t& r) { return r.contains(gotPltStartAddr); }) != s.end(); 
				});
			// erase startAddr if found.
			if(gotPltRange_it!=sccs.end())
				sccs.erase(gotPltRange_it);	// invalidates all iterators

			auto gotpltEntries=0U;
			for(auto i=0U; i + gotPltRangeSize < (size_t)gotPltSec->get_size(); i+=gotPltEntrySize)
			{
				addRange(gotPltStartAddr+i,gotPltRangeSize);
				gotpltEntries++;
			}
			cout<<"#ATTRIBUTE gotplt_entries="<<dec<<gotpltEntries<<endl;
	
		}

		void doBelongTos(const Range_t &range, const Address_t startAddr)
		{
			const auto sec=exeio.sections.findByAddress(range.first);
			assert(sec);
			const auto secEnd=exeio.sections.findByAddress(range.second-1);
			assert(sec==secEnd);	 // same section.
			const auto data=sec->get_data();
			const auto secStartAddr=sec->get_address();
			const auto range_len=range.second-range.first;
			const auto the_code=(const uint8_t*)(data+(range.first-secStartAddr));

			auto insn=(cs_insn *)nullptr;

			const auto count = cs_disasm(cshandle, the_code, range_len, range.first, 0, &insn);
			if (count > 0) 
			{
				for (auto j = 0U; j < count; j++) 
				{
					outfile<<hex<<"\t"<<insn[j].address<<"\t"<<dec<<insn[j].size<<"\tINSTR BELONGTO\t"<<hex<<startAddr<< "\t; "<<insn[j].mnemonic << " " << insn[j].op_str<<endl;
				}

				cs_free(insn, count);
			} 
			else
			{
				cerr<<"ERROR: Failed to disassemble code at "<<range.first<<"-"<<range.second<<endl;
				exit(1);
			}


		}

		void doBelongTos(const RangeSet_t &scc)
		{
			const auto min=*scc.begin();
			const auto startAddr=min.first;

			for(auto range : scc)
				doBelongTos(range,startAddr);

		}

		void writeAnnotations()
		{
			cout<<"The functions are:"<<endl;
			auto i=0;
			for(const auto &scc : sccs)
			{
				const auto min=*scc.begin();
				const auto max=*prev(scc.end());
				const auto size=max.second-min.first;
		
				cout<<"Function "<<dec<<i++<<" (" <<funcNames[scc] << ") is "<<hex<<min.first<<" "<<dec<<max.second-min.first<<endl;
				const auto usefp=getUseFp(scc);

				outfile<<hex<<"\t"<<min.first<<"\t"<<dec<<size<<"\tFUNC GLOBAL\t"<<funcNames[scc]<<" "<< usefp << endl;
				doBelongTos(scc);
			}
			if(getenv("SELF_VALIDATE"))
				assert(sccs.size()>0);
		}

		string getUseFp(const RangeSet_t scc)
		{
			assert(scc.begin()!=scc.end());
			const auto startAddr=scc.begin()->first;
			const auto fde=ehp->findFDE(startAddr);
			if(!fde) return "NOFP";
			const auto &ehprogram=fde->getProgram();
			const auto ehprogramInstructions=ehprogram.getInstructions();

			const auto def_cfa_rbp_it = find_if(ALLOF(*ehprogramInstructions), [&](const shared_ptr<EHProgramInstruction_t> insn)
				{
					assert(insn);
					const auto &insnBytes=insn->getBytes();
					// 0xd, 0x5 is "def_cfa_register ebp" 
					// 0xd, 0x6 is "def_cfa_register rbp" 
					const auto reg=file_class==ELF64 ? (uint8_t)0x6 : (uint8_t)0x5;
					return insnBytes==EHProgramInstructionByteVector_t({(uint8_t)0xd, reg });
				});
			return def_cfa_rbp_it == ehprogramInstructions->end() ?  "NOFP" : "USEFP";
		}
		



};


ostream& operator<<(ostream& os, const CreateFunctions_t::RangeSet_t& rs)
{
	for(const auto r : rs)
	{
		os<<"("<<r.first<<"-"<<r.second<<"), ";
	}
	return os;
}



int main(int argc, char* argv[])
{

        if(argc < 3)
        {
		usage(argc,argv);
                exit(1);
        }
        // Parse some options for the transform
        const static struct option long_options[] = {
                {"verbose", no_argument, 0, 'v'},
                {"help", no_argument, 0, 'h'},
                {"usage", no_argument, 0, '?'},
                {0,0,0,0}
        };
        auto short_opts="vh?";
        auto verbose=false;
	auto index = (int)0;
        while(1) 
	{
                int c = getopt_long(argc, argv,short_opts, long_options, &index);
                if(c == -1)
                        break;
                switch(c) 
		{
                        case 0:
                                break;
                        case 'v':
				verbose=true;
                                break;
                        case '?':
                        case 'h':
                                usage(argc,argv);
                                exit(1);
                                break;
                        default:
                                break;
                }
        }


	if(optind+2 > argc)
	{
		usage(argc,argv);	
		exit(1);
	}

	auto input_pgm=string(argv[optind]);
	auto output_annot=string(argv[optind+1]);
	for(auto i=optind+2 ; i < argc; i++)
	{
		ofstream out(argv[i]);	// touch file
		if(!out.is_open())
		{
			cerr<<"Cannot touch file "<<argv[i]<<endl;
			exit(1);
		}
		
	}

	CreateFunctions_t create_funcs(input_pgm,output_annot,verbose);
	create_funcs.calculate();
	create_funcs.writeAnnotations();


	return 0;
}
