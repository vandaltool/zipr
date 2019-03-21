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
#include <irdb-core>
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

#define ALLOF(a) begin(a),end(a)

using namespace IRDB_SDK;
using namespace std;
using namespace zipr;
using namespace EXEIO;
using namespace Zipr_SDK;


inline uintptr_t page_round_up(uintptr_t x)
{
	return  ( (((uintptr_t)(x)) + PAGE_SIZE-1)  & (~(PAGE_SIZE-1)) );
}

inline uintptr_t page_round_down(uintptr_t x)
{
	return  ( (((uintptr_t)(x)) - (PAGE_SIZE-1))  & (~(PAGE_SIZE-1)) );
}


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
	ostream *error = &cout, *warn = nullptr;

	registerOptions();

	/*
	 * Parse once to read the global and zipr options.
	 */
	m_zipr_options.parse(nullptr, nullptr);
	if (m_variant->areRequirementMet()) 
	{
		/* setup the interface to the sql server */
		BaseObj_t::setInterface(m_pqxx_interface.get());

		m_variant_id_p=VariantID_t::factory(*m_variant);
		m_variant_id=m_variant_id_p.get();
		assert(m_variant_id);
		assert(m_variant_id->isRegistered()==true);

		if (*m_verbose)
			cout<<"New Variant, after reading registration, is: "<<*m_variant_id << endl;

		auto this_file=m_variant_id->getMainFile(); 

		// read the db
		m_firp_p=FileIR_t::factory(m_variant_id, this_file);
		m_firp=m_firp_p.get();
		assert(m_firp);

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
	if (*m_verbose)
		warn = &cout;
	m_zipr_options.parse(error, warn);

	if (!m_zipr_options.areRequirementsMet()) {
		m_zipr_options.printUsage(cout);
		m_error = true;
		return;
	}
}

ZiprImpl_t::~ZiprImpl_t()
{
	// delete m_firp; // taken care of by the unique_ptr the factory returns.
	try
	{
		m_pqxx_interface->commit();
	}
	catch (DatabaseError_t pnide)
	{
		cout<<"Unexpected database error: "<<pnide<<endl;
	}
}

void ZiprImpl_t::registerOptions()
{
	auto zipr_namespace = m_zipr_options.getNamespace("zipr");
	auto glbl_namespace = m_zipr_options.getNamespace("global");


	m_output_filename           = zipr_namespace->getStringOption ("output", "Output file name.", "b.out");
	m_callbacks                 = zipr_namespace->getStringOption ("callbacks", "Set the path of the file which contains any required callbacks.");
	m_objcopy                   = zipr_namespace->getStringOption ("objcopy", "Set the path of objcopy to use.", "/usr/bin/objcopy");
	m_dollop_map_filename       = zipr_namespace->getStringOption ("dollop_map_filename", "Specify filename to save dollop map.", "dollop.map");
	m_replop                    = zipr_namespace->getBooleanOption("replop", "Replop all dollops.", false);
	m_verbose                   = glbl_namespace->getBooleanOption("verbose", "Enable verbose output", false);
	m_vverbose                  = glbl_namespace->getBooleanOption("very_verbose", "Be very verry verbose, I'm hunting wabbits.", false);
	m_apply_nop                 = glbl_namespace->getBooleanOption("apply_nop", "This flag needs documentation.", false);
	m_add_sections              = glbl_namespace->getBooleanOption("add-sections", "Add sections to the output binary", true);
	m_bss_opts                  = glbl_namespace->getBooleanOption("bss-opts", "Use BSS optimizationg when genreating output file", true);
	m_variant                   = glbl_namespace->getIntegerOption("variant", "Which IRDB variant to Zipr.");
	m_architecture              = zipr_namespace->getIntegerOption("architecture", "Override default system architecture detection");
	m_seed                      = zipr_namespace->getIntegerOption("seed", "Specify a seed for randomization", getpid());
	m_paddable_minimum_distance = zipr_namespace->getIntegerOption("paddable_minimum_distance", "Specify the minimum size of a gap to be filled.", 5*1024);


	m_variant->setRequired(true);

	memory_space.registerOptions(&m_zipr_options);
}


void ZiprImpl_t::CreateBinaryFile()
{
	exeiop->load("a.ncexe");

	if (*m_architecture == 0)
	{
		if (*m_verbose)
			cout << "Doing architecture autodetection." << endl;
		m_architecture->setValue(IRDB_SDK::FileIR_t::getArchitectureBitWidth());
		if (*m_verbose)
			cout << "Autodetected to " << (int)*m_architecture << endl;
	}

	/*
	 * Take the seed and initialize the random number
	 * generator.
	 */
	std::srand((unsigned)*m_seed);
	if (*m_verbose)
		cout << "Seeded the random number generator with " << *m_seed << "." << endl;


	FixTwoByteWithPrefix();	// have to do this before multi-fallthrough in case it creaates some.
	FixNoFallthroughs();	// have to do this before multi-fallthrough in case it creaates some.
	FixMultipleFallthroughs();


	// create ranges, including extra range that's def. big enough.
	FindFreeRanges(*m_output_filename);

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
	OutputBinaryFile(*m_output_filename);

	// print relevant information
	PrintStats();
}

#if 0
static bool in_same_segment(EXEIO::section* sec1, EXEIO::section* sec2, EXEIO::exeio* exeiop)
{
	auto n = exeiop->segments.size();
	for ( auto i = 0; i < n; ++i ) 
	{
		uintptr_t segstart=exeiop->segments[i]->get_virtual_address();
		uintptr_t segsize=exeiop->segments[i]->get_file_size();

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
#endif


//
// check if there's padding we can use between this section and the next section.
//
RangeAddress_t ZiprImpl_t::extend_section(EXEIO::section *sec, EXEIO::section *next_sec)
{
	assert(0);
#if 0
	RangeAddress_t start=sec->get_address();
	RangeAddress_t end=sec->get_size()+start;
	if( (next_sec->get_flags() & SHF_ALLOC) != 0 && in_same_segment(sec,next_sec,elfiop))
	{
		end=next_sec->get_address()-1;
		cout<<"Extending range of " << sec->get_name() <<" to "<<std::hex<<end<<endl;
		sec->set_size(next_sec->get_address() - sec->get_address() - 1);
	}
	return end;
#endif
}

void ZiprImpl_t::CreateExecutableScoops(const std::map<RangeAddress_t, int> &ordered_sections)
{
	int count=0;
	/*
	 * For each section, ...
	 */
	for(auto it = ordered_sections.begin(); it!=ordered_sections.end();  /* empty */ ) 
	{
		auto sec = exeiop->sections[it->second];
		assert(sec);

		// skip non-exec and non-alloc sections.
		// if( (sec->get_flags() & SHF_ALLOC) ==0 || (sec->get_flags() & SHF_EXECINSTR) ==0 )
		if(!sec->isLoadable() || !sec->isExecutable())
		{
			++it;
			continue;
		}

		// setup start of scoop.
		auto text_start=m_firp->addNewAddress(m_firp->getFile()->getBaseID(), sec->get_address());

		/*
		 * ... walk the subsequent sections and coalesce as many as possible
		 * into a single executable address range.
		 */
		while(1)
		{
			sec = exeiop->sections[it->second];

			// skip non-alloc sections.
			// if( (sec->get_flags() & SHF_ALLOC) ==0)
			if(!sec->isLoadable())
			{
				++it;
				continue;
			}
			// stop if not executable.
			// if( (sec->get_flags() & SHF_EXECINSTR) ==0 )
			if(!sec->isExecutable())
				break;

			// try next 
			++it;
			if(it==ordered_sections.end())
				break;
		}

		// setup end of scoop address
		/*
		auto text_end=new AddressID_t();
		// insert into IR
		m_firp->getAddresses().insert(text_end);
		*/
		auto text_end=m_firp->addNewAddress(m_firp->getFile()->getBaseID(),0);

		// two cases for end-of-scoop 
		if (it==ordered_sections.end())
			// 1 ) this is the last section
			text_end->setVirtualOffset(page_round_up(sec->get_address()+sec->get_size()-1)-1);
		else
			// 2 ) another section gets in the way.
			text_end->setVirtualOffset(sec->get_address()-1);



		// setup a scoop for this section.
		// zero init is OK, after zipring we'll update with the right bytes.
		string text_contents;
		string text_name=string(".zipr_text_")+to_string(count++);
		if(count==1)
			text_name=".text"; // use the name .text first.

		text_contents.resize(text_end->getVirtualOffset() - text_start->getVirtualOffset()+1);
		//
		// DataScoop_t* text_scoop=new DataScoop_t(m_firp->GetMaxBaseID()+1, text_name,  text_start, text_end, nullptr, 5 /*R-X*/, false, text_contents);
		// m_firp->getDataScoops().insert(text_scoop);
		//
		auto text_scoop=m_firp->addNewDataScoop(text_name,  text_start, text_end, nullptr, 5 /*R-X*/, false, text_contents);
	
		cout<<"Adding scoop "<<text_scoop->getName()<<hex<<" at "<<hex<<text_start->getVirtualOffset()<<" - "<<text_end->getVirtualOffset()<<endl;
		m_zipr_scoops.insert(text_scoop);
		memory_space.AddFreeRange(Range_t(text_start->getVirtualOffset(),text_end->getVirtualOffset()), true);
	}
}


RangeAddress_t ZiprImpl_t::PlaceUnplacedScoops(RangeAddress_t max_addr)
{
	max_addr=plugman.PlaceScoopsBegin(max_addr);

	auto scoops_by_perms= map<int,DataScoopSet_t>();
	for(auto scoop : m_firp->getDataScoops())
	{
		// check if placed.
		if(scoop->getStart()->getVirtualOffset()==0)
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
			scoop->getStart()->setVirtualOffset(scoop->getStart()->getVirtualOffset()+max_addr);
			scoop->getEnd()->setVirtualOffset(scoop->getEnd()->getVirtualOffset()+max_addr);

			// update so we actually place things at diff locations.
			max_addr=scoop->getEnd()->getVirtualOffset()+1;

			cout<<"Placing scoop "<<scoop->getName()<<" at "
			    <<hex<<scoop->getStart()->getVirtualOffset()<<"-"
			    <<hex<<scoop->getEnd()->getVirtualOffset()<<endl;
		}
	}


	// assert we unpinned everything 
	for(const auto s : m_firp->getDataScoops())
		assert(s->getStart()->getVirtualOffset()!=0);
	
	
	max_addr=plugman.PlaceScoopsEnd(max_addr);

	return max_addr;
}

void ZiprImpl_t::FindFreeRanges(const std::string &name)
{
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
	auto n = exeiop->sections.size();
	for ( auto i = 0; i < n; ++i ) 
	{ 
		auto sec = exeiop->sections[i];
		assert(sec);
		ordered_sections.insert(std::pair<RangeAddress_t,int>(sec->get_address(), i));
	}

	CreateExecutableScoops(ordered_sections);

	// scan sections for a max-addr.
	for (auto p : ordered_sections )
	{ 
		section* sec = exeiop->sections[p.second];
		assert(sec);

		RangeAddress_t start=sec->get_address();
		RangeAddress_t end=sec->get_size()+start-1;

		if (*m_verbose)
			printf("max_addr is %p, end is %p\n", (void*)max_addr, (void*)end);

		if(start && end>max_addr)
		{
			if (*m_verbose)
				printf("new max_addr is %p\n", (void*)max_addr);
			max_addr=end;
		}

		// if( (sec->get_flags() & SHF_ALLOC) ==0 )
		if(!sec->isLoadable())	
			continue;
	}

	/*
	 *  First things first: Let's put empty scoops
	 *  in all the gaps.
	 */

	if (*m_verbose)
		cout << "Filling gaps that are larger than " << std::dec
		     << *m_paddable_minimum_distance << " bytes." << endl;

	/*
	 * Only put pinned data scoops into the list of
	 * scoops to consider for adding gap filling.
	 */
	copy_if(ALLOF(m_firp->getDataScoops()),
	        inserter(sorted_scoop_set, sorted_scoop_set.begin()),
	        [](DataScoop_t* ds)
	        {
	        	return ds->getStart()->getVirtualOffset() != 0;
	        }
	);
	for( auto it=sorted_scoop_set.begin(); it!=sorted_scoop_set.end(); ++it )
	{
		auto this_scoop=*it;
		DataScoop_t* next_scoop=nullptr;
		RangeAddress_t this_end = this_scoop->getEnd()->getVirtualOffset(),
		               next_start = 0;

		assert(this_scoop->getStart()->getVirtualOffset()!=0);

		if (*m_verbose)
			cout << "There's a scoop between " << std::hex
			     << this_scoop->getStart()->getVirtualOffset()
			     << " and " << std::hex << this_scoop->getEnd()->getVirtualOffset()
			     << " with permissions " << std::hex << this_scoop->getRawPerms()
			     << endl;

		/*
		 * Never pad after the last scoop.
		 */
		if (std::next(it,1) != sorted_scoop_set.end())
		{
			next_scoop = *std::next(it,1);
			next_start = next_scoop->getStart()->getVirtualOffset();
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
				if (*m_verbose)
					cout << "Not considering this section because it "
					     << "does not end before the next one starts." << endl;
				continue;
			}

			if (*m_verbose)
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

				if (*m_verbose)
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

				if (*m_verbose)
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
			if ((new_padding_scoop_size>(unsigned int)*m_paddable_minimum_distance) &&
			    (!this_scoop->isWriteable() || !next_scoop->isWriteable())
				 )
			{
				string new_padding_scoop_contents, new_padding_scoop_name;
				int new_padding_scoop_perms = 0x5 /* r-x */;

				new_padding_scoop_name = "zipr_scoop_"+
				                         to_string(new_padding_scoop_start);

				/*
				DataScoop_t *new_padding_scoop = nullptr;
				AddressID_t *new_padding_scoop_start_addr = nullptr,
				            *new_padding_scoop_end_addr = nullptr;
				new_padding_scoop_start_addr = new AddressID_t();
				new_padding_scoop_start_addr->setVirtualOffset(new_padding_scoop_start);
				m_firp->getAddresses().insert(new_padding_scoop_start_addr);
				*/
				auto new_padding_scoop_start_addr=m_firp->addNewAddress(m_firp->getFile()->getBaseID(), new_padding_scoop_start);
				/*
				new_padding_scoop_end_addr = new AddressID_t();
				new_padding_scoop_end_addr->setVirtualOffset(new_padding_scoop_end);
				m_firp->getAddresses().insert(new_padding_scoop_end_addr);
				*/
				auto new_padding_scoop_end_addr  =m_firp->addNewAddress(m_firp->getFile()->getBaseID(), new_padding_scoop_end);

				cout << "Gap filling with a scoop between 0x"
				     << std::hex << new_padding_scoop_start << " and 0x"
				     << std::hex << new_padding_scoop_end
				     << endl;

				auto new_padding_scoop = m_firp->addNewDataScoop(
				                                    new_padding_scoop_name,
				                                    new_padding_scoop_start_addr,
				                                    new_padding_scoop_end_addr,
				                                    nullptr,
				                                    new_padding_scoop_perms,
				                                    false,
				                                    new_padding_scoop_contents);
				new_padding_scoop_contents.resize(new_padding_scoop->getSize());
				new_padding_scoop->setContents(new_padding_scoop_contents);

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
			return a->getEnd()->getVirtualOffset() <
			       b->getEnd()->getVirtualOffset();
		}
	);
	assert(max_addr>=(*max_addr_zipr_scoops_result)->getEnd()->getVirtualOffset());

	max_addr=PlaceUnplacedScoops(max_addr);

	// now that we've looked at the sections, add a (mysterious) extra section in case we need to overflow 
	// the sections existing in the ELF.
	RangeAddress_t new_free_page=page_round_up(max_addr);


	/*
	 * TODO
	 *
	 * Make a scoop out of this. Insert it into m_zipr_scoops
	 * and m_firp->getDataScoops()
	 */
	auto textra_start = RangeAddress_t(new_free_page);
	auto textra_end   = (RangeAddress_t)-1;
	auto textra_name  = string("textra");

	/*
	DataScoop_t *textra_scoop = nullptr;
	AddressID_t *textra_start_addr = new AddressID_t(),
	            *textra_end_addr = new AddressID_t();
		    */
	// textra_start_addr->setVirtualOffset(textra_start);
	// textra_end_addr->setVirtualOffset(textra_end);
	// m_firp->getAddresses().insert(textra_start_addr);
	// m_firp->getAddresses().insert(textra_end_addr);
	auto textra_start_addr=m_firp->addNewAddress(m_firp->getFile()->getBaseID(), textra_start);
	auto textra_end_addr  =m_firp->addNewAddress(m_firp->getFile()->getBaseID(), textra_end);

	cout << "New free space: 0x" << std::hex << textra_start
	     << "-0x"
	     << std::hex << textra_end
	     << endl;

	auto textra_contents=string();
	auto textra_scoop = m_firp->addNewDataScoop(
	    textra_name,
	    textra_start_addr,
	    textra_end_addr,
	    nullptr,
	     /* r-x */5,
	    false,
	    textra_contents);

	/*
	 * Normally we would have to resize the underlying contents here.
	 * Unfortunately that's not a smart idea since it will be really big.
	 * Instead, we are going to do a batch resizing below.
	textra_contents.resize(textra_end - textra_start + 1);
	textra_scoop->setContents(textra_contents);
	 */

	m_zipr_scoops.insert(textra_scoop);

	memory_space.AddFreeRange(Range_t(new_free_page,(RangeAddress_t)-1), true);
	if (*m_verbose)
		printf("Adding (mysterious) free range 0x%p to EOF\n", (void*)new_free_page);
	start_of_new_space=new_free_page;

	for(auto scoop : m_firp->getDataScoops())
	{
		if(scoop->isExecuteable()) continue;
		// put scoops in memory to make sure they are busy,
		// just in case they overlap with free ranges.
		// this came up on Aarch64 because data is in the .text segment.
		cout<<"Pre-allocating scoop "<<scoop->getName() << "=("
		    << scoop->getStart()->getVirtualOffset() << "-" 
		    << scoop->getEnd()  ->getVirtualOffset() << ")"<<endl;
		memory_space.PlopBytes(scoop->getStart()->getVirtualOffset(), 
		                       scoop->getContents().c_str(),
				       scoop->getContents().size()
				      );
	}
}


Instruction_t *ZiprImpl_t::FindPatchTargetAtAddr(RangeAddress_t addr)
{
        std::map<RangeAddress_t,UnresolvedUnpinnedPatch_t>::iterator it=m_PatchAtAddrs.find(addr);
        if(it!=m_PatchAtAddrs.end())
                return it->second.first.getInstrution();
        return nullptr;
}


void ZiprImpl_t::WriteDollops()
{
	for (auto & dollop_to_write : m_dollop_mgr.getDollops()  ) 
	{
		assert(dollop_to_write != nullptr);
		// skip unplaced dollops as they aren't necessary
		if (!dollop_to_write->isPlaced())
			continue;
	
		// write each entry in the dollop
		for (auto &entry_to_write : *dollop_to_write) 
		{
			assert(entry_to_write != nullptr);
			// plop it.
			const auto de_end_loc = _PlopDollopEntry(entry_to_write);

			// sanity check that we didn't go passed the worst case size we calculate for this entry
			const auto de_start_loc = entry_to_write->getPlace();
			const auto should_end_at = de_start_loc + DetermineDollopEntrySize(entry_to_write, false);
			assert(de_end_loc == should_end_at);
			/*
			 * Build up a list of those dollop entries that we have
			 * just written that have a target. See comment above 
			 * ReplopDollopEntriesWithTargets() for the reason that
			 * we have to do this.
			 */
			const auto will_replop=entry_to_write->getTargetDollop()!=nullptr;
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
		Instruction_t *src_insn = nullptr;
		RangeAddress_t src_insn_addr;

		src_insn = entry_to_write->getInstruction();

		src_insn_addr = final_insn_locations[src_insn];
		_PlopDollopEntry(entry_to_write, src_insn_addr);
	}
}

void ZiprImpl_t::PlaceDollops()
{

	auto count_pins=0u;

	/*
	 * Build up initial placement q with destinations of
	 * pins.
	 */
	for (auto p : patch_list)
	{
		const auto uu = p.first;
		const auto patch = p.second;
		auto target_insn = uu.getInstrution();
		auto target_dollop = m_dollop_mgr.getContainingDollop(target_insn);
		assert(target_dollop);

		placement_queue.insert({target_dollop,patch.getAddress()});
		if (*m_verbose) 
		{
			cout << "Original: " << hex << target_insn-> getAddress()-> getVirtualOffset() << " "
			     << "vs. Patch: " << patch.getAddress() << endl;
		}
		count_pins++;
	}

	assert(getenv("SELF_VALIDATE")==nullptr || count_pins > 3 ) ;
	assert(getenv("SELF_VALIDATE")==nullptr || placement_queue.size() > 15 ) ;

	cout<<"# ATTRIBUTE Zipr::pins_detected="<<dec<<count_pins<<endl;
	cout<<"# ATTRIBUTE Zipr::placement_queue_size="<<dec<<placement_queue.size()<<endl;

	/* 
         * used to check if a reference dollop needs to be added to the placement queue
        */
        const auto handle_reloc=[&](const Relocation_t* reloc)
        {
        	auto wrt_insn=dynamic_cast<Instruction_t*>(reloc->getWRT());
                if(wrt_insn)
                {
                	auto containing=m_dollop_mgr.getContainingDollop(wrt_insn);
                        assert(containing!=nullptr);
                        if(!containing->isPlaced())
                        {
                        	placement_queue.insert({containing, wrt_insn->getAddress()->getVirtualOffset()});
                        }
                }
        };

	// Make sure each instruction referenced in a relocation (regardless
	// of if that relocation is on an instruction or a scoop) gets placed.
	for(auto &reloc : m_firp->getRelocations())
        	handle_reloc(reloc);	

	while (!placement_queue.empty())
	{
		auto placement=Range_t();
		auto placer = DLFunctionHandle_t(nullptr);
		auto placed = false;
		auto cur_addr = RangeAddress_t(0);
		auto has_fallthrough = false;
		auto fallthrough = (Zipr_SDK::Dollop_t*)(nullptr);
		auto continue_placing = false;
		auto initial_placement_abuts_pin = false;
		auto initial_placement_abuts_fallthrough = false;
		auto fits_entirely = false;
		auto fallthrough_dollop_place = RangeAddress_t(0);
		auto fallthrough_has_preplacement = false;
		auto pq_entry = *(placement_queue.begin());
		placement_queue.erase(placement_queue.begin());

		auto to_place = pq_entry.first;
		auto from_address = pq_entry.second;

		if (*m_vverbose)
		{
			cout << "Placing dollop with original starting address: " << hex
			     << to_place->front()->getInstruction()->getAddress()->getVirtualOffset() << endl;
		}

		if (to_place->isPlaced())
			continue;

		to_place->reCalculateSize();

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

			if (*m_verbose)
				cout << placer->toString() << " placed this dollop between " 
				     << hex << placement.getStart() << " and " << placement.getEnd()
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
			const auto has_fallthrough = to_place->getFallthroughDollop() != nullptr;
			const auto ibta=has_fallthrough ? to_place->getFallthroughDollop()-> front()-> getInstruction()-> getIndirectBranchTargetAddress() : 0; 
			initial_placement_abuts_pin = has_fallthrough && 
		   	                              ibta && 
		   	                              ibta -> getVirtualOffset()!=0   && 
		   	                              ibta-> getVirtualOffset() == (placement.getStart() + to_place->getSize() - sizer->TRAMPOLINE_SIZE);
			/*
			 * If this dollop has a fallthrough, find out where that 
			 * fallthrough is (or is going to be) placed. That way
			 * we can determine if the current dollop is (or is going to be)
			 * adjacent to the place of the fallthrough. That means
			 * that we can keep from placing a jump to the dollo
			 * and instead just fallthrough.
			 */
			if (to_place->getFallthroughDollop() && allowed_fallthrough) 
			{
				/*
				 * Find out where the fallthrough dollop is placed.
				 */
				if (to_place->getFallthroughDollop()->isPlaced())
				{
					fallthrough_dollop_place = to_place->getFallthroughDollop()->getPlace();
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
					DLFunctionHandle_t fallthrough_placer = nullptr;
					/*
					 * Prospectively get the place for this dollop. That way 
					 * we can determine whether or not we need to use a fallthrough!
					 */
					if (plugman.DoesPluginAddress(to_place->getFallthroughDollop(),
		                                    from_address,
		                                    fallthrough_placement,
		                                    fallthrough_allowed_coalescing,
		                                    fallthrough_allowed_fallthrough,
		                                    fallthrough_placer))
					{
						fallthrough_dollop_place = fallthrough_placement.getStart();
						fallthrough_has_preplacement = true;
					}
				}
			}
			initial_placement_abuts_fallthrough = to_place->getFallthroughDollop() &&
			                                           fallthrough_has_preplacement &&
			                                           fallthrough_dollop_place == (placement.getStart() + to_place->getSize() - sizer->TRAMPOLINE_SIZE);


			auto fits_entirely = (to_place->getSize() <= (placement.getEnd()-placement.getStart()));

			if (*m_verbose)
			{
				cout << "initial_placement_abuts_pin        : "
				     <<initial_placement_abuts_pin << endl
				     << "initial_placement_abuts_fallthrough: " 
				     << initial_placement_abuts_fallthrough << endl
				     << "fits_entirely                      : " 
				     << fits_entirely << endl;
			}

			if ( ((placement.getEnd()-placement.getStart()) < minimum_valid_req_size) &&
			    !(initial_placement_abuts_pin || initial_placement_abuts_fallthrough || fits_entirely)
			   )
			{
				if (*m_verbose)
					cout << "Bad getNearbyFreeRange() result." << endl;
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
			//placement = memory_space.getFreeRange(to_place->getSize());
			placement = sizer->DoPlacement(minimum_valid_req_size);

			/*
			 * Reset allowed_coalescing because DoesPluginAddress
			 * may have reset it and we may have rejected the results
			 * of that addressing.
			 */
			allowed_coalescing = true;
		}

		cur_addr = placement.getStart();
		//cout << "Adjusting cur_addr to " << std::hex << cur_addr << " at A." << endl;
		has_fallthrough = (to_place->getFallthroughDollop() != nullptr);

		if (*m_vverbose)
		{
			cout << "Dollop size=" << dec << to_place->getSize() << ".  Placing in hole size="
			     << (placement.getEnd() - placement.getStart()) << " hole at " << hex << cur_addr << endl;
			cout << "Dollop " << ((has_fallthrough) ? "has " : "does not have ")
		  	     << "a fallthrough" << endl;
		}

		const auto has_pinned_ibta=
				to_place->front()->getInstruction()->getIndirectBranchTargetAddress() && 
				to_place->front()->getInstruction()->getIndirectBranchTargetAddress()->getVirtualOffset()!=0 ;
		const auto pinned_ibta_addr = has_pinned_ibta ?  
				to_place-> front()->getInstruction()-> getIndirectBranchTargetAddress()-> getVirtualOffset() : 
				VirtualOffset_t(0);
		if (has_pinned_ibta && cur_addr == pinned_ibta_addr)
		{
			unsigned int space_to_clear = sizer->SHORT_PIN_SIZE;
			/*
			 * We have placed this dollop at the location where
			 * its first instruction was pinned in memory.
			 */
			if (*m_verbose)
				cout << "Placed atop its own pin!" << endl;

			if (memory_space[cur_addr] == (char)0xe9)
				space_to_clear = sizer->LONG_PIN_SIZE;

			for (unsigned int j = cur_addr; j<(cur_addr+space_to_clear); j++)
			{
				memory_space.mergeFreeRange(j);
			}

			/*
			 * Remove the replaced pin from the patch list.
			 */
			UnresolvedUnpinned_t uu(to_place->front()->getInstruction());
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
			  to_place->getFallbackDollop() && 
			  // and it's already placed.
		    	  to_place->getFallbackDollop()->isPlaced() && 
			  // and the place is adjacent to us
			  ( to_place->getFallbackDollop()->getPlace() + to_place->getFallbackDollop()->getSize() - sizer->TRAMPOLINE_SIZE) == placement.getStart()
			)
		{
			/*
			 * We have placed this dollop at the location where
			 * the fallthrough jump to this dollop was placed.
			 */
			if (*m_verbose)
				cout << "Placed atop its own fallthrough!" << endl;

			/*
			 * Note: We do NOT have to clear any pre-reserved
			 * memory here now that we have pre-checks on
			 * whether the dollop is placed. Because of that
			 * precheck, this range will never be unnecessarily 
			 * reserved for a jump.
			 */
		}

		assert(to_place->getSize() != 0);

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
		
			to_place->reCalculateSize();

			/*
			 * Calculate before we place this dollop.
			 */
			wcds = sizer->DetermineDollopSizeInclFallthrough(to_place);

			to_place->Place(cur_addr);

			// cout << "to_place->getSize(): " << to_place->getSize() << endl;

			fits_entirely = (to_place->getSize() <= (placement.getEnd()-cur_addr));
			all_fallthroughs_fit = (wcds <= (placement.getEnd()-cur_addr));

			auto dit = to_place->begin();
			auto dit_end = to_place->end();
			for ( /* empty */; dit != dit_end; dit++)
			{
				auto dollop_entry = *dit;	
								
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
					(placement.getEnd()>= (cur_addr+DetermineDollopEntrySize(dollop_entry, true)));
				const auto is_last_insn           = next(dit)==dit_end; /* last */ 
				const auto has_fallthrough_dollop = to_place->getFallthroughDollop()!=nullptr ;
				const auto fits_with_fallthrough  = placement.getEnd()>=(cur_addr+ DetermineDollopEntrySize(dollop_entry, has_fallthrough_dollop));
				const auto last_de_fits           = is_last_insn && fits_with_fallthrough;
				const auto could_fit_here         = 
					de_and_fallthrough_fit              || 
					fits_entirely                       || 
					last_de_fits                        || 
					initial_placement_abuts_pin         || 
					initial_placement_abuts_fallthrough ;
				const auto tramp_fits             =
				        (placement.getEnd() - (cur_addr + DetermineDollopEntrySize( dollop_entry, false))) < sizer->TRAMPOLINE_SIZE;
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

				if (*m_vverbose)
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
					if (*m_vverbose) 
					{
						auto d=DecodedInstruction_t::factory(dollop_entry->getInstruction());
						cout << "Placing " << hex << dollop_entry->getInstruction()->getBaseID() 
						     << ":" << d->getDisassembly() << " at "
						     << cur_addr << "-" << next_cur_addr << endl;
					}
					cur_addr=next_cur_addr;
					if (dollop_entry->getTargetDollop())
					{
						if (*m_vverbose)
							cout << "Adding " << std::hex << dollop_entry->getTargetDollop()
							     << " to placement queue." << endl;
						placement_queue.insert({dollop_entry->getTargetDollop(), cur_addr});
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
				auto split_dollop = to_place->split((*dit)->getInstruction());
				m_dollop_mgr.AddDollops(split_dollop);

				to_place->setTruncated(true);
				if (am_coalescing)
					m_stats->truncated_dollops_during_coalesce++;

				if (*m_vverbose)
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

			fallthrough = to_place->getFallthroughDollop();
			if ( fallthrough  != nullptr && !to_place->wasCoalesced() )
			{
				size_t fallthroughs_wcds, fallthrough_wcis, remaining_size;

				/*
				 * We do not care about the fallthrough dollop if its 
				 * first instruction is pinned AND the last entry of this
				 * dollop abuts that pin.
				 */
				const auto has_ibta         = fallthrough-> front()-> getInstruction()-> getIndirectBranchTargetAddress();
				const auto pinned_ibta_addr = has_ibta ? fallthrough-> front()-> getInstruction()-> getIndirectBranchTargetAddress()-> getVirtualOffset() : VirtualOffset_t(0);
				const auto is_pinned_ibta_addr = has_ibta && pinned_ibta_addr!=0;
				const auto is_pinned_here = (cur_addr == pinned_ibta_addr ) ;
				if ( has_ibta && is_pinned_ibta_addr && is_pinned_here )
				{
					if (*m_verbose)
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
				if (!am_coalescing && to_place->getFallthroughDollop() && fallthrough_has_preplacement && fallthrough_dollop_place == cur_addr)
				{
					if (*m_verbose)
						cout << "Dollop had a fallthrough dollop and "
						     << "was placed abutting the fallthrough "
						     << "dollop's first instruction. "
						     << endl;
					/*
					 * We are not coalescing, but we want to make sure that
					 * the fallthrough does get placed if zipr hasn't already
					 * done so. See above for a contrast.
					 */
					if (!to_place->getFallthroughDollop()->isPlaced())
					{
						placement_queue.insert({to_place->getFallthroughDollop(), cur_addr});
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
				remaining_size = placement.getEnd() - cur_addr;

				/*
				 * We compare remaining_size to min(fallthroughs_wdcs,
				 * fallthrough_wcis) since the entirety of the dollop
				 * and its fallthroughs could (its unlikely) be 
				 * smaller than the first instruction fallthrough 
				 * in the fallthrough dollop and the trampoline size.
				 */
				if (*m_vverbose)
					cout << "Determining whether to coalesce: "
					     << "Remaining: " << std::dec << remaining_size
					     << " vs Needed: " << std::dec 
					     << std::min(fallthrough_wcis,fallthroughs_wcds) << endl;

				if (remaining_size < std::min(fallthrough_wcis,fallthroughs_wcds) || 
				    fallthrough->isPlaced()                                       || 
				    !allowed_coalescing
				   )
				{

					string patch_jump_string;
					auto patch = archhelper->createNewJumpInstruction(m_firp, nullptr);
					auto patch_de = new DollopEntry_t(patch, to_place);

					patch_de->setTargetDollop(fallthrough);
					patch_de->Place(cur_addr);
					cur_addr+=DetermineDollopEntrySize(patch_de, false);
					//cout << "Adjusting cur_addr to " << std::hex << cur_addr << " at C." << endl;

					to_place->push_back(patch_de);
					to_place->setFallthroughPatched(true);

					if (*m_vverbose)
						cout << "Not coalescing"
						     << string((fallthrough->isPlaced()) ?  " because fallthrough is placed" : "")
						     << string((!allowed_coalescing) ?  " because I am not allowed" : "")
						     << "; Added jump (via " << std::hex << patch_de
						     << " at " << std::hex << patch_de->getPlace() << ") "
						     << "to fallthrough dollop (" << std::hex 
						     << fallthrough << ")." << endl;

					placement_queue.insert({fallthrough, cur_addr});
					/*
					 * Since we inserted a new instruction, we should
					 * check to see whether a plugin wants to plop it.
					 */
					AskPluginsAboutPlopping(patch_de->getInstruction());

					m_stats->total_did_not_coalesce++;

					/*
					 * Quit the do-while-true loop that is placing
					 * as many dollops in-a-row as possible.
					 */
					break;
				}
				else
				{
					if (*m_vverbose)
						cout << "Coalescing fallthrough dollop." << endl;
					to_place->setCoalesced(true);
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
		if (*m_vverbose)
			cout << "Reserving " << std::hex << placement.getStart()
			     << ", " << std::hex << cur_addr << "." << endl;
		memory_space.splitFreeRange(Range_t(placement.getStart(), cur_addr));
	}
}

void ZiprImpl_t::RecalculateDollopSizes()
{
	for (auto &dollop : m_dollop_mgr.getDollops())
		dollop->reCalculateSize();
}

void ZiprImpl_t::CreateDollops()
{
	if (*m_verbose)
		cout<< "Attempting to create "
		    << patch_list.size()
				<< " dollops for the pins."
				<< endl;
	for (auto patch : patch_list )
		m_dollop_mgr.AddNewDollops(patch.first.getInstrution());

	if (*m_verbose)
		cout << "Done creating dollops for the pins! Updating all Targets" << endl;

	m_dollop_mgr.UpdateAllTargets();

	if (*m_verbose)
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

size_t ZiprImpl_t::DetermineDollopEntrySize(Zipr_SDK::DollopEntry_t *entry, bool account_for_fallthrough)
{
	std::map<Instruction_t*,unique_ptr<list<DLFunctionHandle_t>>>::const_iterator plop_it;
	size_t opening_size = 0, closing_size = 0;
	size_t wcis = DetermineInsnSize(entry->getInstruction(), account_for_fallthrough);

	plop_it = plopping_plugins.find(entry->getInstruction());
	if (plop_it != plopping_plugins.end())
	{
		for (auto handle : *(plop_it->second))
		{
			ZiprPluginInterface_t *zpi = dynamic_cast<ZiprPluginInterface_t*>(handle);
			opening_size += zpi->getDollopEntryOpeningSize(entry);
			closing_size += zpi->getDollopEntryClosingSize(entry);
		}
	}

	if (*m_verbose)
	{
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
			worst_case_size =std::max(zpi->getInsnSize(insn, account_for_fallthrough), worst_case_size);
		}
	}
	else
	{
		worst_case_size = default_worst_case_size;
	}

	if (worst_case_size == 0)
	{
		if (*m_verbose)
			cout << "Asked plugins about WCIS, but none responded." << endl;
		worst_case_size = default_worst_case_size;
	}

	if (*m_vverbose)
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
		if (*m_verbose)
			for (auto pp : *found_plopping_plugins)
			{
				ZiprPluginInterface_t *zipr_plopping_plugin =
					dynamic_cast<ZiprPluginInterface_t*>(pp);
				cout << zipr_plopping_plugin->toString()
				     << " will plop "<<dec<<insn->getBaseID() << ":"
				     << insn->getDisassembly() << endl;
			}

		plopping_plugins[insn] = std::move(found_plopping_plugins);
		return true;
	}
	return false;
}

void ZiprImpl_t::AskPluginsAboutPlopping()
{

	for(auto &insn : m_firp->getInstructions())
		AskPluginsAboutPlopping(insn);
}

void ZiprImpl_t::UpdatePins()
{
	while(!patch_list.empty())
	{
		UnresolvedUnpinned_t uu=(*patch_list.begin()).first;
		Patch_t p=(*patch_list.begin()).second;
		Zipr_SDK::Dollop_t *target_dollop = nullptr;
		Zipr_SDK::DollopEntry_t *target_dollop_entry = nullptr;
		Instruction_t *target_dollop_entry_instruction = nullptr;
		RangeAddress_t patch_addr, target_addr;
		target_dollop = m_dollop_mgr.getContainingDollop(uu.getInstrution());
		assert(target_dollop != nullptr);
		DLFunctionHandle_t patcher = nullptr;

		target_dollop_entry = target_dollop->front();
		assert(target_dollop_entry != nullptr);

		target_dollop_entry_instruction  = target_dollop_entry->getInstruction();
		assert(target_dollop_entry_instruction != nullptr &&
		       target_dollop_entry_instruction == uu.getInstrution());


		patch_addr = p.getAddress();
		target_addr = target_dollop_entry->getPlace();

		if (final_insn_locations.end() != final_insn_locations.find(target_dollop_entry->getInstruction()))
			target_addr = final_insn_locations[target_dollop_entry->getInstruction()];

		if (plugman.DoesPluginRetargetPin(patch_addr, target_dollop, target_addr, patcher))
		{
			if (*m_verbose)
			{
				cout << "Patching retargeted pin at " << hex<<patch_addr << " to "
				     << patcher->toString() << "-assigned address: " << target_addr << endl;
			}
		}
		else
		{
			/*
			 * Even though DoesPluginRetargetPin() returned something other than
			 * Must, it could have still changed target_address. So, we have to
			 * reset it here, just in case.
			 */
			target_addr = target_dollop_entry->getPlace();

			if (final_insn_locations.end() != final_insn_locations.find(target_dollop_entry->getInstruction()))
				target_addr = final_insn_locations[target_dollop_entry->getInstruction()];

			if (*m_verbose)
			{
				const auto d=DecodedInstruction_t::factory(target_dollop_entry_instruction);
				cout << "Patching pin at " << hex << patch_addr << " to "
				     << target_addr << ": " << d->getDisassembly() << endl;
			}
			assert(target_dollop_entry_instruction != nullptr &&
			       target_dollop_entry_instruction == uu.getInstrution());

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
		if (*m_verbose)
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
		if (*m_verbose)
			printf("Found a patch for %p -> %p\n", (void*)from_addr, (void*)to_addr); 
		// Apply Patch
		ApplyPatch(from_addr, to_addr);
	}
}

RangeAddress_t ZiprImpl_t::_PlopDollopEntry(Zipr_SDK::DollopEntry_t *entry, RangeAddress_t override_address)
{
	const auto insn = entry->getInstruction();
	const auto insn_wcis = DetermineInsnSize(insn, false);
	RangeAddress_t updated_addr = 0;
	RangeAddress_t target_address = 0;
	auto placed_insn = false;
	const auto target_dollop=entry->getTargetDollop();
	if (target_dollop && target_dollop->front())
	{
		const auto entry_target_head_insn=entry-> getTargetDollop()-> front()-> getInstruction();
		const auto target_address_iter = final_insn_locations.find(entry_target_head_insn);
		if (target_address_iter != final_insn_locations.end())
		{
			target_address = target_address_iter->second;
			if (*m_verbose)
				cout << "Found an updated target address location: "
				     << std::hex << target_address << endl;
		}
	}	


	auto placed_address = override_address == 0 ? entry->getPlace() : override_address;
	const auto plop_it = plopping_plugins.find(insn);
	if (plop_it != plopping_plugins.end())
	{
		for (auto pp : *(plop_it->second))
		{
			auto pp_placed_insn = false;
			const auto handle = pp;
			const auto zpi = dynamic_cast<ZiprPluginInterface_t*>(handle);
			const auto plugin_ret=zpi->plopDollopEntry(entry, placed_address, target_address, insn_wcis, pp_placed_insn);
			updated_addr = std::max(plugin_ret, updated_addr);
			if (*m_verbose)
			{
				cout << zpi->toString() << " placed entry " 
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
		cout<<"Warning, Moving instruction "<<hex<<insn->getBaseID()<<":"<<insn->getComment()
		    <<" from "<<hex<<old_loc<<" to "<<placed_address<<endl;
		cout<<"Happened for "<<dec<<count++<<" out of "<<m_firp->getInstructions().size()<<" instructions"<<endl;
	}

	final_insn_locations[insn] = placed_address;
	return updated_addr;
}

RangeAddress_t ZiprImpl_t::PlopDollopEntry(
	Zipr_SDK::DollopEntry_t *entry,
	RangeAddress_t override_place,
	RangeAddress_t override_target)
{
	Instruction_t *insn = entry->getInstruction();
	RangeAddress_t ret = entry->getPlace(), addr = entry->getPlace();

	assert(insn);

	if (override_place != 0)
		addr = ret = override_place;

	const auto d=DecodedInstruction_t::factory(insn);

	string raw_data = insn->getDataBits();
	string orig_data = insn->getDataBits();


	if(entry->getTargetDollop() && entry->getInstruction()->getCallback()=="")
	{
		RangeAddress_t target_address = 0;
		auto target_insn = entry->getTargetDollop()->front()->getInstruction();

		if (override_target == 0)
		{	
			if (final_insn_locations.end() != final_insn_locations.find(target_insn))
				target_address = final_insn_locations[target_insn];
		}
		else
		{
			if (*m_verbose)
				cout << "Plopping with overriden target: Was: " 
				     << hex << target_address << " Is: " << override_target << endl;
			target_address = override_target;
		}

		if (*m_verbose)
		{
			const auto print_target=((target_address != 0) ? target_address : entry->getTargetDollop()->getPlace());
			cout << "Plopping '"<<entry->getInstruction()->getDisassembly() <<"' at " << hex << addr
			     << " with target " << print_target << endl;
		}
		ret=PlopDollopEntryWithTarget(entry, addr, target_address);
	}
	else if(entry->getInstruction()->getCallback()!="")
	{
		if (*m_verbose)
			cout << "Plopping at " << hex << addr << " with callback to " 
			     << entry->getInstruction()->getCallback() << endl;

		ret=PlopDollopEntryWithCallback(entry, addr);
	}
	else
	{
		if (*m_verbose)
			cout << "Plopping non-ctl "<<insn->getDisassembly()<<" at " << hex << addr << endl;
		memory_space.PlopBytes(addr, insn->getDataBits().c_str(), insn->getDataBits().length());
		ret+=insn->getDataBits().length();
	}

	/* Reset the data bits for the instruction back to th
	 * need to re-plop this instruction later.  we need t
	 * so we can replop appropriately. 
	 */
	insn->setDataBits(orig_data);
	return ret;
}

RangeAddress_t ZiprImpl_t::PlopDollopEntryWithTarget(
	Zipr_SDK::DollopEntry_t *entry,
	RangeAddress_t override_place,
	RangeAddress_t override_target)
{
	return sizer->PlopDollopEntryWithTarget(entry,override_place,override_target);
}

RangeAddress_t ZiprImpl_t::PlopDollopEntryWithCallback(
	Zipr_SDK::DollopEntry_t *entry,
	RangeAddress_t override_place)
{
	auto at = entry->getPlace();
	auto originalAt = entry->getPlace();

	if (override_place != 0)
		at = originalAt = override_place;

	// emit call <callback>
	{
	char bytes[]={(char)0xe8,(char)0,(char)0,(char)0,(char)0}; // call rel32
	memory_space.PlopBytes(at, bytes, sizeof(bytes));
	unpatched_callbacks.insert({entry,at});
	at+=sizeof(bytes);
	}

	// pop bogus ret addr
	if(m_firp->getArchitectureBitWidth()==64)
	{
		char bytes[]={(char)0x48,(char)0x8d,(char)0x64,(char)0x24,(char)(m_firp->getArchitectureBitWidth()/0x08)}; // lea rsp, [rsp+8]
		memory_space.PlopBytes(at, bytes, sizeof(bytes));
		at+=sizeof(bytes);
	}
	else if(m_firp->getArchitectureBitWidth()==32)
	{
		char bytes[]={(char)0x8d,(char)0x64,(char)0x24,(char)(m_firp->getArchitectureBitWidth()/0x08)}; // lea esp, [esp+4]
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
	const auto find_it=find_if(ALLOF(m_firp->getDataScoops()),
		[&](const DataScoop_t* scoop)
		{
			return scoop->getStart()->getVirtualOffset() <= addr &&
			       addr < scoop->getEnd()->getVirtualOffset()    ;
		});
	return find_it==m_firp->getDataScoops().end() ?  nullptr : *find_it;
}


void ZiprImpl_t::OutputBinaryFile(const string &name)
{
	// now that the textra scoop has been crated and setup, we have the info we need to 
	// re-generate the eh information.
	RelayoutEhInfo(); 

	const auto file_type = m_firp->getArchitecture()->getFileType();
	const auto is_elf    = file_type == IRDB_SDK::adftELFEXE || file_type ==  IRDB_SDK::adftELFSO;
	const auto is_pe     = file_type == IRDB_SDK::adftPE;
	const auto bit_width = m_firp->getArchitectureBitWidth();
	const auto output_filename="c.out";

	if(is_elf)
	{
		// create the output file in a totally different way using elfwriter. later we may 
		// use this instead of the old way.

		auto elfiop=reinterpret_cast<ELFIO::elfio*>(exeiop->get_elfio());
		auto ew=unique_ptr<ElfWriter>();
		ew.reset(
			bit_width == 64 ? (ElfWriter*)new ElfWriter64(m_firp, *m_add_sections, *m_bss_opts) :
			bit_width == 32 ? (ElfWriter*)new ElfWriter32(m_firp, *m_add_sections, *m_bss_opts) :
			throw invalid_argument("Unknown machine width")
			);
		ew->Write(elfiop,m_firp, output_filename, "a.ncexe");
		ew.reset(nullptr); // explicitly free ew as we're done with it
	}
	else if (is_pe)
	{
		assert(m_firp->getArchitectureBitWidth()==64);
		auto pe_write=new PeWriter64(m_firp, *m_add_sections, *m_bss_opts);
		pe_write->Write(exeiop,output_filename, "a.ncexe");
	}
	else
	{
		cout << "Cannot create output file of correct type " << endl;
		assert(0); 
		abort(); 
	}

	// change permissions on output file
	auto chmod_cmd=string("chmod +x ")+output_filename;
	auto res=system(chmod_cmd.c_str());
	assert(res!=-1);

}


void ZiprImpl_t::PrintStats()
{
	// do something like print stats as #ATTRIBUTES.
	m_dollop_mgr.PrintStats(cout);
	m_dollop_mgr.PrintPlacementMap(memory_space, *m_dollop_map_filename);
	m_stats->PrintStats(cout);

	// and dump a map file of where we placed instructions.  maybe guard with an option.
	// default to dumping to zipr.map 
	dump_scoop_map();
	dump_instruction_map();
}

void ZiprImpl_t::UpdateCallbacks()
{
	// first byte of this range is the last used byte.
	const auto range_it=memory_space.FindFreeRange((RangeAddress_t) -1);
	assert(memory_space.IsValidRange(range_it));


	for(const auto &p : unpatched_callbacks)
	{
		auto entry=p.first;
		Instruction_t *insn = entry->getInstruction();
		RangeAddress_t at=p.second;
		RangeAddress_t to=0x0;//FindCallbackAddress(end_of_new_space,start_addr,insn->getCallback());
		DLFunctionHandle_t patcher = nullptr;

		if (plugman.DoesPluginRetargetCallback(at, entry, to, patcher))
		{
			if (*m_verbose)
			{
				cout << "Patching retargeted callback at " << std::hex << at << " to "
				     << patcher->toString() << "-assigned address: "
						 << std::hex << to << endl;
			}
		}

		if(to)
		{
			cout<<"Patching callback "<< insn->getCallback()<<" at "<<std::hex<<at<<" to jump to "<<to<<endl;
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

	for(const auto &scoop : m_firp->getDataScoops())
	{
		ofs << hex << setw(10)<<scoop->getBaseID()
		    <<hex<<left<<setw(10)<<scoop->getStart()->getVirtualOffset()
		    <<hex<<left<<setw(10)<< scoop->getSize()
		    <<hex<<left<<setw(10)<< scoop->getRawPerms()
		    <<hex<<left<<setw(10)<< scoop->getName()
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

	for(std::map<IRDB_SDK::Instruction_t*,RangeAddress_t>::iterator it=final_insn_locations.begin();
		it!=final_insn_locations.end(); ++it)
	{
		Instruction_t* insn=it->first;
		AddressID_t* ibta=insn->getIndirectBranchTargetAddress();
		RangeAddress_t addr=it->second;

		ofs << hex << setw(10)<<insn->getBaseID()
		    <<hex<<left<<setw(10)<<insn->getAddress()->getVirtualOffset()
		    <<hex<<left<<setw(10)<< (ibta ? ibta->getVirtualOffset() : 0)
		    <<hex<<left<<setw(10)<<addr
		    <<hex<<left<<setw(10)<<( insn->getFunction() ? insn->getFunction()->getBaseID() : -1 )
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
		VirtualOffset_t first_valid_address=0;
		VirtualOffset_t last_valid_address=0;

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

		if (scoop->getName() == "textra")
		{
			/*
			 * We have to do special handling for the textra scoop.
			 * If we do not, then the sheer scale of the default size
			 * of the textra scoop will cause Zipr to bomb during the
			 * next loop.
			 */
			auto frit=memory_space.FindFreeRange((RangeAddress_t) -1);
			assert(memory_space.IsValidRange(frit));
			scoop->getEnd()->setVirtualOffset(frit->getStart());
		}

		for(auto i=scoop->getStart()->getVirtualOffset();
		    i<= scoop->getEnd()->getVirtualOffset();
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
			if (*m_verbose)
				cout << "Removing an empty scoop (" << scoop->getName() << ")." << endl;
			/*
			assert(first_valid_address==0);
			assert(last_valid_address==0);
			m_firp->getAddresses().erase(scoop->getStart());
			m_firp->getAddresses().erase(scoop->getEnd());


			m_firp->getDataScoops().erase(*it);

			// Delete addresses and then the scoop itself.
			delete scoop->getStart();
			delete scoop->getEnd();
			delete scoop;
			*/
			it = m_zipr_scoops.erase(it);
			m_firp->removeScoop(scoop);
			scoop=nullptr;
		}
		else
		{
			if ((scoop->getStart()->getVirtualOffset() != first_valid_address ||
			   scoop->getEnd()->getVirtualOffset() != last_valid_address) &&
			   *m_verbose)
			{
				cout <<"Shrinking scoop "<<scoop->getName()
				     <<" to "
				     << std::hex << first_valid_address << "-"
				     << std::hex << last_valid_address << endl;
			}
			else if (*m_verbose)
			{
				cout<<"Leaving scoop "<<scoop->getName()<<" alone. "<<endl;
			}

			cout << "Updating a scoop named " << scoop->getName() << endl;
			assert(first_valid_address!=0);
			assert(last_valid_address!=0);
			scoop->getStart()->setVirtualOffset(first_valid_address);
			scoop->getEnd()->setVirtualOffset(last_valid_address);

			/*
			 * Resize the contents.
			 */
			auto scoop_contents = scoop->getContents();
			scoop_contents.resize(scoop->getEnd()->getVirtualOffset() -
			                      scoop->getStart()->getVirtualOffset() + 1);
			assert(scoop->getSize() == scoop_contents.size());

			/*
			 * And now update the contents.
			 */
			for(auto i=scoop->getStart()->getVirtualOffset();
			    i<= scoop->getEnd()->getVirtualOffset();
			    i++)
			{
				scoop_contents[i-scoop->getStart()->getVirtualOffset()]=memory_space[i];
			}
			scoop->setContents(scoop_contents);
			// m_firp->getDataScoops().insert(scoop); we added this earlier when we created the obj.
			// jdh -- a bit worried that this'll break assumptions in other places
			++it;
		}
	}
	return;
}

void  ZiprImpl_t::FixNoFallthroughs()
{
        auto hlt=archhelper->createNewHaltInstruction(m_firp, nullptr);
        auto jmp=archhelper->createNewJumpInstruction(m_firp, nullptr);

	hlt->setFallthrough(jmp);
	jmp->setTarget(hlt);

	for(const auto insn : m_firp->getInstructions())
	{
		if(insn==hlt) continue;
		if(insn==jmp) continue;

		if(insn->getFallthrough()==nullptr)
		{
			const auto d=DecodedInstruction_t::factory(insn);
			if(d->isConditionalBranch())
				insn->setFallthrough(hlt);
		}
		if(insn->getTarget()==nullptr)
		{
			const auto d=DecodedInstruction_t::factory(insn);
			if(d->isBranch() && !d->isReturn() && d->hasOperand(0) && d->getOperand(0)->isConstant())
				insn->setTarget(hlt);
		}
		
	}
		
	
}

void  ZiprImpl_t::FixTwoByteWithPrefix()
{
	for(const auto insn : m_firp->getInstructions())
	{
		const auto d=DecodedInstruction_t::factory(insn);
		if(!d->isBranch()) continue;	// skip non-branches
		if(d->isReturn()) continue;	// skip returns
		if(d->getOperands().size()!=1) continue;	// skip branches that have no operands or more than one
		if(!d->getOperand(0)->isConstant()) continue;	// skip anything where the operand isn't a constant
		if(d->getPrefixCount()==0) continue;	// prevents arm instructions from being xformed.

		
		while (true)
		{
			const auto b=insn->getDataBits()[0];
			// basic prefix check
			const auto prefixes=set<uint8_t>({0x2e, 0x3e, 0x64, 0x65, 0xf2, 0xf3});
			if(prefixes.find(b)!=end(prefixes))
			{
				// remove prefix 
				insn->setDataBits(insn->getDataBits().erase(0,1));	
			}
			// remove rex prefix when unnecessary 
			else if(m_firp->getArchitectureBitWidth()==64 && (b&0xf0)==0x40 /* has rex prefix */)
			{
				insn->setDataBits(insn->getDataBits().erase(0,1));	
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
	
	for(auto & insn : m_firp->getInstructions())
	{
		auto ft=insn->getFallthrough();
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
			
				auto newjmp=archhelper->createNewJumpInstruction(m_firp,nullptr);
				count++;
				newjmp->setTarget(ft);
				from->setFallthrough(newjmp);

			});
		};
	}

	// after we've inserted all the jumps, assemble them.
	m_firp->assembleRegistry();

	cout<<"# ATTRIBUTE Zipr::jumps_inserted_for_multiple_fallthroughs="<<dec<<count<<endl;
}


void  ZiprImpl_t::RelayoutEhInfo()
{
	const auto found_eh_ir_it = find_if( 
		m_firp->getInstructions().begin(), 
		m_firp->getInstructions().end(),
	 	[](const Instruction_t* i)
			{ 
				return (i->getEhProgram()!=nullptr || i->getEhCallSite()!=nullptr);
			} 
		);

	// do nothing if we didn't find any IR.
	if(found_eh_ir_it==m_firp->getInstructions().end())
		return;
		
	auto eh = (EhWriter_t *)nullptr;
	if(m_firp->getArchitectureBitWidth()==64)
		eh=new EhWriterImpl_t<8>(*this);
	else if(m_firp->getArchitectureBitWidth()==32)
		eh=new EhWriterImpl_t<4>(*this);
	else
		assert(0);

	eh->GenerateNewEhInfo();
}


void ZiprImpl_t::ApplyNopToPatch(RangeAddress_t addr)
{
	if (!*m_apply_nop)
	{
		if (*m_verbose)
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

