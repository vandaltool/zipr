#ifndef eh_frame_hpp
#define eh_frame_hpp

#include <libIRDB-core.hpp>
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

#include <exeio.h>
#include <ehp.hpp>




using OffsetMap_t = std::map<libIRDB::virtual_offset_t, libIRDB::Instruction_t*>;

class split_eh_frame_t 
{
	public:

		virtual void build_ir() const =0;
		virtual void print() const=0;
		virtual libIRDB::Instruction_t* find_lp(libIRDB::Instruction_t*) const =0;

		static std::unique_ptr<split_eh_frame_t> factory(libIRDB::FileIR_t *firp);

};

template <int ptrsize>
class split_eh_frame_impl_t : public split_eh_frame_t
{
	private: 

	libIRDB::FileIR_t* firp;
	libIRDB::DataScoop_t* eh_frame_scoop;
	libIRDB::DataScoop_t* eh_frame_hdr_scoop;
	libIRDB::DataScoop_t* gcc_except_table_scoop;
	OffsetMap_t offset_to_insn_map;

	std::unique_ptr<const EHP::EHFrameParser_t> eh_frame_parser;
	std::shared_ptr<const EHP::FDEVector_t> fdes;

	bool init_offset_map();

	bool lsda_call_site_appliesTo
		(
		    const EHP::LSDACallSite_t& cs, 
		    const libIRDB::Instruction_t* insn
		) const;
	void lsda_call_site_build_ir
		(
		    const EHP::LSDACallSite_t& cs,
		    libIRDB::Instruction_t* insn, 
		    /* const std::vector<lsda_type_table_entry_t <ptrsize> > &*/ std::shared_ptr<EHP::TypeTableVector_t> type_table_ptr, 
		    const uint8_t& tt_encoding
		) const;
	void lsda_build_ir
		(
		    const EHP::LSDA_t& lsda,
		    libIRDB::Instruction_t* insn
		) const;
        bool fde_contents_appliesTo
		(
		    const EHP::FDEContents_t& fde,
		    const libIRDB::Instruction_t* insn
		) const;
	void fde_contents_build_ir
		(
		    const EHP::FDEContents_t& fde,
		    libIRDB::Instruction_t* insn
		) const;

	public:

	split_eh_frame_impl_t(libIRDB::FileIR_t* p_firp);


	void print() const;

	void build_ir() const;

	libIRDB::Instruction_t* find_lp(libIRDB::Instruction_t*) const ;
};

void split_eh_frame(libIRDB::FileIR_t* firp);

#endif
