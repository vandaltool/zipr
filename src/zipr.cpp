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


inline uintptr_t page_round_up(uintptr_t x)
{
	const int page_size=4096;
	return  ( (((uintptr_t)(x)) + page_size-1)  & (~(page_size-1)) );
}

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

#if 0
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

#endif

#ifdef support_stratafier_mode
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


	m_add_sections.SetDescription("Enable writing of section headers using elfwriter.");
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
	m_dollop_map_filename.SetDescription("Specify filename to save dollop map.");

	// our pid is a fine default value -- had issues with time(NULL) as two copies of zipr from 
	// were getting the same time(NULL) return value since we invoked them in parallel.
	m_seed.SetValue((int)getpid());	

	zipr_namespace->AddOption(&m_output_filename);
	zipr_namespace->AddOption(&m_callbacks);
	zipr_namespace->AddOption(&m_architecture);
	zipr_namespace->AddOption(&m_replop);
	zipr_namespace->AddOption(&m_objcopy);
	zipr_namespace->AddOption(&m_seed);
	zipr_namespace->AddOption(&m_dollop_map_filename);

	global->AddOption(&m_variant);
	global->AddOption(&m_verbose);
	global->AddOption(&m_apply_nop);
	global->AddOption(&m_add_sections);

	zipr_namespace->MergeNamespace(memory_space.RegisterOptions(global));
	return zipr_namespace;
}

void ZiprImpl_t::CreateBinaryFile()
{
	m_stats = new Stats_t();


	/* load the elfiop for the orig. binary */
	lo = new pqxx::largeobject(m_firp->GetFile()->GetELFOID());
	lo->to_file(m_pqxx_interface.GetTransaction(),string(m_output_filename).c_str());

	/* use ELFIO to load the sections */
	assert(elfiop);
	elfiop->load(m_output_filename);
	ELFIO::dump::section_headers(cout,*elfiop);

/* have to figure this out.  we'll want to really strip the binary
 * but also remember the strings. 
 */
#ifdef support_stratafier_mode
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


	FixMultipleFallthroughs();

	// create ranges, including extra range that's def. big enough.
	FindFreeRanges(m_output_filename);

	plugman.PinningBegin();

	// Initial creation of the set of pinned instructions.
	AddPinnedInstructions();

	// Reserve space for pins.
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

	RecalculateDollopSizes();

	plugman.DollopBegin();

	PlaceDollops();

	plugman.DollopEnd();

	WriteDollops();

	ReplopDollopEntriesWithTargets();

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

	// Update any scoops that were written by Zipr.
	UpdateScoops();

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

void ZiprImpl_t::CreateExecutableScoops(const std::map<RangeAddress_t, int> &ordered_sections)
{
	int count=0;
	/*
	 * For each section, ...
	 */
	for(std::map<RangeAddress_t, int>::const_iterator it = ordered_sections.begin();
		it!=ordered_sections.end();
		) 
	{
		section* sec = elfiop->sections[it->second];
		assert(sec);

		// skip non-exec and non-alloc sections.
		if( (sec->get_flags() & SHF_ALLOC) ==0 || (sec->get_flags() & SHF_EXECINSTR) ==0 )
		{
			++it;
			continue;
		}

		// setup start of scoop.
		AddressID_t *text_start=new AddressID_t();
		text_start->SetVirtualOffset(sec->get_address());
		m_firp->GetAddresses().insert(text_start);

		/*
		 * ... walk the subsequent sections and coalesce as many as possible
		 * into a single executable address range.
		 */
		while(1)
		{
			sec = elfiop->sections[it->second];

			// skip non-alloc sections.
			if( (sec->get_flags() & SHF_ALLOC) ==0)
			{
				++it;
				continue;
			}
			// stop if not executable.
			if( (sec->get_flags() & SHF_EXECINSTR) ==0 )
				break;

			// try next 
			++it;
			if(it==ordered_sections.end())
				break;
		}

		// setup end of scoop address
		AddressID_t *text_end=new AddressID_t();

		// two cases for end-of-scoop 
		if (it==ordered_sections.end())
			// 1 ) this is the last section
			text_end->SetVirtualOffset(page_round_up(sec->get_address()+sec->get_size()-1)-1);
		else
			// 2 ) another section gets in the way.
			text_end->SetVirtualOffset(sec->get_address()-1);

		// insert into IR
		m_firp->GetAddresses().insert(text_end);


		// setup a scoop for this section.
		// zero init is OK, after zipring we'll update with the right bytes.
		string text_contents;
		string text_name=string(".zipr_text_")+to_string(count++);
		if(count==1)
			text_name=".text"; // use the name .text first.

		text_contents.resize(text_end->GetVirtualOffset() - text_start->GetVirtualOffset()+1);
		DataScoop_t* text_scoop=new DataScoop_t(BaseObj_t::NOT_IN_DATABASE, text_name,  text_start, text_end, NULL, 5, false, text_contents);
		m_firp->GetDataScoops().insert(text_scoop);
	
		cout<<"Adding scoop "<<text_scoop->GetName()<<hex<<" at "<<hex<<text_start->GetVirtualOffset()<<" - "<<text_end->GetVirtualOffset()<<endl;
		m_zipr_scoops.insert(text_scoop);
		memory_space.AddFreeRange(Range_t(text_start->GetVirtualOffset(),text_end->GetVirtualOffset()), true);
	}
}


RangeAddress_t ZiprImpl_t::PlaceUnplacedScoops(RangeAddress_t max_addr)
{
	max_addr=plugman.PlaceScoopsBegin(max_addr);

	map<int,DataScoopSet_t> scoops_by_perms;

	for(
		DataScoopSet_t::iterator dit=m_firp->GetDataScoops().begin(); 
		dit!=m_firp->GetDataScoops().end();
		++dit
	   )
	{
		DataScoop_t* scoop=*dit;

		// check if placed.
		if(scoop->GetStart()->GetVirtualOffset()==0)
			scoops_by_perms[scoop->isRelRo() << 16 | scoop->getRawPerms()].insert(scoop);
	}
	
	for(map<int,DataScoopSet_t>::iterator pit=scoops_by_perms.begin(); pit!=scoops_by_perms.end(); ++pit)
	{
		// start by rounding up to a page boundary so that page perms don't get unioned.
		max_addr=page_round_up(max_addr);
		for( DataScoopSet_t::iterator dit=pit->second.begin(); dit!=pit->second.end(); ++dit)
		{
			DataScoop_t* scoop=*dit;
			max_addr=align_up_to(max_addr,(RangeAddress_t)16);	// 16 byte align.
			scoop->GetStart()->SetVirtualOffset(scoop->GetStart()->GetVirtualOffset()+max_addr);
			scoop->GetEnd()->SetVirtualOffset(scoop->GetEnd()->GetVirtualOffset()+max_addr);

			// update so we actually place things at diff locations.
			max_addr=scoop->GetEnd()->GetVirtualOffset()+1;

			cout<<"Placing scoop "<<scoop->GetName()<<" at "
			    <<hex<<scoop->GetStart()->GetVirtualOffset()<<"-"
			    <<hex<<scoop->GetEnd()->GetVirtualOffset()<<endl;
		}
	}
	
	
	max_addr=plugman.PlaceScoopsEnd(max_addr);

	return max_addr;
}

void ZiprImpl_t::FindFreeRanges(const std::string &name)
{
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


	CreateExecutableScoops(ordered_sections);


	// scan sections for a max-addr.
	std::map<RangeAddress_t, int>::iterator it = ordered_sections.begin();
	for (;it!=ordered_sections.end();) 
	{ 
		section* sec = elfiop->sections[it->second];
		assert(sec);

		RangeAddress_t start=sec->get_address();
		RangeAddress_t end=sec->get_size()+start-1;

		++it;

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


	}

	// scan scoops for a max-addr.
	for(
		DataScoopSet_t::iterator it=m_firp->GetDataScoops().begin(); 
		it!=m_firp->GetDataScoops().end();
		++it
	   )
	{
		DataScoop_t* scoop=*it;
		RangeAddress_t  end=scoop->GetEnd()->GetVirtualOffset();

		if(end >= max_addr)
		{
			max_addr=end+1;
		}
	}

	max_addr=PlaceUnplacedScoops(max_addr);

	// now that we've looked at the sections, add a (mysterious) extra section in case we need to overflow 
	// the sections existing in the ELF.
	RangeAddress_t new_free_page=page_round_up(max_addr);


	memory_space.AddFreeRange(Range_t(new_free_page,(RangeAddress_t)-1), true);
	if (m_verbose)
		printf("Adding (mysterious) free range 0x%p to EOF\n", (void*)new_free_page);
	start_of_new_space=new_free_page;
}

void ZiprImpl_t::AddPinnedInstructions()
{
	// find the big chunk of free memory in case we need it for unassigned pins.
	virtual_offset_t next_pin_addr=memory_space.GetInfiniteFreeRange().GetStart();

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

		// deal with unassigned IBTAs.
		if(insn->GetIndirectBranchTargetAddress()->GetVirtualOffset()==0)
		{
			insn->GetIndirectBranchTargetAddress()->SetVirtualOffset(next_pin_addr);
			next_pin_addr+=5;// sizeof pin
		}

		unresolved_pinned_addrs.insert(UnresolvedPinned_t(insn));
	}
}

Instruction_t *ZiprImpl_t::FindPatchTargetAtAddr(RangeAddress_t addr)
{
        std::map<RangeAddress_t,UnresolvedUnpinnedPatch_t>::iterator it=m_PatchAtAddrs.find(addr);
        if(it!=m_PatchAtAddrs.end())
                return it->second.first.GetInstruction();
        return NULL;
}

Instruction_t *ZiprImpl_t::FindPinnedInsnAtAddr(RangeAddress_t addr)
{
        std::map<RangeAddress_t,std::pair<libIRDB::Instruction_t*, size_t> >::iterator it=m_InsnSizeAtAddrs.find(addr);
        if(it!=m_InsnSizeAtAddrs.end())
                return it->second.first;
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
		/*
		* Record the size of thing that we are pinning.
		* We are going to use this information for doing
		* sleds, if we have to.
		*
		* There are two different possibilities: 
		*
		* 1. The instruction is turned into a jump. By default,
		* we assume that turns into a two byte relative jump.
		* This information will *not* be used in the case that
		* this pin is overriden by a patch. In other words, when
		* this pinned address is handled in ExpandPinnedInstructions()
		* then it might be expanded into a five byter. However,
		* when we deal this in the context of putting down a sled,
		* we check for patches before falling back to this method.
		*
		*
		* 2. The instruction cannot be turned into a jump -- it
		* must be pinned immediately. In that case, we want to
		* record the size of the instruction itself.
		*/
		if (ShouldPinImmediately(insn))
			m_InsnSizeAtAddrs[ibta_addr]=std::pair<Instruction_t*, size_t>(insn,insn->GetDataBits().length());
		else					 
			m_InsnSizeAtAddrs[ibta_addr]=std::pair<Instruction_t*, size_t>(insn,2);
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
	bool repeat = false;

	do
	{
		repeat = false;
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
			
			if (m_AddrInSled[addr])
			{
				/*
				 * There is no need to consider this pin at all! It was
				 * subsumed w/in a sled which has already handled it.
				 * Move along.
				 */
				if (m_verbose)
					cout << "Two byte pin at 0x" 
					     << std::hex << addr 
							 << " is w/in a sled ... skipping and erasing." << endl;
				two_byte_pins.erase(it++);
				continue;
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

				//if (m_verbose)
				//	printf("Looking for %d-byte jump targets to pre-reserve.\n", size);
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

						/*
						 * We add chain entries early, as soon as the patch is 
						 * prereserved. When we use it, we don't have to worry about it
						 * already being written as a patch. However, if we don't use it,
						 * we will need to remove it. See ExpandPinnedInstructions() for
						 * the place where we do the removal.
						 *
						 * addr: place of prereserved memory
						 * size: size of the amount of prereserved memory
						 */
						UnresolvedUnpinned_t uu(up);
						Patch_t patch(up.GetRange().GetStart(),
						              UnresolvedType_t::UncondJump_rel32);
						if (size == 2)
							patch.SetType(UnresolvedType_t::UncondJump_rel8);
						UnresolvedUnpinnedPatch_t uup(uu, patch);

						if (m_verbose)
							cout << "Adding a chain entry at address "
							     << std::hex << up.GetRange().GetStart() << endl;
						m_PatchAtAddrs.insert(
							std::pair<RangeAddress_t, UnresolvedUnpinnedPatch_t>(up.GetRange().GetStart(),uup));

						found_close_target = true;
						break;
					}
				}
				if (found_close_target)
					break;
			}

			if (!found_close_target)
			{
				/*
				 * In the case that we did not find a nearby
				 * space for placing a 2/5 jmp, we are going
				 * to fall back to using a sled.
				 */
				
				/*
				 * Algorithm:
				 * 1. Insert the sled at addr and record where it ends (end_addr).
				 * 2. For each pinned instruction, i, between addr and end_addr:
				 *    a. If i exists in two_byte_pins indicating that it will
				 *       be handled, we remove it.
				 *    b. If i has prereserved space associated with it, we
				 *       we clear that space.
				 */
				printf("Warning: No location for near jump reserved at 0x%x.\n", addr);

				/*
				 * The first thing that we have to do is to tell
				 * ourselves that this is a chain entry.
				 */
				if (m_verbose)
					cout << "Added emergency patch entry at "
					     << std::hex << addr << endl;
				Patch_t emergency_patch(addr, UnresolvedType_t::UncondJump_rel8);
				UnresolvedUnpinned_t uu(up);
				UnresolvedUnpinnedPatch_t uup(uu, emergency_patch);
				m_PatchAtAddrs.insert(
					std::pair<RangeAddress_t, UnresolvedUnpinnedPatch_t>(addr,uup));

				RangeAddress_t end_of_sled = Do68Sled(addr);

				m_firp->AssembleRegistry();
				for (RangeAddress_t i = addr; i<end_of_sled; i++)
				{
					Instruction_t *found_pinned_insn = NULL;
					if (found_pinned_insn = FindPinnedInsnAtAddr(i))
					{
						/*
						 * TODO: Continue from here. Continue implementing the above
						 * algorithm. Don't forget to handle the case that Jason was
						 * explaining where a range of bytes in this sled may actually
						 * contain a two byte jmp that points to the ultimate
						 * five byte jump that ultimately terminates the chain.
						 */
					}
				}
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
	} while (repeat);
}

static int ceildiv(int a, int b)
{
	return 	(a+b-1)/b;
}

void ZiprImpl_t::InsertJumpPoints68SledArea(Sled_t &sled)
{
	for (RangeAddress_t addr = sled.SledRange().GetStart();
	     addr < sled.SledRange().GetEnd();
			 addr++)
	{
		bool is_pin_point = false, is_patch_point = false;
		/*
		 * There is the possibility that the sled is being put here
		 * because we have a pin that cannot be properly handled.
		 */
		is_pin_point = (NULL != FindPinnedInsnAtAddr(addr));
		if (m_verbose && is_pin_point)
			cout << "There is a pin at 0x" << std::hex << addr
			     << " inside a sled." << endl;

		/*
		 * There is the possibility that the sled is being put here
		 * because we have a chain entry that cannot properly be 
		 * handled.
		 */
		is_patch_point = FindPatchTargetAtAddr(addr);
		if (is_patch_point)
		{
			
			cout << "There is a patch at 0x"
			     << std::hex << addr << " inside a sled." << endl;
		}

		if (is_pin_point || is_patch_point)
		{
			if (m_verbose)
				cout << "Adding Jump Point at 0x" << std::hex << addr << endl;
			sled.AddJumpPoint(addr);
		}
	}
}

Instruction_t* ZiprImpl_t::Emit68Sled(RangeAddress_t addr, Sled_t sled, Instruction_t* next_sled)
{
	Instruction_t *sled_start_insn = NULL;
	unsigned int sled_number = addr - sled.SledRange().GetStart();
	size_t sled_size = sled.SledRange().GetEnd() - sled.SledRange().GetStart();

	sled_start_insn = FindPinnedInsnAtAddr(addr);
	if (!sled_start_insn)
		sled_start_insn = FindPatchTargetAtAddr(addr);
	assert(sled_start_insn != NULL);	

	if (m_verbose)
		cout << "Begin emitting the 68 sled @ " << addr << "." << endl;

	const uint32_t push_lookup[]={0x68686868,
	                              0x90686868,
	                              0x90906868,
	                              0x90909068,
	                              0x90909090};
	const int number_of_pushed_values=ceildiv(sled_size-sled_number, 5);
	vector<uint32_t> pushed_values(number_of_pushed_values, 0x68686868);

	// first pushed value is variable depending on the sled's index
	pushed_values[0] = push_lookup[4-((sled_size-sled_number-1)%5)];

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
	lea->SetFallthrough(sled_start_insn);

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

	/*
	 * now that all the cmp/jmp's are inserted, we are done with this sled.
	 */
	return old_cmp;
}

Instruction_t* ZiprImpl_t::Emit68Sled(Sled_t sled)// RangeAddress_t addr, int sled_size)
{

	Instruction_t *top_of_sled=addNewAssembly(m_firp, NULL, "hlt"); 

	for (std::set<RangeAddress_t>::reverse_iterator
	     addr_iter=sled.JumpPointsReverseBegin();
	     addr_iter != sled.JumpPointsReverseEnd();
			 addr_iter++)
	{
		RangeAddress_t addr = *addr_iter;
		if (m_verbose)
			cout << "Specific Emit68Sled(" 
			     << std::hex << addr << ","
			     << sled << ","
			     << std::hex << top_of_sled << ");" << endl;

		top_of_sled=Emit68Sled(addr, sled, top_of_sled);
	}
	return top_of_sled;
}

/*
 * Put the new sled into the existing sled.
 * and do some other things.
 * Note that the clearable sled is just the difference
 * between the new and the old. We do not 
 * need to clear out any of the existing sled 
 * since it is just PUSHs at this point!
 */
void ZiprImpl_t::Update68Sled(Sled_t new_sled, Sled_t &existing_sled)
{
	Range_t clearable_sled_range(new_sled.SledRange().GetStart(),
	                             existing_sled.SledRange().GetStart());
	Sled_t clearable_sled(memory_space, clearable_sled_range);

	if (m_verbose)
		cout << "Updating sled: " << existing_sled
		     << " with new sled: " << new_sled << endl;

	/*
	 * Put the jump points into the new sled area.
	 */
	InsertJumpPoints68SledArea(new_sled);

	cout << "Clearable sled: " << clearable_sled << endl;
	clearable_sled.MergeSledJumpPoints(new_sled);
	cout << "(Merged) Clearable sled: " << clearable_sled << endl;
	/*
	 * Clear the chains in the new sled!
	 */
	Clear68SledArea(clearable_sled);

	/*
	 * Put in PUSHs in the new_sled.
	 */
	size_t i=0;
	RangeAddress_t addr=new_sled.SledRange().GetStart();
	for(;i<existing_sled.SledRange().GetStart()-new_sled.SledRange().GetStart();
	    i++)
	{
		if (m_verbose)
			cout << "Adding 68 at "
			     << std::hex << addr+i 
					 << " for sled at 0x"
					 << std::hex << addr << endl;
		
		/*
		 * Do not assert that we are writing into a free space.
		 * We may be writing over a PUSH that was there before!
		 */
		assert(memory_space.IsByteFree(addr+i) || memory_space[addr+i]==0x68);
		memory_space[addr+i] = 0x68;
		m_AddrInSled[addr+i] = true;
		memory_space.SplitFreeRange(addr+i);
	}

	existing_sled.MergeSled(new_sled);

	assert(existing_sled.Disambiguation());

	Instruction_t *sled_disambiguation = Emit68Sled(existing_sled);

	if (m_verbose)
		cout << "Generated sled_disambiguation (in Update68Sled()): " << std::hex << sled_disambiguation << endl;
	/*
	 * Update the disambiguation
	 *
	 * What we are doing is walking through the
	 * expanded or unexpanded pins and seeing 
	 * if they match the end instruction that
	 * we are doing now. We updated the end
	 * instruction in the MergeSled(). Why is it
	 * that this might fail?
	 * 
	 * TODO: This is really bad slow.
	 */
	
	Instruction_t *disambiguations[2] = {existing_sled.Disambiguation(),
	                                     new_sled.Disambiguation()};
	bool disambiguation_updated = false;	
	for (int disambiguation_iter = 0; 
	     disambiguation_iter<2;
			 disambiguation_iter++)
	{
		Instruction_t *disambiguation_to_update =
		               disambiguations[disambiguation_iter];
		/*
		 * The pin pointing to the disambiguation is only ever a 5 byte pin.
		 */
		for(
			std::map<UnresolvedPinned_t,RangeAddress_t>::iterator it=five_byte_pins.begin();
				it!=five_byte_pins.end() /*&& !sled_disambiguation*/;
				it++
		   )
		{
			RangeAddress_t addr=(*it).second;
			UnresolvedPinned_t up=(*it).first;

			cout << std::hex << up.GetInstruction() << " 5b vs " << disambiguation_to_update << endl;
			if (up.GetInstruction() == disambiguation_to_update)
			{
				five_byte_pins.erase(it);
				UnresolvedPinned_t cup(sled_disambiguation);
				cup.SetUpdatedAddress(up.GetUpdatedAddress());
				five_byte_pins[cup] = addr;

				disambiguation_updated = true;
				break;
			}
		}
	}
	assert(disambiguation_updated);

	existing_sled.Disambiguation(sled_disambiguation);
}

RangeAddress_t ZiprImpl_t::Do68Sled(RangeAddress_t addr)
{
	char jmp_rel32_bytes[]={(char)0xe9,(char)0,(char)0,(char)0,(char)0};
	const size_t nop_overhead=4;	// space for nops.
	const size_t jmp_overhead=sizeof(jmp_rel32_bytes);	// space for nops.
	const size_t sled_overhead = nop_overhead + jmp_overhead;
	const int sled_size=Calc68SledSize(addr, sled_overhead);
	Sled_t sled(memory_space, Range_t(addr,addr+sled_size), m_verbose);
	set<Sled_t>::iterator sled_it;

	if (m_verbose)
		cout << "Adding 68-sled at 0x" << std::hex << addr 
		     << " size="<< std::dec << sled_size << endl;
	
	for (sled_it = m_sleds.begin();
	     sled_it != m_sleds.end();
			 sled_it++)
	{
		Sled_t sled_i = *sled_it;
		if (sled_i.Overlaps(sled))
		{
			if (m_verbose)
				cout << "Found overlapping sled: " << sled_i << " and " << sled << endl;

			m_sleds.erase(sled_it);
			Update68Sled(sled, sled_i);
			m_sleds.insert(sled_i);
			/*
			 * Return the final address of the updated sled.
			 */
			return sled_i.SledRange().GetEnd();
		}
	}


	InsertJumpPoints68SledArea(sled);

	/*
	 * It's possible that the sled that we are going to put here
	 * is actually going to overwrite some pins and chain entries.
	 * So, we have to make sure to unreserve that space.
	 */
	Clear68SledArea(sled);

	/* Now, let's (speculatively) clear out the overhead space.
	 */
	if (m_verbose)
		cout << "Clearing overhead space at " << std::hex
		     << "(" << addr+sled_size << "."
		     << addr+sled_size+sled_overhead << ")." << endl;
	for (size_t i=0;i<sled_overhead;i++)
		if (!memory_space.IsByteFree(addr+sled_size+i))
			memory_space.MergeFreeRange(addr+sled_size+i);

	/*
	 * Put down the sled.
	 */
	for(size_t i=0;i<sled_size;i++)
	{
		if (m_verbose)
			cout << "Adding 68 at "
			     << std::hex << addr+i 
					 << " for sled at 0x"
					 << std::hex << addr << endl;
		assert(memory_space.IsByteFree(addr+i));
		memory_space[addr+i]=0x68;
		m_AddrInSled[addr+i] = true;
		memory_space.SplitFreeRange(addr+i);
	}
	/*
	 * Put down the NOPs
	 */
	for(size_t i=0;i<nop_overhead;i++)
	{
		if (m_verbose)
			cout << "Adding 90 at "
			     << std::hex << addr+sled_size+i 
					 << " for sled at 0x"
					 << std::hex << addr << endl;

		assert(memory_space.IsByteFree(addr+sled_size+i));
		memory_space[addr+sled_size+i] = 0x90;
		m_AddrInSled[addr+sled_size+i] = true;
		memory_space.SplitFreeRange(addr+sled_size+i);
	}

	/*
	 * That brings us to the part that actually, you know, does
	 * the jump to the proper target depending upon where we
	 * landed.
	 */
	Instruction_t* sled_disambiguation=Emit68Sled(sled);

	if (m_verbose)
		cout << "Generated sled_disambiguation (in Do68Sled()): " << std::hex << sled_disambiguation << endl;

	if (m_verbose)
		cout << "Pin for 68-sled  at 0x"
		     << std::hex << addr <<" is "
				 << std::hex << (addr+sled_size+nop_overhead) << endl;

	/*
	 * Reserve the bytes for the jump at the end of the sled that
	 * will take us to the (above) disambiguator.
	 */
	for(size_t i=0;i<jmp_overhead;i++)
	{
		assert(memory_space.IsByteFree(addr+sled_size+nop_overhead+i));
		memory_space[addr+sled_size+nop_overhead+i]=jmp_rel32_bytes[i];
		memory_space.SplitFreeRange(addr+sled_size+nop_overhead+i);
		//m_AddrInSled[addr+sled_size+nop_overhead+i] = true;
	}

	/*
	 * We know that the jmp we just put down is a two byte jump.
	 * We want it to point to the sled_disambiguation so we
	 * put a two byte pin down. This will affect the work of the
	 * loop above where we are putting down two byte pins and 
	 * attempting to expand them through chaining.
	 */
	UnresolvedPinned_t cup(sled_disambiguation);
	cup.SetUpdatedAddress(addr+sled_size+nop_overhead);
	five_byte_pins[cup] = addr+sled_size+nop_overhead;
	if (m_verbose)
		cout << "Put in a five byte jmp to the disambiguation " << std::hex << sled_disambiguation << " at " << std::hex << addr+sled_size+nop_overhead << endl;

	sled.Disambiguation(sled_disambiguation);

	if (m_verbose)
		cout << "Inserting sled: " << sled << endl;
	m_sleds.insert(sled);

	return addr+sled_size+nop_overhead+jmp_overhead;
}


void ZiprImpl_t::Clear68SledArea(Sled_t sled)
{
	for (std::set<RangeAddress_t>::iterator addr_iter = sled.JumpPointsBegin();
	     addr_iter != sled.JumpPointsEnd();
			 addr_iter++)
	{
		RangeAddress_t addr = *addr_iter;
		size_t clear_size = 0;
		if (m_verbose)
			cout << "Testing " << std::hex << addr << endl;
		if (!(sled.SledRange().GetStart() <= addr && addr <= sled.SledRange().GetEnd()))
		{	
			if (m_verbose)
				cout << std::hex << addr << " outside sled range." << endl;
			continue;
		}
		if (FindPatchTargetAtAddr(addr))
		{
			UnresolvedUnpinnedPatch_t uup = m_PatchAtAddrs.at(addr);
			clear_size = uup.second.GetSize();

			if (m_verbose)
				cout << "Need to clear a " << std::dec << clear_size
				     << " byte chain entry at " << std::hex << (addr) << endl;
		}
		else if (FindPinnedInsnAtAddr(addr))
		{
			std::map<RangeAddress_t, std::pair<libIRDB::Instruction_t*, size_t> >
			   ::iterator pinned_it = m_InsnSizeAtAddrs.find(addr);

			assert(pinned_it != m_InsnSizeAtAddrs.end());

			clear_size = pinned_it->second.second;

			if (m_verbose)
				cout << "Need to clear a " << std::dec << clear_size
				     << " byte pin at " << std::hex << (addr) << endl;
		}
		else
			assert(false);

		clear_size = std::min(sled.SledRange().GetEnd(), addr+clear_size) - addr;
		if (m_verbose)
			cout << "Need to clear " << std::dec << clear_size << " bytes." << endl;

		if (clear_size>0)
		{
			/*
			 * We do want to free this space, but only if it
			 * is already in use.
			 */
			if (!memory_space.IsByteFree(addr))
				memory_space.MergeFreeRange(Range_t(addr, addr+clear_size));
			assert(memory_space.IsByteFree(addr));
		}
	}
}

int ZiprImpl_t::Calc68SledSize(RangeAddress_t addr, size_t sled_overhead)
{
	int sled_size=0;
	while(true)
	{
		int i=0;
		for(i=0;i<sled_overhead;i++)
		{
			if (FindPinnedInsnAtAddr(addr+sled_size+i))
			{
				if (m_verbose)
					cout << "Sled free space broken up by pin at " 
					     << std::hex << (addr+sled_size+i) << endl;
				break;
			}
			else if (FindPatchTargetAtAddr(addr+sled_size+i))
			{
				if (m_verbose)
					cout << "Sled free space broken up by chain entry at " 
					     << std::hex << (addr+sled_size+i) << endl;
				break;
			}
			else
			{
				if (m_verbose)
					cout << "Sled free space at " << std::hex << (addr+sled_size+i) << endl;
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
		sled_size+=(i+1);
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


	/* first, for each pinned instruction, try to 
	 * put down a jump for the pinned instruction
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
			printf("Working two byte pinning decision at %p for: ", (void*)addr);
			printf("%s\n", upinsn->GetComment().c_str());
		}


		// if the byte at x+1 is free, we can try a 2-byte jump (which may end up being converted to a 5-byte jump later).
		if (FindPinnedInsnAtAddr(addr+1)==NULL)
		{
			/* so common it's not worth printing 
			if (m_verbose)
			{
				printf("Can fit two-byte pin (%p-%p).  fid=%d\n", 
					(void*)addr,
					(void*)(addr+sizeof(bytes)-1),
					upinsn->GetAddress()->GetFileID());
			}
			*/
		
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
			up.SetRange(Range_t(addr, addr+2));
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
			 * Insert a new UnresolvePinned_t that tells future
			 * loops that we are going to use an updated address
			 * to place this instruction.
			 * 
			 * This is a type of fiction because these won't really
			 * be pins in the strict sense. But, it's close enough.
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

				/*
				 * It's possible that this pin is going to be the last 
				 * one in the program. TODO: If this instruction abuts the 
				 * end of the program's address space then there 
				 * could be a problem. As of now, we assume that 
				 * this is not the case.
				 */
				if (it==unresolved_pinned_addrs.end())
					break;
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

		std::map<RangeAddress_t,UnresolvedUnpinnedPatch_t>::iterator patch_it;
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

		if (m_AddrInSled[addr])
		{
			/*
			 * There is no need to consider this pin at all! It was
			 * subsumed w/in a sled which has already handled it.
			 * Move along.
			 */
			if (m_verbose)
				cout << "Two byte pin at 0x" 
				     << std::hex << addr 
						 << " is w/in a sled ... skipping and erasing." << endl;
			two_byte_pins.erase(it++);
			continue;
		}

		char bytes[]={(char)0xe9,(char)0,(char)0,(char)0,(char)0}; // jmp rel8
		bool can_update=memory_space.AreBytesFree(addr+2,sizeof(bytes)-2);
		if (m_verbose && can_update)
			printf("Found %p can be updated to 5-byte jmp\n", (void*)addr);

		can_update &= !m_AddrInSled[up.GetRange().GetStart()];
		if (m_verbose && can_update && m_AddrInSled[up.GetRange().GetStart()])
			printf("%p was already fixed into a sled. Cannot update.\n", (void*)addr);
		if(can_update)
		{
			memory_space.PlopJump(addr);

			/*
			 * Unreserve those bytes that we reserved before!
			 */
			for (unsigned int j = up.GetRange().GetStart(); j<up.GetRange().GetEnd(); j++)
			{
				if (!m_AddrInSled[j])
					memory_space.MergeFreeRange(j);
			}
		
			/*
			 * We have a chain entry prereserved and we want to get
			 * rid of it now!
			 */
			if (m_verbose)
				cout << "Erasing chain entry at 0x" 
				     << std::hex << up.GetRange().GetStart() << endl;
			patch_it = m_PatchAtAddrs.find(up.GetRange().GetStart());
			assert(patch_it != m_PatchAtAddrs.end());
			m_PatchAtAddrs.erase(patch_it);

			up.SetRange(Range_t(0,0));
			five_byte_pins[up]=addr;
			m_InsnSizeAtAddrs[addr] = std::pair<Instruction_t*, size_t>(
			                          m_InsnSizeAtAddrs[addr].first, 5);
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

		/*
		 * This might have already been handled in a sled.
		 */
		if (m_AddrInSled[addr])
		{
			if (m_verbose)
				cout << "Skipping two byte pin at " 
				     << std::hex << addr << " because it is in a sled." << endl;
			/*
			 * If there are some reserved bytes for this then
			 * we want to unreserve it (but only if it is not
			 * in a sled itself since it would not be good to
			 * imply that space is now open).
			 */
			if (up.HasRange())
			{
				for (unsigned int j = up.GetRange().GetStart(); j<up.GetRange().GetEnd(); j++)
				{
					if (!m_AddrInSled[j])
						memory_space.MergeFreeRange(j);
				}
			}
			two_byte_pins.erase(it++);
			continue;
		}

		if (up.HasRange())
		{
			/*
			 * Always clear out the previously reserved space.
			 * Do this here because some/most of the algorithms
			 * that we use below assume that it is unreserved.
			 */
			for (unsigned int j = up.GetRange().GetStart(); j<up.GetRange().GetEnd(); j++)
			{
				if (!m_AddrInSled[j])
					memory_space.MergeFreeRange(j);
			}

			if (m_AddrInSled[up.GetRange().GetStart()])
			{
				if (m_verbose)
					printf("Using previously reserved spot of 2-byte->x-byte conversion "
					"(%p-%p)->(%p-%p) (orig: %p) because it was subsumed under sled\n", 
					(void*)addr,
					(void*)(addr+1),
					(void*)(up.GetRange().GetStart()),
					(void*)(up.GetRange().GetEnd()),
					(upinsn->GetIndirectBranchTargetAddress() != NULL) ?
					(void*)(uintptr_t)upinsn->GetIndirectBranchTargetAddress()->GetVirtualOffset() : 0x0);

				/*
				 * We simply patch the jump to this target and do not add
				 * it to any list for further processing. We are done.
				 */
				PatchJump(addr, up.GetRange().GetStart());
				two_byte_pins.erase(it++);
			}
			else if (up.GetRange().Is5ByteRange()) 
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
				new_up.SetRange(up.GetRange());

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
	DollopList_t::iterator it, it_end;
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
			should_end = start +
			             DetermineWorstCaseDollopEntrySize(entry_to_write, false);
			assert(end <= should_end);
			/*
			 * Build up a list of those dollop entries that we have
			 * just written that have a target. See comment above 
			 * ReplopDollopEntriesWithTargets() for the reason that
			 * we have to do this.
			 */
			if (entry_to_write->TargetDollop())
				m_des_to_replop.push_back(entry_to_write);
		}
	}
}

/*
 * We have to potentially replop dollop entries with targets
 * because:
 *
 * A plugin that writes dollop entries may put the instructions
 * NOT in the first position. This is particularly common in CFI:
 *
 * 0x...01: f4
 * 0x...02: INSN
 *
 * However, the writer cannot know every place where that happens
 * until after the entire WriteDollops() function has completed.
 * So, we go back and do another pass here once we know all those
 * actual instruction addresses (which are completely and fully
 * assigned during the call to _PlopDollopEntry.).
 */
void ZiprImpl_t::ReplopDollopEntriesWithTargets()
{
	for (DollopEntry_t *entry_to_write : m_des_to_replop)
	{
		Instruction_t *src_insn = NULL;
		RangeAddress_t src_insn_addr;

		src_insn = entry_to_write->Instruction();

		src_insn_addr = final_insn_locations[src_insn];
		_PlopDollopEntry(entry_to_write, src_insn_addr);
	}
}

void ZiprImpl_t::PlaceDollops()
{
	int count_pins=0;

//	set<pair<Dollop_t*,RangeAddress_t>, placement_queue_comp> placement_queue;
	//list<pair<Dollop_t*, RangeAddress_t>> placement_queue;
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

		placement_queue.insert(pair<Dollop_t*,RangeAddress_t>(target_dollop,patch.GetAddress()));
		if (m_verbose) {
			cout << "Original: " << std::hex << target_insn->
			                                    GetAddress()->
			                                    GetVirtualOffset() << " "
			     << "vs. Patch: " << std::hex << patch.GetAddress() << endl;
		}
		count_pins++;
	}

	cout<<"#ATTRIBUTE pins_detected="<<dec<<count_pins<<endl;
	cout<<"#ATTRIBUTE placement_queue_size="<<dec<<placement_queue.size()<<endl;

	while (!placement_queue.empty())
	{
		pair<Dollop_t*, RangeAddress_t> pq_entry(NULL,NULL);
		Dollop_t *to_place = NULL;
		RangeAddress_t from_address =0;
		size_t minimum_valid_req_size = 0;
		Range_t placement = {0,0};
		DLFunctionHandle_t placer = NULL;
		bool placed = false;
		list<DollopEntry_t*>::const_iterator dit, dit_end;
		RangeAddress_t cur_addr = 0 ;
		bool has_fallthrough = false;
		Dollop_t *fallthrough = NULL;
		bool continue_placing = false;
		bool am_coalescing = false;
		bool allowed_coalescing = true;
		bool initial_placement_abuts_pin = false;
		bool initial_placement_abuts_fallthrough = false;
		bool fits_entirely = false;
		RangeAddress_t fallthrough_dollop_place = 0;
		bool fallthrough_has_preplacement = false;

		//pq_entry = placement_queue.front();
		pq_entry = *(placement_queue.begin());
		//placement_queue.pop_front();
		placement_queue.erase(placement_queue.begin());

		to_place = pq_entry.first;
		from_address = pq_entry.second;

		if (m_verbose)
			cout << "Original starting address: " << std::hex
			     << to_place->front()->Instruction()->GetAddress()->GetVirtualOffset()
					 << endl;


		if (to_place->IsPlaced())
			continue;

		to_place->ReCalculateWorstCaseSize();

		minimum_valid_req_size = std::min(
			DetermineWorstCaseDollopEntrySize(to_place->front(), true),
			Utils::DetermineWorstCaseDollopSizeInclFallthrough(to_place));
		/*
		 * Ask the plugin manager if there are any plugins
		 * that want to tell us where to place this dollop.
		 */
		if (plugman.DoesPluginAddress(to_place,
		                              from_address,
		                              placement,
		                              allowed_coalescing,
		                              placer))
		{
			placed = true;

			if (m_verbose)
				cout << placer->ToString() << " placed this dollop between " 
				     << std::hex << placement.GetStart() << " and " 
						 << std::hex << placement.GetEnd()
						 << endl;

			/*
			 * Check if the size that we got back is enough to hold
			 * at least a little bit of what we wanted. 
			 *
			 * (1) We want to make sure that there is enough room for at least
			 * the first instruction of the dollop and space for a jump
			 * to the remainder of the dollop. 
			 * 
			 * (2) However, it's possible that the entirety of this dollop, plus
			 * any fallthroughs are going to fit. So, we need to check that 
			 * possibility too. 
			 *
			 * (3) Then there's the possibility that the dollop *has* a fallthrough
			 * but that the fallthrough is actually pinned and 
			 * that pin is abutting the end of the dollop in which 
			 * case we elide (I hate that term) the fallthrough jump.
			 *
			 * (4) Then there's the possibility that the dollop has a 
			 * fallthrough but that the fallthrough is actually abutting
			 * the beginning of it's fallthrough dollop in which case we elide
			 * (still hate that term) the fallthrough jump. Very similar
			 * to case (3).
			 *
			 * TODO: Consider that allowed_coalescing may invalidate the
			 * possibility of the validity of the placement in (2).
			 */
			initial_placement_abuts_pin = to_place->FallthroughDollop() && 
			                              to_place->FallthroughDollop()->
			                              front()->
			                              Instruction()->
			                              GetIndirectBranchTargetAddress() && 
			                              to_place->FallthroughDollop()->
			                                        front()->
			                                        Instruction()->
			                                        GetIndirectBranchTargetAddress()->
			                                        GetVirtualOffset() == 
			                         (placement.GetStart() + to_place->GetSize() - 5);
			/*
			 * If this dollop has a fallthrough, find out where that 
			 * fallthrough is (or is going to be) placed. That way
			 * we can determine if the current dollop is (or is going to be)
			 * adjacent to the place of the fallthrough. That means
			 * that we can keep from placing a jump to the dollo
			 * and instead just fallthrough.
			 */
			if (to_place->FallthroughDollop()) 
			{
				/*
				 * Find out where the fallthrough dollop is placed.
				 */
				if (to_place->FallthroughDollop()->IsPlaced())
				{
					fallthrough_dollop_place = to_place->FallthroughDollop()->Place();
					fallthrough_has_preplacement = true;
				}
				/*
				 * Find out where the fallthrough dollop is
				 * going to be placed. We only have to ask 
				 * plugins about this since we know that zipr-proper
				 * does not preallocate placements like plugins
				 * are known to do.
				 */
				else
				{
					Range_t fallthrough_placement;
					bool fallthrough_allowed_coalescing = false;
					DLFunctionHandle_t fallthrough_placer = NULL;
					/*
					 * Prospectively get the place for this dollop. That way 
					 * we can determine whether or not we need to use a fallthrough!
					 */
					if (plugman.DoesPluginAddress(to_place->FallthroughDollop(),
		                                    from_address,
		                                    fallthrough_placement,
		                                    fallthrough_allowed_coalescing,
		                                    fallthrough_placer))
					{
						fallthrough_dollop_place = fallthrough_placement.GetStart();
						fallthrough_has_preplacement = true;
					}
				}
			}
			initial_placement_abuts_fallthrough = to_place->FallthroughDollop() &&
			                                      fallthrough_has_preplacement &&
																						fallthrough_dollop_place == 
			                         (placement.GetStart() + to_place->GetSize() - 5);


			fits_entirely = (to_place->GetSize() <= 
			                (placement.GetEnd()-placement.GetStart()));

			if (m_verbose)
			{
				cout << "initial_placement_abuts_pin        : "
				     <<initial_placement_abuts_pin << endl
				     << "initial_placement_abuts_fallthrough: " 
				     << initial_placement_abuts_fallthrough << endl
				     << "fits_entirely                      : " 
				     << fits_entirely << endl;
			}

			if (((placement.GetEnd()-placement.GetStart()) < minimum_valid_req_size)&&
			    !(initial_placement_abuts_pin || 
					  initial_placement_abuts_fallthrough ||
						fits_entirely)
			   )
			{
				if (m_verbose)
					cout << "Bad GetNearbyFreeRange() result." << endl;
				placed = false;
			}
		}

		if (!placed) {
			cout << "Using default place locator." << endl;
			/*
			 * TODO: Re-enable this ONCE we figure out why the dollop
			 * sizes are not being recalculated correctly.
			 */
			//placement = memory_space.GetFreeRange(to_place->GetSize());
			placement = memory_space.GetFreeRange(minimum_valid_req_size);

			/*
			 * Reset allowed_coalescing because DoesPluginAddress
			 * may have reset it and we may have rejected the results
			 * of that addressing.
			 */
			allowed_coalescing = true;
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

		if (to_place->front()->Instruction()->GetIndirectBranchTargetAddress() &&
		    cur_addr == to_place->
				            front()->
										Instruction()->
										GetIndirectBranchTargetAddress()->
										GetVirtualOffset()
		   )
		{
			unsigned int space_to_clear = Utils::SHORT_PIN_SIZE;
			/*
			 * We have placed this dollop at the location where
			 * its first instruction was pinned in memory.
			 */
			if (m_verbose)
				cout << "Placed atop its own pin!" << endl;

			if (memory_space[cur_addr] == (char)0xe9)
				space_to_clear = Utils::LONG_PIN_SIZE;

			for (unsigned int j = cur_addr; j<(cur_addr+space_to_clear); j++)
			{
				memory_space.MergeFreeRange(j);
			}

			/*
			 * Remove the replaced pin from the patch list.
			 */
			UnresolvedUnpinned_t uu(to_place->front()->Instruction());
			Patch_t emptypatch(cur_addr, UncondJump_rel32);

			auto found_patch = patch_list.find(uu);
			assert(found_patch != patch_list.end());
			patch_list.erase(found_patch);
		}
		/*
		 * Handle the case where the placer put us atop the fallthrough
		 * link from it's FallbackDollop()
		 */
		else if (to_place->FallbackDollop() &&
		    to_place->FallbackDollop()->IsPlaced() &&
				(to_place->FallbackDollop()->Place() +
				 to_place->FallbackDollop()->GetSize() - 5) ==
		    placement.GetStart())
		{
			/*
			 * We have placed this dollop at the location where
			 * the fallthrough jump to this dollop was placed.
			 */
			if (m_verbose)
				cout << "Placed atop its own fallthrough!" << endl;

			/*
			 * Note: We do NOT have to clear any pre-reserved
			 * memory here now that we have pre-checks on
			 * whether the dollop is placed. Because of that
			 * precheck, this range will never be unnecessarily 
			 * reserved for a jump.
			 */
		}

		assert(to_place->GetSize() != 0);

		do {
			bool all_fallthroughs_fit = false;
			size_t wcds = 0;

			if (am_coalescing)
			{
				/*
				 * Only reset this if we are on a 
				 * second, third, fourth ... go-round.
				 */
				fits_entirely = false;
			}
			/*
			 * TODO: From here, we want to place the dollop
			 * that we just got a placement for, and subsequently
			 * place any dollops that are fallthroughs!
			 */

			/*
			 * Assume that we will stop placing after this dollop.
			 */
			continue_placing = false;
		
			to_place->ReCalculateWorstCaseSize();

			/*
			 * Calculate before we place this dollop.
			 */
			wcds = Utils::DetermineWorstCaseDollopSizeInclFallthrough(to_place);

			to_place->Place(cur_addr);

			cout << "to_place->GetSize(): " << to_place->GetSize() << endl;

			fits_entirely = (to_place->GetSize() <= (placement.GetEnd()-cur_addr));
			all_fallthroughs_fit = (wcds <= (placement.GetEnd()-cur_addr));

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
				 * 3. This dollop has no fallthrough and fits entirely
				 *    within the space allotted.
				 *    Call this the fits_entirely case.
				 * 4. The dollop and all of its fallthroughs will fit
				 *    "[A]ll of its fallthoughs will fit" encompasses
				 *    the possibility that one of those is already 
				 *    placed -- we use the trampoline size at that point.
				 *    See DetermineWorstCaseDollopSizeInclFallthrough().
				 *    Call this the all_fallthroughs_fit case.
				 * 5. NOT (All fallthroughs fit is the only way that we are
				 *    allowed to proceed placing this dollop but we are not 
				 *    allowed to coalesce and we are out of space for the 
				 *    jump to the fallthrough.)
				 *    Call this the disallowed_override case
				 * 6. There is enough room for this instruction AND it is
				 *    the last entry of this dollop AND the dollop has a
				 *    fallthrough AND that fallthrough is to a pin that
				 *    immediately follows this instruction in memory.
				 *    Call this initial_placement_abuts (calculated above).
				 */
				bool de_and_fallthrough_fit = false;
				bool last_de_fits = false;
				bool disallowed_override = true;
				de_and_fallthrough_fit = (placement.GetEnd()>= /* fits */
				     (cur_addr+DetermineWorstCaseDollopEntrySize(dollop_entry, true))
				                         );
				last_de_fits = (std::next(dit,1)==dit_end) /* last */ &&
				               (placement.GetEnd()>=(cur_addr+ /* fits */
							DetermineWorstCaseDollopEntrySize(dollop_entry,
							                            to_place->FallthroughDollop()!=NULL))
							                            /* with or without fallthrough */
							         );
				disallowed_override = !allowed_coalescing && 
				                      !(de_and_fallthrough_fit ||
															  fits_entirely ||
															  last_de_fits ||
															  initial_placement_abuts_pin ||
															  initial_placement_abuts_fallthrough
															 ) && 
															 all_fallthroughs_fit && 
				                       ((placement.GetEnd() - 
				                        (cur_addr + 
				                         DetermineWorstCaseDollopEntrySize(
				                           dollop_entry,
				                           false)
				                         ) < Utils::TRAMPOLINE_SIZE
				                        ));

				if (m_verbose)
				{
#if 0
					cout << "de_and_fallthrough_fit             : " 
					     << de_and_fallthrough_fit << endl
					     << "last_de_fits                       : " 
							 << last_de_fits << endl
					     << "fits_entirely                      : " 
							 << fits_entirely << endl
					     << "all_fallthroughs_fit               : " 
							 << all_fallthroughs_fit << endl
					     << "initial_placement_abuts_pin        : " 
							 << initial_placement_abuts_pin << endl
					     << "initial_placement_abuts_fallthrough: " 
							 << initial_placement_abuts_fallthrough << endl
					     << "initial_placement_abuts_pin        : " 
							 << initial_placement_abuts_pin << endl
					     << "disallowed_override                : " 
							 << disallowed_override << endl;
#else
					struct custom_bool : numpunct<char>
					{
						protected:
						string do_truename() const override { return "t" ; }
						string do_falsename() const override { return "f" ; }
					};
					static struct custom_bool *cb=new custom_bool;

					// set cout to print t/f
					cout.imbue( { cout.getloc(), cb } );


					cout << "Placement stats: " 
					     << de_and_fallthrough_fit               << ", "
							 << last_de_fits                         << ", "
							 << fits_entirely                        << ", "
							 << all_fallthroughs_fit                 << ", "
							 << initial_placement_abuts_pin          << ", "
							 << initial_placement_abuts_fallthrough  << ", "
							 << initial_placement_abuts_pin          << ", "
							 << disallowed_override                  << noboolalpha << endl;

#endif
				}

				if ((de_and_fallthrough_fit ||
				    last_de_fits ||
						fits_entirely ||
						initial_placement_abuts_fallthrough ||
						initial_placement_abuts_pin ||
						all_fallthroughs_fit) && !disallowed_override)
				{
#if 1
					if (m_verbose) {
						DISASM d;
						dollop_entry->Instruction()->Disassemble(d);
						cout << std::hex << dollop_entry->Instruction()->GetBaseID() 
						     << ":" << d.CompleteInstr << endl;
					}
#endif
					dollop_entry->Place(cur_addr);
					cur_addr+=DetermineWorstCaseDollopEntrySize(dollop_entry,
					                                      false);
					if (dollop_entry->TargetDollop())
					{
						if (m_verbose)
							cout << "Adding " << std::hex << dollop_entry->TargetDollop()
							     << " to placement queue." << endl;
						placement_queue.insert(pair<Dollop_t*, RangeAddress_t>(
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
				if (am_coalescing)
					m_stats->truncated_dollops_during_coalesce++;

				if (m_verbose)
					cout << "Split a " 
					     << ((am_coalescing) ? "coalesced " : " ")
							 << "dollop because it didn't fit. Fallthrough to "
					     << std::hex << split_dollop << "." << endl;
			}
			/*
			 * (from above) ... we do not want to "jump" to the
			 * fallthrough if we can simply place it following
			 * this one!
			 */

			if ((fallthrough = to_place->FallthroughDollop()) != NULL &&
			    !to_place->WasCoalesced())
			{
				size_t fallthroughs_wcds, fallthrough_wcis, remaining_size;

				/*
				 * We do not care about the fallthrough dollop if its 
				 * first instruction is pinned AND the last entry of this
				 * dollop abuts that pin.
				 */
				if ((fallthrough->
				     front()->
				     Instruction()->
				     GetIndirectBranchTargetAddress()
				    ) &&
				    (cur_addr == fallthrough->
				                 front()->
				                 Instruction()->
				                 GetIndirectBranchTargetAddress()->
				                 GetVirtualOffset()
				    )
				   )
				{
					if (m_verbose)
						cout << "Dollop had a fallthrough dollop and "
						     << "was placed abutting the fallthrough " 
						     << "dollop's pinned first instruction. "
						     << endl;
					/*
					 * Because the fallthrough dollop is pinned, we
					 * know that it is already in the placement q. That's
					 * the reason that we do not have to add it here. See
					 * below for a contrast.
					 */
					m_stats->total_did_not_coalesce++;
					break;
				}

				/*
				 * If the fallthrough is placed and it is immediately after
				 * this instruction, then we don't want to write anything else!
				 *
				 * TODO: This calculation is only valid if we are NOT coalescing.
				 * We need to change this condition or reset some of the variables
				 * so that we do not rely on !am_coalescing as a condition.
				 * Actually, we should make it work correctly -- ie, make sure that
				 * even if we do coaelesce something its fallthrough could
				 * be preplaced ...
				 */
				if (!am_coalescing &&
				    to_place->FallthroughDollop() &&
				    fallthrough_has_preplacement &&
						fallthrough_dollop_place == cur_addr)
				{
					if (m_verbose)
						cout << "Dollop had a fallthrough dollop and "
						     << "was placed abutting the fallthrough "
						     << "dollop's first instruction. "
						     << endl;
					/*
					 * We are not coalescing, but we want to make sure that
					 * the fallthrough does get placed if zipr hasn't already
					 * done so. See above for a contrast.
					 */
					if (!to_place->FallthroughDollop()->IsPlaced())
					{
						placement_queue.insert(pair<Dollop_t*, RangeAddress_t>(
								to_place->FallthroughDollop(),
								cur_addr));
					}
					m_stats->total_did_not_coalesce++;
					break;
				}
					
				/*
				 * We could fit the entirety of the dollop (and
				 * fallthroughs) ...
				 */
				fallthroughs_wcds = 
					Utils::DetermineWorstCaseDollopSizeInclFallthrough(fallthrough);
				/*
				 * ... or maybe we just want to start the next dollop.
				 */
				fallthrough_wcis=DetermineWorstCaseDollopEntrySize(fallthrough->
				                                                   front(),
																													 true);
				remaining_size = placement.GetEnd() - cur_addr;

				/*
				 * We compare remaining_size to min(fallthroughs_wdcs,
				 * fallthrough_wcis) since the entirety of the dollop
				 * and its fallthroughs could (its unlikely) be 
				 * smaller than the first instruction fallthrough 
				 * in the fallthrough dollop and the trampoline size.
				 */
				if (m_verbose)
					cout << "Determining whether to coalesce: "
					     << "Remaining: " << std::dec << remaining_size
							 << " vs Needed: " << std::dec 
							 << std::min(fallthrough_wcis,fallthroughs_wcds) << endl;
				if (remaining_size < std::min(fallthrough_wcis,fallthroughs_wcds) ||
				    fallthrough->IsPlaced() ||
						!allowed_coalescing)
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
					cur_addr+=DetermineWorstCaseDollopEntrySize(patch_de, false);

					to_place->push_back(patch_de);
					to_place->FallthroughPatched(true);

					if (m_verbose)
						cout << "Not coalescing"
						     << string((fallthrough->IsPlaced()) ?
								    " because fallthrough is placed" : "")
						     << string((!allowed_coalescing) ?
								    " because I am not allowed" : "")
						     << "; Added jump (at " << std::hex << patch_de->Place() << ") "
						     << "to fallthrough dollop (" << std::hex 
						     << fallthrough << ")." << endl;

					placement_queue.insert(pair<Dollop_t*, RangeAddress_t>(
							fallthrough,
							cur_addr));
					/*
					 * Since we inserted a new instruction, we should
					 * check to see whether a plugin wants to plop it.
					 */
					AskPluginsAboutPlopping(patch_de->Instruction());

					m_stats->total_did_not_coalesce++;

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
					to_place->WasCoalesced(true);
					/*
					 * Fallthrough is not placed and there is enough room to
					 * put (at least some of) it right below the previous one.
					 */
					to_place = fallthrough;
					continue_placing = true;
					m_stats->total_did_coalesce++;
					am_coalescing = true;
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

void ZiprImpl_t::RecalculateDollopSizes()
{
	auto dollop_it = m_dollop_mgr.dollops_begin();
	auto dollop_it_end = m_dollop_mgr.dollops_end();
	for ( ;
	     dollop_it != dollop_it_end;
			 dollop_it++)
	{
		(*dollop_it)->ReCalculateWorstCaseSize();
	}
}

void ZiprImpl_t::CreateDollops()
{
	multimap<const UnresolvedUnpinned_t,Patch_t>::const_iterator pin_it,
	                                                             pin_it_end;
	if (m_verbose)
		cout<< "Attempting to create "
		    << patch_list.size()
				<< " dollops for the pins."
				<< endl;

	for (pin_it = patch_list.begin(), pin_it_end = patch_list.end();
	     pin_it != pin_it_end;
	     pin_it++)
	{
		UnresolvedUnpinned_t uu = (*pin_it).first;
		m_dollop_mgr.AddNewDollops(uu.GetInstruction());
	}
	if (m_verbose)
		cout << "Done creating dollops for the pins! Updating all Targets" << endl;

	m_dollop_mgr.UpdateAllTargets();

	if (m_verbose)
		cout << "Created " <<std::dec << m_dollop_mgr.Size() 
		     << " total dollops." << endl;
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

		if (m_AddrInSled[addr])
		{
			if (m_verbose)
				cout << "Skipping five byte pin at " 
				     << std::hex << addr << " because it is in a sled." << endl;
			it++;
			continue;
		}

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
			break;
		}
		default:
		{
			assert(false);
		}
	}
}

size_t ZiprImpl_t::DetermineWorstCaseDollopEntrySize(DollopEntry_t *entry, bool account_for_fallthrough)
{
	std::map<Instruction_t*,unique_ptr<list<DLFunctionHandle_t>>>::const_iterator plop_it;
	size_t opening_size = 0, closing_size = 0;
	size_t wcis = DetermineWorstCaseInsnSize(entry->Instruction(), account_for_fallthrough);

	plop_it = plopping_plugins.find(entry->Instruction());
	if (plop_it != plopping_plugins.end())
	{
		for (auto handle : *(plop_it->second))
		{
			ZiprPluginInterface_t *zpi = dynamic_cast<ZiprPluginInterface_t*>(handle);
			opening_size += zpi->DollopEntryOpeningSize(entry);
			closing_size += zpi->DollopEntryClosingSize(entry);
		}
	}

	if (m_verbose)
	{
#if 0
		cout << "Adding opening size of " << opening_size << "." << endl;
		cout << "Adding closing size of " << closing_size << "." << endl;
		cout << "WCDES of " << std::hex << entry << ":" 
		     << std::dec << wcis+opening_size+closing_size << endl;
#else
		// if we need these, please add additional levels of verbose logging!
		// this log line accounts for 80% of a 744mb log file.
		//cout << "Open/close/wcdes for "<<hex<<entry<<": " << dec << opening_size 
		//     <<"/" << closing_size << "/" << wcis+opening_size+closing_size << endl;
#endif
	}

	return wcis+opening_size+closing_size;
}

size_t ZiprImpl_t::DetermineWorstCaseInsnSize(Instruction_t* insn, bool account_for_fallthrough)
{
	std::map<Instruction_t*,unique_ptr<list<DLFunctionHandle_t>>>::const_iterator plop_it;
	size_t worst_case_size = 0;
	size_t default_worst_case_size = 0;

	default_worst_case_size = Utils::DetermineWorstCaseInsnSize(insn, account_for_fallthrough);

	plop_it = plopping_plugins.find(insn);
	if (plop_it != plopping_plugins.end())
	{
		for (auto handle : *(plop_it->second))
		{
			ZiprPluginInterface_t *zpi = dynamic_cast<ZiprPluginInterface_t*>(handle);
			worst_case_size =std::max(zpi->WorstCaseInsnSize(insn,
			                                                 account_for_fallthrough),
			                          worst_case_size);
		}
	}
	else
	{
		worst_case_size = default_worst_case_size;
	}

	if (worst_case_size == 0)
	{
		if (m_verbose)
			cout << "Asked plugins about WCIS, but none responded." << endl;
		worst_case_size = default_worst_case_size;
	}

#if 0
// if you need this log, please implement a more-verbose logging option
// this log line accounts for 80% of a 744mb log file.
	if (m_verbose)
		cout << "Worst case size" 
		     << ((account_for_fallthrough) ? " (including jump)" : "")
		     << ": " << worst_case_size << endl;
#endif

	return worst_case_size;
}

bool ZiprImpl_t::AskPluginsAboutPlopping(Instruction_t *insn)
{
	/*
	 * Plopping plugins should hold a set.
	 */
	unique_ptr<list<DLFunctionHandle_t>> found_plopping_plugins = 
	unique_ptr<list<DLFunctionHandle_t>>(new std::list<DLFunctionHandle_t>());

	if (plugman.DoPluginsPlop(insn, *found_plopping_plugins))
	{
		if (m_verbose)
			for (auto pp : *found_plopping_plugins)
			{
				ZiprPluginInterface_t *zipr_plopping_plugin =
					dynamic_cast<ZiprPluginInterface_t*>(pp);
				cout << zipr_plopping_plugin->ToString()
				     << " will plop "<<dec<<insn->GetBaseID() << ":"
				     << insn->getDisassembly() << endl;
			}

		plopping_plugins[insn] = std::move(found_plopping_plugins);
		return true;
	}
	return false;
}

void ZiprImpl_t::AskPluginsAboutPlopping()
{

	for(set<Instruction_t*>::const_iterator it=m_firp->GetInstructions().begin();
	    it!=m_firp->GetInstructions().end();
	    ++it)
	{
		Instruction_t* insn=*it;
		assert(insn);
		AskPluginsAboutPlopping(insn);
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
		DLFunctionHandle_t patcher = NULL;

		target_dollop_entry = target_dollop->front();
		assert(target_dollop_entry != NULL);

		target_dollop_entry_instruction  = target_dollop_entry->Instruction();
		assert(target_dollop_entry_instruction != NULL &&
		       target_dollop_entry_instruction == uu.GetInstruction());

		target_dollop_entry_instruction->Disassemble(d);

		patch_addr = p.GetAddress();
		target_addr = target_dollop_entry->Place();

		if (final_insn_locations.end() != final_insn_locations.find(target_dollop_entry->Instruction()))
			target_addr = final_insn_locations[target_dollop_entry->Instruction()];

		if (plugman.DoesPluginRetargetPin(patch_addr, target_dollop, target_addr, patcher))
		{
			if (m_verbose)
			{
				cout << "Patching retargeted pin at " << std::hex<<patch_addr << " to "
				     << patcher->ToString() << "-assigned address: "
						 << std::hex << target_addr << endl;
			}
		}
		else
		{
			/*
			 * Even though DoesPluginRetargetPin() returned something other than
			 * Must, it could have still changed target_address. So, we have to
			 * reset it here, just in case.
			 */
			target_addr = target_dollop_entry->Place();

			if (final_insn_locations.end() != final_insn_locations.find(target_dollop_entry->Instruction()))
				target_addr = final_insn_locations[target_dollop_entry->Instruction()];

			if (m_verbose)
				cout << "Patching pin at " << std::hex << patch_addr << " to "
				     << std::hex << target_addr << ": " << d.CompleteInstr << endl;
			assert(target_dollop_entry_instruction != NULL &&
			       target_dollop_entry_instruction == uu.GetInstruction());
		}

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

RangeAddress_t ZiprImpl_t::_PlopDollopEntry(DollopEntry_t *entry, RangeAddress_t override_address)
{
	Instruction_t *insn = entry->Instruction();
	size_t insn_wcis = DetermineWorstCaseInsnSize(insn, false);
	RangeAddress_t updated_addr = 0;
	RangeAddress_t placed_address = entry->Place();
	RangeAddress_t target_address = 0;
	bool placed_insn = false;

	if (entry->TargetDollop() && entry->TargetDollop()->front())
	{
		auto target_address_iter = final_insn_locations.find(entry->
		                                                     TargetDollop()->
		                                                     front()->
		                                                     Instruction());
		if (target_address_iter != final_insn_locations.end())
		{
			target_address = target_address_iter->second;
			if (m_verbose)
				cout << "Found an updated target address location: "
				     << std::hex << target_address << endl;
		}
	}	

	map<Instruction_t*,unique_ptr<std::list<DLFunctionHandle_t>>>::const_iterator plop_it;

	if (override_address != 0)
		placed_address = override_address;
	
	plop_it = plopping_plugins.find(insn);
	if (plop_it != plopping_plugins.end())
	{
		for (auto pp : *(plop_it->second))
		{
			bool pp_placed_insn = false;
			DLFunctionHandle_t handle = pp;
			ZiprPluginInterface_t *zpi = dynamic_cast<ZiprPluginInterface_t*>(handle);
			updated_addr = std::max(zpi->PlopDollopEntry(entry,
			                                             placed_address,
			                                             target_address,
																									 insn_wcis,
																									 pp_placed_insn),
			                        updated_addr);
			if (m_verbose)
				cout << zpi->ToString() << " placed entry " 
				     << std::hex << entry 
						 << " at address: " << std::hex << placed_address 
						 << " " << (pp_placed_insn ? "and placed" : "but did not place")
						 << " the instruction."
						 << endl;
		
			placed_insn |= pp_placed_insn;
		}
	}

	/*
	 * If no plugin actually placed the instruction,
	 * then we are going to do it ourselves.
	 */
	if (!placed_insn)
	{
		updated_addr = PlopDollopEntry(entry, placed_address, target_address);
	}

	final_insn_locations[insn] = placed_address;
	return updated_addr;
}

#define IS_RELATIVE(A) \
((A.ArgType & MEMORY_TYPE) && (A.ArgType & RELATIVE_))

RangeAddress_t ZiprImpl_t::PlopDollopEntry(
	DollopEntry_t *entry,
	RangeAddress_t override_place,
	RangeAddress_t override_target)
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
		RangeAddress_t target_address = 0;
		Instruction_t *target_insn = entry->TargetDollop()->front()->Instruction();

		if (override_target != 0)
		{	
			if (final_insn_locations.end() != final_insn_locations.find(target_insn))
				target_address = final_insn_locations[target_insn];
		}
		else
		{
			if (m_verbose)
				cout << "Plopping with overriden target: " 
				     << std::hex << override_target << endl;
			target_address = override_target;
		}

		if (m_verbose)
			cout << "Plopping at " << std::hex << addr
			     << " with target " << std::hex << ((target_address != 0) ? target_address : entry->TargetDollop()->Place())
					 << endl;
		ret=PlopDollopEntryWithTarget(entry, addr, target_address);
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
	RangeAddress_t override_place,
	RangeAddress_t override_target)
{
	Instruction_t *insn = entry->Instruction();
	RangeAddress_t target_addr, addr, ret;

	assert(entry->TargetDollop());

	addr = entry->Place();
	target_addr = entry->TargetDollop()->Place();
	ret = addr;

	if (override_place != 0)
		addr = ret = override_place;

	if (override_target != 0)
		target_addr = override_target;

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

			/*
			 * This means that we have a fallthrough for this dollop entry
			 * that is actually the fallthrough of the dollop! Wowser.
			 * TODO: Before doing this, check to make sure that _entry_ 
			 * is the last of the dollop. 
			 */
			if (!fallthrough_de)
				fallthrough_de = entry->MemberOfDollop()->FallthroughDollop()->front();

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

	if (override_place != 0)
		at = originalAt = override_place;

	// emit call <callback>
	{
	char bytes[]={(char)0xe8,(char)0,(char)0,(char)0,(char)0}; // call rel32
	memory_space.PlopBytes(at, bytes, sizeof(bytes));
	unpatched_callbacks.insert(pair<DollopEntry_t*,RangeAddress_t>(entry,at));
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


DataScoop_t* ZiprImpl_t::FindScoop(const RangeAddress_t &addr)
{
	for(
		DataScoopSet_t::iterator it=m_firp->GetDataScoops().begin(); 
		it!=m_firp->GetDataScoops().end();
		++it
	   )
	{
		DataScoop_t* scoop=*it;
		if(scoop->GetStart()->GetVirtualOffset() <= addr &&
			addr < scoop->GetEnd()->GetVirtualOffset() )
		{
			return scoop;
		}
	}
	return NULL;
}

void ZiprImpl_t::WriteScoop(section* sec, FILE* fexe)
{
	// skip any nobits/tls sections.  
	if ( (sec->get_flags() & SHF_TLS) == SHF_TLS && sec->get_type()  ==  SHT_NOBITS )
		return;

	RangeAddress_t start=sec->get_address();
	RangeAddress_t end=sec->get_size()+start;
	for(RangeAddress_t i=start;i<end;i++)
	{
		DataScoop_t* scoop=FindScoop(i);
		if(!scoop)
			continue;

		const string &the_contents=scoop->GetContents();
		char  b=the_contents[i-scoop->GetStart()->GetVirtualOffset()];

		if( sec->get_type()  ==  SHT_NOBITS )
		{
			assert(b==0);	// cannot write non-zero's to NOBITS sections.

			// and we can't do the write, because the sec. isn't in the binary.
			continue;
		}

		int file_off=sec->get_offset()+i-start;
		fseek(fexe, file_off, SEEK_SET);
		fwrite(&b,1,1,fexe);
		if(i-start<200)// keep verbose output short enough.
		{
			if (m_verbose)
				printf("Writing scoop byte %#2x at %p, fileoffset=%x\n",
					((unsigned)b)&0xff, (void*)i, file_off);
		}

	}
}

static bool InScoop(virtual_offset_t addr, DataScoop_t* scoop)
{
	if(scoop->GetStart()->GetVirtualOffset()==0)
		return false;

	// check before
	if(addr < scoop->GetStart()->GetVirtualOffset())
		return false;
	if(scoop->GetEnd()->GetVirtualOffset()<addr)
		return false;
	return true;

}

void ZiprImpl_t::FillSection(section* sec, FILE* fexe)
{
	RangeAddress_t start=sec->get_address();
	RangeAddress_t end=sec->get_size()+start;
	DataScoop_t* scoop=NULL;

	if (m_verbose)
		printf("Dumping addrs %p-%p\n", (void*)start, (void*)end);
	for(RangeAddress_t i=start;i<end;i++)
	{
		if(scoop==NULL || InScoop(i,scoop))
		{
			scoop=FindScoop(i);
		}

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

#ifdef support_stratafier_mode
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
			WriteScoop(sec, fexe);
		else
			FillSection(sec, fexe);
        }
	fclose(fexe);

	string tmpname=name+string(".to_insert");
	printf("Opening %s\n", tmpname.c_str());
	FILE* to_insert=fopen(tmpname.c_str(),"w");

	if(!to_insert)
		perror( "void ZiprImpl_t::OutputBinaryFile(const string &name)");

#endif // support_stratafier_mode

// for elfwriter
	// find end of textra space.
	RangeSet_t::iterator it=memory_space.FindFreeRange((RangeAddress_t) -1);
	assert(memory_space.IsValidRange(it));
	RangeAddress_t end_of_new_space=it->GetStart();

	if(start_of_new_space != end_of_new_space )
	{

		// first byte of this range is the last used byte.
		AddressID_t *textra_start=new AddressID_t();
		textra_start->SetVirtualOffset(start_of_new_space);
		m_firp->GetAddresses().insert(textra_start);
		AddressID_t *textra_end=new AddressID_t();
		textra_end->SetVirtualOffset(end_of_new_space);
		m_firp->GetAddresses().insert(textra_end);
		string textra_contents;
		textra_contents.resize(end_of_new_space-start_of_new_space);
		DataScoop_t* textra_scoop=new DataScoop_t(BaseObj_t::NOT_IN_DATABASE, ".textra", textra_start, textra_end, NULL, 5, false, textra_contents);
		m_firp->GetDataScoops().insert(textra_scoop);

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
			textra_scoop->GetContents()[i-start_of_new_space]=b;
	#ifdef support_stratafier_mode
			fwrite(&b,1,1,to_insert);
	#endif
		}

	}
#ifdef support_stratafier_mode
	fclose(to_insert);

	callback_file_name = AddCallbacksToNewSegment(tmpname,end_of_new_space);
	InsertNewSegmentIntoExe(name,callback_file_name,start_of_new_space);
#endif


	// create the output file in a totally different way using elfwriter. later we may 
	// use this instead of the old way.


	string elfwriter_filename="c.out";
	ElfWriter *ew=NULL;
	if(m_firp->GetArchitectureBitWidth()==64)
	{
		ew=new ElfWriter64(m_firp, m_add_sections);
	}
	else if(m_firp->GetArchitectureBitWidth()==32)
	{
		ew=new ElfWriter32(m_firp, m_add_sections);
	}
	else assert(0);

	ew->Write(elfiop,m_firp,elfwriter_filename, "a.ncexe");
	delete ew;
	string chmod_cmd=string("chmod +x "); 
	chmod_cmd=chmod_cmd+elfwriter_filename;
	system(chmod_cmd.c_str());
}


void ZiprImpl_t::PrintStats()
{
	// do something like print stats as #ATTRIBUTES.
	m_dollop_mgr.PrintStats(cout);
	m_dollop_mgr.PrintPlacementMap(memory_space, m_dollop_map_filename);
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
#elif support_stratafier_mode
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
#endif	// #elif support_stratafier_mode from #ifndef CGC
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
	RangeSet_t::iterator range_it=memory_space.FindFreeRange((RangeAddress_t) -1);
	assert(memory_space.IsValidRange(range_it));

	RangeAddress_t end_of_new_space=range_it->GetStart();
	RangeAddress_t start_addr=GetCallbackStartAddr();

	set<std::pair<DollopEntry_t*,RangeAddress_t> >::iterator it, it_end;

	for(it=unpatched_callbacks.begin(), it_end=unpatched_callbacks.end();
	    it!=it_end;
	    it++
	   )
	{
		DollopEntry_t *entry=it->first;
		Instruction_t *insn = entry->Instruction();
		RangeAddress_t at=it->second;
		RangeAddress_t to=FindCallbackAddress(end_of_new_space,start_addr,insn->GetCallback());
		DLFunctionHandle_t patcher = NULL;

		if (plugman.DoesPluginRetargetCallback(at, entry, to, patcher))
		{
			if (m_verbose)
			{
				cout << "Patching retargeted callback at " << std::hex << at << " to "
				     << patcher->ToString() << "-assigned address: "
						 << std::hex << to << endl;
			}
		}

		if(to)
		{
			cout<<"Patching callback "<< insn->GetCallback()<<" at "<<std::hex<<at<<" to jump to "<<to<<endl;
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
	    <<left<<setw(10)<<"FuncID"
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
		    <<hex<<left<<setw(10)<<( insn->GetFunction() ? insn->GetFunction()->GetBaseID() : -1 )
		    << left<<insn->getDisassembly()<<endl;

		
	}



}

void ZiprImpl_t::UpdateScoops()
{
	for(
		DataScoopSet_t::iterator it=m_zipr_scoops.begin(); 
		it!=m_zipr_scoops.end();
	   )
	{
		DataScoop_t* scoop=*it;

		if(!scoop->isExecuteable())
		{
			++it;
			continue;
		}
		assert(m_zipr_scoops.find(scoop)!=m_zipr_scoops.end());

		virtual_offset_t first_valid_address=0;
		virtual_offset_t last_valid_address=0;
		for(virtual_offset_t i=scoop->GetStart()->GetVirtualOffset() ; i<= scoop->GetEnd()->GetVirtualOffset(); i++ )
		{
			if( ! memory_space.IsByteFree(i) )
			{
				// record beginning if not already recorded.
				if(first_valid_address==0)
					first_valid_address=i;

				// update the scoop
				scoop->GetContents()[i-scoop->GetStart()->GetVirtualOffset()]=memory_space[i];

				// record that this address was valid.
				last_valid_address=i;

			}
		}


		if(last_valid_address==0 || first_valid_address==0)
		{
			assert(first_valid_address==0);
			assert(last_valid_address==0);
			m_firp->GetAddresses().erase(scoop->GetStart());
			m_firp->GetAddresses().erase(scoop->GetEnd());

			// erase and move to next element.
			it=m_firp->GetDataScoops().erase(it);

			// remove addresses and scoop
			delete scoop->GetStart();
			delete scoop->GetEnd();
			delete scoop;
			scoop=NULL;
			
			
		}
		else
		{
			if(scoop->GetStart()->GetVirtualOffset() != first_valid_address || scoop->GetEnd()->GetVirtualOffset() != last_valid_address)
				cout<<"Shrinking scoop "<<scoop->GetName()<<" to "<<hex<<first_valid_address<<"-"<<last_valid_address<<endl;
			else
				cout<<"Leaving scoop "<<scoop->GetName()<<" alone. "<<endl;
			assert(first_valid_address!=0);
			assert(last_valid_address!=0);
			scoop->GetStart()->SetVirtualOffset(first_valid_address);
			scoop->GetEnd()->SetVirtualOffset(last_valid_address);
			++it;
		}

	}
	return;
}



void  ZiprImpl_t::FixMultipleFallthroughs()
{
	int count=0;
	map<Instruction_t*, InstructionSet_t> fallthrough_from;
	for_each(m_firp->GetInstructions().begin(), m_firp->GetInstructions().end(), 
		[&fallthrough_from](Instruction_t* insn)
	{
		Instruction_t* ft=insn->GetFallthrough();
		if(ft)
			fallthrough_from[ft].insert(insn);
	});

	for_each(fallthrough_from.begin(), fallthrough_from.end(), [&](const pair<Instruction_t*, InstructionSet_t>& p)
	{
		Instruction_t* ft=p.first;
		if(p.second.size()>1)
		{
			// skip the first one, because something can fallthrough, just not everything.
			for_each(next(p.second.begin()), p.second.end(), [&](Instruction_t* from)
			{
			
				Instruction_t* newjmp=addNewAssembly(m_firp, NULL, "jmp 0");
				count++;

				newjmp->SetTarget(ft);
				from->SetFallthrough(newjmp);

			});
		};
	});

	// after we've inserted all the jumps, assemble them.
	m_firp->AssembleRegistry();

	cout<<"#ATTRIBUTE zipr::jumps_inserted_for_multiple_fallthroughs="<<dec<<count<<endl;
}
