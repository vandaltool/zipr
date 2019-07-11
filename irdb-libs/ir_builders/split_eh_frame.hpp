#ifndef eh_frame_hpp
#define eh_frame_hpp

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

#include <exeio.h>
#include <ehp.hpp>




using OffsetMap_t = std::map<IRDB_SDK::VirtualOffset_t, IRDB_SDK::Instruction_t*>;

class split_eh_frame_t 
{
	public:

		virtual void build_ir() const =0;
		virtual void print() const=0;
		virtual IRDB_SDK::Instruction_t* find_lp(IRDB_SDK::Instruction_t*) const =0;

		static std::unique_ptr<split_eh_frame_t> factory(IRDB_SDK::FileIR_t *firp);

};

template <int ptrsize>
class split_eh_frame_impl_t : public split_eh_frame_t
{
	private: 

	IRDB_SDK::FileIR_t* firp;
	IRDB_SDK::DataScoop_t* eh_frame_scoop;
	IRDB_SDK::DataScoop_t* eh_frame_hdr_scoop;
	IRDB_SDK::DataScoop_t* gcc_except_table_scoop;
	OffsetMap_t offset_to_insn_map;

	std::unique_ptr<const EHP::EHFrameParser_t> eh_frame_parser;
	const EHP::FDEVector_t* fdes;

	bool init_offset_map();

	bool lsda_call_site_appliesTo
		(
		    const EHP::LSDACallSite_t& cs, 
		    const IRDB_SDK::Instruction_t* insn
		) const;
	void lsda_call_site_build_ir
		(
		    const EHP::LSDACallSite_t& cs,
		    IRDB_SDK::Instruction_t* insn, 
		    /* const std::vector<lsda_type_table_entry_t <ptrsize> > &*/ const EHP::TypeTableVector_t* type_table_ptr, 
		    const uint8_t& tt_encoding
		) const;
	void lsda_build_ir
		(
		    const EHP::LSDA_t& lsda,
		    IRDB_SDK::Instruction_t* insn
		) const;
        bool fde_contents_appliesTo
		(
		    const EHP::FDEContents_t& fde,
		    const IRDB_SDK::Instruction_t* insn
		) const;
	void fde_contents_build_ir
		(
		    const EHP::FDEContents_t& fde,
		    IRDB_SDK::Instruction_t* insn
		) const;

	public:

	split_eh_frame_impl_t(IRDB_SDK::FileIR_t* p_firp);


	void print() const;

	void build_ir() const;

	IRDB_SDK::Instruction_t* find_lp(IRDB_SDK::Instruction_t*) const ;
};

void split_eh_frame(IRDB_SDK::FileIR_t* firp);

#endif
