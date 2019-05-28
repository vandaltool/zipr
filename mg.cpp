#include "mg.hpp"


#include <assert.h>
#include <stdexcept>
#include <unistd.h>
#include <memory>
#include <inttypes.h>
#include <algorithm>
#include <elf.h>
#include <cctype>
#include <iomanip>
#include <cstdlib>
#include <random>


using namespace std;
using namespace IRDB_SDK;
using namespace EXEIO;

#define ALLOF(s) begin(s), end(s)



// use this to determine whether a scoop has a given name.
static struct ScoopFinder : binary_function<DataScoop_t*,string,bool>
{
	// declare a simple scoop finder function that finds scoops by name
	bool operator()(const DataScoop_t* scoop, const string word)  const
	{
		return (scoop->getName() == word);
	};
} finder;

template<class S, class T> inline
static bool contains(const S &container, const T& value)
{
	return find(container.begin(), container.end(), value) != container.end();
}



static bool arg_has_memory(const DecodedOperand_t &arg)
{
	/* if it's relative memory, watch out! */
	if(arg.isMemory())
		return true;

	return false;
}

static bool arg_has_relative(const DecodedOperand_t &arg)
{
	/* if it's relative memory, watch out! */
	if(arg.isMemory() && arg.isPcrel())
		return true;
	return false;
}

static DecodedOperandVector_t::iterator find_memory_operand(DecodedOperandVector_t &operands)
{
	// const auto operands=disasm.getOperands();
	auto the_arg=operands.end();
	if(operands.size()>0 && arg_has_memory(*operands[0]))
		the_arg=next(operands.begin(),0);
	if(operands.size()>1 && arg_has_memory(*operands[1]))
		the_arg=next(operands.begin(),1);
	if(operands.size()>2 && arg_has_memory(*operands[2]))
		the_arg=next(operands.begin(),2);
	if(operands.size()>3 && arg_has_memory(*operands[3]))
		the_arg=next(operands.begin(),3);
	return the_arg;
}


template< typename T >
static std::string to_hex_string( T i )
{
	std::stringstream stream;
	stream << "0x"
		<< std::hex << i;
	return stream.str();
}


template <class T_Sym, class  T_Rela, class T_Rel, class T_Dyn, class T_Extractor>
bool MoveGlobals_t<T_Sym,T_Rela,T_Rel,T_Dyn,T_Extractor>::is_elftable(DataScoop_t* ret)
{ 
	return find(ALLOF(elftable_names), ret->getName()) != elftable_names.end() ;  
}; 

template <class T_Sym, class  T_Rela, class T_Rel, class T_Dyn, class T_Extractor>
bool MoveGlobals_t<T_Sym,T_Rela,T_Rel,T_Dyn,T_Extractor>::is_noptr_table(DataScoop_t* ret)
{ 
	return find(ALLOF(elftable_nocodeptr_names), ret->getName()) != elftable_nocodeptr_names.end() ;  
}; 

template <class T_Sym, class  T_Rela, class T_Rel, class T_Dyn, class T_Extractor>
MoveGlobals_t<T_Sym,T_Rela,T_Rel,T_Dyn,T_Extractor>::MoveGlobals_t(
	VariantID_t *p_variantID, 
	FileIR_t *p_variantIR, 
	const string &p_dont_move, 
	const string &p_move_only, 
	const int p_max_mov,
        const bool p_random,
	const bool p_aggressive,
	const bool p_use_stars)
	:
	Transform_t(p_variantIR),
	exe_reader(NULL),
	tied_unpinned(0),
	tied_pinned(0),
	tied_nochange(0),
	ties_for_folded_constants(0),
	dont_move(p_dont_move),
	move_only(p_move_only),
	max_moveables(p_max_mov),
        random(p_random),
	aggressive(p_aggressive),
	m_use_stars(p_use_stars)

{

}

#if 0
template <class T_Sym, class  T_Rela, class T_Rel, class T_Dyn, class T_Extractor>
MEDS_Annotations_t& MoveGlobals_t<T_Sym, T_Rela, T_Rel, T_Dyn, T_Extractor>::getAnnotations()
{
	assert(m_use_stars);
	return m_annotationParser->getAnnotations();
}
#endif

template <class T_Sym, class  T_Rela, class T_Rel, class T_Dyn, class T_Extractor>
int MoveGlobals_t<T_Sym,T_Rela,T_Rel,T_Dyn,T_Extractor>::execute(pqxxDB_t &pqxx_interface)
{

	// read the executeable file

	// load the executable.
	exe_reader = new EXEIO::exeio;
	assert(exe_reader);
	exe_reader->load((char*)"a.ncexe");

	if(m_use_stars)
	{
		auto deep_analysis=DeepAnalysis_t::factory(getFileIR(), aeSTARS,  {"SetDeepLoopAnalyses=true", "SetConstantPropagation=true"});
		deep_global_static_ranges = deep_analysis -> getStaticGlobalRanges();
		sentinels                 = deep_analysis -> getRangeSentinels();
		cout<<dec;
		cout<<"#ATTRIBUTE "<<deep_global_static_ranges->size() <<" num_global_static_range_annotations" <<endl;
		cout<<"#ATTRIBUTE "<<sentinels->size()                 <<" num_sentinel_annotations"            <<endl;
	}




	ParseSyms(exe_reader);
	SetupScoopMap();
	FilterScoops();
	TieScoops();
	FindInstructionReferences();	// may record some scoops are tied together
	FindDataReferences();
	FilterAndCoalesceTiedScoops();
	UpdateScoopLocations();
	PrintStats();

	return 0;
}

// go through the .symtab and .dynsym bits of the table and make scoops for each symbol.
template <class T_Sym, class  T_Rela, class T_Rel, class T_Dyn, class T_Extractor>
void MoveGlobals_t<T_Sym,T_Rela,T_Rel,T_Dyn,T_Extractor>::SetupScoopMap()
{
	for(auto &s : getFileIR()->getDataScoops())
	{
		RangePair_t p(s->getStart()->getVirtualOffset(), s->getEnd()->getVirtualOffset());
		scoop_map[p]=s;
	}
}

template <class T_Sym, class  T_Rela, class T_Rel, class T_Dyn, class T_Extractor>
DataScoop_t* MoveGlobals_t<T_Sym,T_Rela,T_Rel,T_Dyn,T_Extractor>::findScoopByAddress(const IRDB_SDK::VirtualOffset_t a) const
{
	RangePair_t p(a,a);
	auto smit=scoop_map.find(p);
	if(smit==scoop_map.end())
		return NULL;
	return smit->second;
}

template <class T_Sym, class  T_Rela, class T_Rel, class T_Dyn, class T_Extractor>
bool MoveGlobals_t<T_Sym, T_Rela, T_Rel, T_Dyn, T_Extractor>::AreScoopsAdjacent(const DataScoop_t *a, const DataScoop_t *b) const
{
	bool adjacent = true;
	const IRDB_SDK::VirtualOffset_t aStart = a->getStart()->getVirtualOffset();
	const IRDB_SDK::VirtualOffset_t aEnd = a->getEnd()->getVirtualOffset();
	const IRDB_SDK::VirtualOffset_t bStart = b->getStart()->getVirtualOffset();
	const IRDB_SDK::VirtualOffset_t bEnd = b->getEnd()->getVirtualOffset();
	IRDB_SDK::VirtualOffset_t FirstEnd, SecondStart;
	if (aStart > bStart)
	{
		FirstEnd = bEnd;
		SecondStart = aStart;
	}
	else 
	{
		FirstEnd = aEnd;
		SecondStart = bStart;
	}
	for (IRDB_SDK::VirtualOffset_t i = FirstEnd + 1; adjacent && (i < SecondStart); ++i)
	{
		DataScoop_t *c = findScoopByAddress(i);
		if (c)
		{
			adjacent = false; // found intervening scoop before SecondStart
		}		
	}

	return adjacent;
} // end of AreScoopsAdjacent()

// go through the .symtab and .dynsym bits of the table and make scoops for each symbol.
template <class T_Sym, class  T_Rela, class T_Rel, class T_Dyn, class T_Extractor>
void MoveGlobals_t<T_Sym,T_Rela,T_Rel,T_Dyn,T_Extractor>::ParseSyms(EXEIO::exeio * readerp)
{

	auto max_id=getFileIR()->getMaxBaseID();

	if(getenv("MG_VERBOSE"))
		cout<<"Initial scoops:"<<endl;
	for(const auto &scoop : getFileIR()->getDataScoops())
	{
		if(getenv("MG_VERBOSE"))
		{
			cout<<"scoop: "<<scoop->getName()<<" ("<<hex<<scoop->getStart()->getVirtualOffset()
				<<"-"<<scoop->getEnd()->getVirtualOffset()<<")"<<endl;
		}



		const auto moveable_sections=set<string>({ 
						".interp",
						".note.ABI-tag",
						".note.gnu.build-id",
						".gnu.hash",
						".dynsym",
						".dynstr",
						".gnu.version",
						".gnu.version_r",
						".rel.dyn",
						".rel.plt",
						".rela.dyn",
						".rela.plt",
						".init_array",
						".fini_array",
						".jcr",
						".dynamic",
						".got",
						".got.plt"
						});
		// white list some scoops as moveable, despite the symbol table
		if(moveable_sections.find(scoop->getName())!=moveable_sections.end()) 
		{
			cout<<"Register scoop "<<scoop->getName()<<" as movable"<<endl;
			moveable_scoops.insert(scoop);
		}
	}

	assert(readerp);
	auto elfiop=reinterpret_cast<ELFIO::elfio*>(readerp->get_elfio());
	assert(elfiop);
	auto &reader=*elfiop;

	auto splits=0u;

	// for each section in the elf file.
	auto n = (Elf_Half) reader.sections.size();
	for ( auto i = (Elf_Half ) 0; i < n; ++i ) 
	{
		// For all sections
		auto sec = reader.sections[i];
		char* max_splits = getenv("MG_MAX_SPLITS");

		// if it's a symtab section
		if ( SHT_SYMTAB == sec->get_type() || SHT_DYNSYM == sec->get_type() ) 
		{
			auto symbols = ELFIO::symbol_section_accessor ( reader, sec );

			// for each symbol in the section
			auto sym_no = symbols.get_symbols_num();
			for (auto i = (decltype(sym_no))0; i < sym_no; ++i ) 
			{
				// check to see if we've been directed to not split everything up.
				if (max_splits && (splits >= strtoul(max_splits, NULL, 0)))
					break;

				auto name=std::string();
				auto value=(Elf64_Addr)0;	// note:  elf64_addr OK for 32-bit machines still.
				auto size=(Elf_Xword)0;
				auto bind=(unsigned char)0;
				auto type=(unsigned char)0;
				auto section=(Elf_Half)0;
				auto other=(unsigned char)0;

				// elfio always takes a value of type Elf64-Addr regardless of mach type.
				symbols.get_symbol( i, name, value, size, bind, type, section, other );

				// if it's a symbol that describes an object (as opposed to a binding, or a function or a ...)
				if(type==STT_OBJECT && (bind==STB_LOCAL || bind==STB_GLOBAL) && value!=0 && size!=0)
				{
					auto tosplit=getFileIR()->findScoop(value);	

					// something went wrong if we can't find the scoop for this object.
					if(tosplit==NULL) continue;

					cout << "Section: "<<sec->get_name() << " name="<<  name << " size="
						 <<hex<<size<< " addr="<<hex<<value<<" scoop: "<<tosplit->getName()<<endl;

					auto before=(DataScoop_t*)NULL, containing=(DataScoop_t*)NULL, after=(DataScoop_t*)NULL;

					if(getenv("MG_VERBOSE"))
					{
						cout<<"\ttosplit: "<<hex<<tosplit->getStart()->getVirtualOffset()<<"-"
							<<tosplit->getEnd()->getVirtualOffset();
					}
	
					if(value+size-1 > tosplit->getEnd()->getVirtualOffset())
					{
						cout<<"Skipping symbol "<<name<<" due to an object that's already split?"<<endl;
						cout<<"Start (but not end) of "<<name<<" is in in object " <<
							tosplit->getName()<<":("<<hex<<tosplit->getStart()->getVirtualOffset()<<"-" <<
							tosplit->getEnd()->getVirtualOffset()<<")"<<endl;;
						continue; // try next symbol
					}

					if(moveable_scoops.find(tosplit)!=end(moveable_scoops))
					{
						cout<<"Avoiding resplit of "<<name<<" due to an object that's already split?"<<endl;
						// don't re-split something that's arlready moveable.	
						continue;
					}

					getFileIR()->splitScoop(tosplit, value, size, before,containing,after,&max_id);

					if(getenv("MG_VERBOSE"))
					{
						if(before)
						{
							cout<<"\tBefore: "<<hex<<before->getStart()->getVirtualOffset()
								<<"-"<<before->getEnd()->getVirtualOffset();
						}
						cout<<"\tContaining: "<<hex<<containing->getStart()->getVirtualOffset()
							<<"-"<<containing->getEnd()->getVirtualOffset();
						if(after)
						{
							cout<<"\tAfter: "<<hex<<after->getStart()->getVirtualOffset()
								<<"-"<<after->getEnd()->getVirtualOffset();
						}
						cout<<endl;
					}

					assert(containing);
					containing->setName(name);
					moveable_scoops.insert(containing);

					splits++;
					

				}
			}
			cout << std::endl;
		}

	}

        // guarantee unique scoop names
        auto scoop_names=set<string>();
        for(auto & s : getFileIR()->getDataScoops())
        {
                while(scoop_names.find(s->getName())!=scoop_names.end())
                {
                        cout<<"Rename scoop because of name conflict: "<<s->getName()<<" --> ";
                        s->setName(s->getName()+"-renamed"+to_string(rand()));
                        cout<<s->getName()<<endl;
                }
                scoop_names.insert(s->getName());
        }

	cout<<"# ATTRIBUTE Non-Overlapping_Globals::data_scoop_splits_performed="<<dec<<splits<<endl;
}

template <class T_Sym, class  T_Rela, class T_Rel, class T_Dyn, class T_Extractor>
void MoveGlobals_t<T_Sym,T_Rela,T_Rel,T_Dyn,T_Extractor>::FilterScoops()
{
	const auto mg_env = getenv("MG_VERBOSE");


	// filter using the move_only option
	DataScoopSet_t move_only_scoops;	
	// for each word in move_only
	istringstream mo_ss(move_only);
	for_each(istream_iterator<string>(mo_ss),
		istream_iterator<string>(), [&](const string & word)
	{
		// find the scoop
		auto it=find_if(ALLOF(moveable_scoops), bind2nd(finder, word));
		// if found, insert into the move_only set.
		if(it!=moveable_scoops.end())
		{
			if(mg_env)
				cout<<"Keeping scoop (for mo_ss) "<< word << endl;
			move_only_scoops.insert(*it);
		}
		else
		{
			if(mg_env)
				cout<<"Skipping scoop (for mo_ss) "<< word << endl;
		}
		
	});

	// update the moveable_scoops based on the move_only set.
	if(move_only != "" )
	{
		moveable_scoops.clear();
		moveable_scoops.insert(ALLOF(move_only_scoops));

		if(mg_env)
		{
			cout<<"Moveable Scoops after move_only filter:"<<endl;
			for(auto &s : moveable_scoops)
				cout<<s->getName()<<endl;
			cout<<endl;

		}
	}


	// filter based on the dont_move option
	// for each word in dont_move
	istringstream dm_ss(dont_move);
	for_each(istream_iterator<string>(dm_ss),
		istream_iterator<string>(), [&](const string & word)
	{
		// find scoop by that name.
		auto it=find_if(ALLOF(moveable_scoops), bind2nd(finder,word));
		if(it!=moveable_scoops.end())
		{
			moveable_scoops.erase(*it);
		}
		
	});
	if(dont_move!="")
	{
		if(getenv("MG_VERBOSE"))
		{
			cout<<"Moveable Scoops after dont_move filter:"<<endl;
			for(auto &s : moveable_scoops)
				cout<<s->getName()<<endl;
			cout<<endl;

		}
	}

	if(max_moveables>0)
	{
                mt19937 generator(time(0));
                uniform_real_distribution<double> distribution(0.0,1.0);
		while(moveable_scoops.size() > (unsigned)max_moveables)
		{
			if (random == true)
			{
				double rand_num = distribution(generator);
				int rand_idx = (int) (rand_num * moveable_scoops.size());
				auto it = moveable_scoops.begin();
				advance(it, rand_idx);
				moveable_scoops.erase(it);
			}
			else 
				moveable_scoops.erase(prev(moveable_scoops.end()));
		}
 	}
}

template <class T_Sym, class  T_Rela, class T_Rel, class T_Dyn, class T_Extractor>
void MoveGlobals_t<T_Sym,T_Rela,T_Rel,T_Dyn,T_Extractor>::TieScoops()
{
	struct scoop_pairs_t 
	{
		string first, second;
	}scoop_pairs[] = {
		{ ".rel.dyn", ".rel.plt" }, // the dynamic linker goes through both sections together when LD_BIND_NOW is set.  
		{ ".rela.dyn", ".rela.plt" }
// can't tie .got and .got.plt because of relro differences.
// can make insanity happen.
//		{ ".got", ".got.plt" }
	};

	for_each(ALLOF(scoop_pairs), [this](const scoop_pairs_t pair)
	{
		auto it1=find_if(ALLOF(moveable_scoops), bind2nd(finder,pair.first));
		auto it2=find_if(ALLOF(moveable_scoops), bind2nd(finder,pair.second));

		// both exist, tie together.
		if(it1!=moveable_scoops.end() && it2!=moveable_scoops.end())
			tied_scoops.insert(ScoopPair_t(*it1,*it2));

		// first exists, rename for easier management later.
		else if(it1!=moveable_scoops.end() && it2==moveable_scoops.end())
			(*it1)->setName(pair.first+" coalesced w/"+ pair.second);

		// second exists, rename for easier management later.
		else if(it1==moveable_scoops.end() && it2!=moveable_scoops.end())
			(*it2)->setName(pair.first+" coalesced w/"+ pair.second);

		// or, none exists at all.
	});
}


template <class T_Sym, class  T_Rela, class T_Rel, class T_Dyn, class T_Extractor>
void MoveGlobals_t<T_Sym,T_Rela,T_Rel,T_Dyn,T_Extractor>::HandleMemoryOperand(DecodedInstruction_t& disasm, const DecodedOperandVector_t::iterator the_arg, Instruction_t* insn, const DecodedOperandVector_t &the_arg_container)
{
	// no mem arg.
	if(the_arg==the_arg_container.end())
	{
		if(getenv("MG_VERBOSE"))
		{
			cout << "Note:  "<<hex<<" no memory op in:";
			cout << insn->getBaseID()<<":"<<disasm.getDisassembly();
			cout << endl;
		}
		return;
	}

	// shared objects don't need this, you have to use a pcrel addressing mode.
	if(!arg_has_relative(**the_arg) && exe_reader->isDLL())
	{
		if(getenv("MG_VERBOSE"))
		{
			cout << "Note:  "<<hex<<" no dll-style address in:";
			cout << insn->getBaseID()<<":"<<disasm.getDisassembly();
			cout << endl;
		}
		return;
	}

	const auto small_memory_threshold= exe_reader->isDLL() ? 10 : 4096*10;

	auto to1 = (DataScoop_t*) NULL;
	// examine the memory operation to see if there's a pc-rel
	if ((*the_arg)->isMemory() && 
	    (*the_arg)->hasMemoryDisplacement() && 
	    (*the_arg)->getMemoryDisplacementEncodingSize() == 4

	   )
	{
		auto rel_addr1 = (VirtualOffset_t)(*the_arg)->getMemoryDisplacement() /*Memory.Displacement*/;
		if (arg_has_relative(*(*the_arg)))
			rel_addr1 += insn->getDataBits().size();
		to1 = DetectProperScoop(disasm, the_arg, insn, rel_addr1, false, the_arg_container);

		auto disp_offset = disasm.getMemoryDisplacementOffset(the_arg->get(),insn); // the_arg->Memory.DisplacementAddr-disasm.EIP;
		auto disp_size = (*the_arg)->getMemoryDisplacementEncodingSize(); // the_arg->Memory.DisplacementSize;
		assert((0 < disp_offset) && (disp_offset <= (insn->getDataBits().size() - disp_size)));

		// skip if not found, executable, or not moveable.
		if (to1 && (to1->isExecuteable() || moveable_scoops.find(to1) == moveable_scoops.end())) 	  
		{  
			// do nothing, no log or action is necessary for pointers to code.
			if(getenv("MG_VERBOSE"))
			{
				cout<<"Skipping (scoop exists, but exe scoop, or not moveable scoop) pcrel mem op in insn: "
					<< hex << insn->getBaseID()<<":"<<disasm.getDisassembly()<<" to "
					<< to1->getName()<<" ("
					<<hex<<to1->getStart()->getVirtualOffset()<<"-" 
					<<hex<<to1->getEnd()->getVirtualOffset()<<")"<<endl; 
			}
		}
		else if(to1)
		{

			// look for any pcrel relative relocs from fix_calls
			Relocation_t* pcrel_reloc=FindRelocationWithType(insn,"pcrel");
			if(pcrel_reloc)
			{
				if(getenv("MG_VERBOSE"))
				{
					cout<<"Setting pcrel mem op in insn: "
						<< hex <<insn->getBaseID()<<":"<<disasm.getDisassembly()<<" to "
						<< to1->getName()<<" ("
						<<hex<<to1->getStart()->getVirtualOffset()<<"-" 
						<<hex<<to1->getEnd()->getVirtualOffset()<<")"<<endl; 
				}
				//ApplyPcrelMemoryRelocation(insn,to1);
				pcrel_refs_to_scoops.insert({insn,to1});
			}
			else 
			{
				if(getenv("MG_VERBOSE"))
				{
					cout<<"Absolute mem-op to scoop in insn: "
						<< hex << insn->getBaseID()<<":"<<disasm.getDisassembly()<<" to "
						<< to1->getName()<<" ("
						<<hex<<to1->getStart()->getVirtualOffset()<<"-" 
						<<hex<<to1->getEnd()->getVirtualOffset()<<")"<<endl; 
				}
				//ApplyAbsoluteMemoryRelocation(insn,to1);
				absolute_refs_to_scoops.insert({insn,to1});
			}
		}
		else if ( -small_memory_threshold < (int)rel_addr1 && (int)rel_addr1 < small_memory_threshold )
		{
			if((0 != rel_addr1) && getenv("MG_VERBOSE"))
			{
				cout << "Note:  "<<hex<<rel_addr1<<" not declared address in (low addr thresh) :";
				cout << insn->getBaseID()<<":"<<disasm.getDisassembly();
				cout << endl;
			}
		}
		else 
		{
			if ((0 != rel_addr1) && getenv("MG_VERBOSE"))
			{
				cout << "Note:  "<<hex<<rel_addr1<<" not declared address in (no scoop):";
				cout << insn->getBaseID()<<":"<<disasm.getDisassembly();
				cout << endl;
			}
		}
	}
	else
	{
		if(getenv("MG_VERBOSE"))
		{
			cout << "Note:  "<<hex<<" no address in:";
			cout << insn->getBaseID()<<":"<<disasm.getDisassembly();
			cout << endl;
		}
	}
}

template <class T_Sym, class  T_Rela, class T_Rel, class T_Dyn, class T_Extractor>
void MoveGlobals_t<T_Sym,T_Rela,T_Rel,T_Dyn,T_Extractor>::ApplyPcrelMemoryRelocation(Instruction_t* insn, DataScoop_t* to)
{
	const auto disasmp=DecodedInstruction_t::factory(insn);
	const auto &disasm=*disasmp;
	auto operands=disasm.getOperands();

#if 1 
	// don't change instructions that reference re-pinned scoops.
	// This was necessary because we were not getting the zipr_unpin_plugin
	//  to undo our changes to the instruction in the case of a re-pinned scoop.
	//  That problem is fixed, but it is more efficient and safer to
	//  avoid editing instructions that reference re-pinned scoops.
	if (moveable_scoops.find(to) == moveable_scoops.cend()) {
		if (getenv("MG_VERBOSE")) {
			cout << "Avoiding editing of insn at " << hex << insn->getBaseID() << " after repinning scoop "
				<< to->getName() << endl;
		}
		return;
	}
#endif

	auto the_arg=find_memory_operand(operands);
	assert(the_arg!=operands.end());
	unsigned int disp_offset=disasm.getMemoryDisplacementOffset(the_arg->get(),insn)/*the_arg->Memory.DisplacementAddr-disasm.EIP*/;
	unsigned int disp_size=(*the_arg)->getMemoryDisplacementEncodingSize() /*the_arg->Memory.DisplacementSize*/;
	Relocation_t* pcrel_reloc=FindRelocationWithType(insn,"pcrel");
	pcrel_reloc->setWRT(to);	
// note about this case:  the pcrel reloc already exists for the 
// case where an instruction is moving.  
// now the relocs WRT field indicates that the target might move too.
// will have to edit push_relocs.zpi to handle this.
	assert(0<disp_offset && disp_offset<=(insn->getDataBits().size() - disp_size));
	assert(disp_size==4);
	unsigned int new_disp=(*the_arg)->getMemoryDisplacement() /*the_arg->Memory.Displacement*/ - to->getStart()->getVirtualOffset();
	insn->setDataBits(insn->getDataBits().replace(disp_offset, disp_size, (char*)&new_disp, disp_size));
}

template <class T_Sym, class  T_Rela, class T_Rel, class T_Dyn, class T_Extractor>
void MoveGlobals_t<T_Sym,T_Rela,T_Rel,T_Dyn,T_Extractor>::ApplyAbsoluteMemoryRelocation(Instruction_t* insn, DataScoop_t* to)
{
	//DISASM disasm;
	//Disassemble(insn,disasm);
	const auto disasmp=DecodedInstruction_t::factory(insn);
	const auto &disasm=*disasmp;
	auto operands=disasm.getOperands();

#if 1 
	// don't change instructions that reference re-pinned scoops.
	// This was necessary because we were not getting the zipr_unpin_plugin
	//  to undo our changes to the instruction in the case of a re-pinned scoop.
	//  That problem is fixed, but it is more efficient and safer to
	//  avoid editing instructions that reference re-pinned scoops.
	if (moveable_scoops.find(to) == moveable_scoops.cend()) {
		if (getenv("MG_VERBOSE")) {
			cout << "Avoiding editing of insn at " << hex << insn->getBaseID() << " after repinning scoop "
				<< to->getName() << endl;
		}
		return;
	}
#endif

	auto the_arg = find_memory_operand(operands);
	unsigned int disp_offset=disasm.getMemoryDisplacementOffset(the_arg->get(),insn) /*the_arg->Memory.DisplacementAddr-disasm.EIP*/;
	unsigned int disp_size=(*the_arg)->getMemoryDisplacementEncodingSize() /*the_arg->Memory.DisplacementSize*/;
	assert(0<disp_offset && disp_offset<=insn->getDataBits().size() - disp_size);
	auto reloc=getFileIR()->addNewRelocation(insn,0, "absoluteptr_to_scoop",to);
	(void)reloc; // just giving to the ir

	assert(0<disp_offset && disp_offset<=(insn->getDataBits().size() - disp_size));
	assert(disp_size==4);
	unsigned int new_disp=(*the_arg)->getMemoryDisplacement() /*the_arg->Memory.Displacement*/ - to->getStart()->getVirtualOffset();
	insn->setDataBits(insn->getDataBits().replace(disp_offset, disp_size, (char*)&new_disp, disp_size));
}

// See if STARS analyzed the instruction and determined which scoop it references.
template <class T_Sym, class  T_Rela, class T_Rel, class T_Dyn, class T_Extractor>
DataScoop_t* MoveGlobals_t<T_Sym, T_Rela, T_Rel, T_Dyn, T_Extractor>::DetectAnnotationScoop(Instruction_t* insn)
{
	if (!m_use_stars)
		return nullptr;

	const auto dgsr_it     = deep_global_static_ranges->find(insn);
       	const auto dgsr_found  = dgsr_it != deep_global_static_ranges->end();
	const auto sentinel_it = sentinels->find(insn);
	const auto is_sentinel = sentinel_it != sentinels->end();
	
	auto ReferencedScoop = (DataScoop_t*)nullptr;
	if(dgsr_found && is_sentinel)
	{
		const auto  StartAddr = dgsr_it->second;
		ReferencedScoop = findScoopByAddress(StartAddr);
	}
	return ReferencedScoop;
} // end of DetectAnnotationScoop()

template <class T_Sym, class  T_Rela, class T_Rel, class T_Dyn, class T_Extractor>
DataScoop_t* MoveGlobals_t<T_Sym,T_Rela,T_Rel,T_Dyn,T_Extractor>::DetectProperScoop(const DecodedInstruction_t& disasm, const DecodedOperandVector_t::iterator the_arg, Instruction_t* insn, VirtualOffset_t insn_addr, bool immed, const DecodedOperandVector_t &the_arg_container)
{
	assert(insn);
	assert(immed || (the_arg != the_arg_container.end()));	// immeds don't need an argument, but memory ops do.

	if (immed && (0 == insn_addr))
		return NULL; // immed value of zero is not a scoop address

	const auto small_memory_threshold = exe_reader->isDLL() ? 10 : 4096 * 10;
	const auto ValidImmed             = immed && (small_memory_threshold <= ((int)insn_addr));

	auto ret = findScoopByAddress(insn_addr);

	// so far, we haven't run into any problems with not finding a scoop.  we could later.
	if (!ret)
	{
		// check for things that _just_ run off the end of a scoop.
		for (auto i = 0; (i < 8) && (ret == NULL); i++)
			ret = findScoopByAddress(insn_addr - i);	
		// check for things that just miss the beginning of a scoop 
		for (auto i = 0; (i < 8) && (ret == NULL); i++)
			ret = findScoopByAddress(insn_addr + i);	
	}
	
	// See if STARS analyzed the instruction and determined which scoop it references.
	const auto retSTARS = (immed && (!ValidImmed)) ? (DataScoop_t*)nullptr : DetectAnnotationScoop(insn);

	if (!ret)
	{
		if (nullptr != retSTARS)
		{
			cout << "Detected proper scoop using annotation, not using after DetectProperScoop failure for insn at " << hex << insn->getBaseID() << endl;
		}
		return ret;
	}
	

	/* check to see if it's directly pointing at an elftable that isn't allowed to have pointers */
	if (is_noptr_table(ret))
	{
		/* it's an elftable, so we don't need to look so hard because */
		/* we probably aren't pointing to an elf table from an instruction */
		/* find middle of table */
		const auto mid_of_table = (ret->getStart()->getVirtualOffset() / 2) + (ret->getEnd()->getVirtualOffset() / 2);

		/* look forward if above middle, else look backwards */
		const auto op = (insn_addr < mid_of_table)
			? [](const VirtualOffset_t i, const VirtualOffset_t j) { return i - j; }
			: [](const VirtualOffset_t i, const VirtualOffset_t j) { return i + j; }
			;

		/* start at begin/end of table depending on direction */
		const auto addr = (insn_addr < mid_of_table)
			? ret->getStart()->getVirtualOffset()
			: ret->getEnd()->getVirtualOffset()
			;

		/* scan 128 bytes looking for a relevant scoop */
		const auto thres = 128;
		for (auto i = 1; i < thres; i++)
		{
			/* check what's here */
			auto candidate = findScoopByAddress(op(addr, i));
			if (candidate != NULL)
				return candidate;
		}
		/* didn't find anything */
	} /* if elftable */

	/* Not an elf table use conservative and/or aggressive heuristics*/
	ret = DetectProperScoop_ConsiderEndOfPrev(disasm, the_arg, insn, insn_addr, immed, ret, the_arg_container);

	if (!aggressive)
		ret = DetectProperScoop_ConsiderStartOfNext(disasm, the_arg, insn, insn_addr, immed, ret, the_arg_container);

	if (nullptr != retSTARS)
	{
		if (nullptr == ret)
		{
			// ret = retSTARS; // Dangerous to use; e.g. mov [rdi+0x200],rax will cause edit of 0x200 because RDI was resolved by STARS to a scoop address
			cout << "Detected proper scoop using annotation, not using after DetectProperScoop final failure for insn at " << hex << insn->getBaseID() << endl;
		}
		else if (retSTARS != ret)
		{
			// We have two different non-null choices. We will tie the two scoops
			//  together if they are adjacent, and pin them both otherwise.
			if (AreScoopsAdjacent(ret, retSTARS)) // tie adjacent scoops
			{
				cout << "Tieing adjacent scoops due to STARS vs. DetectProperScoop conflict for insn at " << hex << insn->getBaseID() << endl;
				if (ret->getStart()->getVirtualOffset() < retSTARS->getStart()->getVirtualOffset()) 
					tied_scoops.insert({ret, retSTARS});
				else 
					tied_scoops.insert({retSTARS, ret});
			}
			else // not adjacent; must pin
			{
				cout << "Pinning non-adjacent scoops due to STARS vs. DetectProperScoop conflict for insn at " << hex << insn->getBaseID() << endl;
				if(!is_elftable(ret)) 
					moveable_scoops.erase(ret);
				if(!is_elftable(retSTARS)) 
					moveable_scoops.erase(retSTARS);
			}
		}

	}
	return ret;
} // end of DetectProperScoop()

template <class T_Sym, class  T_Rela, class T_Rel, class T_Dyn, class T_Extractor>
DataScoop_t* MoveGlobals_t<T_Sym,T_Rela,T_Rel,T_Dyn,T_Extractor>::DetectProperScoop_ConsiderStartOfNext(
	const DecodedInstruction_t& disasm, 	
	const DecodedOperandVector_t::iterator mem_arg, 	
	Instruction_t* insn, 	
	VirtualOffset_t insn_addr, 	
	bool immed, 	
	DataScoop_t* candidate_scoop,
	const DecodedOperandVector_t &mem_arg_container
	)
{

	assert(immed || mem_arg!=mem_arg_container.end());	// immeds don't need an argument, but memory ops do.

	const auto is_lea=disasm.getMnemonic() /*string(disasm.Instruction.Mnemonic)*/==string("lea");
	const auto consider_multiple_sizes= is_lea || immed;

	auto strides= consider_multiple_sizes ? set<int>({1,2,4,8}) : set<int>({ (int)(*mem_arg)->getArgumentSizeInBytes() /*ArgSize/8*/});

	// get other strides from the containing function
	if(insn->getFunction())
		for_each(ALLOF(insn->getFunction()->getInstructions()), [&strides](Instruction_t* insn)
		{
			//auto d=DISASM({});
			//Disassemble(insn,d);
			const auto dp=DecodedInstruction_t::factory(insn);
			const auto &d=*dp;

			auto potential_stride=0;
			// if( string(d.Instruction.Mnemonic)=="add " || string(d.Instruction.Mnemonic)=="sub " )
			if( d.getMnemonic()=="add" || d.getMnemonic()=="sub")
			{
				potential_stride=d.getImmediate(); //.Instruction.Immediat;	
			}

			//if(string(d.Instruction.Mnemonic)=="lea ")
			if(d.getMnemonic()=="lea")
			{
				potential_stride=d.getOperand(1)->getMemoryDisplacement(); /*d.Argument2.Memory.Displacement;	*/
			}

			if(abs(potential_stride)<500  && potential_stride!=0)
			{
				strides.insert(potential_stride);
				strides.insert(-potential_stride);
			}
		});

	const auto stride_multipliers= set<int>({-1,1});

	//const auto NO_REG=0;
	const auto contains_base_reg =  mem_arg!=mem_arg_container.end() && (*mem_arg)->hasBaseRegister(); // mem_arg ? mem_arg->Memory.BaseRegister != NO_REG : false;
	const auto contains_index_reg =  mem_arg!=mem_arg_container.end() && (*mem_arg)->hasIndexRegister(); // mem_arg ? mem_arg->Memory.IndexRegister != NO_REG : false;
	const auto contains_reg = contains_base_reg || contains_index_reg;
	const auto memory_access= mem_arg!=mem_arg_container.end() && !is_lea;
	const auto is_direct_memory_access=memory_access && !contains_reg;

	// check for a direct memory access
	if(is_direct_memory_access)
	{
		return candidate_scoop;
	}


	// calculate each offset=stride*multiplier pair
	auto candidate_offsets=set<int>();
	for_each(ALLOF(strides), [&](const int stride)
	{
		for_each(ALLOF(stride_multipliers), [&](const int multiplier)
		{
			candidate_offsets.insert(stride*multiplier);
		});
		
	});

	// how to tie two scoops
	auto insert_scoop_pair=[&](DataScoop_t* a, DataScoop_t* b, int i, int offset)
	{
		const auto tied_scoop_pair = ScoopPair_t(a,b) ;
		assert(tied_scoop_pair.first->getEnd()->getVirtualOffset()+1 == tied_scoop_pair.second->getStart()->getVirtualOffset());
		tied_scoops.insert(tied_scoop_pair);
		cout<<"	Tieing scoops "<<tied_scoop_pair.first->getName()<<" and "<<tied_scoop_pair.second->getName()<<" for i="<<dec<<i<<" offset="<<offset<<endl;
		ties_for_folded_constants++;
	};

	// how to decide if a scoop at offset i should be tied.
	// no scoop ->  no tie
	// un-tie-able scoop -> no tie
	// else tie
	auto should_tie=[&](const int i, DataScoop_t* prev_scoop) -> DataScoop_t* 
	{
		DataScoop_t *this_scoop=findScoopByAddress(insn_addr+i);	
		// no scoop at this addr?
		if(this_scoop==nullptr)
			return nullptr;
		// un-tie-able scoop at this addr?
		if(find(ALLOF(elftable_nocodeptr_names), this_scoop->getName())!=elftable_nocodeptr_names.end())
			return nullptr;

		// if both scoops are already pinned, no reason to tie.
		const auto is_prev_moveable = moveable_scoops.find(prev_scoop)!=moveable_scoops.end();
		const auto is_this_moveable = moveable_scoops.find(this_scoop)!=moveable_scoops.end();
		if(!is_prev_moveable && !is_this_moveable) 
			return nullptr;

		// else, tie
		return this_scoop;
	};


	// check each offset for a scoop that needings tieing tot his one.
	for_each(ALLOF(candidate_offsets), [&](const int offset)
	{
		assert(offset!=0);
		auto candidate_offset_scoop=findScoopByAddress(insn_addr+offset) ;
	
		// check to see if the offset is in a different scoop 
		if(candidate_scoop != candidate_offset_scoop)
		{

			// yes, therefore we have to tie all scoops between the start and end together.
			// stop if there's an untieable scoop in the way.
			auto prev_scoop=candidate_scoop;
			if(offset < 0 ) 
			{
				for(auto i=(int)-1;i>=offset; i--)
				{
					auto this_scoop=should_tie(i,prev_scoop);
					if(this_scoop)
					{
						if(this_scoop!=prev_scoop)
							insert_scoop_pair(this_scoop,prev_scoop, i, offset);
						prev_scoop=this_scoop;
					}
					else 
						break;
				}
			}
			else
			{
				for(auto i=(int)1;i<=offset; i++)
				{
					auto this_scoop=should_tie(i,prev_scoop);
					if(this_scoop)
					{
						if(this_scoop!=prev_scoop)
							insert_scoop_pair(prev_scoop,this_scoop, i, offset);
						prev_scoop=this_scoop;
					}
					else 
						break;
				}
			}
		}
	});

	return candidate_scoop;
}

template <class T_Sym, class  T_Rela, class T_Rel, class T_Dyn, class T_Extractor>
DataScoop_t* MoveGlobals_t<T_Sym,T_Rela,T_Rel,T_Dyn,T_Extractor>::DetectProperScoop_ConsiderEndOfPrev(
	const DecodedInstruction_t& disasm, 
	const DecodedOperandVector_t::iterator the_arg, 
	Instruction_t* insn, 
	VirtualOffset_t insn_addr, 
	bool immed, 
	DataScoop_t* ret,
	const DecodedOperandVector_t &the_arg_container
	)
{

	// possibility for future work:  identify cases where 
	// 	[addr+rbx*8] that came from something like =a[i-1].  And addr==a[-1].
	// for now, memory operands that actually access memory, there's no additional analysis needed 
	//if(!immed && string(disasm.Instruction.Mnemonic)!=string("lea "))	
	if(!immed && disasm.getMnemonic()!=string("lea"))	
		// this should filter out cmp, move, test, add,  with a memory operation
		return ret;

	// now we have an immediate or an lea (i.e., no cmp reg, [mem] operations)
	// that's pointing to a scoop.   Let's check if it's a boundary between two scoops
	if(insn_addr!=ret->getStart()->getVirtualOffset())	
		// it's not, so just continue.
		return ret;


	// now look to see if there's a scoop regsitered that abuts this scoop; 
	DataScoop_t *scoop_for_prev=findScoopByAddress(insn_addr-1);	

	// if not found, we know we aren't in a boundary case.
	if(!scoop_for_prev)
		return ret;

	/* check to see if the immediate next instruction dereferences the destination of an lea. */
	Instruction_t* next_insn=insn->getFallthrough();
	if(next_insn == NULL) 
		next_insn=insn->getTarget();

	if(next_insn && disasm.getMnemonic() /*string(disasm.Instruction.Mnemonic)*/==string("lea"))	
	{
		//DISASM lea_disasm;
		//Disassemble(insn,lea_disasm);
		const auto lea_disasmp=DecodedInstruction_t::factory(insn);
		const auto &lea_disasm=*lea_disasmp;;
		string dstreg=lea_disasm.getOperand(0)->getString(); // Argument1.ArgMnemonic;

		//DISASM next_disasm;
		//Disassimble(next_insn,next_disasm);
		const auto next_disasmp=DecodedInstruction_t::factory(next_insn);
		const auto &next_disasm=*next_disasmp;
		auto memarg_container=next_disasm.getOperands();
		const auto memarg=find_memory_operand(memarg_container);

		// if we found a memory operation that uses the register, with no indexing, then conclude that 
		// we must access the variable after the address (not the variable before the address) 
		// if(memarg && string(next_disasm.Instruction.Mnemonic)!="lea " && string(memarg->ArgMnemonic)==dstreg )
		if(memarg!=memarg_container.end() && next_disasm.getMnemonic()!="lea" && (*memarg)->getString()/*string(memarg->ArgMnemonic)*/==dstreg )
			return ret;
		
	}
	

	// if we're in a function
	// check that function for other references to scoop_for_prev
	if(insn->getFunction())
	{
		auto found_insn_it=find_if(
			ALLOF(insn->getFunction()->getInstructions()), 
			[&](Instruction_t* func_insn)
			{
				// disassemble instruction 
				//DISASM func_insn_disasm;
				//Disassemble(func_insn,func_insn_disasm);
				const auto func_insn_disasmp=DecodedInstruction_t::factory(func_insn);
				const auto &func_insn_disasm=*func_insn_disasmp;
				auto func_insn_disasm_operands=func_insn_disasm.getOperands();

				// enter instructions have 2 immediates, so we can't just "getImmediate()"
				if(func_insn_disasm.getMnemonic()=="enter")
					return false;

				// check the immediate
				// if(getFileIR()->findScoop(func_insn_disasm.Instruction.Immediat) == scoop_for_prev)	
				 if(scoop_for_prev->getStart()->getVirtualOffset() <= (VirtualOffset_t)func_insn_disasm.getImmediate() && 
				    (VirtualOffset_t)func_insn_disasm.getImmediate() <= scoop_for_prev->getEnd()->getVirtualOffset())	
					return true;	// return from lamba that we found an insn.

				// don't bother with the memory check unless we're an LEA
				//if(func_insn_disasm.Instruction.Mnemonic!=string("lea "))
				if(func_insn_disasm.getMnemonic()!=string("lea"))
					return false; 

				// check the memory -- find the argument that's the mem ref;
				const auto the_arg=find_memory_operand(func_insn_disasm_operands);
				if(the_arg!=func_insn_disasm_operands.end())
				{
					// see if the lea has a scoop reference.
					VirtualOffset_t addr=(*the_arg)->getMemoryDisplacement();
					if(arg_has_relative(*(*the_arg)))
						addr+=insn->getDataBits().size();
		
					if(getFileIR()->findScoop(addr) == scoop_for_prev)	
						return true;	// return from lamba
					
				}

				// not found in this insn
				return false; // lambda return
				

			});

		// no reference to prev_scoop found, just return;
		if(found_insn_it==insn->getFunction()->getInstructions().end())
		{
			return ret;
		}

	}


	// if we make it this far, we note that a single function has sketchy (aka address-generating) references
	// to both scoop_for_prev and ret;
	// in this case, we need to make keep these two scoops together since we can't tell which way the sketchy ref's go.
	// for now, just record the sketchy refs.

	cout<<"Boundary note:  instruction "<<insn->getBaseID()<<":"<<disasm.getDisassembly()<<" has immed/lea that points at boundary case.";
	if(insn->getFunction())
		cout<<" In "<<insn->getFunction()->getName()<<".";
	cout<<endl;
	cout<<"Keep together "<<
		scoop_for_prev->getName()<<" ("<<hex<< scoop_for_prev->getStart()->getVirtualOffset()<<"-"<<scoop_for_prev->getEnd()->getVirtualOffset()<<") and "<<
		ret->getName()<<" ("<<hex<< ret->getStart()->getVirtualOffset()<<"-"<<ret->getEnd()->getVirtualOffset()<<")"<<endl;

	tied_scoops.insert(ScoopPair_t(scoop_for_prev,ret));
	return ret;
}



template <class T_Sym, class  T_Rela, class T_Rel, class T_Dyn, class T_Extractor>
void MoveGlobals_t<T_Sym,T_Rela,T_Rel,T_Dyn,T_Extractor>::ApplyImmediateRelocation(Instruction_t *insn, DataScoop_t* to)
{
	const auto disasmp=DecodedInstruction_t::factory(insn);
	const auto &disasm=*disasmp;
	VirtualOffset_t rel_addr2=disasm.getImmediate(); // Instruction.Immediat;

#if 1 // don't change instructions that reference re-pinned scoops.
	// This was necessary because we were not getting the zipr_unpin_plugin
	//  to undo our changes to the instruction in the case of a re-pinned scoop.
	//  That problem is fixed, but it is more efficient and safer to
	//  avoid editing instructions that reference re-pinned scoops.
	if (moveable_scoops.find(to) == moveable_scoops.cend()) {
		if (getenv("MG_VERBOSE")) {
			cout << "Avoiding editing of insn at " << hex << insn->getBaseID() << " after repinning scoop "
				<< to->getName() << endl;
		}
		return;
	}
#endif

	/*
	Relocation_t* reloc = new Relocation_t(BaseObj_t::NOT_IN_DATABASE, 0, "immedptr_to_scoop", to);
	insn->getRelocations().insert(reloc);
	getFileIR()->getRelocations().insert(reloc);
	*/
	auto reloc=getFileIR()->addNewRelocation(insn,0, "immedptr_to_scoop", to);
	(void)reloc; // not used, just giving to the IR
// fixme: insn bits changed here 
	assert(strtoumax(disasm.getOperand(1)->getString().c_str() /*Argument2.ArgMnemonic*/, NULL, 0) ==  rel_addr2);

	VirtualOffset_t new_addr = rel_addr2 - to->getStart()->getVirtualOffset();
   assert(4 < insn->getDataBits().size());
	insn->setDataBits(insn->getDataBits().replace(insn->getDataBits().size()-4, 4, (char*)&new_addr, 4));

	cout<<"Non-Overlapping_Globals::ApplyImmediateReloc::Setting "<<hex<<insn->getBaseID()<<" to "<<insn->getDisassembly()<<endl;
}


template <class T_Sym, class  T_Rela, class T_Rel, class T_Dyn, class T_Extractor>
void MoveGlobals_t<T_Sym,T_Rela,T_Rel,T_Dyn,T_Extractor>::HandleImmediateOperand(const DecodedInstruction_t& disasm, const DecodedOperandVector_t::iterator the_arg, Instruction_t* insn)
{
	// shared objects don't need this, you have to use a pcrel addressing mode.
	if(exe_reader->isDLL())
	{
		return;
	}
	const int small_memory_threshold= exe_reader->isDLL() ? 10 : 4096*10;

	// enter instructions have 2 immediates, so we can't just "getImmediate()"
	if(disasm.getMnemonic()=="enter")
		return;

	VirtualOffset_t rel_addr2=disasm.getImmediate(); //Instruction.Immediat;
	auto operands=disasm.getOperands();
	DataScoop_t *to2=DetectProperScoop(disasm, operands.end(), insn, rel_addr2, true, operands);


	// skip if not found, executable, or not moveable.
	if( to2 && (to2->isExecuteable() || moveable_scoops.find(to2) == moveable_scoops.end()))
	{  
		// do nothing, no log or action is necessary for (potential) pointers to code or 
		// (potential) pointers to non-moveable data.
	}
	else if(to2)
	{

		// there's no need to find pointers in other types of instructions, 
		// such as mul or vfmasubadd231 (yes, that's a real instruction on x86)
		// note: yes other instructions may have a memory operand with a pointer, but that's handled above.
		// this is for instruction's immediate fields, not their memory operand's displacement.
		//
		// compares, tests are often used because the compiler strength reduces.
		// moves are used to load addresses into a register.
		// adds are used to load addresses plus an offset into a register.
		// here's an example where sub is used with a pointer:
		//
		//  	DegenCount[strchr(Alphabet,iupac)-Alphabet] = ...
		//
		// 	0x0000000000402a99 <+25>:	call   0x401620 <strchr@plt>
		// 	0x0000000000402a9e <+30>:	mov    rbp <- rax
   		//	0x0000000000402aa1 <+33>:	mov    rdi <- rbx
   		//	0x0000000000402aa4 <+36>:	sub    rbp <- 0x65b500  # note:  constant is a poitner here!
   		//	0x0000000000402aab <+43>:	eax <-  ...
   		//	0x0000000000402ab0 <+48>:	mov    DWORD PTR [rbp*4+0x65b520] <- eax

		if(disasm.getMnemonic() == string("mov") ||
		   disasm.getMnemonic() == string("cmp") ||
		   disasm.getMnemonic() == string("test") ||
		   disasm.getMnemonic() == string("add")  ||
		   disasm.getMnemonic() == string("sub") )
		{
			if(getenv("MG_VERBOSE"))
			{
				cout<<"Found non-mem ref in insn: "<<insn->getBaseID()<<":"<<disasm.getDisassembly()<<" to "
					<< to2->getName() <<"("
					<<hex<<to2->getStart()->getVirtualOffset()<<"-" 
					<<hex<<to2->getEnd()->getVirtualOffset()<<")"<<endl; 
			}
			
			unsigned int size=immed_refs_to_scoops.size();
			immed_refs_to_scoops.insert({insn,to2});
			assert( (size+1)==immed_refs_to_scoops.size());
				


		}
	}
	else  
	{
		if ((int)rel_addr2 < -small_memory_threshold || (int) rel_addr2 > small_memory_threshold || getenv("MG_VERBOSE"))
		{
			if ((0 != rel_addr2) && getenv("MG_VERBOSE"))
			{
				cout << "Note:  " << hex << rel_addr2 << " not declared address in:";
				cout << insn->getBaseID() << ":" << disasm.getDisassembly();
				cout << endl;
			}
		}
	}
}

// put in links between scoops and any references to them.
template <class T_Sym, class  T_Rela, class T_Rel, class T_Dyn, class T_Extractor>
void MoveGlobals_t<T_Sym,T_Rela,T_Rel,T_Dyn,T_Extractor>::FindInstructionReferences()
{


	for(InstructionSet_t::iterator iit=getFileIR()->getInstructions().begin();
		iit!=getFileIR()->getInstructions().end();
		++iit
	   )
	{
		Instruction_t* insn=*iit;
		//DISASM disasm;
		//Disassemble(insn,disasm);
		auto disasmp=DecodedInstruction_t::factory(insn);
		auto &disasm=*disasmp;
		auto disasm_operands=disasm.getOperands();

		// find memory arg.
		const auto the_arg=find_memory_operand(disasm_operands);

		if(getenv("MG_VERBOSE"))
			cout<<"Considering "<<hex<<insn->getBaseID()<<":"<<disasm.getDisassembly()<<endl;
		HandleMemoryOperand(disasm,the_arg,insn, disasm_operands);
		HandleImmediateOperand(disasm,the_arg,insn);
	}

}


template <class T_Sym, class  T_Rela, class T_Rel, class T_Dyn, class T_Extractor>
void MoveGlobals_t<T_Sym,T_Rela,T_Rel,T_Dyn,T_Extractor>::ApplyDataRelocation(DataScoop_t *from, unsigned int offset, DataScoop_t* to)
{
	assert(to && from);

	const char* data=from->getContents().c_str();
	unsigned int byte_width=getFileIR()->getArchitectureBitWidth()/8;
	VirtualOffset_t val=(VirtualOffset_t)NULL;

	if(byte_width==4)
		val=*(int*)&data[offset];
	else if(byte_width==8)
		val=*(long long*)&data[offset];
	else
		assert(0);
		
	/*
	Relocation_t* reloc=new Relocation_t(BaseObj_t::NOT_IN_DATABASE, offset, "dataptr_to_scoop", to);
	from->getRelocations().insert(reloc);
	getFileIR()->getRelocations().insert(reloc);
	*/
	auto reloc=getFileIR()->addNewRelocation(from,offset, "dataptr_to_scoop", to);
	(void)reloc; // just giving to ir

	VirtualOffset_t newval=val-to->getStart()->getVirtualOffset();

	auto str=from->getContents();
	// create new value for pointer.
	if(byte_width==4)
	{
		unsigned int intnewval=(unsigned int)newval;	 // 64->32 narrowing OK. 
		str.replace(offset, byte_width, (char*)&intnewval, byte_width);
	}
	else if(byte_width==8)
	{
		str.replace(offset, byte_width, (char*)&newval, byte_width);
	}
	else
		assert(0);
	from->setContents(str);
}



//
// check if val is a pointer or part of a string that mimics a pointer
//
static inline bool is_part_of_string(VirtualOffset_t val, const DataScoop_t* from,  const DataScoop_t* to, int offset)
{
	assert(from && to);

	// locate strings that look like pointers but aren't.  e.g.:  "ion\0" and "ren\0".  Note that both are null terminated. 
	// this is a problem on 64-bit code because we screw up the string.
	
	// note:  the most sigificant byte is 0, and the lower 3 signfiicant bytes are printable.


	// the least significant byte is special.  In a valid pointer, it's almost always 00 or 01 for 64-bit code or shared libraries, 
	// and 0x08 0x09 for 32-bit main executables.   Very very rarely is it anything else.
	// however, for 0x01, 0x08, and 0x09 aren't printable, so we don't confuse these bytes in a string for an address and we don't need to detect this.
	if ( ((val >> 24) & 0xff) != 0 )	// check for non-0
		return false;
	if ( !isprint(((val >> 16) & 0xff)))	// and 3 printable characters.
		return false;
	if ( !isprint(((val >> 8) & 0xff)))
		return false;
	if ( !isprint(((val >> 0) & 0xff)))
		return false;

	// number of bytes that must precede the pointer and be string bytes to disambiguate a string's end from a pointer.
	const int string_preheader_size=4;

	// if we dont' have enough bytes of preheader, skip it.
	if( offset < string_preheader_size ) 
		return false;

	// check each byte preceeding the candidate pointer to see if it's printable.
	for(auto i=0;i<string_preheader_size;i++)
	{
		if(i>offset)
			return false;
		unsigned char b=from->getContents()[offset-i];
		if(!isprint(b))
			return false;
	}

	// we found enough string chars before the (candidate) pointer value, so we think that a string is here, not a pointer.
	if(getenv("MG_VERBOSE"))
	{
		cout<<"Found string as non-ref "<<hex<<val<<" at "<<from->getName()<<"+"<<offset<<" ("
			<<hex<<from->getStart()->getVirtualOffset()<<"-" 
			<<hex<<from->getEnd()->getVirtualOffset()<<") to "
			<<to->getName()<<" ("
			<<hex<<to->getStart()->getVirtualOffset()<<"-" 
			<<hex<<to->getEnd()->getVirtualOffset()<<")"<<endl;
	}
	return true;

}

// put in links between scoops and any references to them.
template <class T_Sym, class  T_Rela, class T_Rel, class T_Dyn, class T_Extractor>
void MoveGlobals_t<T_Sym,T_Rela,T_Rel,T_Dyn,T_Extractor>::FindDataReferences()
{
	unsigned int byte_width=getFileIR()->getArchitectureBitWidth()/8;

	typedef function<void (DataScoop_t*)> ScannerFunction_t;

	auto read_bytewidth=[&](const char* data, const int i) -> long long
	{
		auto val=(long long)0;
		if(byte_width==4)
			val=*(int*)&data[i];
		else if(byte_width==8)
			val=*(long long*)&data[i];
		else
			assert(0);
		return val;
	};

	ScannerFunction_t got_scanner=[&](DataScoop_t* scoop)
	{
		// got scanner doesn't scan data section for shared objects since they can't have a constant address
		if(exe_reader->isDLL())
			return;

		auto data=scoop->getContents().c_str();
		auto len=scoop->getContents().size();

		for ( auto i=0u; i+byte_width-1<len; i+=byte_width)
		{
			const auto val=read_bytewidth(data,i);
			auto to=findScoopByAddress(val);	
			if(to)
			{
				if(getenv("MG_VERBOSE"))
				{
					cout<<"Found ref "<<hex<<val<<" at "<<scoop->getName()<<"+"<<i<<" ("
						<<hex<<scoop->getStart()->getVirtualOffset()<<"-" 
						<<hex<<scoop->getEnd()->getVirtualOffset()<<") to "
						<<to->getName()<<" ("
						<<hex<<to->getStart()->getVirtualOffset()<<"-" 
						<<hex<<to->getEnd()->getVirtualOffset()<<")"<<endl;
				}

				data_refs_to_scoops.insert({scoop,i,to}); 
			}
		}
	};

	ScannerFunction_t default_scanner=[&](DataScoop_t* scoop)
	{
		// default scanner doesn't scan data section for shared objects since they can't have a constant address
		if(exe_reader->isDLL())
			return;

		auto data=scoop->getContents().c_str();
		auto len=scoop->getContents().size();

		// try not to overrun the array
		for ( auto i=0u; i+byte_width-1<len; i+=byte_width)
		{
			auto val=read_bytewidth(data,i);
			auto to=findScoopByAddress(val);	

			if(to)
			{

				auto aggressive_qualify_for_moving = [this](const DataScoop_t* from, 
								DataScoop_t* &to, 
								bool &move_ok, 
								bool &disqualifies_to, 
								const VirtualOffset_t addr, unsigned int offset_in_scoop
							       ) -> void
				{
					move_ok=true;
					disqualifies_to=false;
					if( !to->isExecuteable() && 
					    moveable_scoops.find(to) != moveable_scoops.end() && 
					    !is_part_of_string(addr,from,to,offset_in_scoop)
					  )
					{
						return;	
					}
					move_ok=false;
				};

				auto qualify_for_moving = [this](const DataScoop_t* from, 
								DataScoop_t* &to, 
								bool &move_ok, 
								bool &disqualifies_to, 
								const VirtualOffset_t addr, unsigned int offset_in_scoop
							       ) -> void
				{
					move_ok=true;
					disqualifies_to=false;

					// if points at executable scoop, we aren't doing that here!
					if(to->isExecuteable())
					{ move_ok=false;  disqualifies_to=false; return ; }

					// if not moveable, we aren't doing that here.	
 					if ( moveable_scoops.find(to) == moveable_scoops.end())
					{ move_ok=false;  disqualifies_to=false; return ; }


					/* the above worked ok-ish, but not great.  trying this method to be more conservative */
					{ move_ok=false;  disqualifies_to=true; return ; }

/*
					// if this constant appears to be part of a string, skip it!
					if(is_part_of_string(addr,from,to,offset_in_scoop))
					{ move_ok=false;  disqualifies_to=false; return ; }

					// very few variables start at an address that ends in 0x000 and often address-looking constants do
					// if we see such an address, pin-and-win.
					if ( (addr&0xfff) == 0x000 && addr==to->getStart()->getVirtualOffset())
					{ move_ok=false;  disqualifies_to=true; return ; }

					// if we point at the start of a scoop, it's OK to move.
					if(addr==to->getStart()->getVirtualOffset())
					{ move_ok=true;  disqualifies_to=false; return ; }

					// if it points near a scoop, but not directly at it, it's hard to tell if it's moveable or not
					if(abs((long)addr-(long)to->getStart()->getVirtualOffset()) < 16 )
					{ move_ok=false;  disqualifies_to=true; return ; }

					// else, it's pointing in the middle of a scoop, so it's probably not a 
					// pointer at all.
					{ move_ok=false;  disqualifies_to=false; return ; }
*/

				};

				auto move_ok=false;
				auto disqualifies_to=false;
				if(aggressive)
					aggressive_qualify_for_moving(scoop, to,move_ok,disqualifies_to,val, i);
				else
					qualify_for_moving(scoop, to,move_ok,disqualifies_to,val, i);

				if(move_ok)
				{
					if(getenv("MG_VERBOSE"))
					{
						cout<<"Found ref "<<hex<<val<<" at "<<scoop->getName()<<"+"<<i<<" ("
							<<hex<<scoop->getStart()->getVirtualOffset()<<"-" 
							<<hex<<scoop->getEnd()->getVirtualOffset()<<") to "
							<<to->getName()<<" ("
							<<hex<<to->getStart()->getVirtualOffset()<<"-" 
							<<hex<<to->getEnd()->getVirtualOffset()<<")"<<endl;
					}


					// put those bytes back in the string.
					//ApplyDataRelocations(*sit,i,to);

					data_refs_to_scoops.insert({scoop,i,to}); 
				}
				else
				{
					if(getenv("MG_VERBOSE"))
					{
						cout<<"Found ref-looking-constant "<<hex<<val<<" at "<<scoop->getName()<<"+"<<i<<" ("
							<<hex<<scoop->getStart()->getVirtualOffset()<<"-" 
							<<hex<<scoop->getEnd()->getVirtualOffset()<<") which would otherwise be to "
							<<to->getName()<<" ("
							<<hex<<to->getStart()->getVirtualOffset()<<"-" 
							<<hex<<to->getEnd()->getVirtualOffset()<<")"<<endl;
					}
				}
				if(disqualifies_to)
				{
					if(!is_elftable(to))
					{
						if(getenv("MG_VERBOSE"))
						{
							cout<<"Ref-looking-constant "<<hex<<val<<" at "<<scoop->getName()<<"+"<<i<<" ("
								<<hex<<scoop->getStart()->getVirtualOffset()<<"-" 
								<<hex<<scoop->getEnd()->getVirtualOffset()<<") is inconclusive.  Repinning "
								<<to->getName()<<" ("
								<<hex<<to->getStart()->getVirtualOffset()<<"-" 
								<<hex<<to->getEnd()->getVirtualOffset()<<")"<<endl;
						}
						moveable_scoops.erase(to);
					}
					else
					{
						if(getenv("MG_VERBOSE"))
						{
							cout<<"Ref-looking-constant "<<hex<<val<<" at "<<scoop->getName()<<"+"<<i<<" ("
								<<hex<<scoop->getStart()->getVirtualOffset()<<"-" 
								<<hex<<scoop->getEnd()->getVirtualOffset()<<") is inconclusive.  Not repinning because is elftable "
								<<to->getName()<<" ("
								<<hex<<to->getStart()->getVirtualOffset()<<"-" 
								<<hex<<to->getEnd()->getVirtualOffset()<<")"<<endl;
						}
					}
				}	
			}
			else
			{
				if((0 != val) && getenv("MG_VERBOSE"))
				{
					cout<<"Constant "<<hex<<val<<" at "<<scoop->getName()<<"+"<<i<<" ("
						<<hex<<scoop->getStart()->getVirtualOffset()<<"-" 
						<<hex<<scoop->getEnd()->getVirtualOffset()<<") doesn't point at scoop."<<endl;
				}
			}
		}
	};

	ScannerFunction_t dynsym_scanner=[&](DataScoop_t* scoop) 
	{ 
		const char* data=scoop->getContents().c_str();
		unsigned int len=scoop->getContents().size();
		T_Sym* symptr=(T_Sym*)data;
		const char* end=data+len;

		while((const char*)symptr<end)
		{

			VirtualOffset_t val=symptr->st_value;
			DataScoop_t *to=findScoopByAddress(val);	
			if(to)
			{
				unsigned int offset=(unsigned int)((VirtualOffset_t)symptr)-((VirtualOffset_t)data);
				offset+=((VirtualOffset_t)&symptr->st_value)-(VirtualOffset_t)symptr;

				if(getenv("MG_VERBOSE"))
				{

					cout<<"Found dynsym:st_value ref "<<hex<<val<<" at "<<scoop->getName()<<"+"<<offset<<" ("
						<<hex<<scoop->getStart()->getVirtualOffset()<<"-" 
						<<hex<<scoop->getEnd()->getVirtualOffset()<<") to "
						<<to->getName()<<" ("
						<<hex<<to->getStart()->getVirtualOffset()<<"-" 
						<<hex<<to->getEnd()->getVirtualOffset()<<")"<<endl;
				}

				data_refs_to_scoops.insert({scoop,offset,to}); 
			}
	
			symptr++; // next symbol
		}
		
	};
	ScannerFunction_t rel_scanner=[&](DataScoop_t* scoop) 
	{  
		const char* data=scoop->getContents().c_str();
		unsigned int len=scoop->getContents().size();

		T_Rela * symptr=(T_Rela*)data;	
		const char* end=data+len;

		while((const char*)symptr<end)
		{
			// handle offset field
			{
				VirtualOffset_t val=symptr->r_offset;
				DataScoop_t *to=findScoopByAddress(val);	
				if(to)
				{
					unsigned int offset=(unsigned int)((VirtualOffset_t)symptr)-((VirtualOffset_t)data);
					offset+=((VirtualOffset_t)&symptr->r_offset)-(VirtualOffset_t)symptr;

					if(getenv("MG_VERBOSE"))
					{
						cout<<"Found rela:r_offset ref "<<hex<<val<<" at "<<scoop->getName()<<"+"<<offset<<" ("
							<<hex<<scoop->getStart()->getVirtualOffset()<<"-" 
							<<hex<<scoop->getEnd()->getVirtualOffset()<<") to "
							<<to->getName()<<" ("
							<<hex<<to->getStart()->getVirtualOffset()<<"-" 
							<<hex<<to->getEnd()->getVirtualOffset()<<")"<<endl;
					}

					data_refs_to_scoops.insert({scoop,offset,to}); 
				}
			}
	
			symptr++; // next symbol
		}
	}; 
	ScannerFunction_t rela_scanner=[&](DataScoop_t* scoop)
	{ 
		const char* data=scoop->getContents().c_str();
		unsigned int len=scoop->getContents().size();

		T_Rela * symptr=(T_Rela*)data;	
		const char* end=data+len;

		while((const char*)symptr<end)
		{
			// handle addend field
			{
				VirtualOffset_t val=symptr->r_addend;
				DataScoop_t *to=findScoopByAddress(val);	
				if(to)
				{
					unsigned int offset=(unsigned int)((VirtualOffset_t)symptr)-((VirtualOffset_t)data);
					offset+=((VirtualOffset_t)&symptr->r_addend)-(VirtualOffset_t)symptr;

					if(getenv("MG_VERBOSE"))
					{
						cout<<"Found rela:r_added ref "<<hex<<val<<" at "<<scoop->getName()<<"+"<<offset<<" ("
							<<hex<<scoop->getStart()->getVirtualOffset()<<"-" 
							<<hex<<scoop->getEnd()->getVirtualOffset()<<") to "
							<<to->getName()<<" ("
							<<hex<<to->getStart()->getVirtualOffset()<<"-" 
							<<hex<<to->getEnd()->getVirtualOffset()<<")"<<endl;
					}

					data_refs_to_scoops.insert({scoop,offset,to}); 
				}
			}
			// handle offset field
			{
				VirtualOffset_t val=symptr->r_offset;
				DataScoop_t *to=findScoopByAddress(val);	
				if(to)
				{
					unsigned int offset=(unsigned int)((VirtualOffset_t)symptr)-((VirtualOffset_t)data);
					offset+=((VirtualOffset_t)&symptr->r_offset)-(VirtualOffset_t)symptr;

					if(getenv("MG_VERBOSE"))
					{
						cout<<"Found rela:r_offset ref "<<hex<<val<<" at "<<scoop->getName()<<"+"<<offset<<" ("
							<<hex<<scoop->getStart()->getVirtualOffset()<<"-" 
							<<hex<<scoop->getEnd()->getVirtualOffset()<<") to "
							<<to->getName()<<" ("
							<<hex<<to->getStart()->getVirtualOffset()<<"-" 
							<<hex<<to->getEnd()->getVirtualOffset()<<")"<<endl;
					}

					data_refs_to_scoops.insert({scoop,offset,to}); 
				}
			}
	
			symptr++; // next symbol
		}
	};
	ScannerFunction_t dynamic_scanner=[&](DataScoop_t* scoop)
	{ 
		const auto data=scoop->getContents().c_str();
		const auto len=scoop->getContents().size();
		auto symptr=(T_Dyn*)data;
		const char* end=data+len;

		while((const char*)symptr<end)
		{

			switch(symptr->d_tag)
			{
				case DT_INIT_ARRAY:
				case DT_FINI_ARRAY:
				case DT_GNU_HASH:
				case DT_STRTAB:
				case DT_SYMTAB:
				case DT_PLTGOT:
				case DT_JMPREL:
				case DT_RELA:
				case DT_VERNEED:
				case DT_VERSYM:
				{
					const auto val=symptr->d_un.d_val;
					auto *to=findScoopByAddress(val);	

					if(to)
					{

						auto offset=(unsigned int) (((VirtualOffset_t)symptr)-((VirtualOffset_t)data));
						offset+=((VirtualOffset_t)&symptr->d_un.d_val)-(VirtualOffset_t)symptr;

						if(getenv("MG_VERBOSE"))
						{

							cout<<"Found .dynamic:d_val ref "<<hex<<val<<" at "<<scoop->getName()<<"+"<<offset<<" ("
								<<hex<<scoop->getStart()->getVirtualOffset()<<"-" 
								<<hex<<scoop->getEnd()->getVirtualOffset()<<") to "
								<<to->getName()<<" ("
								<<hex<<to->getStart()->getVirtualOffset()<<"-" 
								<<hex<<to->getEnd()->getVirtualOffset()<<")"<<endl;
						}

						data_refs_to_scoops.insert({scoop,offset,to}); 
					}
					break;
				}
				default:  // do nothing

					break;
			}

			symptr++; // next symbol
		}
		
	};


	// special scanners for special sections
	const struct scoop_scanners_t
	{	string name;
		ScannerFunction_t scanner_fn;
	} scoop_scanners[] = {
		{ ".dynsym", dynsym_scanner }, 
		{ ".got", got_scanner }, 
		{ ".got.plt", got_scanner }, 
		{ ".rel.dyn", rel_scanner }, 
		{ ".rel.plt", rel_scanner }, 
		{ ".rel.dyn coalesced w/.rel.plt", rel_scanner }, 
		{ ".rela.dyn", rela_scanner }, 
		{ ".rela.plt", rela_scanner }, 
		{ ".rela.dyn coalesced w/.rela.plt", rela_scanner }, 
		{ ".dynamic", dynamic_scanner } 
	};

	// main algorithm:  apply the right scanner for each scoop
	for_each(ALLOF(getFileIR()->getDataScoops()), [&](DataScoop_t* scoop)
	{
		auto scanner=find_if(ALLOF(scoop_scanners), [&](const scoop_scanners_t scanner)
		{
			return scanner.name==scoop->getName();

		});
		if(scanner!=end(scoop_scanners))
			scanner->scanner_fn(scoop);
		else
			default_scanner(scoop);

	});
}


template <class T_Sym, class  T_Rela, class T_Rel, class T_Dyn, class T_Extractor>
void MoveGlobals_t<T_Sym,T_Rela,T_Rel,T_Dyn,T_Extractor>::FilterAndCoalesceTiedScoops()
{
	const auto is_in_dont_coalesce_scoops = [](const DataScoop_t* to_find) -> bool
		{

			const string dont_coalesce_scoops[] =
			{
				".dynamic",
				".jcr"
			};
			const auto a_binder = bind1st(finder, to_find);
			const auto it=find_if(ALLOF(dont_coalesce_scoops), a_binder);

			return (it!=end(dont_coalesce_scoops));
		};



	// step 1:  find everything that's tied to a pinned scoop and pin it.
	// repeat until no changes.
	bool changed=true;
	while(changed)
	{
		changed=false;
		for(auto it=tied_scoops.begin(); it!=tied_scoops.end(); /* nop */)
		{
			auto current=it++;
			const ScoopPair_t& p=*current;
			DataScoop_t* s1=p.first;
			DataScoop_t* s2=p.second;
			bool s1_moveable=contains(moveable_scoops, s1);
			bool s2_moveable=contains(moveable_scoops, s2);

			if(is_in_dont_coalesce_scoops(s1) || is_in_dont_coalesce_scoops(s2)) 
			{
				cout<<"Skipping coalesce of "<<s1->getName()<<" and "<<s2->getName()<<endl;
				tied_scoops.erase(current);
				continue;
			}

			if(s1_moveable && s2_moveable)
			{
				// do nothing if they're both unpinned.
				tied_unpinned++;
			}
			else  if(s1_moveable)
			{
				tied_pinned++;

				// s1 is pinned to an unmoveable, so it's unmoveable.
				cout<<"Re-pinning "<<s1->getName()<<endl;
				moveable_scoops.erase(s1);	 
				tied_scoops.erase(current);
				changed=true;
			}
			else  if(s2_moveable)
			{
				cout<<"Re-pinning "<<s2->getName()<<endl;
				tied_pinned++;
				// s2 is pinned to an unmoveable.
				moveable_scoops.erase(s2); 
				tied_scoops.erase(current);
				changed=true;
			}
			else
			{
				tied_nochange++;
				tied_scoops.erase(current);
			}


		}
	}

	// step 2, coalesce
	changed=true;
	while(changed)
	{
		changed=false;
		for(auto it=tied_scoops.begin(); it!=tied_scoops.end(); )
		{
			auto current=it++; 
			const ScoopPair_t& p=*current;
			DataScoop_t* s1=p.first;
			DataScoop_t* s2=p.second;


			if(is_in_dont_coalesce_scoops(s1) || is_in_dont_coalesce_scoops(s2)) 
			{
				cout<<"Skipping coalesce of "<<s1->getName()<<" and "<<s2->getName()<<endl;
				continue;
			}

			bool s1_moveable=contains(moveable_scoops, s1);
			bool s2_moveable=contains(moveable_scoops, s2);

			// we previously removed anything that's pinned from moveable 
			if(s1_moveable && s2_moveable)
			{
				// assert order is right
				assert(s1->getStart()->getVirtualOffset() < s2->getStart()->getVirtualOffset());

				// check if these are adjacent.
				if(s1->getEnd()->getVirtualOffset()+1 < s2->getStart()->getVirtualOffset())
				{
					// pad s1 to fill hole	
					string new_contents=s1->getContents();
					new_contents.resize(s2->getStart()->getVirtualOffset()-s1->getStart()->getVirtualOffset());
					s1->getEnd()->setVirtualOffset(s2->getStart()->getVirtualOffset()-1);
				}
				else if(s1->getEnd()->getVirtualOffset()+1 == s2->getStart()->getVirtualOffset())
				{
					// do nothing if they fit perfectly.
				}
				else
					assert(0); // overlapping scoops?

				cout<<"Coalescing 2-tied, but unpinned scoops "<<s1->getName()<<" and "<<s2->getName()<<"."<<endl;


				// update our inteneral data structures for how to apply relocs.
				auto insn_fixup_updater=[s1,s2](set<Insn_fixup_t> &the_set)
					{
						unsigned int size=the_set.size();
						set<Insn_fixup_t> new_elements;
						auto it=the_set.begin();
						while(it!=the_set.end())
						{
							auto current = it++;
							auto replacer=*current;
							if(replacer.to == s2) 
							{
								the_set.erase(current);
								replacer.to=s1;
								new_elements.insert(replacer);
							}
						}
						the_set.insert(new_elements.begin(), new_elements.end());
						assert(size==the_set.size());
					};
				insn_fixup_updater(pcrel_refs_to_scoops);
				insn_fixup_updater(absolute_refs_to_scoops);
				insn_fixup_updater(immed_refs_to_scoops);

				auto scoop_fixup_updater=[s1,s2](set<Scoop_fixup_t> &the_set)
					{
						set<Scoop_fixup_t> new_elements;
						auto it=the_set.begin();
						while(it!=the_set.end())
						{
							auto current = it++;
							if(current->to == s2 || current->from==s2) 
							{
								auto replacer=*current;
								if(replacer.to==s2)
									replacer.to=s1;
							
								if(replacer.from==s2)
								{
									replacer.from=s1;
									cout<<"Updating data_ref_to_scoops offset from "<<hex<<replacer.offset<<" to "<<replacer.offset+s1->getSize()<<endl;
									replacer.offset+=s1->getSize();
								}
								the_set.erase(current);
								new_elements.insert(replacer);
							}
						}
						the_set.insert(new_elements.begin(), new_elements.end());
					};
				scoop_fixup_updater(data_refs_to_scoops);

				for(auto &r : getFileIR()->getRelocations())
				{
					// s2 just came into existence, didn't it?
					// assert(r->getWRT()!=s2);
					// yes, but there may be relocs pointing at the s2 part of 
					// a split object, and so the reloc might get updated to point to s2 instead.
					if( r->getWRT()==s2)
					{
						r->setWRT(s1);
						r->setAddend(r->getAddend()+s1->getSize());
					}
				}


				/*
				don't remove scoop here, as it will delete s2.  this bit is moved later.	
				*/
				// s2's end addresss is about to go away, so
				// update s1's end VO instead of using s2 end addr.
				s1->getEnd()->setVirtualOffset(s2->getEnd()->getVirtualOffset()); 
				moveable_scoops.erase(s2);		// remove it from our analysis
				unsigned int old_s1_size=s1->getContents().size();
				s1->setContents(s1->getContents()+s2->getContents());
				s1->setName(s1->getName()+" coalesced w/"+ s2->getName());
				if(!s2->isRelRo())
					s1->clearRelRo();
				s1->setRawPerms( s1->getRawPerms() | s2->getRawPerms());

				// we just created s2 in this pass, right?
				// no, s2 could be one of the sections from the orig binary that we've been asked to move
				// and it might have relocs for unpinning
				//assert(s2->getRelocations().size()==0); // assert no relocs that're part of s2.

				// add s2's relocs to s1.
				for(auto reloc : s2->getRelocations())
				{
					cout<<"Adjusting reloc "<< s2->getName()<<"+"<<reloc->getOffset()<<":"<<reloc->getType()<<" to ";

					reloc->setOffset(reloc->getOffset()+old_s1_size);

					auto s1_relocs=s1->getRelocations();
					s1_relocs.insert(reloc);
					s1->setRelocations(s1_relocs);
	
					cout << s1->getName()<<"+"<<reloc->getOffset()<<":"<<reloc->getType()<<endl;
				}

				// tell s2 it has no relocs so when we remove it, they don't go away.
				s2->setRelocations({});

				// we've processed this one.
				tied_scoops.erase(current);	
				auto scoop_pair_first_finder=
					[s2](const ScoopPair_t& p2)
					{
						return (p2.first==s2);	
					};
				auto found=find_if(ALLOF(tied_scoops), scoop_pair_first_finder);
				if( found!=tied_scoops.end())
				{
					ScoopPair_t p2=*found;
					p2.first=s1;
					tied_scoops.erase(found);
					tied_scoops.insert(p2);
				}
				assert(find_if(ALLOF(tied_scoops), scoop_pair_first_finder) ==tied_scoops.end());

				// finally remove s2 from the IR.
				getFileIR()->removeScoop(s2);
				changed=true;
				break;
			}
			else
				assert(0); // why are there pinned scoops still?
		}
	}
	// ensure we handled eveything.
	assert(tied_scoops.size()==0);
	
}

template <class T_Sym, class  T_Rela, class T_Rel, class T_Dyn, class T_Extractor>
void MoveGlobals_t<T_Sym,T_Rela,T_Rel,T_Dyn,T_Extractor>::UpdateScoopLocations()
{

	// apply all 3 types of relocs for instructions
	for_each(ALLOF(pcrel_refs_to_scoops),
		[this] (const Insn_fixup_t  & it)
	{
		if (getenv("MG_VERBOSE"))
			cout << "Applying pcrel w/wrt from " << it.from->getDisassembly() << " to " << it.to->getName() << " at " << hex << it.from->getBaseID() << endl;
		ApplyPcrelMemoryRelocation(it.from,it.to);
	});

	for_each(ALLOF(absolute_refs_to_scoops),
		[this] (const Insn_fixup_t  & it)
	{
		if (getenv("MG_VERBOSE"))
			cout << "Applying absptr_to_scoop from " << it.from->getDisassembly() << " to " << it.to->getName() << " at " << hex << it.from->getBaseID() << endl;
		ApplyAbsoluteMemoryRelocation(it.from,it.to);
	});

	for_each(ALLOF(immed_refs_to_scoops),
		[this] (const Insn_fixup_t  & it)
	{
		if (getenv("MG_VERBOSE"))
			cout << "Applying immedptr_to_scoop from " << it.from->getDisassembly() << " to " << it.to->getName() << " at " << hex << it.from->getBaseID() << endl;
		ApplyImmediateRelocation(it.from, it.to);
	});
	for_each(ALLOF(data_refs_to_scoops),
		[this] (const Scoop_fixup_t  & it)
	{
		if (getenv("MG_VERBOSE"))
			cout << "Applying dataptr_to_scoop from " << it.from->getName() << " to " << it.to->getName() << " at " << hex << it.offset << endl;
		ApplyDataRelocation(it.from, it.offset, it.to);
	});


	// unpin all the moveable scoops.
	for (auto sit : moveable_scoops)
	{
		VirtualOffset_t newend = sit->getEnd()->getVirtualOffset() -  sit->getStart()->getVirtualOffset();
		sit->getEnd()->setVirtualOffset(newend);
		sit->getStart()->setVirtualOffset(0);
	}
}

// would be nice to have a FindRelocation function that takes a parameterized type.
template <class T_Sym, class  T_Rela, class T_Rel, class T_Dyn, class T_Extractor>
Relocation_t* MoveGlobals_t<T_Sym,T_Rela,T_Rel,T_Dyn,T_Extractor>::FindRelocationWithType(BaseObj_t* obj, std::string type)
{
	RelocationSet_t::iterator rit = obj->getRelocations().begin();
	for( ; rit!=obj->getRelocations().end(); rit++)
	{
		Relocation_t *reloc=*rit;
		if (reloc->getType() == type)
			return reloc;
	}
	return NULL;
}

template <class T_Sym, class  T_Rela, class T_Rel, class T_Dyn, class T_Extractor>
void MoveGlobals_t<T_Sym,T_Rela,T_Rel,T_Dyn,T_Extractor>::PrintStats()
{
	const auto resorted_moveable_scoops=DataScoopSet_t(ALLOF(moveable_scoops));
	DataScoopSet_t unmoveable_scoops;
	unsigned long long moveable_scoop_bytes=0;
	unsigned long long unmoveable_scoop_bytes=0;
	unsigned long long total_scoop_bytes=0;

	set_difference(
		ALLOF(getFileIR()->getDataScoops()),
		ALLOF(resorted_moveable_scoops),
		inserter(unmoveable_scoops,unmoveable_scoops.end()));
		
	
	if(getenv("MG_VERBOSE"))
	{
		cout<<"Moveable scoops: "<<endl;
		for_each(ALLOF(moveable_scoops), [](DataScoop_t* scoop)
		{
			cout<<"\t"<<scoop->getName()<<", contents: "<<endl;
			auto i=0u;
			const auto max_prints_env=getenv("MG_MAX_SCOOP_CONTENT_PRINT");
			const auto max_prints=max_prints_env ? strtoul(max_prints_env,NULL,0) : 16ul; 

			for(i=0;i+8<scoop->getSize() && i<max_prints;i+=8)
				cout<<"\t\tat:"<<hex<<i<<" value:0x"<<hex<<*(uint64_t*)&scoop->getContents().c_str()[i]<<" "<<endl;
			for(/* empty init */;i<scoop->getSize() && i<max_prints;i++)
				cout<<"\t\tat:"<<hex<<i<<" value:0x"<<hex<< + *(uint8_t*)&scoop->getContents().c_str()[i]<<" "<<endl;
		});
		cout<<"Not moveable scoops: "<<endl;
		for_each(ALLOF(unmoveable_scoops), [](DataScoop_t* scoop)
		{
			cout<<"\t"<<scoop->getName()<<" at  "<<hex<<scoop->getStart()->getVirtualOffset()<<endl;
		});
	}
	// gather number of moveable bytes	
	for_each(moveable_scoops.begin(), moveable_scoops.end(), [&moveable_scoop_bytes, &total_scoop_bytes](DataScoop_t* scoop)
	{
		moveable_scoop_bytes += scoop->getSize();
		total_scoop_bytes+=scoop->getSize();
	});

	// gather number of unmoveable bytes
	for_each(unmoveable_scoops.begin(), unmoveable_scoops.end(), [&unmoveable_scoop_bytes,&total_scoop_bytes](DataScoop_t* scoop)
	{
		unmoveable_scoop_bytes +=scoop->getSize();
		total_scoop_bytes+=scoop->getSize();
	});

        assert(getenv("SELF_VALIDATE")==nullptr || moveable_scoops.size() >= 5);
        assert(getenv("SELF_VALIDATE")==nullptr || (immed_refs_to_scoops.size() + pcrel_refs_to_scoops.size()+absolute_refs_to_scoops.size()) > 5);


	cout<<"# ATTRIBUTE ASSURANCE_Non-Overlapping_Globals::Total_data_items="<<dec<<unmoveable_scoops.size()+moveable_scoops.size()<<endl;
	cout<<"# ATTRIBUTE ASSURANCE_Non-Overlapping_Globals::Unmoveable_data_items="<<dec<<unmoveable_scoops.size()<<endl;
	cout<<"# ATTRIBUTE ASSURANCE_Non-Overlapping_Globals::Moveable_data_items="<<dec<<moveable_scoops.size()<<endl;
	cout<<"# ATTRIBUTE ASSURANCE_Non-Overlapping_Globals::Percent_data_items_moveable="<<std::fixed<<std::setprecision(1)<< ((float)moveable_scoops.size()/((float)(unmoveable_scoops.size()+moveable_scoops.size())))*100.00<<"%"<< endl; 

	cout<<"# ATTRIBUTE ASSURANCE_Non-Overlapping_Globals::Unmoveable_data_items_in_bytes="<<dec<<unmoveable_scoop_bytes<<endl;
	cout<<"# ATTRIBUTE ASSURANCE_Non-Overlapping_Globals::Moveable_data_items_in_bytes="<<dec<<moveable_scoop_bytes<<endl;
	cout<<"# ATTRIBUTE ASSURANCE_Non-Overlapping_Globals::Total_data_items_in_bytes="<<dec<<total_scoop_bytes<<endl;
	cout << "# ATTRIBUTE ASSURANCE_Non-Overlapping_Globals::Percent_data_item_bytes_moved="<<std::fixed<<std::setprecision(1) << ((double)moveable_scoop_bytes/(double)total_scoop_bytes)*100.00 <<"%"<< endl;
	cout << "# ATTRIBUTE ASSURANCE_Non-Overlapping_Globals::Percent_data_item_bytes_not_moved=" << std::fixed <<std::setprecision(1)<< ((double)unmoveable_scoop_bytes/(double)total_scoop_bytes)*100.00 <<"%"<< endl;

	cout<<"# ATTRIBUTE Non-Overlapping_Globals::tied_scoops="<<dec<<tied_scoops.size()<<endl;
	cout<<"# ATTRIBUTE Non-Overlapping_Globals::pcrel_refs="<<dec<<pcrel_refs_to_scoops.size()<<endl;
	cout<<"# ATTRIBUTE Non-Overlapping_Globals::abs_refs="<<dec<<absolute_refs_to_scoops.size()<<endl;
	cout<<"# ATTRIBUTE Non-Overlapping_Globals::imm_refs="<<dec<<immed_refs_to_scoops.size()<<endl;
	cout<<"# ATTRIBUTE Non-Overlapping_Globals::data_refs="<<dec<<data_refs_to_scoops.size()<<endl;
	cout<<"# ATTRIBUTE Non-Overlapping_Globals::coalesced_scoops="<<dec<<tied_unpinned<<endl;
	cout<<"# ATTRIBUTE Non-Overlapping_Globals::repinned_scoops="<<dec<<tied_pinned<<endl;
	cout<<"# ATTRIBUTE Non-Overlapping_Globals::ties_for_folded_constants="<<dec<<ties_for_folded_constants<<endl;
	cout<<"# ATTRIBUTE Non-Overlapping_Globals::tied_scoop_pairs_that_were_already_pinned="<<dec<<tied_nochange<<endl;
	cout<<"#ATTRIBUTE mg::unmoveable_scoops="<<dec<<unmoveable_scoops.size()<<endl;
	cout<<"#ATTRIBUTE mg::moveable_scoops="<<dec<<moveable_scoops.size()<<endl;
	cout<<"#ATTRIBUTE mg::pcrel_refs="<<dec<<pcrel_refs_to_scoops.size()<<endl;
	cout<<"#ATTRIBUTE mg::abs_refs="<<dec<<absolute_refs_to_scoops.size()<<endl;
	cout<<"#ATTRIBUTE mg::imm_refs="<<dec<<immed_refs_to_scoops.size()<<endl;
	cout<<"#ATTRIBUTE mg::data_refs="<<dec<<data_refs_to_scoops.size()<<endl;
	cout<<"#ATTRIBUTE mg::coalesced_scoops="<<dec<<tied_unpinned<<endl;
	cout<<"#ATTRIBUTE mg::repinned_scoops="<<dec<<tied_pinned<<endl;
	cout<<"#ATTRIBUTE mg::ties_for_folded_constants="<<dec<<ties_for_folded_constants<<endl;
	cout<<"#ATTRIBUTE mg::tied_scoop_pairs_that_were_already_pinned="<<dec<<tied_nochange<<endl;
}


// explicit instatnation for elf32 and elf64
template class MoveGlobals_t<Elf32_Sym, Elf32_Rela, Elf32_Rel, Elf32_Dyn, Extractor32_t>;
template class MoveGlobals_t<Elf64_Sym, Elf64_Rela, Elf64_Rel, Elf64_Dyn, Extractor64_t>;

