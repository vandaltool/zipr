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

#define ALLOF(a) begin(a),end(a)

using namespace libIRDB;
using namespace std;
using namespace zipr;
using namespace ELFIO;
using namespace IRDBUtility;
using namespace Zipr_SDK;


inline uintptr_t page_round_up(uintptr_t x)
{
	return  ( (((uintptr_t)(x)) + PAGE_SIZE-1)  & (~(PAGE_SIZE-1)) );
}

inline uintptr_t page_round_down(uintptr_t x)
{
	return  ( (((uintptr_t)(x)) - (PAGE_SIZE-1))  & (~(PAGE_SIZE-1)) );
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

void ZiprImpl_t::Init()
{

	// allocate stats
	m_stats = new Stats_t();

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

	// need a file IR to create the arch-specific stuff.
	// without it, we won't run anything anyhow
	if(m_firp)
	{
		archhelper=ZiprArchitectureHelperBase_t::factory(this);
		pinner =archhelper->getPinner ();
		patcher=archhelper->getPatcher();
		sizer  =archhelper->getSizer  ();
	}

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
	m_bss_opts.SetDescription("Enable/Disable optimizing BSS segments so they aren't written to the binary.");
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
	m_paddable_minimum_distance.SetDescription("Specify the minimum size of a gap to be filled.");

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
	zipr_namespace->AddOption(&m_paddable_minimum_distance);

	global->AddOption(&m_variant);
	global->AddOption(&m_verbose);
	global->AddOption(&m_vverbose);
	global->AddOption(&m_apply_nop);
	global->AddOption(&m_add_sections);
	global->AddOption(&m_bss_opts);

	zipr_namespace->MergeNamespace(memory_space.RegisterOptions(global));
	return zipr_namespace;
}


void ZiprImpl_t::CreateBinaryFile()
{


	/* load the elfiop for the orig. binary */
	lo = new pqxx::largeobject(m_firp->GetFile()->GetELFOID());
	lo->to_file(m_pqxx_interface.GetTransaction(),string(m_output_filename).c_str());

	/* use ELFIO to load the sections */
	assert(elfiop);
	elfiop->load(m_output_filename);
	ELFIO::dump::section_headers(cout,*elfiop);

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


	FixTwoByteWithPrefix();	// have to do this before multi-fallthrough in case it creaates some.
	FixNoFallthroughs();	// have to do this before multi-fallthrough in case it creaates some.
	FixMultipleFallthroughs();


	// create ranges, including extra range that's def. big enough.
	FindFreeRanges(m_output_filename);

	plugman.PinningBegin();

	// allocate and execute a pinning algorithm.
	assert(pinner);
	pinner->doPinning();

	// tell plugins we are done pinning.
	plugman.PinningEnd();

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
	for(auto it = ordered_sections.begin(); it!=ordered_sections.end();  /* empty */ ) 
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
		auto text_end=new AddressID_t();

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
		DataScoop_t* text_scoop=new DataScoop_t(m_firp->GetMaxBaseID()+1, text_name,  text_start, text_end, NULL, 5 /*R-X*/, false, text_contents);
		m_firp->GetDataScoops().insert(text_scoop);
	
		cout<<"Adding scoop "<<text_scoop->GetName()<<hex<<" at "<<hex<<text_start->GetVirtualOffset()<<" - "<<text_end->GetVirtualOffset()<<endl;
		m_zipr_scoops.insert(text_scoop);
		memory_space.AddFreeRange(Range_t(text_start->GetVirtualOffset(),text_end->GetVirtualOffset()), true);
	}
}


RangeAddress_t ZiprImpl_t::PlaceUnplacedScoops(RangeAddress_t max_addr)
{
	max_addr=plugman.PlaceScoopsBegin(max_addr);

	auto scoops_by_perms= map<int,DataScoopSet_t>();
	for(auto scoop : m_firp->GetDataScoops())
	{
		// check if placed.
		if(scoop->GetStart()->GetVirtualOffset()==0)
			scoops_by_perms[scoop->isRelRo() << 16 | scoop->getRawPerms()].insert(scoop);
	}
	
	// for(auto pit=scoops_by_perms.begin(); pit!=scoops_by_perms.end(); ++pit)
	for(auto &p : scoops_by_perms )
	{
		// start by rounding up to a page boundary so that page perms don't get unioned.
		max_addr=page_round_up(max_addr); 
		for(auto &scoop  : p.second)
		{
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


	// assert we unpinned everything 
	for(const auto s : m_firp->GetDataScoops())
		assert(s->GetStart()->GetVirtualOffset()!=0);
	
	
	max_addr=plugman.PlaceScoopsEnd(max_addr);

	return max_addr;
}

void ZiprImpl_t::FindFreeRanges(const std::string &name)
{
	DataScoop_t *textra_scoop = nullptr;
	AddressID_t *textra_start_addr = new AddressID_t(),
	            *textra_end_addr = new AddressID_t();
	RangeAddress_t textra_start, textra_end;
	string textra_contents, textra_name;
	RangeAddress_t max_addr=0;
	std::map<RangeAddress_t, int> ordered_sections;
	DataScoopByAddressSet_t sorted_scoop_set;


	/*
	 * This function should be the *only* place where
	 * scoops are added to m_zipr_scoops. This assert
	 * is here to maintain that variant.
	 */
	assert(m_zipr_scoops.empty());

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
	for (auto p : ordered_sections )
	{ 
		section* sec = elfiop->sections[p.second];
		assert(sec);

		RangeAddress_t start=sec->get_address();
		RangeAddress_t end=sec->get_size()+start-1;

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

	/*
	 *  First things first: Let's put empty scoops
	 *  in all the gaps.
	 */

	if (m_verbose)
		cout << "Filling gaps that are larger than " << std::dec
		     << m_paddable_minimum_distance << " bytes." << endl;

	/*
	 * Only put pinned data scoops into the list of
	 * scoops to consider for adding gap filling.
	 */
	copy_if(ALLOF(m_firp->GetDataScoops()),
	        inserter(sorted_scoop_set, sorted_scoop_set.begin()),
	        [](DataScoop_t* ds)
	        {
	        	return ds->GetStart()->GetVirtualOffset() != 0;
	        }
	);
	for( auto it=sorted_scoop_set.begin(); it!=sorted_scoop_set.end(); ++it )
	{
		auto this_scoop=*it;
		DataScoop_t* next_scoop=NULL;
		RangeAddress_t this_end = this_scoop->GetEnd()->GetVirtualOffset(),
		               next_start = 0;

		assert(this_scoop->GetStart()->GetVirtualOffset()!=0);

		if (m_verbose)
			cout << "There's a scoop between " << std::hex
			     << this_scoop->GetStart()->GetVirtualOffset()
			     << " and " << std::hex << this_scoop->GetEnd()->GetVirtualOffset()
			     << " with permissions " << std::hex << this_scoop->getRawPerms()
			     << endl;

		/*
		 * Never pad after the last scoop.
		 */
		if (std::next(it,1) != sorted_scoop_set.end())
		{
			next_scoop = *std::next(it,1);
			next_start = next_scoop->GetStart()->GetVirtualOffset();
			unsigned int new_padding_scoop_size = 0;
			RangeAddress_t new_padding_scoop_start = this_end + 1;
			RangeAddress_t new_padding_scoop_end = next_start - 1;

			if (this_end > next_start) {
				/*
				 * It's possible that sections overlap
				 * one another. Computing the distance
				 * as an unsigned (as below) causes problems.
				 * So, we make a special check here.
				 */
				if (m_verbose)
					cout << "Not considering this section because it "
					     << "does not end before the next one starts." << endl;
				continue;
			}

			if (m_verbose)
				cout << "Considering a gap between: 0x" << std::hex 
				     << new_padding_scoop_start << "-0x"
				     << std::hex << new_padding_scoop_end
				     << endl;

			/*
			 * If the adjacent scoop is writable, we
			 * do not want to put an executable scoop
			 * in the same page.
			 */
			if (this_scoop->isWriteable())
			{
				new_padding_scoop_start = page_round_up(new_padding_scoop_start);

				if (m_verbose)
					cout << "Adjacent scoop is writable. Adjusting start up to 0x"
					     << std::hex << new_padding_scoop_start << "." << endl;
			}

			/*
			 * If the next scoop is writable, we
			 * do not want to put an executable scoop
			 * in the same page.
			 */
			if (next_scoop->isWriteable())
			{
				new_padding_scoop_end = page_round_down(new_padding_scoop_end);

				if (m_verbose)
					cout << "Next scoop is writable. Adjusting end down to 0x"
					     << std::hex << new_padding_scoop_end << "." << endl;
			}

			/*
			 * After making the proper adjustments, we know
			 * the size of the gap. So, now we have to determine
			 * whether to pad or not:
			 *
			 * 1. Is the gap bigger than the user-defined gap criteria
			 * 2. One or both of the surrounding segments are not
			 *    writable (a policy decision not to pad between
			 *    writable segments.
			 */
			new_padding_scoop_size = new_padding_scoop_start - new_padding_scoop_end;
			if ((new_padding_scoop_size>(unsigned int)m_paddable_minimum_distance) &&
			    (!this_scoop->isWriteable() || !next_scoop->isWriteable())
				 )
			{
				DataScoop_t *new_padding_scoop = nullptr;
				string new_padding_scoop_contents, new_padding_scoop_name;
				int new_padding_scoop_perms = 0x5 /* r-x */;
				AddressID_t *new_padding_scoop_start_addr = nullptr,
				            *new_padding_scoop_end_addr = nullptr;

				new_padding_scoop_name = "zipr_scoop_"+
				                         to_string(new_padding_scoop_start);

				new_padding_scoop_start_addr = new AddressID_t();
				new_padding_scoop_end_addr = new AddressID_t();
				new_padding_scoop_start_addr->SetVirtualOffset(new_padding_scoop_start);
				new_padding_scoop_end_addr->SetVirtualOffset(new_padding_scoop_end);
				m_firp->GetAddresses().insert(new_padding_scoop_start_addr);
				m_firp->GetAddresses().insert(new_padding_scoop_end_addr);

				cout << "Gap filling with a scoop between 0x"
				     << std::hex << new_padding_scoop_start << " and 0x"
				     << std::hex << new_padding_scoop_end
				     << endl;

				new_padding_scoop = new DataScoop_t(m_firp->GetMaxBaseID()+1,
				                                    new_padding_scoop_name,
				                                    new_padding_scoop_start_addr,
				                                    new_padding_scoop_end_addr,
				                                    NULL,
				                                    new_padding_scoop_perms,
				                                    false,
				                                    new_padding_scoop_contents);
				new_padding_scoop_contents.resize(new_padding_scoop->GetSize());
				new_padding_scoop->SetContents(new_padding_scoop_contents);

				/*
				 * Insert this scoop into a list of scoops that Zipr added.
				 */
				m_zipr_scoops.insert(new_padding_scoop);

				/*
				 * Tell Zipr that it can put executable code in this section.
				 */
				memory_space.AddFreeRange(Range_t(new_padding_scoop_start,
				                                  new_padding_scoop_end), true);
			}
		}
	}

	/*
	 * Scan the scoops that we added to see if we went beyond
	 * the previously highest known address. This should never
	 * happen because we never pad after the last scoop.
	 */
	auto max_addr_zipr_scoops_result = max_element(ALLOF(m_zipr_scoops),
		[](DataScoop_t *a, DataScoop_t *b)
		{
			return a->GetEnd()->GetVirtualOffset() <
			       b->GetEnd()->GetVirtualOffset();
		}
	);
	assert(max_addr>=(*max_addr_zipr_scoops_result)->GetEnd()->GetVirtualOffset());

	max_addr=PlaceUnplacedScoops(max_addr);

	// now that we've looked at the sections, add a (mysterious) extra section in case we need to overflow 
	// the sections existing in the ELF.
	RangeAddress_t new_free_page=page_round_up(max_addr);


	/*
	 * TODO
	 *
	 * Make a scoop out of this. Insert it into m_zipr_scoops
	 * and m_firp->GetDataScoops()
	 */
	textra_start = new_free_page;
	textra_end = (RangeAddress_t)-1;
	textra_name = "textra";

	textra_start_addr->SetVirtualOffset(textra_start);
	textra_end_addr->SetVirtualOffset(textra_end);

	cout << "New free space: 0x" << std::hex << textra_start
	     << "-0x"
	     << std::hex << textra_end
	     << endl;

	textra_scoop = new DataScoop_t(m_firp->GetMaxBaseID()+1,
	                                    textra_name,
	                                    textra_start_addr,
	                                    textra_end_addr,
	                                    NULL,
	                                     /* r-x */5,
	                                    false,
	                                    textra_contents);

	/*
	 * Normally we would have to resize the underlying contents here.
	 * Unfortunately that's not a smart idea since it will be really big.
	 * Instead, we are going to do a batch resizing below.
	textra_contents.resize(textra_end - textra_start + 1);
	textra_scoop->SetContents(textra_contents);
	 */

	m_zipr_scoops.insert(textra_scoop);
	m_firp->GetAddresses().insert(textra_start_addr);
	m_firp->GetAddresses().insert(textra_end_addr);

	memory_space.AddFreeRange(Range_t(new_free_page,(RangeAddress_t)-1), true);
	if (m_verbose)
		printf("Adding (mysterious) free range 0x%p to EOF\n", (void*)new_free_page);
	start_of_new_space=new_free_page;

	for(auto scoop : m_firp->GetDataScoops())
	{
		if(scoop->isExecuteable()) continue;
		// put scoops in memory to make sure they are busy,
		// just in case they overlap with free ranges.
		// this came up on Aarch64 because data is in the .text segment.
		cout<<"Pre-allocating scoop "<<scoop->GetName() << "=("
		    << scoop->GetStart()->GetVirtualOffset() << "-" 
		    << scoop->GetEnd()  ->GetVirtualOffset() << ")"<<endl;
		memory_space.PlopBytes(scoop->GetStart()->GetVirtualOffset(), 
		                       scoop->GetContents().c_str(),
				       scoop->GetContents().size()
				      );
	}
}


Instruction_t *ZiprImpl_t::FindPatchTargetAtAddr(RangeAddress_t addr)
{
        std::map<RangeAddress_t,UnresolvedUnpinnedPatch_t>::iterator it=m_PatchAtAddrs.find(addr);
        if(it!=m_PatchAtAddrs.end())
                return it->second.first.GetInstruction();
        return NULL;
}


void ZiprImpl_t::WriteDollops()
{
	for (auto & dollop_to_write : m_dollop_mgr.GetDollops()  ) 
	{
		assert(dollop_to_write != nullptr);
		// skip unplaced dollops as they aren't necessary
		if (!dollop_to_write->IsPlaced())
			continue;
	
		// write each entry in the dollop
		for (auto &entry_to_write : *dollop_to_write) 
		{
			assert(entry_to_write != nullptr);
			// plop it.
			const auto de_end_loc = _PlopDollopEntry(entry_to_write);

			// sanity check that we didn't go passed the worst case size we calculate for this entry
			const auto de_start_loc = entry_to_write->Place();
			const auto should_end_at = de_start_loc + DetermineDollopEntrySize(entry_to_write, false);
			assert(de_end_loc == should_end_at);
			/*
			 * Build up a list of those dollop entries that we have
			 * just written that have a target. See comment above 
			 * ReplopDollopEntriesWithTargets() for the reason that
			 * we have to do this.
			 */
			const auto will_replop=entry_to_write->TargetDollop()!=nullptr;
			if (will_replop)
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
	for (auto entry_to_write : m_des_to_replop)
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

	/*
	 * Build up initial placement q with destinations of
	 * pins.
	 */
	for (auto p : patch_list)
	{
		auto uu = p.first;
		auto patch = p.second;
		auto target_insn = uu.GetInstruction();
		auto target_dollop = m_dollop_mgr.GetContainingDollop(target_insn);
		assert(target_dollop);

		placement_queue.insert(pair<Dollop_t*,RangeAddress_t>(target_dollop,patch.GetAddress()));
		if (m_verbose) 
		{
			cout << "Original: " << hex << target_insn-> GetAddress()-> GetVirtualOffset() << " "
			     << "vs. Patch: " << patch.GetAddress() << endl;
		}
		count_pins++;
	}

	assert(getenv("SELF_VALIDATE")==nullptr || count_pins > 5 ) ;
	assert(getenv("SELF_VALIDATE")==nullptr || placement_queue.size() > 15 ) ;

	cout<<"# ATTRIBUTE Zipr::pins_detected="<<dec<<count_pins<<endl;
	cout<<"# ATTRIBUTE Zipr::placement_queue_size="<<dec<<placement_queue.size()<<endl;

	while (!placement_queue.empty())
	{
		auto placement=Range_t();
		DLFunctionHandle_t placer = NULL;
		auto placed = false;
		RangeAddress_t cur_addr = 0 ;
		bool has_fallthrough = false;
		Dollop_t *fallthrough = NULL;
		auto continue_placing = false;

		auto initial_placement_abuts_pin = false;
		auto initial_placement_abuts_fallthrough = false;
		auto fits_entirely = false;
		RangeAddress_t fallthrough_dollop_place = 0;
		bool fallthrough_has_preplacement = false;

		auto pq_entry = *(placement_queue.begin());
		placement_queue.erase(placement_queue.begin());

		auto to_place = pq_entry.first;
		auto from_address = pq_entry.second;

		if (m_vverbose)
		{
			cout << "Placing dollop with original starting address: " << hex
			     << to_place->front()->Instruction()->GetAddress()->GetVirtualOffset() << endl;
		}

		if (to_place->IsPlaced())
			continue;

		to_place->ReCalculateSize();

		auto minimum_valid_req_size = std::min(
			DetermineDollopEntrySize(to_place->front(), true),
			sizer->DetermineDollopSizeInclFallthrough(to_place));
		/*
		 * Ask the plugin manager if there are any plugins
		 * that want to tell us where to place this dollop.
		 */
		auto am_coalescing = false;
		auto allowed_coalescing = true;
	       	auto allowed_fallthrough = true;
		if (plugman.DoesPluginAddress(to_place, from_address, placement, allowed_coalescing, allowed_fallthrough, placer))
		{
			placed = true;

			if (m_verbose)
				cout << placer->ToString() << " placed this dollop between " 
				     << hex << placement.GetStart() << " and " << placement.GetEnd()
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
			const auto has_fallthrough = to_place->FallthroughDollop() != nullptr;
			const auto ibta=has_fallthrough ? to_place->FallthroughDollop()-> front()-> Instruction()-> GetIndirectBranchTargetAddress() : 0; 
			initial_placement_abuts_pin = has_fallthrough && 
		   	                              ibta && 
		   	                              ibta -> GetVirtualOffset()!=0   && 
		   	                              ibta-> GetVirtualOffset() == (placement.GetStart() + to_place->GetSize() - sizer->TRAMPOLINE_SIZE);
			/*
			 * If this dollop has a fallthrough, find out where that 
			 * fallthrough is (or is going to be) placed. That way
			 * we can determine if the current dollop is (or is going to be)
			 * adjacent to the place of the fallthrough. That means
			 * that we can keep from placing a jump to the dollo
			 * and instead just fallthrough.
			 */
			if (to_place->FallthroughDollop() && allowed_fallthrough) 
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
					bool fallthrough_allowed_fallthrough = false;
					DLFunctionHandle_t fallthrough_placer = NULL;
					/*
					 * Prospectively get the place for this dollop. That way 
					 * we can determine whether or not we need to use a fallthrough!
					 */
					if (plugman.DoesPluginAddress(to_place->FallthroughDollop(),
		                                    from_address,
		                                    fallthrough_placement,
		                                    fallthrough_allowed_coalescing,
		                                    fallthrough_allowed_fallthrough,
		                                    fallthrough_placer))
					{
						fallthrough_dollop_place = fallthrough_placement.GetStart();
						fallthrough_has_preplacement = true;
					}
				}
			}
			initial_placement_abuts_fallthrough = to_place->FallthroughDollop() &&
			                                           fallthrough_has_preplacement &&
			                                           fallthrough_dollop_place == (placement.GetStart() + to_place->GetSize() - sizer->TRAMPOLINE_SIZE);


			auto fits_entirely = (to_place->GetSize() <= (placement.GetEnd()-placement.GetStart()));

			if (m_verbose)
			{
				cout << "initial_placement_abuts_pin        : "
				     <<initial_placement_abuts_pin << endl
				     << "initial_placement_abuts_fallthrough: " 
				     << initial_placement_abuts_fallthrough << endl
				     << "fits_entirely                      : " 
				     << fits_entirely << endl;
			}

			if ( ((placement.GetEnd()-placement.GetStart()) < minimum_valid_req_size) &&
			    !(initial_placement_abuts_pin || initial_placement_abuts_fallthrough || fits_entirely)
			   )
			{
				if (m_verbose)
					cout << "Bad GetNearbyFreeRange() result." << endl;
				placed = false;
			}
		}

		if (!placed) 
		{
			// cout << "Using default place locator." << endl;
			/*
			 * TODO: Re-enable this ONCE we figure out why the dollop
			 * sizes are not being recalculated correctly.
			 */
			//placement = memory_space.GetFreeRange(to_place->GetSize());
			placement = sizer->DoPlacement(minimum_valid_req_size);

			/*
			 * Reset allowed_coalescing because DoesPluginAddress
			 * may have reset it and we may have rejected the results
			 * of that addressing.
			 */
			allowed_coalescing = true;
		}

		cur_addr = placement.GetStart();
		//cout << "Adjusting cur_addr to " << std::hex << cur_addr << " at A." << endl;
		has_fallthrough = (to_place->FallthroughDollop() != NULL);

		if (m_vverbose)
		{
			cout << "Dollop size=" << dec << to_place->GetSize() << ".  Placing in hole size="
			     << (placement.GetEnd() - placement.GetStart()) << " hole at " << hex << cur_addr << endl;
			cout << "Dollop " << ((has_fallthrough) ? "has " : "does not have ")
		  	     << "a fallthrough" << endl;
		}

		const auto has_pinned_ibta=
				to_place->front()->Instruction()->GetIndirectBranchTargetAddress() && 
				to_place->front()->Instruction()->GetIndirectBranchTargetAddress()->GetVirtualOffset()!=0 ;
		const auto pinned_ibta_addr = has_pinned_ibta ?  
				to_place-> front()-> Instruction()-> GetIndirectBranchTargetAddress()-> GetVirtualOffset() : 
				virtual_offset_t(0);
		if (has_pinned_ibta && cur_addr == pinned_ibta_addr)
		{
			unsigned int space_to_clear = sizer->SHORT_PIN_SIZE;
			/*
			 * We have placed this dollop at the location where
			 * its first instruction was pinned in memory.
			 */
			if (m_verbose)
				cout << "Placed atop its own pin!" << endl;

			if (memory_space[cur_addr] == (char)0xe9)
				space_to_clear = sizer->LONG_PIN_SIZE;

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
		else if ( // has dollop that falls through to us.
			  to_place->FallbackDollop() && 
			  // and it's already placed.
		    	  to_place->FallbackDollop()->IsPlaced() && 
			  // and the place is adjacent to us
			  ( to_place->FallbackDollop()->Place() + to_place->FallbackDollop()->GetSize() - sizer->TRAMPOLINE_SIZE) == placement.GetStart()
			)
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

		do 
		{
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
		
			to_place->ReCalculateSize();

			/*
			 * Calculate before we place this dollop.
			 */
			wcds = sizer->DetermineDollopSizeInclFallthrough(to_place);

			to_place->Place(cur_addr);

			// cout << "to_place->GetSize(): " << to_place->GetSize() << endl;

			fits_entirely = (to_place->GetSize() <= (placement.GetEnd()-cur_addr));
			all_fallthroughs_fit = (wcds <= (placement.GetEnd()-cur_addr));

			auto dit = to_place->begin();
			auto dit_end = to_place->end();
			for ( /* empty */; dit != dit_end; dit++)
			{
				DollopEntry_t *dollop_entry = *dit;
				/* 
				 * first, check if we need to add any reference dollops to the placement queue
				 */
				const auto handle_reloc=[&](const Relocation_t* reloc)
				{
					auto wrt_insn=dynamic_cast<Instruction_t*>(reloc->GetWRT());
					if(wrt_insn)
					{
						auto containing=m_dollop_mgr.GetContainingDollop(wrt_insn);
						assert(containing!=nullptr);
						if(!containing->IsPlaced())
						{
							placement_queue.insert(pair<Dollop_t*, RangeAddress_t>( containing, cur_addr));
							// cout<<"Adding to placement queue for reloc of type="<<reloc->GetType()<<endl;
						}
					}
				};


				// make sure each instruction referenced via a relocation is placed in a dollop
				auto insn=dollop_entry->Instruction();
				for(auto &reloc : insn->GetRelocations())
					handle_reloc(reloc);
				auto ehpgm=insn->GetEhProgram();
				if(ehpgm)
					for(auto &reloc : ehpgm->GetRelocations())
						handle_reloc(reloc);

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
				 *    See DetermineDollopSizeInclFallthrough().
				 *    Call this the all_fallthroughs_fit case.
				 * 5. NOT (All fallthroughs fit is the only way that we are
				 *    allowed to proceed placing this dollop but we are not 
				 *    allowed to coalesce and we are out of space for the 
				 *    jump to the fallthrough.)
				 *    Call this the !allowed_override case
				 * 6. There is enough room for this instruction AND it is
				 *    the last entry of this dollop AND the dollop has a
				 *    fallthrough AND that fallthrough is to a pin that
				 *    immediately follows this instruction in memory.
				 *    Call this initial_placement_abuts (calculated above).
				 */
				const auto de_and_fallthrough_fit = 
					// does this fit, i.e., end>current+rest_of_dollop
					(placement.GetEnd()>= (cur_addr+DetermineDollopEntrySize(dollop_entry, true)));
				const auto is_last_insn           = next(dit)==dit_end; /* last */ 
				const auto has_fallthrough_dollop = to_place->FallthroughDollop()!=nullptr ;
				const auto fits_with_fallthrough  = placement.GetEnd()>=(cur_addr+ DetermineDollopEntrySize(dollop_entry, has_fallthrough_dollop));
				const auto last_de_fits           = is_last_insn && fits_with_fallthrough;
				const auto could_fit_here         = 
					de_and_fallthrough_fit              || 
					fits_entirely                       || 
					last_de_fits                        || 
					initial_placement_abuts_pin         || 
					initial_placement_abuts_fallthrough ;
				const auto tramp_fits             =
				        (placement.GetEnd() - (cur_addr + DetermineDollopEntrySize( dollop_entry, false))) < sizer->TRAMPOLINE_SIZE;
				const auto allowed_override       = 
					allowed_coalescing    || 
					could_fit_here        || 
					!all_fallthroughs_fit || 
					!tramp_fits           ;
				const auto beneficial_to_override = 
					de_and_fallthrough_fit || 
					last_de_fits                        || 
					fits_entirely                       || 
					initial_placement_abuts_fallthrough || 
					initial_placement_abuts_pin         || 
					all_fallthroughs_fit                ;

				if (m_vverbose)
				{
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
					     << allowed_override                     << noboolalpha << endl;

				}

				if ( beneficial_to_override && allowed_override )
				{
					dollop_entry->Place(cur_addr);
					const auto wcsz=DetermineDollopEntrySize(dollop_entry, false);
					const auto next_cur_addr=cur_addr+wcsz;
					if (m_vverbose) 
					{
						DecodedInstruction_t d(dollop_entry->Instruction());
						cout << "Placing " << hex << dollop_entry->Instruction()->GetBaseID() 
						     << ":" << d.getDisassembly() << " at "
						     << cur_addr << "-" << next_cur_addr << endl;
					}
					cur_addr=next_cur_addr;
					if (dollop_entry->TargetDollop())
					{
						if (m_vverbose)
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
				auto split_dollop = to_place->Split((*dit)->Instruction());
				m_dollop_mgr.AddDollops(split_dollop);

				to_place->WasTruncated(true);
				if (am_coalescing)
					m_stats->truncated_dollops_during_coalesce++;

				if (m_vverbose)
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

			fallthrough = to_place->FallthroughDollop();
			if ( fallthrough  != nullptr && !to_place->WasCoalesced() )
			{
				size_t fallthroughs_wcds, fallthrough_wcis, remaining_size;

				/*
				 * We do not care about the fallthrough dollop if its 
				 * first instruction is pinned AND the last entry of this
				 * dollop abuts that pin.
				 */
				const auto has_ibta         = fallthrough-> front()-> Instruction()-> GetIndirectBranchTargetAddress();
				const auto pinned_ibta_addr = has_ibta ? fallthrough-> front()-> Instruction()-> GetIndirectBranchTargetAddress()-> GetVirtualOffset() : virtual_offset_t(0);
				const auto is_pinned_ibta_addr = has_ibta && pinned_ibta_addr!=0;
				const auto is_pinned_here = (cur_addr == pinned_ibta_addr ) ;
				if ( has_ibta && is_pinned_ibta_addr && is_pinned_here )
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
				if (!am_coalescing && to_place->FallthroughDollop() && fallthrough_has_preplacement && fallthrough_dollop_place == cur_addr)
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
						placement_queue.insert(pair<Dollop_t*, RangeAddress_t>( to_place->FallthroughDollop(), cur_addr));
					}
					m_stats->total_did_not_coalesce++;
					break;
				}
					
				/*
				 * We could fit the entirety of the dollop (and
				 * fallthroughs) ...
				 */
				fallthroughs_wcds = sizer->DetermineDollopSizeInclFallthrough(fallthrough);
				/*
				 * ... or maybe we just want to start the next dollop.
				 */
				fallthrough_wcis=DetermineDollopEntrySize(fallthrough-> front(),
																													 true);
				remaining_size = placement.GetEnd() - cur_addr;

				/*
				 * We compare remaining_size to min(fallthroughs_wdcs,
				 * fallthrough_wcis) since the entirety of the dollop
				 * and its fallthroughs could (its unlikely) be 
				 * smaller than the first instruction fallthrough 
				 * in the fallthrough dollop and the trampoline size.
				 */
				if (m_vverbose)
					cout << "Determining whether to coalesce: "
					     << "Remaining: " << std::dec << remaining_size
					     << " vs Needed: " << std::dec 
					     << std::min(fallthrough_wcis,fallthroughs_wcds) << endl;

				if (remaining_size < std::min(fallthrough_wcis,fallthroughs_wcds) || 
				    fallthrough->IsPlaced()                                       || 
				    !allowed_coalescing
				   )
				{

					string patch_jump_string;
					Instruction_t *patch = archhelper->createNewJumpInstruction(m_firp, NULL);
					DollopEntry_t *patch_de = new DollopEntry_t(patch, to_place);

					patch_de->TargetDollop(fallthrough);
					patch_de->Place(cur_addr);
					cur_addr+=DetermineDollopEntrySize(patch_de, false);
					//cout << "Adjusting cur_addr to " << std::hex << cur_addr << " at C." << endl;

					to_place->push_back(patch_de);
					to_place->FallthroughPatched(true);

					if (m_vverbose)
						cout << "Not coalescing"
						     << string((fallthrough->IsPlaced()) ?  " because fallthrough is placed" : "")
						     << string((!allowed_coalescing) ?  " because I am not allowed" : "")
						     << "; Added jump (via " << std::hex << patch_de
						     << " at " << std::hex << patch_de->Place() << ") "
						     << "to fallthrough dollop (" << std::hex 
						     << fallthrough << ")." << endl;

					placement_queue.insert(pair<Dollop_t*, RangeAddress_t>( fallthrough, cur_addr));
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
					if (m_vverbose)
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
		} while (continue_placing); 
		/* 
		 * This is the end of the do-while-true loop
		 * that will place as many fallthrough-linked 
		 * dollops as possible.
		 */

		/*
		 * Reserve the range that we just used.
		 */
		if (m_vverbose)
			cout << "Reserving " << std::hex << placement.GetStart()
			     << ", " << std::hex << cur_addr << "." << endl;
		memory_space.SplitFreeRange(Range_t(placement.GetStart(), cur_addr));
	}
}

void ZiprImpl_t::RecalculateDollopSizes()
{
	for (auto &dollop : m_dollop_mgr.GetDollops())
		dollop->ReCalculateSize();
}

void ZiprImpl_t::CreateDollops()
{
	if (m_verbose)
		cout<< "Attempting to create "
		    << patch_list.size()
				<< " dollops for the pins."
				<< endl;
	for (auto patch : patch_list )
		m_dollop_mgr.AddNewDollops(patch.first.GetInstruction());

	if (m_verbose)
		cout << "Done creating dollops for the pins! Updating all Targets" << endl;

	m_dollop_mgr.UpdateAllTargets();

	if (m_verbose)
		cout << "Created " <<std::dec << m_dollop_mgr.Size() << " total dollops." << endl;
}

void ZiprImpl_t::CallToNop(RangeAddress_t at_addr)
{
	assert(patcher);
	patcher->CallToNop(at_addr);
	return;
}

void ZiprImpl_t::PatchCall(RangeAddress_t at_addr, RangeAddress_t to_addr)
{
	assert(patcher);
	patcher->PatchCall(at_addr,to_addr);
}

size_t ZiprImpl_t::DetermineDollopEntrySize(DollopEntry_t *entry, bool account_for_fallthrough)
{
	std::map<Instruction_t*,unique_ptr<list<DLFunctionHandle_t>>>::const_iterator plop_it;
	size_t opening_size = 0, closing_size = 0;
	size_t wcis = DetermineInsnSize(entry->Instruction(), account_for_fallthrough);

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

size_t ZiprImpl_t::DetermineInsnSize(Instruction_t* insn, bool account_for_fallthrough)
{
	std::map<Instruction_t*,unique_ptr<list<DLFunctionHandle_t>>>::const_iterator plop_it;
	size_t worst_case_size = 0;
	size_t default_worst_case_size = 0;

	default_worst_case_size = sizer->DetermineInsnSize(insn, account_for_fallthrough);

	plop_it = plopping_plugins.find(insn);
	if (plop_it != plopping_plugins.end())
	{
		for (auto handle : *(plop_it->second))
		{
			ZiprPluginInterface_t *zpi = dynamic_cast<ZiprPluginInterface_t*>(handle);
			worst_case_size =std::max(zpi->InsnSize(insn,
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

	if (m_vverbose)
	{
		const auto inc_jmp=((account_for_fallthrough) ? " (including jump)" : "");
		cout << "Worst case size" << inc_jmp << ": " << worst_case_size << endl;
	}

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

	for(auto &insn : m_firp->GetInstructions())
		AskPluginsAboutPlopping(insn);
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
		RangeAddress_t patch_addr, target_addr;
		target_dollop = m_dollop_mgr.GetContainingDollop(uu.GetInstruction());
		assert(target_dollop != NULL);
		DLFunctionHandle_t patcher = NULL;

		target_dollop_entry = target_dollop->front();
		assert(target_dollop_entry != NULL);

		target_dollop_entry_instruction  = target_dollop_entry->Instruction();
		assert(target_dollop_entry_instruction != NULL &&
		       target_dollop_entry_instruction == uu.GetInstruction());


		patch_addr = p.GetAddress();
		target_addr = target_dollop_entry->Place();

		if (final_insn_locations.end() != final_insn_locations.find(target_dollop_entry->Instruction()))
			target_addr = final_insn_locations[target_dollop_entry->Instruction()];

		if (plugman.DoesPluginRetargetPin(patch_addr, target_dollop, target_addr, patcher))
		{
			if (m_verbose)
			{
				cout << "Patching retargeted pin at " << hex<<patch_addr << " to "
				     << patcher->ToString() << "-assigned address: " << target_addr << endl;
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
			{
				const auto d=DecodedInstruction_t(target_dollop_entry_instruction);
				cout << "Patching pin at " << hex << patch_addr << " to "
				     << target_addr << ": " << d.getDisassembly() << endl;
			}
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
	auto thepatch=Patch_t(from_addr,UncondJump_rel32);

	const auto it=final_insn_locations.find(to_insn);
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
		/*
		 * TODO: This debugging output is not really exactly correct.
		 */
		if (m_verbose)
			printf("Found a patch for %p -> %p\n", (void*)from_addr, (void*)to_addr); 
		// Apply Patch
		ApplyPatch(from_addr, to_addr);
	}
}

RangeAddress_t ZiprImpl_t::_PlopDollopEntry(DollopEntry_t *entry, RangeAddress_t override_address)
{
	const auto insn = entry->Instruction();
	const auto insn_wcis = DetermineInsnSize(insn, false);
	RangeAddress_t updated_addr = 0;
	RangeAddress_t target_address = 0;
	auto placed_insn = false;
	const auto target_dollop=entry->TargetDollop();
	if (target_dollop && target_dollop->front())
	{
		const auto entry_target_head_insn=entry-> TargetDollop()-> front()-> Instruction();
		const auto target_address_iter = final_insn_locations.find(entry_target_head_insn);
		if (target_address_iter != final_insn_locations.end())
		{
			target_address = target_address_iter->second;
			if (m_verbose)
				cout << "Found an updated target address location: "
				     << std::hex << target_address << endl;
		}
	}	


	auto placed_address = override_address == 0 ? entry->Place() : override_address;
	const auto plop_it = plopping_plugins.find(insn);
	if (plop_it != plopping_plugins.end())
	{
		for (auto pp : *(plop_it->second))
		{
			auto pp_placed_insn = false;
			const auto handle = pp;
			const auto zpi = dynamic_cast<ZiprPluginInterface_t*>(handle);
			const auto plugin_ret=zpi->PlopDollopEntry(entry, placed_address, target_address, insn_wcis, pp_placed_insn);
			updated_addr = std::max(plugin_ret, updated_addr);
			if (m_verbose)
			{
				cout << zpi->ToString() << " placed entry " 
				     << std::hex << entry 
				     << " at address: " << std::hex << placed_address 
				     << " " << (pp_placed_insn ? "and placed" : "but did not place")
				     << " the instruction."
				     << endl;
			}
		
			placed_insn |= pp_placed_insn;
		}
	}

	/*
	 * If no plugin actually placed the instruction,
	 * then we are going to do it ourselves.
	 */
	if (!placed_insn)
	{
		/* Some plugins, like scfi, may place the entry but leave it 
		   up to zipr to place the instruction. This does assume that 
		   zipr will place the instruction in a way that is compatible 
		   with what the plugin is trying to do.
		   
		   TODO: Should we continue to allow this?
		*/
		const auto zipr_ret = PlopDollopEntry(entry, placed_address, target_address); 
		updated_addr = std::max(zipr_ret, updated_addr);
	}

	// sanity check that we aren't moving an instruction that's already been placed.	
	const auto old_loc=final_insn_locations[insn];
	if(old_loc != 0 && old_loc != placed_address )
	{
		static int count=0;
		cout<<"Warning, Moving instruction "<<hex<<insn->GetBaseID()<<":"<<insn->GetComment()
		    <<" from "<<hex<<old_loc<<" to "<<placed_address<<endl;
		cout<<"Happened for "<<dec<<count++<<" out of "<<m_firp->GetInstructions().size()<<" instructions"<<endl;
	}

	final_insn_locations[insn] = placed_address;
	return updated_addr;
}

RangeAddress_t ZiprImpl_t::PlopDollopEntry(
	DollopEntry_t *entry,
	RangeAddress_t override_place,
	RangeAddress_t override_target)
{
	Instruction_t *insn = entry->Instruction();
	RangeAddress_t ret = entry->Place(), addr = entry->Place();

	assert(insn);

	if (override_place != 0)
		addr = ret = override_place;

	const auto d=DecodedInstruction_t(insn);

	string raw_data = insn->GetDataBits();
	string orig_data = insn->GetDataBits();


	if(entry->TargetDollop() && entry->Instruction()->GetCallback()=="")
	{
		RangeAddress_t target_address = 0;
		auto target_insn = entry->TargetDollop()->front()->Instruction();

		if (override_target == 0)
		{	
			if (final_insn_locations.end() != final_insn_locations.find(target_insn))
				target_address = final_insn_locations[target_insn];
		}
		else
		{
			if (m_verbose)
				cout << "Plopping with overriden target: Was: " 
				     << hex << target_address << " Is: " << override_target << endl;
			target_address = override_target;
		}

		if (m_verbose)
		{
			const auto print_target=((target_address != 0) ? target_address : entry->TargetDollop()->Place());
			cout << "Plopping '"<<entry->Instruction()->getDisassembly() <<"' at " << hex << addr
			     << " with target " << print_target << endl;
		}
		ret=PlopDollopEntryWithTarget(entry, addr, target_address);
	}
	else if(entry->Instruction()->GetCallback()!="")
	{
		if (m_verbose)
			cout << "Plopping at " << hex << addr << " with callback to " 
			     << entry->Instruction()->GetCallback() << endl;

		ret=PlopDollopEntryWithCallback(entry, addr);
	}
	else
	{
		if (m_verbose)
			cout << "Plopping non-ctl "<<insn->getDisassembly()<<" at " << hex << addr << endl;
		memory_space.PlopBytes(addr, insn->GetDataBits().c_str(), insn->GetDataBits().length());
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
	return sizer->PlopDollopEntryWithTarget(entry,override_place,override_target);
#if 0

	auto insn = entry->Instruction();

	assert(entry->TargetDollop());

	auto addr = entry->Place();
	auto target_addr = entry->TargetDollop()->Place();
	auto ret = addr;

	if (override_place != 0)
		addr = ret = override_place;

	if (override_target != 0)
		target_addr = override_target;

	if(insn->GetDataBits().length() >2) 
	{
		memory_space.PlopBytes(ret,
		                       insn->GetDataBits().c_str(),
		                       insn->GetDataBits().length()
		                      );
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
			{
				if(entry->MemberOfDollop()->FallthroughDollop())
					fallthrough_de = entry->MemberOfDollop()->FallthroughDollop()->front();
				else
					// even a cond branch may have a null fallthrough, account for that here 
					// by plopping nothing
					return ret;
			}

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
#endif
}

RangeAddress_t ZiprImpl_t::PlopDollopEntryWithCallback(
	DollopEntry_t *entry,
	RangeAddress_t override_place)
{
	auto at = entry->Place();
	auto originalAt = entry->Place();

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

	assert(sizer->CALLBACK_TRAMPOLINE_SIZE<=(at-originalAt));
	return at;
}




DataScoop_t* ZiprImpl_t::FindScoop(const RangeAddress_t &addr)
{
	const auto find_it=find_if(ALLOF(m_firp->GetDataScoops()),
		[&](const DataScoop_t* scoop)
		{
			return scoop->GetStart()->GetVirtualOffset() <= addr &&
			       addr < scoop->GetEnd()->GetVirtualOffset()    ;
		});
	return find_it==m_firp->GetDataScoops().end() ?  nullptr : *find_it;
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
#endif //EXTEND_SECTIONS
	}
	rewrite_headers_elfiop->save(name);


        string myfn=name;
#ifdef CGC
        if(!use_stratafier_mode)
                myfn+=".stripped";
#endif //CGC

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

#endif //support_stratafier_mode

#ifdef support_stratafier_mode
	fclose(to_insert);

	callback_file_name = AddCallbacksToNewSegment(tmpname,end_of_new_space);
	InsertNewSegmentIntoExe(name,callback_file_name,start_of_new_space);
#endif


	// now that the textra scoop has been crated and setup, we have the info we need to 
	// re-generate the eh information.
	RelayoutEhInfo(); 


	// create the output file in a totally different way using elfwriter. later we may 
	// use this instead of the old way.


	string elfwriter_filename="c.out";
	ElfWriter *ew=NULL;
	if(m_firp->GetArchitectureBitWidth()==64)
	{
		ew=new ElfWriter64(m_firp, m_add_sections, m_bss_opts);
	}
	else if(m_firp->GetArchitectureBitWidth()==32)
	{
		ew=new ElfWriter32(m_firp, m_add_sections, m_bss_opts);
	}
	else assert(0);

	ew->Write(elfiop,m_firp,elfwriter_filename, "a.ncexe");
	delete ew;
	string chmod_cmd=string("chmod +x "); 
	chmod_cmd=chmod_cmd+elfwriter_filename;
	auto res=system(chmod_cmd.c_str());
	assert(res!=-1);
}


void ZiprImpl_t::PrintStats()
{
	// do something like print stats as #ATTRIBUTES.
	m_dollop_mgr.PrintStats(cout);
	m_dollop_mgr.PrintPlacementMap(memory_space, m_dollop_map_filename);
	m_stats->PrintStats(cout);

	// and dump a map file of where we placed instructions.  maybe guard with an option.
	// default to dumping to zipr.map 
	dump_scoop_map();
	dump_instruction_map();
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
		if(seg->get_type() != PT_LOAD)
			continue;
		if(last_seg && (last_seg->get_virtual_address() + last_seg->get_memory_size()) > (seg->get_virtual_address() + seg->get_memory_size())) 
			continue;
		if(seg->get_file_size()==0)
			continue;
		last_seg=seg;
		last_seg_index=i;
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

	set<std::pair<DollopEntry_t*,RangeAddress_t> >::iterator it, it_end;

	for(it=unpatched_callbacks.begin(), it_end=unpatched_callbacks.end();
	    it!=it_end;
	    it++
	   )
	{
		DollopEntry_t *entry=it->first;
		Instruction_t *insn = entry->Instruction();
		RangeAddress_t at=it->second;
		RangeAddress_t to=0x0;//FindCallbackAddress(end_of_new_space,start_addr,insn->GetCallback());
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
		{
			CallToNop(at);
		}
	}
}

void ZiprImpl_t::dump_scoop_map()
{
	string filename="scoop.map";	// parameterize later.
    	std::ofstream ofs(filename.c_str(), ios_base::out);
	ofs <<left<<setw(10)<<"ID"
	    <<left<<setw(10)<<"StartAddr"
	    <<left<<setw(10)<<"Size"
	    <<left<<setw(10)<<"Perms"
	    <<left<<setw(10)<<"Name"<<endl;

	for(const auto &scoop : m_firp->GetDataScoops())
	{
		ofs << hex << setw(10)<<scoop->GetBaseID()
		    <<hex<<left<<setw(10)<<scoop->GetStart()->GetVirtualOffset()
		    <<hex<<left<<setw(10)<< scoop->GetSize()
		    <<hex<<left<<setw(10)<< scoop->getRawPerms()
		    <<hex<<left<<setw(10)<< scoop->GetName()
		    << endl;
	}
}
void ZiprImpl_t::dump_instruction_map()
{
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
		virtual_offset_t first_valid_address=0;
		virtual_offset_t last_valid_address=0;

		if(!scoop->isExecuteable())
		{
			++it;
			continue;
		}

		assert(m_zipr_scoops.find(scoop)!=m_zipr_scoops.end());

		/*
		 * Yes, I know that this adds another iteration, but we need to know
		 * beforehand about shrinking.
		 */

		if (scoop->GetName() == "textra")
		{
			/*
			 * We have to do special handling for the textra scoop.
			 * If we do not, then the sheer scale of the default size
			 * of the textra scoop will cause Zipr to bomb during the
			 * next loop.
			 */
			auto frit=memory_space.FindFreeRange((RangeAddress_t) -1);
			assert(memory_space.IsValidRange(frit));
			scoop->GetEnd()->SetVirtualOffset(frit->GetStart());
		}

		for(auto i=scoop->GetStart()->GetVirtualOffset();
		    i<= scoop->GetEnd()->GetVirtualOffset();
		    i++ )
		{
			if( ! memory_space.IsByteFree(i) )
			{
				// record beginning if not already recorded.
				if(first_valid_address==0)
					first_valid_address=i;

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

			if (m_verbose)
				cout << "Removing an empty scoop (" << scoop->GetName() << ")." << endl;

			m_firp->GetDataScoops().erase(*it);
			it = m_zipr_scoops.erase(it);

			// Delete addresses and then the scoop itself.
			delete scoop->GetStart();
			delete scoop->GetEnd();
			delete scoop;
			scoop=NULL;
		}
		else
		{
			if ((scoop->GetStart()->GetVirtualOffset() != first_valid_address ||
			   scoop->GetEnd()->GetVirtualOffset() != last_valid_address) &&
			   m_verbose)
			{
				cout <<"Shrinking scoop "<<scoop->GetName()
				     <<" to "
				     << std::hex << first_valid_address << "-"
				     << std::hex << last_valid_address << endl;
			}
			else if (m_verbose)
			{
				cout<<"Leaving scoop "<<scoop->GetName()<<" alone. "<<endl;
			}

			cout << "Updating a scoop named " << scoop->GetName() << endl;
			assert(first_valid_address!=0);
			assert(last_valid_address!=0);
			scoop->GetStart()->SetVirtualOffset(first_valid_address);
			scoop->GetEnd()->SetVirtualOffset(last_valid_address);

			/*
			 * Resize the contents.
			 */
			string scoop_contents = scoop->GetContents();
			scoop_contents.resize(scoop->GetEnd()->GetVirtualOffset() -
			                      scoop->GetStart()->GetVirtualOffset() + 1);
			scoop->SetContents(scoop_contents);
			assert(scoop->GetSize() == scoop_contents.size());

			/*
			 * And now update the contents.
			 */
			for(virtual_offset_t i=scoop->GetStart()->GetVirtualOffset();
			    i<= scoop->GetEnd()->GetVirtualOffset();
			    i++)
			{
				scoop->GetContents()[i-scoop->GetStart()->GetVirtualOffset()]=memory_space[i];
			}
			m_firp->GetDataScoops().insert(scoop);
			++it;
		}
	}
	return;
}

void  ZiprImpl_t::FixNoFallthroughs()
{
        auto hlt=archhelper->createNewHaltInstruction(m_firp, NULL);
        auto jmp=archhelper->createNewJumpInstruction(m_firp, NULL);

	hlt->SetFallthrough(jmp);
	jmp->SetTarget(hlt);

	for(const auto insn : m_firp->GetInstructions())
	{
		if(insn==hlt) continue;
		if(insn==jmp) continue;

		if(insn->GetFallthrough()==NULL)
		{
			const auto d=DecodedInstruction_t(insn);
			if(d.isConditionalBranch())
				insn->SetFallthrough(hlt);
		}
		if(insn->GetTarget()==NULL)
		{
			const auto d=DecodedInstruction_t(insn);
			if(d.isBranch() && !d.isReturn() && d.hasOperand(0) && d.getOperand(0).isConstant())
				insn->SetTarget(hlt);
		}
		
	}
		
	
}

void  ZiprImpl_t::FixTwoByteWithPrefix()
{
	for(const auto insn : m_firp->GetInstructions())
	{
		const auto d=DecodedInstruction_t(insn);
		if(!d.isBranch()) continue;	// skip non-branches
		if(d.isReturn()) continue;	// skip returns
		if(d.getOperands().size()!=1) continue;	// skip branches that have no operands or more than one
		if(!d.getOperand(0).isConstant()) continue;	// skip anything where the operand isn't a constant
		if(d.getPrefixCount()==0) continue;	// prevents arm instructions from being xformed.

		
		while (true)
		{
			const auto b=insn->GetDataBits()[0];
			// basic prefix check
			const auto prefixes=set<uint8_t>({0x2e, 0x3e, 0x64, 0x65, 0xf2, 0xf3});
			if(prefixes.find(b)!=end(prefixes))
			{
				// remove prefix 
				insn->SetDataBits(insn->GetDataBits().erase(0,1));	
			}
			// remove rex prefix when unnecessary 
			else if(m_firp->GetArchitectureBitWidth()==64 && (b&0xf0)==0x40 /* has rex prefix */)
			{
				insn->SetDataBits(insn->GetDataBits().erase(0,1));	
			}
			else 
				break;
		}

	}
}


void  ZiprImpl_t::FixMultipleFallthroughs()
{
	auto count=0;
	auto fallthrough_from=map<Instruction_t*, InstructionSet_t>();
	
	for(auto & insn : m_firp->GetInstructions())
	{
		auto ft=insn->GetFallthrough();
		if(ft)
			fallthrough_from[ft].insert(insn);
	};

	for(auto &p : fallthrough_from)
	{
		auto ft=p.first;
		if(p.second.size()>1)
		{
			// skip the first one, because something can fallthrough, just not everything.
			for_each(next(p.second.begin()), p.second.end(), [&](Instruction_t* from)
			{
			
				auto newjmp=archhelper->createNewJumpInstruction(m_firp,NULL);
				count++;
				newjmp->SetTarget(ft);
				from->SetFallthrough(newjmp);

			});
		};
	}

	// after we've inserted all the jumps, assemble them.
	m_firp->AssembleRegistry();

	cout<<"# ATTRIBUTE Zipr::jumps_inserted_for_multiple_fallthroughs="<<dec<<count<<endl;
}


void  ZiprImpl_t::RelayoutEhInfo()
{
	const auto found_eh_ir_it = find_if( 
		m_firp->GetInstructions().begin(), 
		m_firp->GetInstructions().end(),
	 	[](const Instruction_t* i)
			{ 
				return (i->GetEhProgram()!=NULL || i->GetEhCallSite()!=NULL);
			} 
		);

	// do nothing if we didn't find any IR.
	if(found_eh_ir_it==m_firp->GetInstructions().end())
		return;
		
	auto eh = (EhWriter_t *)NULL;
	if(m_firp->GetArchitectureBitWidth()==64)
		eh=new EhWriterImpl_t<8>(*this);
	else if(m_firp->GetArchitectureBitWidth()==32)
		eh=new EhWriterImpl_t<4>(*this);
	else
		assert(0);

	eh->GenerateNewEhInfo();
}


void ZiprImpl_t::ApplyNopToPatch(RangeAddress_t addr)
{
	if (!m_apply_nop)
	{
		if (m_verbose)
			cout << "Skipping chance to apply nop to fallthrough patch." << endl;
		return;
	}
	assert(patcher);
	patcher->ApplyNopToPatch(addr);
}
void ZiprImpl_t::ApplyPatch(RangeAddress_t from_addr, RangeAddress_t to_addr)
{
	assert(patcher);
	patcher->ApplyPatch(from_addr,to_addr);
}
void ZiprImpl_t::PatchJump(RangeAddress_t at_addr, RangeAddress_t to_addr)
{
	assert(patcher);
	patcher->PatchJump(at_addr,to_addr);
}

