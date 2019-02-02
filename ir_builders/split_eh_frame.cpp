#include <irdb-core>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <limits>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <assert.h>
#include <elf.h>
#include <algorithm>
#include <memory>
#include <tuple>
#include <functional>

#include <exeio.h>

#include "split_eh_frame.hpp"
#include "ehp.hpp"

using namespace std;
using namespace EXEIO;
using namespace IRDB_SDK;
using namespace EHP;

#define ALLOF(s) begin(s), end(s)


struct EhProgramPlaceHolder_t
{
	uint8_t caf;
	int8_t daf;
	int8_t rr;
	uint8_t ptrsize; // needed for interpreting programs
	IRDB_SDK::EhProgramListing_t cie_program;
	IRDB_SDK::EhProgramListing_t fde_program;

	//IRDB_SDK::EhProgramListing_t& getCIEProgram() { return cie_program; }
	//IRDB_SDK::EhProgramListing_t& getFDEProgram() { return fde_program; }
	const IRDB_SDK::EhProgramListing_t& getCIEProgram() const { return cie_program; }
	const IRDB_SDK::EhProgramListing_t& getFDEProgram() const { return fde_program; }
	uint8_t getCodeAlignmentFactor() const { return caf; } 
	int8_t getDataAlignmentFactor() const { return daf; }
	int8_t getReturnRegNumber() const { return rr; }
	uint8_t getPointerSize() const { return ptrsize; }

};

template <int ptrsize>
bool split_eh_frame_impl_t<ptrsize>::lsda_call_site_appliesTo(const LSDACallSite_t& cs, const Instruction_t* insn)  const
{
	assert(insn && insn->getAddress());
	auto insn_addr=insn->getAddress()->getVirtualOffset();

	const auto call_site_addr=cs.getCallSiteAddress();
	const auto call_site_end_addr=cs.getCallSiteEndAddress();

	return ( call_site_addr <=insn_addr && insn_addr<call_site_end_addr );
}

template <int ptrsize>
void split_eh_frame_impl_t<ptrsize>::lsda_call_site_build_ir
	(
	    const LSDACallSite_t& cs,
	    Instruction_t* insn, 
	    const /*vector<lsda_type_table_entry_t <ptrsize> > &*/ std::shared_ptr<EHP::TypeTableVector_t> type_table_ptr, 
	    const uint8_t& tt_encoding
	) const
{
	const auto &type_table=*type_table_ptr;
	const auto &om=offset_to_insn_map;
	const auto landing_pad_addr=cs.getLandingPadAddress();
	const auto action_table_ptr=cs.getActionTable();
	const auto &action_table=*action_table_ptr;
	assert(lsda_call_site_appliesTo(cs,insn));

	// find landing pad instruction.
	auto lp_insn=(IRDB_SDK::Instruction_t*)NULL;
	auto lp_it=om.find(landing_pad_addr);
	if(lp_it!=om.end())
		lp_insn=lp_it->second;

	// create the callsite.
	auto new_ehcs = firp->addEhCallSite_t(insn, tt_encoding, lp_insn);

	//cout<<"landing pad addr : 0x"<<hex<<landing_pad_addr<<endl;
	if(action_table.size() == 0 ) 
	{
		new_ehcs->setHasCleanup();
		// cout<<"Destructors to call, but no exceptions to catch"<<endl;
	}
	else
	{
		for_each(action_table.begin(), action_table.end(), [&](const shared_ptr<LSDACallSiteAction_t>& p)
		{
			const auto action=p->getAction();
			if(action==0)
			{
				auto new_ttov=new_ehcs->getTTOrderVector();
				new_ttov.push_back(action);
				new_ehcs->setTTOrderVector(new_ttov);
				//cout<<"Cleanup only (no catches) ."<<endl;
			}
			else if(action>0)
			{
				auto new_ttov=new_ehcs->getTTOrderVector();
				new_ttov.push_back(action);
				new_ehcs->setTTOrderVector(new_ttov);
				// new_ehcs->getTTOrderVector().push_back(action);
				const auto index=action - 1;
				//cout<<"Catch for type:  ";
				// the type table reveral was done during parsing, type table is right-side-up now.
				//type_table.at(index).print();
				auto wrt=(DataScoop_t*)NULL; 
				if(type_table.at(index)->getTypeInfoPointer()!=0)
				{
					wrt=firp->findScoop(type_table.at(index)->getTypeInfoPointer());
					assert(wrt);
				}
				const auto offset=index*type_table.at(index)->getTTEncodingSize();
				auto addend=0;
				if(wrt!=NULL) 
					addend=type_table.at(index)->getTypeInfoPointer()-wrt->getStart()->getVirtualOffset();
				/*
				auto newreloc=new Relocation_t(BaseObj_t::NOT_IN_DATABASE, offset, "type_table_entry", wrt, addend);
				new_ehcs->getRelocations().insert(newreloc);
				firp->getRelocations().insert(newreloc);
				*/
				auto newreloc=firp->addNewRelocation(new_ehcs,offset, "type_table_entry", wrt, addend);
				(void)newreloc; // just give it to the ir

				//if(wrt==NULL)
				//	cout<<"Catch all in type table"<<endl;
				//else
				//	cout<<"Catch for type at "<<wrt->getName()<<"+0x"<<hex<<addend<<"."<<endl;
			}
			else if(action<0)
			{
				static auto already_warned=false;
				if(!already_warned)
				{
					ofstream fout("warning.txt"); 
					fout<<"Dynamic exception specification in eh_frame not fully supported."<<endl; 
					already_warned=true;
				}

				// this isn't right at all, but pretend it's a cleanup!
				new_ehcs->setHasCleanup();
				//cout<<"Cleanup only (no catches) ."<<endl;
			}
			else
			{
				cout<<"What? :"<< action <<endl;
				exit(1);
			}
		});
	}
}

template <int ptrsize>
void split_eh_frame_impl_t<ptrsize>::lsda_build_ir(const LSDA_t& lsda, Instruction_t* insn) const
{
	const auto  call_site_table_ptr=lsda.getCallSites();
	const auto& call_site_table=*call_site_table_ptr;
	const auto& type_table_ptr=lsda.getTypeTable();
//	auto& type_table=*type_table_ptr;

	const auto cs_ptr_it=find_if(ALLOF(call_site_table), [&](const shared_ptr<LSDACallSite_t> &p)
	{
		return lsda_call_site_appliesTo(*p, insn);
	});

	if(cs_ptr_it!= call_site_table.end())
	{
		const auto cs_ptr=*cs_ptr_it;
		lsda_call_site_build_ir(*cs_ptr,insn, type_table_ptr, lsda.getTTEncoding());
	}
	else
	{
		// no call site table entry for this instruction.
	}
}

template <int ptrsize>
bool split_eh_frame_impl_t<ptrsize>::fde_contents_appliesTo(const FDEContents_t& fde, const Instruction_t* insn)  const
{
	assert(insn && insn->getAddress());
	auto insn_addr=insn->getAddress()->getVirtualOffset();
	const auto fde_start_addr=fde.getStartAddress();
	const auto fde_end_addr=fde.getEndAddress();
	return ( fde_start_addr<=insn_addr && insn_addr<fde_end_addr );
}

template <int ptrsize>
void split_eh_frame_impl_t<ptrsize>::fde_contents_build_ir(const FDEContents_t& fde, Instruction_t* insn) const
{
	const auto fde_start_addr=fde.getStartAddress();
	const auto fde_end_addr=fde.getEndAddress();
	const auto lsda_ptr=fde.getLSDA();
	const auto &lsda=*lsda_ptr;
	const auto lsda_addr=fde.getLSDAAddress();
	
	// assert this is the right FDE.
	assert( fde_start_addr<= insn->getAddress()->getVirtualOffset() && insn->getAddress()->getVirtualOffset() <= fde_end_addr);

	//eh_pgm.print(fde_start_addr);
	if(lsda_addr!=0)
		lsda_build_ir(lsda,insn);
}

template <int ptrsize>
bool split_eh_frame_impl_t<ptrsize>::init_offset_map() 
{
	for(const auto i : firp->getInstructions())
	{
		offset_to_insn_map[i->getAddress()->getVirtualOffset()]=i;
	};
	return false;
}


template <int ptrsize>
void split_eh_frame_impl_t<ptrsize>::build_ir() const
{
	class whole_pgm_t
	{
		public:
			whole_pgm_t(EhProgramPlaceHolder_t& _pgm, EhProgram_t* cp, uint64_t _personality)
				: hashcode(hashPgm(_pgm)), pgm(_pgm), cached_pgm(cp), personality(_personality)
			{
			}
			void setCachedProgram(EhProgram_t* cp) { cached_pgm=cp; }
			EhProgram_t* getCachedProgram() const { return cached_pgm; }
			uint64_t getPersonality() const { return personality; }
		private:
			static uint64_t hashPgm(const EhProgramPlaceHolder_t& vec)
			{
				auto seed = vec.getCIEProgram().size() + vec.getFDEProgram().size();
				auto hash_fn=hash<string>();

				for(const auto& i : vec.getCIEProgram()) 
					seed ^= hash_fn(i) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
				for(const auto& i : vec.getFDEProgram()) 
					seed ^= hash_fn(i) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
				return seed;
			};
		uint64_t hashcode;
		EhProgramPlaceHolder_t pgm;
		EhProgram_t* cached_pgm;
		uint64_t personality;

		friend struct EhProgramComparator_t;
	};

	//const auto fdes_ptr=eh_frame_parser->getFDEs();
	//const auto &fdes=*fdes_ptr;

	auto reusedpgms=size_t(0);
	struct EhProgramComparator_t 
	{
		bool operator() (const whole_pgm_t& lhs, const whole_pgm_t& rhs) 
		{
			const auto &a=(lhs.pgm);
			const auto &b=(rhs.pgm);
			if(lhs.hashcode == rhs.hashcode)
				return
					make_tuple(
					    a.getCIEProgram(),
					    a.getFDEProgram(),
					    a.getCodeAlignmentFactor(),
					    a.getDataAlignmentFactor(),
					    a.getReturnRegNumber(),
					    a.getPointerSize(), 
					    lhs.personality
					   )
					<
					make_tuple(
					    b.getCIEProgram(),
					    b.getFDEProgram(),
					    b.getCodeAlignmentFactor(),
					    b.getDataAlignmentFactor(),
					    b.getReturnRegNumber(),
					    b.getPointerSize(), 
					    rhs.personality
					   );
			else 
				return lhs.hashcode < rhs.hashcode;
//			return tie(*a.first, a.second) < tie(*b.first,b.second); 
		}
	};

	// this is used to avoid adding duplicate entries to the program's IR, it allows a lookup by value
	// instead of the IR's set which allows duplicates.
	auto eh_program_cache = set<whole_pgm_t, EhProgramComparator_t>();

	// find the right cie and fde, and build the IR from those for this instruction.
	auto build_ir_insn=[&](Instruction_t* insn) -> void
	{
		const auto find_addr=insn->getAddress()->getVirtualOffset();
		static auto fie_ptr=shared_ptr<FDEContents_t>(); 
		static auto cie_instructions=shared_ptr<EHProgramInstructionVector_t>();
		static auto fde_instructions=shared_ptr<EHProgramInstructionVector_t>();

		if (fie_ptr && fie_ptr->getStartAddress() <= find_addr && find_addr < fie_ptr->getEndAddress())
		{
			// fie_ptr already points at right thing, do nothing
		}
		else 
		{
			fie_ptr=eh_frame_parser->findFDE(find_addr);
			if(fie_ptr)
			{
				cie_instructions=fie_ptr->getCIE().getProgram().getInstructions();
				fde_instructions=fie_ptr->getProgram().getInstructions();
			}
		}

		if(fie_ptr != nullptr ) 
		{
			// const auto fie_ptr=*fie_ptr_it;

			if(getenv("EHIR_VERBOSE")!=NULL)
			{
				cout<<hex<<insn->getAddress()->getVirtualOffset()<<":"
				    <<insn->getBaseID()<<":"<<insn->getDisassembly()<<" -> "<<endl;
				fie_ptr->print();
			}

			const auto fde_addr=fie_ptr->getStartAddress();
			const auto caf=(uint8_t)fie_ptr->getCIE().getCAF(); 
			const auto daf=(int8_t)fie_ptr->getCIE().getDAF(); 
			const auto return_reg=(int8_t)fie_ptr->getCIE().getReturnRegister(); 
			const auto personality=fie_ptr->getCIE().getPersonality(); 
			const auto insn_addr=insn->getAddress()->getVirtualOffset();

			auto import_pgm = [&](EhProgramListing_t& out_pgm_final, const shared_ptr<EHProgramInstructionVector_t> &in_pgm_instructions_ptr) -> void
			{
				auto out_pgm=vector<shared_ptr<EHProgramInstruction_t> >();
				auto cur_addr=fde_addr;
				const auto in_pgm_instructions=*in_pgm_instructions_ptr;
				for(const auto & insn_ptr : in_pgm_instructions)
				{
					const auto & insn=*insn_ptr;
					if(insn.advance(cur_addr, caf))
					{	
						if(cur_addr > insn_addr)
							break;
					}
					else if(insn.isNop())
					{
						// skip nops 
					}
					else if(insn.isRestoreState())
					{
						// if a restore state happens, pop back out any instructions until
						// we find the corresponding remember_state
						while(1)
						{
							if(out_pgm.size()==0)
							{
								// unmatched remember state
								cerr<<"Error in CIE/FDE program:  unmatched restore_state command"<<endl;
								break;
							}
							const auto back_insn=out_pgm.back();
							out_pgm.pop_back();
							//const auto back_insn=eh_program_insn_t<ptrsize>(back_str);
							if(back_insn->isRememberState())
								break;
						}
					}
					else
					{
						out_pgm.push_back(insn_ptr);
					}

				}
				if(getenv("EHIR_VERBOSE")!=NULL)
				{
					cout<<"\tPgm has insn_count="<<out_pgm.size()<<endl;
				}
				transform
				    (
				        ALLOF(out_pgm), 
				        back_inserter(out_pgm_final), 
				        [](const shared_ptr<EHProgramInstruction_t>& p){ return string(ALLOF(p->getBytes()));}
				    ); 
			}; 


			// build an eh program on the stack;
			//
			auto ehpgm=EhProgramPlaceHolder_t({caf,daf,return_reg, ptrsize, {}, {}});
			import_pgm(ehpgm.cie_program, cie_instructions);
			import_pgm(ehpgm.fde_program, fde_instructions);


			//if(getenv("EHIR_VERBOSE")!=NULL)
			//	ehpgm.print();
			// see if we've already built this one.
			auto ehpgm_it = eh_program_cache.find(whole_pgm_t(ehpgm, nullptr, personality)) ;
			if(ehpgm_it != eh_program_cache.end())
			{
				// yes, use the cached program.
				insn->setEhProgram(ehpgm_it->getCachedProgram());
				if(getenv("EHIR_VERBOSE")!=NULL)
					cout<<"Re-using existing Program!"<<endl;
				reusedpgms++;
			}
			else /* doesn't yet exist! */
			{
				
				if(getenv("EHIR_VERBOSE")!=NULL)
					cout<<"Allocating new Program!"<<endl;

				// allocate a new pgm in the heap so we can give it to the IR.
				/*
				auto newehpgm=new EhProgram_t(ehpgm); // copy constructor
				assert(newehpgm);
				firp->getAllEhPrograms().insert(newehpgm);
				insn->setEhProgram(newehpgm);
				*/
				auto newehpgm=firp->addEhProgram(insn, ehpgm.caf, ehpgm.daf,ehpgm.rr, ehpgm.ptrsize, ehpgm.cie_program, ehpgm.fde_program);

				// allocate a relocation for the personality and give it to the IR.	
				auto personality_scoop=firp->findScoop(personality);
				auto personality_insn_it=offset_to_insn_map.find(personality);
				auto personality_insn=personality_insn_it==offset_to_insn_map.end() ? (Instruction_t*)NULL : personality_insn_it->second;
				auto personality_obj = personality_scoop ? (BaseObj_t*)personality_scoop : (BaseObj_t*)personality_insn;
				auto addend= personality_scoop ? personality - personality_scoop->getStart()->getVirtualOffset() : 0;
				assert(personality==0 || personality_obj!=NULL);

				if(personality_obj==NULL)
				{
					if(getenv("EHIR_VERBOSE")!=NULL)
						cout<<"Null personality obj: 0x"<<hex<<personality<<endl;
				}
				else if(personality_scoop)
				{
					if(getenv("EHIR_VERBOSE")!=NULL)
						cout<<"Found personality scoop: 0x"<<hex<<personality<<" -> "
						    <<personality_scoop->getName()<<"+0x"<<hex<<addend<<endl;
				}
				else if(personality_insn)
				{
					if(getenv("EHIR_VERBOSE")!=NULL)
						cout<<"Found personality insn: 0x"<<hex<<personality<<" -> "
						    <<personality_insn->getBaseID()<<":"<<personality_insn->getDisassembly()<<endl;
				}
				else
					assert(0);

				/*
				auto newreloc=new Relocation_t(BaseObj_t::NOT_IN_DATABASE, 0, "personality", personality_obj, addend);
				assert(newreloc);	
				newehpgm->getRelocations().insert(newreloc);
				firp->getRelocations().insert(newreloc);
				*/
				auto newreloc=firp->addNewRelocation(newehpgm,0, "personality", personality_obj, addend);
				(void)newreloc; // not used, just give it to the IR

				// update cache.
				eh_program_cache.insert( whole_pgm_t(ehpgm,newehpgm,personality));
			}
			
			// build the IR from the FDE.
			fde_contents_build_ir(*fie_ptr.get(), insn);
		}
		else
		{
			if(getenv("EHIR_VERBOSE")!=NULL)
			{
				cout<<hex<<insn->getAddress()->getVirtualOffset()<<":"
				    <<insn->getBaseID()<<":"<<insn->getDisassembly()<<" has no FDE "<<endl;
			}
		}
		
	};

#if 0
	auto remove_reloc=[&](Relocation_t* r) -> void
	{
		firp->getRelocations().erase(r);
		delete r;
	};

	auto remove_address=[&](AddressID_t* a) -> void
	{
		firp->getAddresses().erase(a);
		for(auto &r : a->getRelocations()) remove_reloc(r);
		for(auto &r : firp->getRelocations()) assert(r->getWRT() != a);
		delete a;	
	};

	auto remove_scoop=[&] (DataScoop_t* s) -> void 
	{ 
		if(s==NULL)
			return;
		firp->getDataScoops().erase(s);
		remove_address(s->getStart());
		remove_address(s->getEnd());
		for(auto &r : s->getRelocations()) remove_reloc(r);
		for(auto &r : firp->getRelocations()) assert(r->getWRT() != s);
		delete s;
	};
#endif

	for(Instruction_t* i : firp->getInstructions())
	{
		build_ir_insn(i);
	}

	cout<<"# ATTRIBUTE Split_Exception_Handler::total_eh_programs_created="<<dec<<firp->getAllEhPrograms().size()<<endl;
	cout<<"# ATTRIBUTE Split_Exception_Handler::total_eh_programs_reused="<<dec<<reusedpgms<<endl;
	cout<<"# ATTRIBUTE Split_Exception_Handler::total_eh_programs="<<dec<<firp->getAllEhPrograms().size()+reusedpgms<<endl;
	cout<<"# ATTRIBUTE Split_Exception_Handler::pct_eh_programs="<<std::fixed
	    <<((float)firp->getAllEhPrograms().size()/((float)firp->getAllEhPrograms().size()+reusedpgms))*100.00
	    <<"%" <<endl;
	cout<<"# ATTRIBUTE Split_Exception_Handler::pct_eh_programs_reused="<<std::fixed
	    <<((float)reusedpgms/((float)firp->getAllEhPrograms().size()+reusedpgms))*100.00
	    <<"%"<<endl;

	firp->removeScoop(eh_frame_scoop);
	firp->removeScoop(eh_frame_hdr_scoop);
	firp->removeScoop(gcc_except_table_scoop);

}

template <int ptrsize>
Instruction_t* split_eh_frame_impl_t<ptrsize>::find_lp(Instruction_t* i) const 
{
	const auto find_addr=i->getAddress()->getVirtualOffset();
	const auto fde_ptr=eh_frame_parser->findFDE(find_addr);

	if(fde_ptr==nullptr)
		return nullptr;

	const auto &the_fde=*fde_ptr;
	const auto &the_lsda_ptr=the_fde.getLSDA();
	const auto &the_lsda=*the_lsda_ptr;
	const auto &cstab_ptr  = the_lsda.getCallSites();
	const auto &cstab  = *cstab_ptr;

	const auto cstab_it=find_if(ALLOF(cstab), [&](const shared_ptr<LSDACallSite_t>& cs)
		{ return lsda_call_site_appliesTo(*cs,i); });

	if(cstab_it==cstab.end())
		return nullptr;

	const auto &the_cstab_entry_ptr=*cstab_it;
	const auto &the_cstab_entry=*the_cstab_entry_ptr;
	const auto lp_addr= the_cstab_entry.getLandingPadAddress();

	const auto om_it=offset_to_insn_map.find(lp_addr);

	if(om_it==offset_to_insn_map.end())
		return nullptr;

	auto lp=om_it->second;
	return lp;
}

template <int ptrsize>
void split_eh_frame_impl_t<ptrsize>::print() const
{
	eh_frame_parser->print();
}

template <int ptrsize>
split_eh_frame_impl_t<ptrsize>::split_eh_frame_impl_t(FileIR_t* p_firp)
	: firp(p_firp),
	  eh_frame_scoop(NULL),
	  eh_frame_hdr_scoop(NULL),
	  gcc_except_table_scoop(NULL)
{
	assert(firp!=NULL);

	// function to find a scoop by name.
	auto lookup_scoop_by_name=[&](const string &the_name) -> DataScoop_t* 
	{
		const auto scoop_it=find_if(
			firp->getDataScoops().begin(), 
			firp->getDataScoops().end(), 
			[the_name](DataScoop_t* scoop)
			{
				return scoop->getName()==the_name;
			});

		if(scoop_it!=firp->getDataScoops().end())
			return *scoop_it;
		return NULL;
	};
	auto scoop_address=[&](const DataScoop_t* p) -> uint64_t { return p==NULL ? 0  : p->getStart()->getVirtualOffset(); };
	auto scoop_contents=[&](const DataScoop_t* p) -> string  { return p==NULL ? "" : p->getContents(); };

	eh_frame_scoop=lookup_scoop_by_name(".eh_frame");
	eh_frame_hdr_scoop=lookup_scoop_by_name(".eh_frame_hdr");
	gcc_except_table_scoop=lookup_scoop_by_name(".gcc_except_table");

	eh_frame_parser=EHFrameParser_t::factory
		( 
		    ptrsize,
		    scoop_contents(eh_frame_scoop),         scoop_address(eh_frame_scoop), 
		    scoop_contents(eh_frame_hdr_scoop),     scoop_address(eh_frame_hdr_scoop), 
		    scoop_contents(gcc_except_table_scoop), scoop_address(gcc_except_table_scoop)
		);

	if(eh_frame_parser!=NULL)
		fdes=eh_frame_parser->getFDEs();

	(void)init_offset_map();
}

unique_ptr<split_eh_frame_t> split_eh_frame_t::factory(FileIR_t *firp)
{
	if( firp->getArchitectureBitWidth()==64)
		return unique_ptr<split_eh_frame_t>(new split_eh_frame_impl_t<8>(firp));
	else
		return unique_ptr<split_eh_frame_t>(new split_eh_frame_impl_t<4>(firp));

}

void split_eh_frame(FileIR_t* firp)
{
	auto found_err=false;
	//auto eh_frame_splitter=(unique_ptr<split_eh_frame_t>)NULL;
	const auto eh_frame_splitter=split_eh_frame_t::factory(firp);
	eh_frame_splitter->build_ir();

	assert(!found_err);
}
