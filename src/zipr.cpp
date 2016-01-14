/***************************************************************************
 * Copyright (c)  2014  Zephyr Software LLC. All rights reserved.
 *
 * This software is furnished under a license and/or other restrictive
 * terms and may be used and copied only in accordance with such terms
 * and the inclusion of the above copyright notice. This software or
 * any other copies thereof may not be provided or otherwise made
 * available to any other person without the express written consent
 * of an authorized representative of Zephyr Software LCC. Title to,
 * ownership of, and all rights in the software is retained by
 * Zephyr Software LCC.
 *
 * Zephyr Software LLC. Proprietary Information
 *
 * Unless otherwise specified, the information contained in this
 * directory, following this legend, and/or referenced herein is
 * Zephyr Software LLC. (Zephyr) Proprietary Information.
 *
 * CONTACT
 *
 * For technical assistance, contact Zephyr Software LCC. at:
 *
 *
 * Zephyr Software, LLC
 * 2040 Tremont Rd
 * Charlottesville, VA 22911
 * 
 * E-mail: jwd@zephyr-software.com
 **************************************************************************/

#include <zipr_all.h>
#include <libIRDB-core.hpp>
#include <Rewrite_Utility.hpp>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <assert.h>
#include <sys/mman.h>
#include <ctype.h>
#include <iostream>   // std::cout
#include <string>     // std::string, std::to_string
#include <fstream>

#include "elfio/elfio.hpp"
#include "elfio/elfio_dump.hpp"
#include "targ-config.h"
#include "beaengine/BeaEngine.h"

using namespace libIRDB;
using namespace std;
using namespace zipr;
using namespace ELFIO;


int find_magic_segment_index(ELFIO::elfio *elfiop);

template < typename T > std::string to_hex_string( const T& n )
{
	std::ostringstream stm ;
	stm << std::hex<< "0x"<< n ;
	return stm.str() ;
}

template < typename T > std::string to_string( const T& n )
{
	std::ostringstream stm ;
	stm << n ;
	return stm.str() ;
}


static Instruction_t* addNewAssembly(FileIR_t* firp, Instruction_t *p_instr, string p_asm)
{
        Instruction_t* newinstr;
        if (p_instr)
                newinstr = allocateNewInstruction(firp,p_instr->GetAddress()->GetFileID(), p_instr->GetFunction());
        else
                newinstr = allocateNewInstruction(firp,BaseObj_t::NOT_IN_DATABASE, NULL);

        firp->RegisterAssembly(newinstr, p_asm);

        if (p_instr)
        {
                newinstr->SetFallthrough(p_instr->GetFallthrough());
                p_instr->SetFallthrough(newinstr);
        }

        return newinstr;
}

#ifdef CGC
static std::ifstream::pos_type filesize(const char* filename)
{
    	std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);

	if(!in.is_open())
	{
		cerr<<"Cannot open file: "<<filename<<endl;
		throw string("Cannot open file\n");
	}
   	return in.tellg();
}
#endif

void ZiprImpl_t::Init()
{
	bss_needed=0;
	use_stratafier_mode=false;

	ostream *error = &cout, *warn = NULL;

	m_zipr_options.AddNamespace(new ZiprOptionsNamespace_t("global"));
	m_zipr_options.AddNamespace(RegisterOptions(m_zipr_options.Namespace("global")));

	/*
	 * Parse once to read the global and zipr options.
	 */
	m_zipr_options.Parse(NULL, NULL);
	if (m_variant.RequirementMet()) {
		/* setup the interface to the sql server */
		BaseObj_t::SetInterface(&m_pqxx_interface);

		m_variant_id=new VariantID_t(m_variant);
		assert(m_variant_id);
		assert(m_variant_id->IsRegistered()==true);

		if (m_verbose)
			cout<<"New Variant, after reading registration, is: "<<*m_variant_id << endl;

		for(set<File_t*>::iterator it=m_variant_id->GetFiles().begin();
		                           it!=m_variant_id->GetFiles().end();
		                         ++it
		   )
		{
			File_t* this_file=*it;
			assert(this_file);
			// only do a.ncexe for now.
			if(this_file->GetURL().find("a.ncexe")==string::npos)
				continue;

			// read the db
			m_firp=new FileIR_t(*m_variant_id, this_file);
			assert(m_firp);

		}
	}
	plugman = ZiprPluginManager_t(this, &m_zipr_options);

	/*
	 * Parse again now that the plugins registered something.
	 */
	if (m_verbose)
		warn = &cout;
	m_zipr_options.Parse(error, warn);

	if (!m_zipr_options.RequirementsMet()) {
		m_zipr_options.PrintUsage(cout);
		m_error = true;
		return;
	}
	// init  pinned addresses map.
	RecordPinnedInsnAddrs();
}

ZiprImpl_t::~ZiprImpl_t()
{
	delete m_firp;
	try
	{
		m_pqxx_interface.Commit();
	}
	catch (DatabaseError_t pnide)
	{
		cout<<"Unexpected database error: "<<pnide<<endl;
	}
}

ZiprOptionsNamespace_t *ZiprImpl_t::RegisterOptions(ZiprOptionsNamespace_t *global)
{
	ZiprOptionsNamespace_t *zipr_namespace = new ZiprOptionsNamespace_t("zipr");

	m_variant.SetRequired(true);

	m_verbose.SetDescription("Enable verbose output");
	m_apply_nop.SetDescription("Apply NOP to patches that fallthrough.");
	m_variant.SetDescription("Variant ID.");
	m_output_filename.SetDescription("Output file name.");
	m_architecture.SetDescription("Override default system "
		"architecture detection");
	m_replop.SetDescription("Replop all dollops.");
	m_objcopy.SetDescription("Set the path of objcopy to use.");
	m_callbacks.SetDescription("Set the path of the file "
		"which contains any required callbacks.");
	m_seed.SetDescription("Seed the random number generator with this value.");

	// our pid is a fine default value -- had issues with time(NULL) as two copies of zipr from 
	// were getting the same time(NULL) return value since we invoked them in parallel.
	m_seed.SetValue((int)getpid());	

	zipr_namespace->AddOption(&m_output_filename);
	zipr_namespace->AddOption(&m_callbacks);
	zipr_namespace->AddOption(&m_architecture);
	zipr_namespace->AddOption(&m_replop);
	zipr_namespace->AddOption(&m_objcopy);
	zipr_namespace->AddOption(&m_seed);

	global->AddOption(&m_variant);
	global->AddOption(&m_verbose);
	global->AddOption(&m_apply_nop);

	zipr_namespace->MergeNamespace(memory_space.RegisterOptions(global));
	return zipr_namespace;
}

void ZiprImpl_t::CreateBinaryFile()
{
	m_stats = new Stats_t();

	lo = new pqxx::largeobject(m_firp->GetFile()->GetELFOID());
	lo->to_file(m_pqxx_interface.GetTransaction(),string(m_output_filename).c_str());

/* have to figure this out.  we'll want to really strip the binary
 * but also remember the strings. 
 */
#ifdef CGC
	char* sec_tran=getenv("SECURITY_TRANSFORMS_HOME");
	if(sec_tran==NULL)
	{
		cerr<<"Cannot find $SECURITY_TRANSFORMS_HOME variable"<<endl;
		exit(100);
	}
	string cmd=string("cp ")+m_output_filename+" "+m_output_filename+".stripped ; "+ sec_tran+"/third_party/ELFkickers-3.0a/sstrip/sstrip"
		" "+m_output_filename+".stripped";
	printf("Attempting: %s\n", cmd.c_str());
	if(-1 == system(cmd.c_str()))
	{
		perror(__FUNCTION__);
	}
#endif

	if (m_architecture == 0)
	{
		if (m_verbose)
			cout << "Doing architecture autodetection." << endl;
		m_architecture.SetValue(libIRDB::FileIR_t::GetArchitectureBitWidth());
		if (m_verbose)
			cout << "Autodetected to " << (int)m_architecture << endl;
	}

	/*
	 * Take the seed and initialize the random number
	 * generator.
	 */
	std::srand((unsigned)m_seed);
	if (m_verbose)
		cout << "Seeded the random number generator with " << m_seed << "." << endl;

	// create ranges, including extra range that's def. big enough.
	FindFreeRanges(m_output_filename);

	plugman.PinningBegin();

	// add pinned instructions
	AddPinnedInstructions();

	// reserve space for pins
	ReservePinnedInstructions();

	// Emit instruction immediately?

	//TODO: Reenable after option parsing is fixed.
#if 0
	if (m_opts.IsEnabledOptimization(Optimizations_t::OptimizationFallthroughPinned))
	{
		OptimizePinnedFallthroughs();
	}
#endif

	PreReserve2ByteJumpTargets();

	// expand 2-byte pins into 5-byte pins
	ExpandPinnedInstructions();

	while (!two_byte_pins.empty()) 
	{
		/*
		 * Put down the five byte targets
		 * for two byte jumps, if any exist.
		 */
		printf("Going to Fix2BytePinnedInstructions.\n");
		Fix2BytePinnedInstructions();

		/*
		 * If there are still two byte pins, 
		 * try the dance again.
		 */
		if (!two_byte_pins.empty())
		{
			printf("Going to Re PreReserve2ByteJumpTargets.\n");
			PreReserve2ByteJumpTargets();
		}
	}

	// Convert all 5-byte pins into full fragments
	OptimizePinnedInstructions();

	// tell plugins we are done pinning.
	plugman.PinningEnd();

//	NonceRelocs_t nr(memory_space,*elfiop, *m_firp, m_opts);
//	nr.HandleNonceRelocs();

	/*
	 * Let's go through all the instructions
	 * and determine if there are going to be
	 * plugins that want to plop an instruction!
	 */
	AskPluginsAboutPlopping();

	CreateDollops();

	PlaceDollops();

	WriteDollops();

	UpdatePins();

	// tell plugins we are done plopping and about to link callbacks.
	plugman.CallbackLinkingBegin();

	// now that all instructions are put down, we can figure out where the callbacks for this file wil go.
	// go ahead and update any callback sites with the new locations 
	UpdateCallbacks();

	// ask plugman to inform the plugins we are done linking callbacks
	plugman.CallbackLinkingEnd();

	// tell the Nonce class to update it's range of high/low addrs if it used any.
//	nr.UpdateAddrRanges(final_insn_locations);

	m_stats->total_free_ranges = memory_space.GetRangeCount();

	// write binary file to disk 
	OutputBinaryFile(m_output_filename);

	// print relevant information
	PrintStats();
}

static bool in_same_segment(ELFIO::section* sec1, ELFIO::section* sec2, ELFIO::elfio* elfiop)
{
	ELFIO::Elf_Half n = elfiop->segments.size();
	for ( ELFIO::Elf_Half i = 0; i < n; ++i ) 
	{
		uintptr_t segstart=elfiop->segments[i]->get_virtual_address();
		uintptr_t segsize=elfiop->segments[i]->get_file_size();

		/* sec1 in segment i? */
		if(segstart <= sec1->get_address() && sec1->get_address() < (segstart+segsize))
		{
			/* return true if sec2 also in segment */
			/* else return false */
			return (segstart <= sec2->get_address() && sec2->get_address() < (segstart+segsize));
		}
	}

	return false;
}


//
// check if there's padding we can use between this section and the next section.
//
RangeAddress_t ZiprImpl_t::extend_section(ELFIO::section *sec, ELFIO::section *next_sec)
{
	RangeAddress_t start=sec->get_address();
	RangeAddress_t end=sec->get_size()+start;
	if( (next_sec->get_flags() & SHF_ALLOC) != 0 && in_same_segment(sec,next_sec,elfiop))
	{
		end=next_sec->get_address()-1;
		cout<<"Extending range of " << sec->get_name() <<" to "<<std::hex<<end<<endl;
		sec->set_size(next_sec->get_address() - sec->get_address() - 1);
	}
	return end;
}

void ZiprImpl_t::FindFreeRanges(const std::string &name)
{
	/* use ELFIO to load the sections */
//	elfiop=new ELFIO::elfio;

	assert(elfiop);
	elfiop->load(name);
//	ELFIO::dump::header(cout,*elfiop);
	ELFIO::dump::section_headers(cout,*elfiop);

	RangeAddress_t last_end=0;
	RangeAddress_t max_addr=0;

	std::map<RangeAddress_t, int> ordered_sections;

	/*
	 * Make an ordered list of the sections
	 * by their starting address.
	 */
	ELFIO::Elf_Half n = elfiop->sections.size();
	for ( ELFIO::Elf_Half i = 0; i < n; ++i ) 
	{ 
		section* sec = elfiop->sections[i];
		assert(sec);
		ordered_sections.insert(std::pair<RangeAddress_t,int>(sec->get_address(), i));
	}

	std::map<RangeAddress_t, int>::iterator it = ordered_sections.begin();
	for (;it!=ordered_sections.end();) 
	{ 
		section* sec = elfiop->sections[it->second];
		assert(sec);

		RangeAddress_t start=sec->get_address();
		RangeAddress_t end=sec->get_size()+start-1;

		if (m_verbose)
			printf("Section %s:\n", sec->get_name().c_str());

#ifdef CGC
#define EXTEND_SECTIONS 
#endif

#ifdef EXTEND_SECTIONS

		std::map<RangeAddress_t, int>::iterator next_sec_it = it;
		++next_sec_it;
		if(next_sec_it!=ordered_sections.end())
		{
			section* next_sec = elfiop->sections[next_sec_it->second];
			end=extend_section(sec,next_sec);
		}
#endif




		++it;
		if (false)
		//if ((++it) != ordered_sections.end())
		{
			/*
			 * TODO: This works. However, the updated
			 * section size is not properly handled
			 * in OutputBinaryFile. So, it is disabled
			 * until that is handled.
			 */
			section *next_section = elfiop->sections[it->second];

			printf("Using %s as the next section (%p).\n", 
				next_section->get_name().c_str(), 
				(void*)next_section->get_address());
			printf("Modifying the section end. Was %p.", (void*)end);

			end = next_section->get_address() - 1;
			sec->set_size(end - start);
			
			printf(". Is %p.\n", (void*)end);

		}

		if (m_verbose)
			printf("max_addr is %p, end is %p\n", (void*)max_addr, (void*)end);
		if(start && end>max_addr)
		{
			if (m_verbose)
				printf("new max_addr is %p\n", (void*)max_addr);
			max_addr=end;
		}

		if( (sec->get_flags() & SHF_ALLOC) ==0 )
			continue;


		if((sec->get_flags() & SHF_EXECINSTR))
		{
			assert(start>last_end);
			last_end=end;
			if (m_verbose)
				printf("Adding free range 0x%p to 0x%p\n", (void*)start,(void*)end);
			memory_space.AddFreeRange(Range_t(start,end));
		}
	}

#define PAGE_SIZE 4096
#define PAGE_ROUND_UP(x) ( (((uintptr_t)(x)) + PAGE_SIZE-1)  & (~(PAGE_SIZE-1)) )

	// now that we've looked at the sections, add a (mysterious) extra section in case we need to overflow 
	// the sections existing in the ELF.
// skip round up?  not needed if callbacks are PIC/PIE.
#ifndef CGC
	RangeAddress_t new_free_page=PAGE_ROUND_UP(max_addr);
	use_stratafier_mode = true;
#else
	int i=find_magic_segment_index(elfiop);
	RangeAddress_t bytes_remaining_in_file=(RangeAddress_t)filesize((name+".stripped").c_str())-(RangeAddress_t)elfiop->segments[i]->get_offset();
	RangeAddress_t bytes_in_seg=elfiop->segments[i]->get_memory_size();
	RangeAddress_t new_free_page=-1;

	if(bytes_remaining_in_file>=bytes_in_seg)
	{
		cout<<"Note: Found that segment can be extended for free because bss_needed==0"<<endl;
		bss_needed=0;
		new_free_page=elfiop->segments[i]->get_virtual_address()+bytes_remaining_in_file;

		// the end of the file has to be loadable into the bss segment.
		assert(bytes_remaining_in_file==elfiop->segments[i]->get_file_size());
	}
	else
	{
/* experimentally, we add 2 pages using the "add_strata_segment" method. */
/* the "bss_needed" method works pretty well unless there is significantly more than 8k of bss space */
#define BSS_NEEDED_THRESHOLD 8096
		bss_needed=bytes_in_seg-bytes_remaining_in_file;

		if(bss_needed < BSS_NEEDED_THRESHOLD)
		{
			new_free_page=elfiop->segments[i]->get_virtual_address()+bytes_in_seg;
			cout<<"Note: Found that segment can be extended with penalty bss_needed=="<<std::dec<<bss_needed<<endl;
		}
		else
		{
			new_free_page=PAGE_ROUND_UP(max_addr);
			use_stratafier_mode=true;
		}
	}
#endif

	memory_space.AddFreeRange(Range_t(new_free_page,(RangeAddress_t)-1));
	if (m_verbose)
		printf("Adding (mysterious) free range 0x%p to EOF\n", (void*)new_free_page);
	start_of_new_space=new_free_page;
}

void ZiprImpl_t::AddPinnedInstructions()
{

        for(   
                set<Instruction_t*>::const_iterator it=m_firp->GetInstructions().begin();
                it!=m_firp->GetInstructions().end();
                ++it
           )
        {
		Instruction_t* insn=*it;
		assert(insn);

		if(!insn->GetIndirectBranchTargetAddress())
			continue;

		unresolved_pinned_addrs.insert(UnresolvedPinned_t(insn));
	}
}

Instruction_t *ZiprImpl_t::FindPinnedInsnAtAddr(RangeAddress_t addr)
{
        std::map<RangeAddress_t,libIRDB::Instruction_t*>::iterator it=m_InsnAtAddrs.find(addr);
        if(it!=m_InsnAtAddrs.end())
                return it->second;
        return NULL;
}

void ZiprImpl_t::RecordPinnedInsnAddrs()
{
        for(
                set<Instruction_t*>::const_iterator it=m_firp->GetInstructions().begin();
                it!=m_firp->GetInstructions().end();
                ++it
        )
        {
                RangeAddress_t ibta_addr;
                Instruction_t* insn=*it;
                assert(insn);

                if(!insn->GetIndirectBranchTargetAddress()) 
		{
                        continue;
                }
                ibta_addr=(RangeAddress_t)insn->
                        GetIndirectBranchTargetAddress()->
                        GetVirtualOffset();

                m_InsnAtAddrs[ibta_addr]=insn;

        }
}


bool ZiprImpl_t::ShouldPinImmediately(Instruction_t *upinsn)
{
	DISASM d;
	upinsn->Disassemble(d);
	Instruction_t *pin_at_next_byte = NULL;
	AddressID_t *upinsn_ibta = NULL, *ft_ibta = NULL;

	if(d.Instruction.BranchType==RetType)
		return true;

	upinsn_ibta=upinsn->GetIndirectBranchTargetAddress();
	assert(upinsn_ibta!=NULL);

	if (upinsn->GetFallthrough() != NULL)
		ft_ibta=upinsn->GetFallthrough()->GetIndirectBranchTargetAddress();

	/* careful with 1 byte instructions that have a pinned fallthrough */ 
	if(upinsn->GetDataBits().length()==1)
	{
		if(upinsn->GetFallthrough()==NULL)
			return true;
		ft_ibta=upinsn->GetFallthrough()->GetIndirectBranchTargetAddress();
		if(ft_ibta && (upinsn_ibta->GetVirtualOffset()+1) == ft_ibta->GetVirtualOffset())
			return true;
	}

	// find the insn pinned at the next byte.
	pin_at_next_byte = FindPinnedInsnAtAddr(upinsn_ibta->GetVirtualOffset() + 1);
	if ( pin_at_next_byte && 

	/* upinsn has lock prefix */
		upinsn->GetDataBits()[0]==(char)(0xF0) 	&&
	/*
	 * upinsn:  lock cmpxchange op1 op2 [pinned at x]
	 *          x    x+1        x+2 x+3
	 * 
	 * AND pin_at_next_byte (x+1) is:
	 */
		pin_at_next_byte->GetDataBits() == upinsn->GetDataBits().substr(1,upinsn->GetDataBits().length()-1) &&  
	/*
         *               cmpxchange op1 op2 [pinned at x+1]
	 *               x+1        x+2 x+3
	 * AND  pin_at_next_byte->fallthrough() == upinsn->Fallthrough()
	 */
		pin_at_next_byte->GetFallthrough() == upinsn->GetFallthrough() ) 
	/*
	 *
	 * x should become nop, put down immediately
	 * x+1 should become the entire lock command.
	 */
	{
		if (m_verbose)
			cout<<"Using pin_at_next_byte special case, addrs="<<
				upinsn_ibta->GetVirtualOffset()<<","<<
				pin_at_next_byte->GetAddress()->GetVirtualOffset()<<endl;
		/*
		 * Because upinsn is longer than 
		 * 1 byte, we must be somehow
		 * pinned into ourselves. Fix!
		 */

		/*
		 * Make pin_at_next_byte look like upinsn.
		 */
		pin_at_next_byte->SetDataBits(upinsn->GetDataBits());
		pin_at_next_byte->SetComment(upinsn->GetComment());
		pin_at_next_byte->SetCallback(upinsn->GetCallback());
		pin_at_next_byte->SetFallthrough(upinsn->GetFallthrough());
		pin_at_next_byte->SetTarget(upinsn->GetTarget());
		/*
		 * Convert upins to nop.
		 */
		string dataBits = upinsn->GetDataBits();
		dataBits.resize(1);
		dataBits[0] = 0x90;
		upinsn->SetDataBits(dataBits);

		return true;
	}
	return false;
}

void ZiprImpl_t::PreReserve2ByteJumpTargets()
{
	for(set<UnresolvedPinned_t>::const_iterator it=two_byte_pins.begin();
		it!=two_byte_pins.end();
		)
	{
		UnresolvedPinned_t up=*it;
		bool found_close_target = false;
		Instruction_t* upinsn=up.GetInstruction();

		RangeAddress_t addr;
		
		if (up.HasUpdatedAddress())
		{
			addr = up.GetUpdatedAddress();
		}
		else
		{
			addr=upinsn->GetIndirectBranchTargetAddress()->GetVirtualOffset();
		}

		/*
		 * Check for near branch instructions
		 * by starting far away!
		 * Note: two byte jump range is 127 bytes, 
		 * but that's from the pc after it's been 
		 * inc, etc. complicated goo. 120 is a 
		 * safe estimate of range.
		 */
		for(int size=5;size>0;size-=3) 
		{
			if (m_verbose)
				printf("Looking for %d-byte jump targets to pre-reserve.\n", size);
			for(int i=120;i>=-120;i--)
			{
				if(memory_space.AreBytesFree(addr+i,size))
				{
					if (m_verbose)
						printf("Found location for 2-byte->%d-byte conversion "
						"(%p-%p)->(%p-%p) (orig: %p)\n", 
						size,
						(void*)addr,
						(void*)(addr+1),
						(void*)(addr+i),
						(void*)(addr+i+size),
						(upinsn->GetIndirectBranchTargetAddress() != NULL) ?
						(void*)(uintptr_t)upinsn->GetIndirectBranchTargetAddress()->GetVirtualOffset() : 0x0);

					up.SetRange(Range_t(addr+i, addr+i+size));
					for (unsigned int j = up.GetRange().GetStart(); j<up.GetRange().GetEnd(); j++)
					{
						memory_space.SplitFreeRange(j);
					}
					found_close_target = true;
					break;
				}
			}
			if (found_close_target)
				break;
		}

		if (!found_close_target)
		{
			printf("FATAL: No location for near jump reserved.\n");
			assert(false);
			++it;
		}
		else
		{
			UnresolvedPinned_t new_up = UnresolvedPinned_t(up.GetInstruction(), up.GetRange());
			if (up.HasUpdatedAddress())
			{
				new_up.SetUpdatedAddress(up.GetUpdatedAddress());
			}
			two_byte_pins.erase(it++);
			two_byte_pins.insert(new_up);
				
		}
	}
}

static int ceildiv(int a, int b)
{
	return 	(a+b-1)/b;
}


Instruction_t* ZiprImpl_t::Emit68Sled(RangeAddress_t addr, int sled_size, int sled_number, Instruction_t* next_sled)
{
	// find out the sled's output points (either top_of_sled for a miss)
	// or good_to_go for a hit
	Instruction_t *good_to_go=FindPinnedInsnAtAddr(addr+sled_number);
	// if there isn't one, we don't need a sled for this case.
	if(!good_to_go)
		return next_sled;


	const uint32_t push_lookup[]={0x68686868, 0x90686868, 0x90906868,0x90909068, 0x90909090};
	const int number_of_pushed_values=ceildiv(sled_size-sled_number, 5);
	vector<uint32_t> pushed_values(number_of_pushed_values);

	// first pushed value is variable depending on the sled's index
	pushed_values[0]=push_lookup[4-((sled_size-sled_number-1)%5)];
	// other case values are all 0x68686868
	for(int i=1;i<number_of_pushed_values;i++)
	{
		pushed_values[i]=0x68686868;
	}

	/* 
	 * Emit something that looks like:
	 * 	if ( *(tos+0*stack_push_size)!=pushed_values[0] )
	 * 			jmp next_sled; //missed
	 * 	if ( *(tos+1*stack_push_size)!=pushed_values[1] )
	 * 			jmp next_sled; //missed
	 * 		...
	 * 	if ( *(tos+number_of_pushed_values*stack_push_size-1)!=pushed_values[number_of_pushed_values-1] )
	 * 			jmp next_sled; //missed
	 * 	lea rsp, [rsp+push_size]
	 * 	jmp dollop's translation	// found
	*/

	string stack_reg="rsp";
	string decoration="qword";
	if(m_firp->GetArchitectureBitWidth()!=64)
	{
		decoration="dword";
		string stack_reg="esp";
	}
	const int stack_push_size=m_firp->GetArchitectureBitWidth()/8;

	string lea_string=string("lea ")+stack_reg+", ["+stack_reg+"+" + to_string(stack_push_size*number_of_pushed_values)+"]"; 
	Instruction_t *lea=addNewAssembly(m_firp, NULL, lea_string);
	lea->SetFallthrough(good_to_go);

	Instruction_t *old_cmp=lea;

	for(int i=0;i<number_of_pushed_values;i++)
	{
		string cmp_str="cmp "+decoration+" ["+stack_reg+"+ "+to_string(i*stack_push_size)+"], "+to_string(pushed_values[i]);
		Instruction_t* cmp=addNewAssembly(m_firp, NULL, cmp_str); 
		Instruction_t *jne=addNewAssembly(m_firp, NULL, "jne 0"); 
		cmp->SetFallthrough(jne);
		jne->SetTarget(next_sled);
		jne->SetFallthrough(old_cmp);

		cout<<"Adding 68-sled bit:  "+cmp_str+", jne 0 for sled at 0x"<<hex<<addr<<" entry="<<dec<<sled_number<<endl;

		old_cmp=cmp;
	}

	// now that all the cmp/jmp's are insert, we are done with this sled.
	return old_cmp;
}

Instruction_t* ZiprImpl_t::Emit68Sled(RangeAddress_t addr, int sled_size)
{

	Instruction_t *top_of_sled=addNewAssembly(m_firp, NULL, "hlt"); 

	for(int i=sled_size-1;i>=0; i--)
	{
		top_of_sled=Emit68Sled(addr, sled_size, i, top_of_sled);
	}
	return top_of_sled;

}

RangeAddress_t ZiprImpl_t::Do68Sled(RangeAddress_t addr)
{
	char bytes[]={(char)0xeb,(char)0}; // jmp rel8
	const int nop_overhead=4;	// space for nops.
	const int jmp_overhead=2;	// space for nops.
	const int sled_size=Calc68SledSize(addr);
	cout<<"Adding 68-sled at 0x"<<hex<<addr<<" size="<<dec<<sled_size<<endl;

	for(int i=0;i<sled_size;i++)
	{
		cout<<"Adding 68 at "<<hex<<addr+i<<" for sled at 0x"<<hex<<addr<<endl;
		assert(memory_space.find(addr+i) == memory_space.end() );
		memory_space[addr+i]=0x68;	// push opcode and/or data depending on the sled.
		memory_space.SplitFreeRange(addr+i);
	}
	for(int i=0;i<nop_overhead;i++)
	{
		cout<<"Adding 90 at "<<hex<<addr+sled_size+i<<" for sled at 0x"<<hex<<addr<<endl;
		assert(memory_space.find(addr+sled_size+i) == memory_space.end() );
		memory_space[addr+sled_size+i]=0x90;	// push opcode and/or data depending on the sled.
		memory_space.SplitFreeRange(addr+sled_size+i);
	}

	Instruction_t* sled_disambiguation=Emit68Sled(addr, sled_size);

	cout<<"Pin for 68-sled  at 0x"<<hex<<addr<<" is "<<hex<<(addr+sled_size+nop_overhead)<<endl;

	// reserve the bytes for the jump at the end of the sled.  The 68's and 90's are already reserved.
	for(unsigned int i=0;i<jmp_overhead;i++)
	{
		assert(memory_space.find(addr+sled_size+nop_overhead+i) == memory_space.end() );
		memory_space[addr+sled_size+nop_overhead+i]=bytes[i];
		memory_space.SplitFreeRange(addr+sled_size+nop_overhead+i);
	}
	UnresolvedPinned_t cup(sled_disambiguation);
	cup.SetUpdatedAddress(addr+sled_size+nop_overhead);
	two_byte_pins.insert(cup);

	return addr+sled_size+nop_overhead+jmp_overhead;
}


int ZiprImpl_t::Calc68SledSize(RangeAddress_t addr)
{
	const int sled_overhead=6;
	int sled_size=0;
	while(true)
	{
		int i=0;
		for(i=0;i<sled_overhead;i++)
		{
			if(FindPinnedInsnAtAddr(addr+sled_size+i))
			{
				break;
			}
		}
		// if i==sled_overhead, that means that we found 6 bytes in a row free
		// in the previous loop.  Thus, we can end the 68 sled.
		// if i<sled_overhead, we found a in-use byte, and the sled must continue.
		if(i==sled_overhead)
		{
			assert(sled_size>2);
			return sled_size;
		}

		// try a sled that's 1 bigger.
		sled_size+=1;

	}

	// cannot reach here?
	assert(0);

}

bool ZiprImpl_t::IsPinFreeZone(RangeAddress_t addr, int size)
{
	for(int i=0;i<size;i++)
		if(FindPinnedInsnAtAddr(addr+i)!=NULL)
			return false;
	return true;
}



void ZiprImpl_t::ReservePinnedInstructions()
{
	set<UnresolvedPinned_t> reserved_pins;


	/* first, for each pinned instruction, try to put down a jump for the pinned instruction
 	 */
        for(   
		set<UnresolvedPinned_t,pin_sorter_t>::const_iterator it=unresolved_pinned_addrs.begin();
                it!=unresolved_pinned_addrs.end();
                ++it
           )
	{
		char bytes[]={(char)0xeb,(char)0}; // jmp rel8
		UnresolvedPinned_t up=*it;
		Instruction_t* upinsn=up.GetInstruction();
		RangeAddress_t addr=(unsigned)upinsn->GetIndirectBranchTargetAddress()
		                                    ->GetVirtualOffset();

		if(upinsn->GetIndirectBranchTargetAddress()->GetFileID() ==
		   BaseObj_t::NOT_IN_DATABASE)
			continue;

		/* sometimes, we need can't just put down a 2-byte jump into the old slot
	   	 * we may need to do alter our technique if there are two consecutive pinned addresses (e.g. 800 and 801).
		 * That case is tricky, as we can't put even a 2-byte jump instruction down. 
		 * so, we attempt to pin any 1-byte instructions with no fallthrough (returns are most common) immediately.
		 * we also attempt to pin any 1-byte insn that falls through to the next pinned address (nops are common).
		 */
		if(ShouldPinImmediately(upinsn))
		{
			if (m_verbose)
				printf("Final pinning %p-%p.  fid=%d\n", (void*)addr, (void*)(addr+upinsn->GetDataBits().size()-1),
				upinsn->GetAddress()->GetFileID());
			for(unsigned int i=0;i<upinsn->GetDataBits().size();i++)
			{
				memory_space[addr+i]=upinsn->GetDataBits()[i];
				memory_space.SplitFreeRange(addr+i);
				m_stats->total_other_space++;
			}
			continue;
		}

		if (m_verbose) {
			printf("Working two byte pinning decision at %p for:\n", 
					(void*)addr);
			printf("%s\n", upinsn->GetComment().c_str());
		}


		// if the byte at x+1 is free, we can try a 2-byte jump (which may end up being converted to a 5-byte jump later).
		if (FindPinnedInsnAtAddr(addr+1)==NULL)
		{
			if (m_verbose)
			{
				printf("Can fit two-byte pin (%p-%p).  fid=%d\n", 
					(void*)addr,
					(void*)(addr+sizeof(bytes)-1),
					upinsn->GetAddress()->GetFileID());
			}
		
			/*
			 * Assert that the space is free.  We already checked that it should be 
			 * with the FindPinnedInsnAtAddr, but just to be safe.
			 */
			for(unsigned int i=0;i<sizeof(bytes);i++)
			{
				assert(memory_space.find(addr+i) == memory_space.end() );
				memory_space[addr+i]=bytes[i];
				memory_space.SplitFreeRange(addr+i);
			}
			// insert the 2-byte pin to be patched later.
			two_byte_pins.insert(up);
		}
		// this is the case where there are two+ pinned bytes in a row start.
		// check and implement the 2-in-a-row test
		// The way this work is to put down this instruction:
		// 68  --opcode for push 4-byte immed (addr+0)
		// ww				      (addr+1)
		// xx				      (addr+2)
		// yy				      (addr+3)
		// zz				      (addr+4)
		// jmp L1		              (addr+5 to addr+6)
		// ...
		// L1: lea rsp, [rsp+8]
		//     jmp dollop(addr)
		// where ww,xx are un-specified here (later, they will become a 2-byte jump for the pin at addr+1, which will 
		// be handled in other parts of the code.)  However, at a minimum, the bytes for the jmp l1 need to be free
		// and there is little flexibility on the 68 byte, which specifies that ww-zz are an operand to the push.
		// Thus, the jump is at a fixed location.   So, bytes addr+5 and addr+6 must be free.  Also, for the sake of simplicity,
		// we will check that xx, yy and zz are free so that later handling of addr+1 is uncomplicated.
		// This technique is refered to as a "push disambiguator" or sometimes a "push sled" for short.
		else if (IsPinFreeZone(addr+2,5)) 
		{
			if (m_verbose)
				printf("Cannot fit two byte pin; Using 2-in-a-row workaround.\n");
			/*
			 * The whole workaround pattern is:
			 * 0x68 0xXX 0xXX 0xXX 0xXX (push imm)
			 * lea rsp, rsp-8
			 * 0xeb 0xXX (jmp)
			 * 
			 * We put the lea into the irdb and then
			 * put down a pin with that as the target.
			 * We put the original instruction as 
			 * the fallthrough for the lea.
			 */
			char push_bytes[]={(char)0x68,(char)0x00, /* We do not actually write */
					   (char)0x00,(char)0x00, /* all these bytes but they */
					   (char)0x00};           /* make counting easier (see*/
					   		          /* below). */
			Instruction_t *lea_insn = NULL;

			if(m_firp->GetArchitectureBitWidth()==64)
				lea_insn = addNewAssembly(m_firp, NULL, "lea rsp, [rsp+8]");
			else
				lea_insn = addNewAssembly(m_firp, NULL, "lea esp, [esp+4]");

			m_firp->AssembleRegistry();
			lea_insn->SetFallthrough(upinsn);

			/*
			 * Write the push opcode.
			 * Do NOT reserve any of the bytes in the imm value
			 * since those are going to contain the two byte pin
			 * to the adjacent pinned address.
			 */
			memory_space[addr] = push_bytes[0];
			memory_space.SplitFreeRange(addr);

			addr += sizeof(push_bytes);

			// reserve the bytes for the jump at the end of the push.
			for(unsigned int i=0;i<sizeof(bytes);i++)
			{
				assert(memory_space.find(addr+i) == memory_space.end() );
				memory_space[addr+i]=bytes[i];
				memory_space.SplitFreeRange(addr+i);
			}

			if (m_verbose)
				printf("Advanced addr to %p\n", (void*)addr);

			/*
			 * Insert a new UnresolveUnpinned_t that tells future
			 * loops that we are going to use an updated address
			 * to place this instruction.
			 */
			UnresolvedPinned_t cup(lea_insn);
			cup.SetUpdatedAddress(addr);
			two_byte_pins.insert(cup);
		} 
		// If, those bytes aren't free, we will default to a "68 sled".
		// the main concept for a 68 sled is that all bytes will be 68 until we get to an opening where we can "nop out" of 
		// the sled, re-sync the instruction stream, and inspect the stack to see what happened.  Here is an example with 7 pins in a row.
		// 0x8000: 68
		// 0x8001: 68
		// 0x8002: 68
		// 0x8003: 68
		// 0x8004: 68
		// 0x8005: 68
		// 0x8006: 68
		// 0x8007: 90
		// 0x8008: 90
		// 0x8009: 90
		// 0x800a: 90
		// <resync stream>:  at this point regardless of where (between 0x8000-0x8006 the program transfered control,
		// execution will resynchronize.  For example, if we jump to 0x8000, the stream will be 
		//	push 68686868
		//	push 68909090
		//	nop
		//	<resync>
		// But if we jump to  0x8006, our stream will be:
		// 	push 90909090
		// 	<resync>
		// Note that the top of stack will contain 68909090,68686868 if we jumped to 0x8000, but 0x90909090 if we jumped to 0x8006
		// After we resync, we have to inspect the TOS elements to see which instruction we jumped to.
		else if (FindPinnedInsnAtAddr(addr+1))
		{
			RangeAddress_t end_of_sled=Do68Sled(addr);

			// skip over some entries until we get passed the sled.
			while (true)
			{
				// get this entry
				UnresolvedPinned_t up=*it;
				Instruction_t* upinsn=up.GetInstruction();
				RangeAddress_t addr=(unsigned)upinsn->GetIndirectBranchTargetAddress()
		                                    ->GetVirtualOffset();

				// is the entry within the sled?
				if(addr>=end_of_sled)
					// nope, skip out of this while loop
					break;
				// inc the iterator so the for loop will continue at the right place.
				++it;
			}
			// back up one, because the last  one still needs to be processed.
			--it;

			// resolve any new instructions added for the sled.
			m_firp->AssembleRegistry();
		}
		else
			assert(0); // impossible to reach, right?
	

	}
}

void ZiprImpl_t::ExpandPinnedInstructions()
{
	/* now, all insns have 2-byte pins.  See which ones we can make 5-byte pins */
	
	for(
		set<UnresolvedPinned_t>::iterator it=two_byte_pins.begin();
		it!=two_byte_pins.end();
		)
	{
		UnresolvedPinned_t up=*it;
		Instruction_t* upinsn=up.GetInstruction();
		RangeAddress_t addr=0;

		/*
		 * This is possible if we moved the address
		 * forward because we had consecutive pinned
		 * instructions and had to apply the workaround.
		 */
		if (up.HasUpdatedAddress())
		{
			addr = up.GetUpdatedAddress();
		}
		else
		{
			addr = upinsn->GetIndirectBranchTargetAddress()->GetVirtualOffset();
		}

		char bytes[]={(char)0xe9,(char)0,(char)0,(char)0,(char)0}; // jmp rel8
		bool can_update=memory_space.AreBytesFree(addr+2,sizeof(bytes)-2);
		if(can_update)
		{
			if (m_verbose)
				printf("Found %p can be updated to 5-byte jmp\n", (void*)addr);
			memory_space.PlopJump(addr);

			/*
			 * Unreserve those bytes that we reserved before!
			 */
			for (unsigned int j = up.GetRange().GetStart(); j<up.GetRange().GetEnd(); j++)
			{
				memory_space.MergeFreeRange(j);
			}
			up.SetRange(Range_t(0,0));

			five_byte_pins[up]=addr;
			two_byte_pins.erase(it++);
			m_stats->total_5byte_pins++;
			m_stats->total_trampolines++;
		}
		else
		{
			++it;
			if (m_verbose)
				printf("Found %p can NOT be updated to 5-byte jmp\n", (void*)addr);
			m_stats->total_2byte_pins++;
			m_stats->total_trampolines++;
			m_stats->total_tramp_space+=2;
		}
	}

	printf("Totals:  2-byters=%d, 5-byters=%d\n", (int)two_byte_pins.size(), (int)five_byte_pins.size());
}


void ZiprImpl_t::Fix2BytePinnedInstructions()
{
	for(
		set<UnresolvedPinned_t>::const_iterator it=two_byte_pins.begin();
		it!=two_byte_pins.end();
		)
	{
		UnresolvedPinned_t up=*it;
		Instruction_t* upinsn=up.GetInstruction();
		RangeAddress_t addr;
		
		if (up.HasUpdatedAddress())
		{
			addr = up.GetUpdatedAddress();
		}
		else
		{
			addr=upinsn->GetIndirectBranchTargetAddress()->GetVirtualOffset();
		}

		if (up.HasRange())
		{
			/*
			 * Always clear out the previously reserved space.
			 */
			for (unsigned int j = up.GetRange().GetStart(); j<up.GetRange().GetEnd(); j++)
			{
				memory_space.MergeFreeRange(j);
			}

			if (up.GetRange().Is5ByteRange()) 
			{
				if (m_verbose)
					printf("Using previously reserved spot of 2-byte->5-byte conversion "
					"(%p-%p)->(%p-%p) (orig: %p)\n", 
					(void*)addr,
					(void*)(addr+1),
					(void*)(up.GetRange().GetStart()),
					(void*)(up.GetRange().GetEnd()),
					(upinsn->GetIndirectBranchTargetAddress() != NULL) ?
					(void*)(uintptr_t)upinsn->GetIndirectBranchTargetAddress()->GetVirtualOffset() : 0x0);

				five_byte_pins[up] = up.GetRange().GetStart();
				memory_space.PlopJump(up.GetRange().GetStart());
				PatchJump(addr, up.GetRange().GetStart());

				two_byte_pins.erase(it++);
			}
			else if (up.HasRange() && up.GetRange().Is2ByteRange()) 
			{
				/*
				 * Add jump to the reserved space.
				 * Make an updated up that has a new
				 * "addr" so that addr is handled 
				 * correctly the next time through.
				 *
				 * Ie tell two_byte_pins list that
				 * the instruction is now at the jump
				 * target location.
				 */
				UnresolvedPinned_t new_up = 
					UnresolvedPinned_t(up.GetInstruction());
				new_up.SetUpdatedAddress(up.GetRange().GetStart());

				char bytes[]={(char)0xeb,(char)0}; // jmp rel8
				for(unsigned int i=0;i<sizeof(bytes);i++)
				{
					assert(memory_space.find(up.GetRange().GetStart()+i) == memory_space.end() );
					memory_space[up.GetRange().GetStart()+i]=bytes[i];
					memory_space.SplitFreeRange(up.GetRange().GetStart()+i);
					assert(!memory_space.IsByteFree(up.GetRange().GetStart()+i));
				}

				if (m_verbose)
					printf("Patching 2 byte to 2 byte: %p to %p (orig: %p)\n", 
					(void*)addr,
					(void*)up.GetRange().GetStart(),
					(void*)(uintptr_t)upinsn->GetIndirectBranchTargetAddress()->GetVirtualOffset());

				PatchJump(addr, up.GetRange().GetStart());
				two_byte_pins.erase(it++);
				two_byte_pins.insert(new_up);
			}
		}
		else
		{
			printf("FATAL: Two byte pin without reserved range: %p\n", (void*)addr);
			assert(false);
			it++;
		}
	}
}

void ZiprImpl_t::WriteDollops()
{
	list<Dollop_t*>::const_iterator it, it_end;
	for (it = m_dollop_mgr.dollops_begin(),
	     it_end = m_dollop_mgr.dollops_end();
	     it != it_end;
	     it++)
	{
		list<DollopEntry_t*>::const_iterator dit, dit_end;
		Dollop_t *dollop_to_write = *it;

		if (!dollop_to_write->IsPlaced())
			continue;

		for (dit = dollop_to_write->begin(), dit_end = dollop_to_write->end();
		     dit != dit_end;
		     dit++)
		{
			RangeAddress_t start, end, should_end;
			DollopEntry_t *entry_to_write = *dit;

			start = entry_to_write->Place();
			end = _PlopDollopEntry(entry_to_write);
			should_end = start + _DetermineWorstCaseInsnSize(entry_to_write->Instruction(), false);
			assert(end == should_end);
		}
	}
}

void ZiprImpl_t::PlaceDollops()
{
	list<pair<Dollop_t*, RangeAddress_t>> placement_queue;
	multimap<const UnresolvedUnpinned_t,Patch_t>::const_iterator pin_it,
	                                                             pin_it_end;
	/*
	 * Build up initial placement q with destinations of
	 * pins.
	 */
	for (pin_it = patch_list.begin(), pin_it_end = patch_list.end();
	     pin_it != pin_it_end;
	     pin_it++)
	{
		Dollop_t *target_dollop = NULL;
		UnresolvedUnpinned_t uu = (*pin_it).first;
		Patch_t patch = (*pin_it).second;
		Instruction_t *target_insn = NULL;

		target_insn = uu.GetInstruction();
		target_dollop = m_dollop_mgr.GetContainingDollop(target_insn);
		assert(target_dollop);

		placement_queue.push_back(pair<Dollop_t*,RangeAddress_t>(target_dollop,patch.GetAddress()));
		if (m_verbose) {
			cout << "Original: " << std::hex << target_insn->
			                                    GetAddress()->
			                                    GetVirtualOffset() << " "
			     << "vs. Patch: " << std::hex << patch.GetAddress() << endl;
		}
	}

	while (!placement_queue.empty())
	{
		pair<Dollop_t*, RangeAddress_t> pq_entry;
		Dollop_t *to_place = NULL;
		RangeAddress_t from_address;
		size_t minimum_valid_req_size = 0;
		Range_t placement;
		DLFunctionHandle_t placer = NULL;
		bool placed = false;
		list<DollopEntry_t*>::const_iterator dit, dit_end;
		RangeAddress_t cur_addr;
		bool has_fallthrough = false;
		Dollop_t *fallthrough = NULL;
		bool continue_placing = false;

		pq_entry = placement_queue.front();
		placement_queue.pop_front();

		to_place = pq_entry.first;
		from_address = pq_entry.second;

		if (to_place->IsPlaced())
			continue;

		minimum_valid_req_size = _DetermineWorstCaseInsnSize(to_place->
		                                                     front()->
																												 Instruction());
		/*
		 * Ask the plugin manager if there are any plugins
		 * that want to tell us where to place this dollop.
		 */
		if (plugman.DoesPluginAddress(to_place,
		                              from_address,
		                              placement,
		                              placer))
		{
			placed = true;

			if (m_verbose)
				cout << placer->ToString() << " placed this dollop between " 
				     << std::hex << placement.GetStart() << " and " 
						 << std::hex << placement.GetEnd()
						 << endl;

			if ((placement.GetEnd()-placement.GetStart()) < minimum_valid_req_size){
				if (m_verbose)
					cout << "Bad GetNearbyFreeRange() result." << endl;
				placed = false;
			}
		}
		if (!placed) {
			cout << "Using default place locator." << endl;
			placement = memory_space.GetFreeRange(to_place->GetSize());
		}

		cur_addr = placement.GetStart();
		has_fallthrough = (to_place->FallthroughDollop() != NULL);

		if (m_verbose)
		{
			cout << "Placing " << std::dec << to_place->GetSize() << " dollop in "
			     << std::dec << (placement.GetEnd() - placement.GetStart()) 
					 << " hole." << endl
					 << "Dollop " << ((has_fallthrough) ? "has " : "does not have ")
					 << "a fallthrough" << endl;
		}

		assert(to_place->GetSize() != 0);

		do {
			/*
			 * TODO: From here, we want to place the dollop
			 * that we just got a placement for, and subsequently
			 * place any dollops that are fallthroughs!
			 */

			/*
			 * Assume that we will stop placing after this dollop.
			 */
			continue_placing = false;

			to_place->Place(cur_addr);

			for (dit = to_place->begin(), dit_end = to_place->end();
			     dit != dit_end;
			     dit++)
			{
				DollopEntry_t *dollop_entry = *dit;
				/*
				 * There are several ways that a dollop could end:
				 * 1. There is no more fallthrough (handled above with
				 * the iterator through the dollop entries)
				 * 2. There is no more room in this range.
				 *    a. Must account for a link between split dollops
				 *    b. Must account for a possible fallthrough.
				 * So, we can put this dollop entry here if any of
				 * the following are true:
				 * 1. There is enough room for the instruction AND fallthrough.
				 *    Call this the de_and_fallthrough_fit case.
				 * 2. There is enough room for the instruction AND it's the
				 *    last instruction in the dollop AND there is no
				 *    fallthrough.
				 *    Call this the last_de_fits case.
				 */
				bool de_and_fallthrough_fit = false;
				bool last_de_fits = false;
				de_and_fallthrough_fit = (placement.GetEnd()>= /* fits */
				     (cur_addr+_DetermineWorstCaseInsnSize(dollop_entry->Instruction()))
				                         );
				last_de_fits = (std::next(dit,1)==dit_end) /* last */ &&
				               (placement.GetEnd()>=(cur_addr+ /* fits */
							_DetermineWorstCaseInsnSize(dollop_entry->Instruction(),
							                            to_place->FallthroughDollop() != NULL))
							                            /* with or without fallthrough */
							         );

				if (de_and_fallthrough_fit || last_de_fits)
				{
#if 0
					if (m_verbose) {
						DISASM d;
						dollop_entry->Instruction()->Disassemble(d);
						cout << std::hex << cur_addr << ": " << d.CompleteInstr << endl;
					}
#endif
					dollop_entry->Place(cur_addr);
					cur_addr+=_DetermineWorstCaseInsnSize(dollop_entry->Instruction(),
					                                      false);
					if (dollop_entry->TargetDollop())
					{
						if (m_verbose)
							cout << "Adding " << std::hex << dollop_entry->TargetDollop()
							     << " to placement queue." << endl;
						placement_queue.push_back(pair<Dollop_t*, RangeAddress_t>(
								dollop_entry->TargetDollop(),
								cur_addr));
					}
				}
				else
				{
					/*
					 * We cannot fit all the instructions. Let's quit early.
					 */
					break;
				}
			}

			if (dit != dit_end)
			{
				/*
				 * Split the dollop where we stopped being able to place it.
				 * In this case, splitting the dollop will give it a fallthrough.
				 * That will be used below to put in the necessary patch. 
				 *
				 * However ... (see below)
				 */
				Dollop_t *split_dollop = to_place->Split((*dit)->Instruction());
				m_dollop_mgr.AddDollops(split_dollop);

				to_place->WasTruncated(true);

				if (m_verbose)
					cout << "Split a dollop because it didn't fit. Fallthrough to "
					     << std::hex << split_dollop << "." << endl;
			}

			/*
			 * (from above) ... we do not want to "jump" to the
			 * fallthrough if we can simply place it following
			 * this one!
			 */

			if (fallthrough = to_place->FallthroughDollop())
			{
				size_t fallthrough_wcis = _DetermineWorstCaseInsnSize(fallthrough->
				                                                      front()->
																															Instruction());
				size_t remaining_size = placement.GetEnd() - cur_addr;
				if (m_verbose)
					cout << "Determining whether to coalesce: "
					     << "Remaining: " << std::dec << remaining_size
							 << " vs Needed: " << std::dec << fallthrough_wcis << endl;
				if (remaining_size < fallthrough_wcis || fallthrough->IsPlaced())
				{

					string patch_jump_string;
					Instruction_t *patch = addNewAssembly(m_firp, NULL, "jmp qword 0");
					DollopEntry_t *patch_de = new DollopEntry_t(patch, to_place);

					patch_jump_string.resize(5);
					patch_jump_string[0] = (char)0xe9;
					patch_jump_string[1] = (char)0x00;
					patch_jump_string[2] = (char)0x00;
					patch_jump_string[3] = (char)0x00;
					patch_jump_string[4] = (char)0x00;
					patch->SetDataBits(patch_jump_string);

					patch_de->TargetDollop(fallthrough);
					patch_de->Place(cur_addr);
					cur_addr+=_DetermineWorstCaseInsnSize(patch_de->Instruction(),
					                                      false);

					to_place->push_back(patch_de);
					if (m_verbose)
						cout << "Not coalescing"
						     << string((fallthrough->IsPlaced()) ?
								    " because fallthrough is placed; " : "; ")
						     << "Added jump (at " << std::hex << patch_de->Place() << ") "
						     << "to fallthrough dollop (" << std::hex 
						     << fallthrough << ")." << endl;

					placement_queue.push_back(pair<Dollop_t*, RangeAddress_t>(
							fallthrough,
							cur_addr));
					/*
					 * Quit the do-while-true loop that is placing
					 * as many dollops in-a-row as possible.
					 */
					break;
				}
				else
				{
					if (m_verbose)
						cout << "Coalescing fallthrough dollop." << endl;
					/*
					 * Fallthrough is not placed and there is enough room to
					 * put (at least some of) it right below the previous one.
					 */
					to_place = fallthrough;
					continue_placing = true;
				}
			}
		} while (continue_placing); /*
		                 * This is the end of the do-while-true loop
										 * that will place as many fallthrough-linked 
										 * dollops as possible.
										 */
		/*
		 * Reserve the range that we just used.
		 */
		if (m_verbose)
			cout << "Reserving " << std::hex << placement.GetStart()
			     << ", " << std::hex << cur_addr << "." << endl;
		memory_space.SplitFreeRange(Range_t(placement.GetStart(), cur_addr));
	}
}

void ZiprImpl_t::CreateDollops()
{
	multimap<const UnresolvedUnpinned_t,Patch_t>::const_iterator pin_it,
	                                                             pin_it_end;
	for (pin_it = patch_list.begin(), pin_it_end = patch_list.end();
	     pin_it != pin_it_end;
	     pin_it++)
	{
		UnresolvedUnpinned_t uu = (*pin_it).first;
		m_dollop_mgr.AddNewDollops(uu.GetInstruction());
	}
	m_dollop_mgr.UpdateAllTargets();
	if (m_verbose)
		cout << "Created " <<std::dec<< m_dollop_mgr.Size() << " dollops." << endl;
}

void ZiprImpl_t::OptimizePinnedInstructions()
{

	// should only be 5-byte pins by now.

	assert(two_byte_pins.size()==0);


	for(
		std::map<UnresolvedPinned_t,RangeAddress_t>::iterator it=five_byte_pins.begin();
			it!=five_byte_pins.end();
	   )
	{
		RangeAddress_t addr=(*it).second;
		UnresolvedPinned_t up=(*it).first;

		// ideally, we'll try to fill out the pinned 5-byte jump instructions with actual instructions
		// from the program.  That's an optimization.  At the moment, let's just create a patch for each one.

		UnresolvedUnpinned_t uu(up.GetInstruction());
		Patch_t	thepatch(addr,UncondJump_rel32);

		patch_list.insert(pair<const UnresolvedUnpinned_t,Patch_t>(uu,thepatch));
		memory_space.PlopJump(addr);

		DISASM d;
		uu.GetInstruction()->Disassemble(d);

		bool can_optimize=false; // fixme
		if(can_optimize)
		{
			//fixme
		}
		else
		{
			if (m_verbose)
				printf("Converting 5-byte pinned jump at %p-%p to patch to %d:%s\n", 
				(void*)addr,(void*)(addr+4), uu.GetInstruction()->GetBaseID(), d.CompleteInstr);
			m_stats->total_tramp_space+=5;
		}

		// remove and move to next pin
		five_byte_pins.erase(it++);
	}
		
}

void ZiprImpl_t::CallToNop(RangeAddress_t at_addr)
{
	char bytes[]={(char)0x90,(char)0x90,(char)0x90,(char)0x90,(char)0x90}; // nop;nop;nop;nop;nop
	memory_space.PlopBytes(at_addr,bytes,sizeof(bytes));
}

void ZiprImpl_t::PatchCall(RangeAddress_t at_addr, RangeAddress_t to_addr)
{
	uintptr_t off=to_addr-at_addr-5;

	assert(!memory_space.IsByteFree(at_addr));
	
	switch(memory_space[at_addr])
	{
		case (char)0xe8:	/* 5byte call */
		{
			assert(off==(uintptr_t)off);
			assert(!memory_space.AreBytesFree(at_addr+1,4));

			memory_space[at_addr+1]=(char)(off>> 0)&0xff;
			memory_space[at_addr+2]=(char)(off>> 8)&0xff;
			memory_space[at_addr+3]=(char)(off>>16)&0xff;
			memory_space[at_addr+4]=(char)(off>>24)&0xff;
			break;
		}
		default:
			assert(0);

	}
}

void ZiprImpl_t::PatchJump(RangeAddress_t at_addr, RangeAddress_t to_addr)
{
	uintptr_t off=to_addr-at_addr-2;

	assert(!memory_space.IsByteFree(at_addr));
	
	switch(memory_space[at_addr])
	{
		case (char)0xe9:	/* 5byte jump */
		{
			RewritePCRelOffset(at_addr,to_addr,5,1);
			break;
		}
		case (char)0xeb:	/* 2byte jump */
		{
			assert(off==(uintptr_t)(char)off);

			assert(!memory_space.IsByteFree(at_addr+1));
			memory_space[at_addr+1]=(char)off;
		}
	}
}

size_t ZiprImpl_t::_DetermineWorstCaseInsnSize(Instruction_t* insn, bool account_for_jump)
{
	std::map<Instruction_t*,DLFunctionHandle_t>::const_iterator plop_it;
	size_t worst_case_size = 0;

	plop_it = plopping_plugins.find(insn);
	if (plop_it != plopping_plugins.end())
	{
		DLFunctionHandle_t handle = plop_it->second;
		ZiprPluginInterface_t *zpi = dynamic_cast<ZiprPluginInterface_t*>(handle);
		worst_case_size = zpi->WorstCaseInsnSize(insn,account_for_jump,this);
	}
	else
		worst_case_size = DetermineWorstCaseInsnSize(insn, account_for_jump);

	if (m_verbose)
		cout << "Worst case size: " << worst_case_size << endl;

	return worst_case_size;
}

//static int DetermineWorstCaseInsnSize(Instruction_t* insn)
int ZiprImpl_t::DetermineWorstCaseInsnSize(Instruction_t* insn, bool account_for_jump)
{
	return Utils::DetermineWorstCaseInsnSize(insn, account_for_jump);
}

void ZiprImpl_t::AskPluginsAboutPlopping()
{

	for(set<Instruction_t*>::const_iterator it=m_firp->GetInstructions().begin();
	    it!=m_firp->GetInstructions().end();
	    ++it)
	{
		Instruction_t* insn=*it;
		assert(insn);
		DLFunctionHandle_t plopping_plugin;
		if (plugman.DoesPluginPlop(insn, plopping_plugin))
		{
			ZiprPluginInterface_t *zipr_plopping_plugin =
				dynamic_cast<ZiprPluginInterface_t*>(plopping_plugin);
			if (m_verbose)
				cout << zipr_plopping_plugin->ToString()
				     << " will plop this instruction!"
						 << endl;
			plopping_plugins[insn] = plopping_plugin;
			continue;
		}
	}
}

void ZiprImpl_t::UpdatePins()
{
	while(!patch_list.empty())
	{
		UnresolvedUnpinned_t uu=(*patch_list.begin()).first;
		Patch_t p=(*patch_list.begin()).second;
		Dollop_t *target_dollop = NULL;
		DollopEntry_t *target_dollop_entry = NULL;
		Instruction_t *target_dollop_entry_instruction = NULL;
		DISASM d;
		RangeAddress_t patch_addr, target_addr;
		target_dollop = m_dollop_mgr.GetContainingDollop(uu.GetInstruction());
		assert(target_dollop != NULL);

		target_dollop_entry = target_dollop->front();
		assert(target_dollop_entry != NULL);

		target_dollop_entry_instruction  = target_dollop_entry->Instruction();
		assert(target_dollop_entry_instruction != NULL &&
		       target_dollop_entry_instruction == uu.GetInstruction());

		target_dollop_entry_instruction->Disassemble(d);

		patch_addr = p.GetAddress();
		target_addr = target_dollop_entry->Place();

		if (m_verbose)
			cout << "Patching pin at " << std::hex << patch_addr << " to "
			     << std::hex << target_addr << ": " << d.CompleteInstr << endl;
		assert(target_dollop_entry_instruction != NULL &&
		       target_dollop_entry_instruction == uu.GetInstruction());


		PatchJump(patch_addr, target_addr);

		patch_list.erase(patch_list.begin());
	}	
}

void ZiprImpl_t::PatchInstruction(RangeAddress_t from_addr, Instruction_t* to_insn)
{

	// addr needs to go to insn, but insn has not yet been been pinned.


	// patch the instruction at address  addr to go to insn.  if insn does not yet have a concrete address,
	// register that it's patch needs to be applied later. 

	UnresolvedUnpinned_t uu(to_insn);
	Patch_t	thepatch(from_addr,UncondJump_rel32);

	std::map<Instruction_t*,RangeAddress_t>::iterator it=final_insn_locations.find(to_insn);
	if(it==final_insn_locations.end())
	{
		if (m_verbose)
			printf("Instruction cannot be patch yet, as target is unknown.\n");

		patch_list.insert(pair<const  UnresolvedUnpinned_t,Patch_t>(uu,thepatch));
	}
	else
	{
		RangeAddress_t to_addr=final_insn_locations[to_insn];
		assert(to_addr!=0);
		if (m_verbose)
			printf("Found a patch for %p -> %p\n", (void*)from_addr, (void*)to_addr); 
		// Apply Patch
		ApplyPatch(from_addr, to_addr);
	}
}

RangeAddress_t ZiprImpl_t::_PlopDollopEntry(DollopEntry_t *entry)
{
	Instruction_t *insn = entry->Instruction();
	RangeAddress_t addr = entry->Place();
	RangeAddress_t updated_addr;
	std::map<Instruction_t*,DLFunctionHandle_t>::const_iterator plop_it;

	plop_it = plopping_plugins.find(insn);
	if (plop_it != plopping_plugins.end())
	{
		DLFunctionHandle_t handle = plop_it->second;
		RangeAddress_t placed_address = 0;
		ZiprPluginInterface_t *zpi = dynamic_cast<ZiprPluginInterface_t*>(handle);
		updated_addr = zpi->PlopDollopEntry(entry, placed_address, this);
		if (m_verbose)
			cout << "Placed address: " << std::hex << placed_address << endl;
		final_insn_locations[insn] = placed_address;
	}
	else
	{
		final_insn_locations[insn] = addr;
		updated_addr = PlopDollopEntry(entry, addr);
	}
	return updated_addr;
}

#define IS_RELATIVE(A) \
((A.ArgType & MEMORY_TYPE) && (A.ArgType & RELATIVE_))

RangeAddress_t ZiprImpl_t::PlopDollopEntry(
	DollopEntry_t *entry,
	RangeAddress_t override_place)
{
	Instruction_t *insn = entry->Instruction();
	RangeAddress_t ret = entry->Place(), addr = entry->Place();
	bool is_instr_relative = false;
	string raw_data, orig_data; 
	DISASM d;

	assert(insn);

	if (override_place != 0)
		addr = ret = override_place;

	insn->Disassemble(d);

	raw_data = insn->GetDataBits();
	orig_data = insn->GetDataBits();

	is_instr_relative = IS_RELATIVE(d.Argument1) ||
	                    IS_RELATIVE(d.Argument2) ||
											IS_RELATIVE(d.Argument3);
	if (is_instr_relative) {
		ARGTYPE *relative_arg = NULL;
		uint32_t abs_displacement;
		uint32_t *displacement;
		char instr_raw[20] = {0,};
		int size;
		int offset;
		assert(raw_data.length() <= 20);

		/*
		 * Which argument is relative? There must be one.
		 */
		if (IS_RELATIVE(d.Argument1)) relative_arg = &d.Argument1;
		if (IS_RELATIVE(d.Argument2)) relative_arg = &d.Argument2;
		if (IS_RELATIVE(d.Argument3)) relative_arg = &d.Argument3;
		assert(relative_arg);

		/*
		 * Calculate the offset into the instruction
		 * of the displacement address.
		 */
		offset = relative_arg->Memory.DisplacementAddr - d.EIP;

		/*
		 * The size of the displacement address must be
		 * four at this point.
		 */
		size = relative_arg->Memory.DisplacementSize;
		assert(size == 4);

		/*
		 * Copy the instruction raw bytes to a place
		 * where we can modify them.
		 */
		memcpy(instr_raw,raw_data.c_str(),raw_data.length());

		/*
		 * Calculate absolute displacement and relative
		 * displacement.
		 */
		displacement = (uint32_t*)(&instr_raw[offset]);
		abs_displacement = *displacement;
		*displacement = abs_displacement - addr;

		cout<<"absolute displacement: "<< hex << abs_displacement<<endl;
		cout<<"relative displacement: "<< hex << *displacement<<endl;

		/*
		 * Update the instruction with the relative displacement.
		 */
		raw_data.replace(0, raw_data.length(), instr_raw, raw_data.length());
		insn->SetDataBits(raw_data);
	}

	if(entry->TargetDollop())
	{
		if (m_verbose)
			cout << "Plopping at " << std::hex << addr
			     << " with target " << std::hex << entry->TargetDollop()->Place()
					 << endl;
		ret=PlopDollopEntryWithTarget(entry, addr);
	}
	else if(entry->Instruction()->GetCallback()!="")
	{
		if (m_verbose)
			cout << "Plopping at " << std::hex << addr
			     << " with callback to " << entry->Instruction()->GetCallback()
					 << endl;
		ret=PlopDollopEntryWithCallback(entry, addr);
	}
	else
	{
		memory_space.PlopBytes(addr,
		                       insn->GetDataBits().c_str(),
													 insn->GetDataBits().length());
		ret+=insn->GetDataBits().length();
	}

	/* Reset the data bits for the instruction back to th
	 * need to re-plop this instruction later.  we need t
	 * so we can replop appropriately. 
	 */
	insn->SetDataBits(orig_data);
	return ret;
}

RangeAddress_t ZiprImpl_t::PlopDollopEntryWithTarget(
	DollopEntry_t *entry,
	RangeAddress_t override_place)
{
	Instruction_t *insn = entry->Instruction();
	RangeAddress_t target_addr, addr, ret;

	assert(entry->TargetDollop());

	addr = entry->Place();
	target_addr = entry->TargetDollop()->Place();
	ret = addr;

	if (override_place != 0)
		addr = ret = override_place;

	if(insn->GetDataBits().length() >2) 
	{
		memory_space.PlopBytes(ret,
		                       insn->GetDataBits().c_str(),
													 insn->GetDataBits().length());
		ApplyPatch(ret, target_addr);
		ret+=insn->GetDataBits().length();
		return ret;
	}

	// call, jmp, jcc of length 2.
	char b=insn->GetDataBits()[0];
	switch(b)
	{
		case (char)0x70:
		case (char)0x71:
		case (char)0x72:
		case (char)0x73:
		case (char)0x74:
		case (char)0x75:
		case (char)0x76:
		case (char)0x77:
		case (char)0x78:
		case (char)0x79:
		case (char)0x7a:
		case (char)0x7b:
		case (char)0x7c:
		case (char)0x7d:
		case (char)0x7e:
		case (char)0x7f:
		{
		// two byte JCC
			char bytes[]={(char)0x0f,(char)0xc0,(char)0x0,(char)0x0,(char)0x0,(char)0x0 }; 	// 0xc0 is a placeholder, overwritten next statement
			bytes[1]=insn->GetDataBits()[0]+0x10;		// convert to jcc with 4-byte offset.
			memory_space.PlopBytes(ret,bytes, sizeof(bytes));
			ApplyPatch(ret, target_addr);
			ret+=sizeof(bytes);
			return ret;
		}

		case (char)0xeb:
		{
			// two byte JMP
			char bytes[]={(char)0xe9,(char)0x0,(char)0x0,(char)0x0,(char)0x0 }; 	
			bytes[1]=insn->GetDataBits()[0]+0x10;		// convert to jcc with 4-byte offset.
			memory_space.PlopBytes(ret,bytes, sizeof(bytes));
			ApplyPatch(ret, target_addr);
			ret+=sizeof(bytes);
			return ret;
		}

		case (char)0xe0:
		case (char)0xe1:
		case (char)0xe2:
		case (char)0xe3:
		{
			// loop, loopne, loopeq, jecxz
			// convert to:
			// <op> +5:
			// jmp fallthrough
			// +5: jmp target
			char bytes[]={0,0x5};
			DollopEntry_t *fallthrough_de = NULL;

			fallthrough_de = entry->MemberOfDollop()->FallthroughDollopEntry(entry);
			assert(fallthrough_de && fallthrough_de->IsPlaced());

			bytes[0]=insn->GetDataBits()[0];
			memory_space.PlopBytes(ret,bytes, sizeof(bytes));
			ret+=sizeof(bytes);

			memory_space.PlopJump(ret);
			ApplyPatch(ret, fallthrough_de->Place());
			ret+=5;

			memory_space.PlopJump(ret);
			ApplyPatch(ret, target_addr);
			ret+=5;
	
			return ret;
			
		}

		default:
			assert(0);
	}
}

RangeAddress_t ZiprImpl_t::PlopDollopEntryWithCallback(
	DollopEntry_t *entry,
	RangeAddress_t override_place)
{
	RangeAddress_t at = entry->Place(), originalAt = entry->Place();
	Instruction_t *insn = entry->Instruction();

	if (override_place != 0)
		at = originalAt = override_place;

	// emit call <callback>
	{
	char bytes[]={(char)0xe8,(char)0,(char)0,(char)0,(char)0}; // call rel32
	memory_space.PlopBytes(at, bytes, sizeof(bytes));
	unpatched_callbacks.insert(pair<Instruction_t*,RangeAddress_t>(insn,at));
	at+=sizeof(bytes);
	}

	// pop bogus ret addr
	if(m_firp->GetArchitectureBitWidth()==64)
	{
		char bytes[]={(char)0x48,(char)0x8d,(char)0x64,(char)0x24,(char)(m_firp->GetArchitectureBitWidth()/0x08)}; // lea rsp, [rsp+8]
		memory_space.PlopBytes(at, bytes, sizeof(bytes));
		at+=sizeof(bytes);
	}
	else if(m_firp->GetArchitectureBitWidth()==32)
	{
		char bytes[]={(char)0x8d,(char)0x64,(char)0x24,(char)(m_firp->GetArchitectureBitWidth()/0x08)}; // lea esp, [esp+4]
		memory_space.PlopBytes(at, bytes, sizeof(bytes));
		at+=sizeof(bytes);
	}
	else
		assert(0);

	assert(Utils::CALLBACK_TRAMPOLINE_SIZE<=(at-originalAt));
	return at;
}

void ZiprImpl_t::RewritePCRelOffset(RangeAddress_t from_addr,RangeAddress_t to_addr, int insn_length, int offset_pos)
{
	int new_offset=((unsigned int)to_addr)-((unsigned int)from_addr)-((unsigned int)insn_length);

	memory_space[from_addr+offset_pos+0]=(new_offset>>0)&0xff;
	memory_space[from_addr+offset_pos+1]=(new_offset>>8)&0xff;
	memory_space[from_addr+offset_pos+2]=(new_offset>>16)&0xff;
	memory_space[from_addr+offset_pos+3]=(new_offset>>24)&0xff;
}

void ZiprImpl_t::ApplyNopToPatch(RangeAddress_t addr)
{
	/*
	 * TODO: Add assertion that this is really a patch.
	 */
	if (!m_apply_nop)
	{
		if (m_verbose)
			cout << "Skipping chance to apply nop to fallthrough patch." << endl;
		return;
	}
	assert(true);

	/*
	 * 0F 1F 44 00 00H
	 */
	memory_space[addr] = (unsigned char)0x0F;
	memory_space[addr+1] = (unsigned char)0x1F;
	memory_space[addr+2] = (unsigned char)0x44;
	memory_space[addr+3] = (unsigned char)0x00;
	memory_space[addr+4] = (unsigned char)0x00;
}

void ZiprImpl_t::ApplyPatch(RangeAddress_t from_addr, RangeAddress_t to_addr)
{
	unsigned char insn_first_byte=memory_space[from_addr];
	unsigned char insn_second_byte=memory_space[from_addr+1];

	switch(insn_first_byte)
	{
		case (unsigned char)0xF: // two byte escape
		{
			assert( insn_second_byte==(unsigned char)0x80 ||	// should be a JCC 
				insn_second_byte==(unsigned char)0x81 ||
				insn_second_byte==(unsigned char)0x82 ||
				insn_second_byte==(unsigned char)0x83 ||
				insn_second_byte==(unsigned char)0x84 ||
				insn_second_byte==(unsigned char)0x85 ||
				insn_second_byte==(unsigned char)0x86 ||
				insn_second_byte==(unsigned char)0x87 ||
				insn_second_byte==(unsigned char)0x88 ||
				insn_second_byte==(unsigned char)0x89 ||
				insn_second_byte==(unsigned char)0x8a ||
				insn_second_byte==(unsigned char)0x8b ||
				insn_second_byte==(unsigned char)0x8c ||
				insn_second_byte==(unsigned char)0x8d ||
				insn_second_byte==(unsigned char)0x8e ||
				insn_second_byte==(unsigned char)0x8f );

			RewritePCRelOffset(from_addr,to_addr,6,2);
			break;
		}

		case (unsigned char)0xe8:	// call
		case (unsigned char)0xe9:	// jmp
		{
			RewritePCRelOffset(from_addr,to_addr,5,1);
			break;
		}

		case (unsigned char)0xf0: // lock
		case (unsigned char)0xf2: // rep/repe
		case (unsigned char)0xf3: // repne
		case (unsigned char)0x2e: // cs override
		case (unsigned char)0x36: // ss override
		case (unsigned char)0x3e: // ds override
		case (unsigned char)0x26: // es override
		case (unsigned char)0x64: // fs override
		case (unsigned char)0x65: // gs override
		case (unsigned char)0x66: // operand size override
		case (unsigned char)0x67: // address size override
		{
			cout << "found patch for instruction with prefix.  prefix is: "<<hex<<insn_first_byte<<".  Recursing at "<<from_addr+1<<dec<<endl;
			// recurse at addr+1 if we find a prefix byte has been plopped.
			return this->ApplyPatch(from_addr+1, to_addr);
		}
		default:
		{
			if(m_firp->GetArchitectureBitWidth()==64) /* 64-bit x86 machine  assumed */
			{
				/* check for REX prefix */
				if((unsigned char)0x40 <= insn_first_byte  && insn_first_byte <= (unsigned char)0x4f)
				{
					cout << "found patch for instruction with prefix.  prefix is: "<<hex<<insn_first_byte<<".  Recursing at "<<from_addr+1<<dec<<endl;
					// recurse at addr+1 if we find a prefix byte has been plopped.
					return this->ApplyPatch(from_addr+1, to_addr);
				}
			}
			std::cerr << "insn_first_byte: 0x" << hex << (int)insn_first_byte << dec << std::endl;
			assert(0);
		}
	}
}


void ZiprImpl_t::FillSection(section* sec, FILE* fexe)
{
	RangeAddress_t start=sec->get_address();
	RangeAddress_t end=sec->get_size()+start;

	if (m_verbose)
		printf("Dumping addrs %p-%p\n", (void*)start, (void*)end);
	for(RangeAddress_t i=start;i<end;i++)
	{
		if(!memory_space.IsByteFree(i))
		{
			// get byte and write it into exe.
			char  b=memory_space[i];
			int file_off=sec->get_offset()+i-start;
			fseek(fexe, file_off, SEEK_SET);
			fwrite(&b,1,1,fexe);
			if(i-start<200)// keep verbose output short enough.
			{
				if (m_verbose)
					printf("Writing byte %#2x at %p, fileoffset=%x\n", 
						((unsigned)b)&0xff, (void*)i, file_off);
			}
		}
	}
}

void ZiprImpl_t::OutputBinaryFile(const string &name)
{
	assert(elfiop);
//	ELFIO::dump::section_headers(cout,*elfiop);

	string callback_file_name;
	ELFIO::elfio *rewrite_headers_elfiop = new ELFIO::elfio;
	ELFIO::Elf_Half total_sections; 

	/*
	 * Unfortunately, we need to have a special 
	 * "pass" to rewrite section header lengths.
	 * Elfio does not work properly otherwise.
	 */
	rewrite_headers_elfiop->load(name);
	total_sections = rewrite_headers_elfiop->sections.size();
	for ( ELFIO::Elf_Half i = 0; i < total_sections; ++i )
	{
		section* sec = rewrite_headers_elfiop->sections[i];
		assert(sec);

		if( (sec->get_flags() & SHF_ALLOC) == 0 )
			continue;
		if( (sec->get_flags() & SHF_EXECINSTR) == 0)
			continue;

#ifdef EXTEND_SECTIONS
		section* next_sec = NULL;
		if ((i+1)<total_sections)
		{
			next_sec = rewrite_headers_elfiop->sections[i+1];
		}
		extend_section(sec, next_sec);
#endif
	}
	rewrite_headers_elfiop->save(name);


        string myfn=name;
#ifdef CGC
        if(!use_stratafier_mode)
                myfn+=".stripped";
#endif

	printf("Opening %s\n", myfn.c_str());
	FILE* fexe=fopen(myfn.c_str(),"r+");
	assert(fexe);

        // For all sections
        ELFIO::Elf_Half n = elfiop->sections.size();
        for ( ELFIO::Elf_Half i = 0; i < n; ++i )
        {
                section* sec = elfiop->sections[i];
                assert(sec);

                if( (sec->get_flags() & SHF_ALLOC) == 0 )
                        continue;
                if( (sec->get_flags() & SHF_EXECINSTR) == 0)
                        continue;
	
		FillSection(sec, fexe);
        }
	fclose(fexe);

	string tmpname=name+string(".to_insert");
	printf("Opening %s\n", tmpname.c_str());
	FILE* to_insert=fopen(tmpname.c_str(),"w");

	if(!to_insert)
		perror( "void ZiprImpl_t::OutputBinaryFile(const string &name)");

	// first byte of this range is the last used byte.
	RangeSet_t::iterator it=memory_space.FindFreeRange((RangeAddress_t) -1);
	assert(memory_space.IsValidRange(it));

	RangeAddress_t end_of_new_space=it->GetStart();

	printf("Dumping addrs %p-%p\n", (void*)start_of_new_space, (void*)end_of_new_space);
	for(RangeAddress_t i=start_of_new_space;i<end_of_new_space;i++)
	{
		char b=0;
		if(!memory_space.IsByteFree(i))
		{
			b=memory_space[i];
		}
		if(i-start_of_new_space<200)// keep verbose output short enough.
		{
			if (m_verbose)
				printf("Writing byte %#2x at %p, fileoffset=%llx\n", ((unsigned)b)&0xff, 
				(void*)i, (long long)(i-start_of_new_space));
		}
		fwrite(&b,1,1,to_insert);
	}
	fclose(to_insert);

	callback_file_name = AddCallbacksToNewSegment(tmpname,end_of_new_space);
	InsertNewSegmentIntoExe(name,callback_file_name,start_of_new_space);
}


void ZiprImpl_t::PrintStats()
{
	// do something like print stats as #ATTRIBUTES.
	m_dollop_mgr.PrintStats(cout);
	m_stats->PrintStats(cout);

	// and dump a map file of where we placed instructions.  maybe guard with an option.
	// default to dumping to zipr.map 
	dump_map();
}


int find_magic_segment_index(ELFIO::elfio *elfiop)
{
        ELFIO::Elf_Half n = elfiop->segments.size();
	ELFIO::Elf_Half i=0;
	ELFIO::segment* last_seg=NULL;
	int last_seg_index=-1;
        for ( i = 0; i < n; ++i )
        {
                ELFIO::segment* seg = elfiop->segments[i];
                assert(seg);
#if 0
                if( (seg->get_flags() & PF_X) == 0 )
                        continue;
                if( (seg->get_flags() & PF_R) == 0)
                        continue;
		last_seg=seg;
		last_seg_index=i;
		break;
#else
		if(seg->get_type() != PT_LOAD)
			continue;
		if(last_seg && (last_seg->get_virtual_address() + last_seg->get_memory_size()) > (seg->get_virtual_address() + seg->get_memory_size())) 
			continue;
		if(seg->get_file_size()==0)
			continue;
		last_seg=seg;
		last_seg_index=i;
#endif
        }
	cout<<"Found magic Seg #"<<std::dec<<last_seg_index<<" has file offset "<<last_seg->get_offset()<<endl;
	return last_seg_index;
}

void ZiprImpl_t::InsertNewSegmentIntoExe(string rewritten_file, string bin_to_add, RangeAddress_t sec_start)
{

// from stratafy.pl
//       #system("objcopy  --add-section .strata=strata.linked.data.$$ --change-section-address .strata=$maxaddr --set-section-flags .strata=alloc --set-start $textoffset $exe_copy $newfile") == 0 or die ("command failed $? \n") ;

//        system("$stratafier/add_strata_segment $newfile $exe_copy ") == 0 or die (" command failed : $? \n");

	string chmod_cmd="";

	if(use_stratafier_mode)
	{
		string objcopy_cmd = "", stratafier_cmd = "", sstrip_cmd;
		//objcopy_cmd= m_opts.GetObjcopyPath() + string(" --add-section .strata=")+bin_to_add+" "+
		objcopy_cmd= string(m_objcopy) + string(" --add-section .strata=")+bin_to_add+" "+
			string("--change-section-address .strata=")+to_string(sec_start)+" "+
			string("--set-section-flags .strata=alloc,code ")+" "+
			// --set-start $textoffset // set-start not needed, as we aren't changing the entry point.
			rewritten_file;  // editing file in place, no $newfile needed. 
	
		printf("Attempting: %s\n", objcopy_cmd.c_str());
		if(-1 == system(objcopy_cmd.c_str()))
		{
			perror(__FUNCTION__);
		}

		if ( elfiop->get_type() == ET_EXEC )
			stratafier_cmd="$STRATAFIER/add_strata_segment";
		else
			//  move_segheaders is needed for shared objects.
			stratafier_cmd="$STRATAFIER/move_segheaders";

		if (m_architecture == 64) {
			stratafier_cmd += "64";
		}
		stratafier_cmd += " " + rewritten_file+ " " + rewritten_file +".addseg"+" .strata";
		printf("Attempting: %s\n", stratafier_cmd.c_str());
		if(-1 == system(stratafier_cmd.c_str()))
		{
			perror(__FUNCTION__);
		}
#ifdef CGC
		sstrip_cmd=string("")+getenv("SECURITY_TRANSFORMS_HOME")+"/third_party/ELFkickers-3.0a/sstrip/sstrip "+rewritten_file+".addseg";
		printf("Attempting: %s\n", sstrip_cmd.c_str());
		if(-1 == system(sstrip_cmd.c_str()))
		{
			perror(__FUNCTION__);
		}

#endif
	}
	else
	{
#ifndef CGC
		assert(0); // "not stratafier" mode available only for CGC
#else
		string cmd="";
		string zeroes_file=rewritten_file+".zeroes";
		cout<<"Note: bss_needed=="<<std::dec<<bss_needed<<endl;
		cmd="cat /dev/zero | head -c "+to_string(bss_needed)+" > "+zeroes_file;
        	printf("Attempting: %s\n", cmd.c_str());
        	if(-1 == system(cmd.c_str()))
        	{
                	perror(__FUNCTION__);
		}
	
        	cmd="cat "+rewritten_file+".stripped "+zeroes_file+" "+bin_to_add+" > "+rewritten_file+".addseg";
        	printf("Attempting: %s\n", cmd.c_str());
        	if(-1 == system(cmd.c_str()))
        	{
                	perror(__FUNCTION__);
        	}
	
        	std::ifstream::pos_type orig_size=filesize((rewritten_file+".stripped").c_str());
        	std::ifstream::pos_type incr_size=bss_needed+filesize(bin_to_add.c_str());
	
        	assert(orig_size+incr_size=filesize((rewritten_file+".addseg").c_str()));
		std::ifstream::pos_type  total_size=orig_size+incr_size;
	
        	ELFIO::elfio *boutaddseg=new ELFIO::elfio;
        	boutaddseg->load(rewritten_file+".addseg");
        	ELFIO::dump::header(cout,*boutaddseg);
        	ELFIO::dump::segment_headers(cout,*boutaddseg);
	
		cout<<"Segments offset is "<<boutaddseg->get_segments_offset()<<endl;
		
	
		ELFIO::Elf_Half i=find_magic_segment_index(boutaddseg);
	
	
		FILE* fboutaddseg=fopen((rewritten_file+".addseg").c_str(),"r+");
		assert(fboutaddseg);
	
		ELFIO::Elf32_Phdr myphdr;
	
		int file_off=boutaddseg->get_segments_offset()+sizeof(ELFIO::Elf32_Phdr)*(i);
	
	cout<<"Seeking to "<<std::hex<<file_off<<endl;
		fseek(fboutaddseg, file_off, SEEK_SET);
		fread(&myphdr, sizeof(myphdr), 1, fboutaddseg);
	cout<<"My phdr has vaddr="<<std::hex<<myphdr.p_vaddr<<endl;
	cout<<"My phdr has phys addr="<<std::hex<<myphdr.p_paddr<<endl;
	cout<<"My phdr has file size="<<std::hex<<myphdr.p_filesz<<endl;
		myphdr.p_filesz=(int)((int)total_size-(int)myphdr.p_offset);
		myphdr.p_memsz=myphdr.p_filesz;
		myphdr.p_flags|=PF_X|PF_R|PF_W;
	cout<<"Updated file size="<<std::hex<<myphdr.p_filesz<<endl;
		fseek(fboutaddseg, file_off, SEEK_SET);
		fwrite(&myphdr, sizeof(myphdr), 1, fboutaddseg);
		fclose(fboutaddseg);
		
		
		ELFIO::Elf32_Shdr myseg_header;
#endif	// #else from #ifndef CGC
	}

	chmod_cmd=string("chmod +x ")+rewritten_file+".addseg";
	printf("Attempting: %s\n", chmod_cmd.c_str());
	if(-1 == system(chmod_cmd.c_str()))
	{
		perror(__FUNCTION__);
	}

}


/*
FIXME
*/
static RangeAddress_t GetCallbackStartAddr()
{
	// add option later, or write code to fix this
	const RangeAddress_t callback_start_addr=0x8048000;
return 0;
	return callback_start_addr;
}


string ZiprImpl_t::AddCallbacksToNewSegment(const string& tmpname, RangeAddress_t end_of_new_space)
{
	const RangeAddress_t callback_start_addr=GetCallbackStartAddr();

	//if(m_opts.GetCallbackFileName() == "" )
	if(m_callbacks == "" )
		return tmpname;
	string tmpname2=tmpname+"2";	
	string tmpname3=tmpname+"3";	
	printf("Setting strata library at: %p\n", (void*)end_of_new_space);
	printf("Strata symbols are at %p+addr(symbol)\n", (void*)(end_of_new_space-callback_start_addr));
#if 0
	string cmd= string("$STRATAFIER/strata_to_data ")+
		m_opts.GetCallbackFileName()+string(" ")+tmpname2+" "+to_hex_string(callback_start_addr);
#else
	/*
		objcopy -O binary /home/jdh8d/umbrella/uvadev.peasoup/zipr_install/bin/callbacks.exe b.out.to_insert2
	*/

	//string cmd= m_opts.GetObjcopyPath() + string(" -O binary ")+ m_opts.GetCallbackFileName()+string(" ")+tmpname2;
	string cmd= string(m_objcopy) + string(" -O binary ")+string(m_callbacks)+string(" ")+tmpname2;
#endif
	printf("Attempting: %s\n", cmd.c_str());
	if(-1 == system(cmd.c_str()))
	{
		perror(__FUNCTION__);
		return tmpname;
	}

	cmd="cat "+tmpname+" "+tmpname2+" > "+tmpname3;
	printf("Attempting: %s\n", cmd.c_str());
	if(-1 == system(cmd.c_str()))
	{
		perror(__FUNCTION__);
		return tmpname;
	}
	return tmpname3;
}

// horrible code, rewrite in C++ please!
static RangeAddress_t getSymbolAddress(const string &symbolFilename, const string &symbol) throw(exception)
{
        string symbolFullName = symbolFilename + "+" + symbol;

// nm -a stratafier.o.exe | egrep " integer_overflow_detector$" | cut -f1 -d' '
        string command = "nm -a " + symbolFilename + " | egrep \" " + symbol + "$\" | cut -f1 -d' '";
        char address[1024]="";

	cerr<<"Attempting: "<<command<<endl;

        FILE *fp = popen(command.c_str(), "r");

        int res=fscanf(fp,"%s", address);
	cerr<<"Looking for "<<symbol<<".  Address string is "<<address<<endl;
        string addressString = string(address);
        pclose(fp);

        RangeAddress_t ret= (uintptr_t) strtoull(addressString.c_str(),NULL,16);

        //TODO: throw exception if address is not found.
        //for now assert the address string isn't empty
        if(addressString.empty() || res==0)
        {
                cerr<<"Cannot find symbol "<< symbol << " in " << symbolFilename << "."<<endl;
		addressString="0x0";
		return 0;
        }
	else
	{
		cerr<<"Found symbol "<< symbol << " in " << symbolFilename << " at " << std::hex << ret << "."<<endl;
		return ret;
	}

}


RangeAddress_t ZiprImpl_t::FindCallbackAddress(RangeAddress_t end_of_new_space, RangeAddress_t start_addr, const string &callback)
{
	if(callback_addrs.find(callback)==callback_addrs.end())
	{

		//RangeAddress_t addr=getSymbolAddress(m_opts.GetCallbackFileName(),callback);
		RangeAddress_t addr=getSymbolAddress(m_callbacks,callback);

		if(addr!=0)
		{
			/* adjust by start of new location, - beginning of old location */
			addr=addr+end_of_new_space-start_addr;
		}
		cout<<" Addr adjusted to "<<std::hex<<addr<<endl;
		callback_addrs[callback]=addr;
	}
	return callback_addrs[callback];

}

void ZiprImpl_t::UpdateCallbacks()
{
        // first byte of this range is the last used byte.
	RangeSet_t::iterator it=memory_space.FindFreeRange((RangeAddress_t) -1);
        assert(memory_space.IsValidRange(it));

        RangeAddress_t end_of_new_space=it->GetStart();
	RangeAddress_t start_addr=GetCallbackStartAddr();

	for( std::set<std::pair<libIRDB::Instruction_t*,RangeAddress_t> >::iterator it=unpatched_callbacks.begin();
		it!=unpatched_callbacks.end();
		++it
	   )
	{
		Instruction_t *insn=it->first;
		RangeAddress_t at=it->second;
		RangeAddress_t to=FindCallbackAddress(end_of_new_space,start_addr,insn->GetCallback());
		if(to)
		{
			cout<<"Patching callback "<< insn->GetCallback()<<"at "<<std::hex<<at<<" to jump to "<<to<<endl;
			PatchCall(at,to);
		}
		else
			CallToNop(at);
	}
}

void ZiprImpl_t::dump_map()
{

// std::map<libIRDB::Instruction_t*,RangeAddress_t> final_insn_locations
	string filename="zipr.map";	// parameterize later.
    	std::ofstream ofs(filename.c_str(), ios_base::out);

	ofs <<left<<setw(10)<<"ID"
	    <<left<<setw(10)<<"OrigAddr"
	    <<left<<setw(10)<<"IBTA"
	    <<left<<setw(10)<<"NewAddr"
	    <<left<<"Disassembly"<<endl;

	for(std::map<libIRDB::Instruction_t*,RangeAddress_t>::iterator it=final_insn_locations.begin();
		it!=final_insn_locations.end(); ++it)
	{
		Instruction_t* insn=it->first;
		AddressID_t* ibta=insn->GetIndirectBranchTargetAddress();
		RangeAddress_t addr=it->second;

		ofs << hex << setw(10)<<insn->GetBaseID()
		    <<hex<<left<<setw(10)<<insn->GetAddress()->GetVirtualOffset()
		    <<hex<<left<<setw(10)<< (ibta ? ibta->GetVirtualOffset() : 0)
		    <<hex<<left<<setw(10)<<addr
		    << left<<insn->getDisassembly()<<endl;

		
	}



}
