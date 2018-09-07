#include <iostream>
#include <assert.h>
#include <set>
#include <algorithm>
#include <getopt.h>
#include <ehp.hpp>
#include <exeio.h>
#include <string>
#include "capstone/capstone.h"
#include <fstream>
#include <elfio/elfio.hpp>
#include <elf.h>


using namespace std;
using namespace EHP;
using namespace EXEIO;
using namespace ELFIO;

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
				bool contains(const Address_t &c) const { return first <= c && c<=second; }
			
		};
		using RangeSet_t = set<Range_t>;
		set < RangeSet_t > sccs;
		map<RangeSet_t,string> funcNames;
		bool verbose;
		exeio_t exeio;
		csh cshandle;
		ofstream outfile;
	public:
		CreateFunctions_t(const string &input_pgm, const string &output_annot, const bool p_verbose)
			: 
			verbose(p_verbose),
			exeio(input_pgm),
			cshandle()
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

			if (cs_open(CS_ARCH_X86, CS_MODE_64, &cshandle) != CS_ERR_OK)
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
			pltSplit<ELFIO::Elf64_Sym>(".plt", ".plt.got");
			// if exeio->elf class == 64-bit
			nameFunctions<ELFIO::Elf64_Sym>();
			// else
			// nameFunctions<Elf32_Rela, Elf32_Rel, Elf32_Sym>();

		}

		template<class T_Sym>
		void nameFunctions()
		{
			// set default names 
			for(const auto &func: sccs)
			{
				assert(func.begin() != func.end());
				const auto first_range=*(func.begin());
				const auto startAddr=first_range.first;
				const auto name=string()+"sub_"+to_string(startAddr);
				if(funcNames[func]=="")	// destructive test OK, next line sets if empty.
					funcNames[func]=name;
			}
	
			// do symbol names.
			parseSyms<T_Sym>(".dynsym", ".dynstr");
			parseSyms<T_Sym>(".symtab", ".strtab");
		}

		template<class T_Sym>
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
				const auto type=ELF64_ST_TYPE(sym->st_info);
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
					auto set1_it=find_if(ALLOF(sccs), [&](const RangeSet_t& s) { return s.find(pair) != s.end(); } );
					assert(set1_it!=sccs.end());

					auto set2_it=find_if(ALLOF(sccs), [&](const RangeSet_t& s) 
						{ 
							return find_if(ALLOF(s), [&](const Range_t& r) { return r.contains(cs->getCallSiteAddress()); }) != s.end(); 
						});
					assert(set2_it!=sccs.end());
					auto set1=*set1_it;
					auto set2=*set2_it;
					sccs.erase(set1);
					sccs.erase(set2);
					auto set3=RangeSet_t();
					set_union(ALLOF(set1), ALLOF(set2), inserter(set3, set3.begin()));
					sccs.insert(set3);
				}
			}


		}

		void addSectionToSccs(const string &sec_name)
		{
			const auto sec=exeio.sections[sec_name];
			const auto range=Range_t(sec->get_address(), sec->get_address()+sec->get_size());
			const auto ranges=RangeSet_t({range});
			sccs.insert(ranges);
		}

		template<class T_Sym>
		void pltSplit(const string &pltSecName, const string &endSecName)
		{
			const auto dynsymSec=exeio.sections[".dynsym"];
			const auto dynstrSec=exeio.sections[".dynstr"];

			const auto addRange=[&](const Address_t s, size_t len)
			{
				if(verbose)
					cout<<"Adding PLT function "<<s<<" "<<len<<endl;
				sccs.insert(RangeSet_t({Range_t({s,s+len})}));
			};

			const auto addName=[&](const Address_t addr, uint64_t dynsymIndex)
			{

				if(!dynsymSec) return;
				if(!dynstrSec) return;

				const auto dynsymData=dynsymSec->get_data();
				const auto dynstrData=dynstrSec->get_data();

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

				// get the name
				const auto name=string(dynstrData+name_offset)+"@plt";

				// find a function 
				auto func_it=find_if(ALLOF(sccs), [&](const RangeSet_t& s) 
					{ 
						return s.begin() -> first == addr;
					});
				if(func_it!=sccs.end())
				{
					cout<<"Setting function at "<<hex<<addr<<" to name "<<name<<endl;
					funcNames[*func_it]=name;
				}
			};

			const auto pltSec=exeio.sections[pltSecName];
			assert(pltSec!=NULL);
			const auto startAddr=pltSec->get_address();
			const auto endAddr=pltSec->get_address()+pltSec->get_size();

			if(verbose)
				cout<<"Found plt function range is "<<hex<<startAddr<<"-"<<endAddr<<endl;

			auto pltRange_it=find_if(ALLOF(sccs), [&](const RangeSet_t& s) 
				{ 
					return find_if(ALLOF(s), [&](const Range_t& r) { return r.contains(startAddr); }) != s.end(); 
				});
			assert(pltRange_it!=sccs.end());
			sccs.erase(pltRange_it);	// invalidates all iterators

			const auto plt_skip=16;
			const auto plt_header_size=12;
			const auto plt_entry_size=16;

			addRange(startAddr,plt_header_size);
			auto dynsymEntryIndex=1;
			for(auto i=startAddr+plt_skip; i<endAddr; i+=plt_skip) 
			{
				addRange(i,plt_entry_size);
				addName(i,dynsymEntryIndex++);
			}

			const auto gotPltSec=exeio.sections[endSecName];
			if(gotPltSec!=NULL)
				addRange(gotPltSec->get_address(),gotPltSec->get_size());
	
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
		
				cout<<"Function "<<dec<<i++<<" is "<<hex<<min.first<<" "<<dec<<max.second-min.first<<endl;
				const auto usefp=getUseFp(scc);

				outfile<<hex<<"\t"<<min.first<<"\t"<<dec<<size<<"\tFUNC GLOBAL\t"<<funcNames[scc]<<" "<< usefp << endl;
				doBelongTos(scc);
			}
		}

		string getUseFp(const RangeSet_t scc)
		{
			assert(scc.begin()!=scc.end());
			const auto startAddr=scc.begin()->first;
			const auto fde=ehp->findFDE(startAddr);
			if(!fde) return "NOFP";
			const auto &ehprogram=fde->getProgram();
			const auto ehprogramInstructions=ehprogram.getInstructions();

			const auto def_cfa_rbp_it = find_if(ALLOF(*ehprogramInstructions), [](const shared_ptr<EHProgramInstruction_t> insn)
				{
					assert(insn);
					const auto &insnBytes=insn->getBytes();
					// 0xd, 0x6 is "def_cfa_register rbp" 
					return insnBytes==EHProgramInstructionByteVector_t({(uint8_t)0xd,(uint8_t)0x6});
				});
			return def_cfa_rbp_it == ehprogramInstructions->end() ?  "NOFP" : "USEFP";
		}
		

};

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
