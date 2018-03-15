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
#include "dwarf2.h"




typedef std::map<libIRDB::virtual_offset_t, libIRDB::Instruction_t*> OffsetMap_t;

template <int ptrsize>
class eh_frame_util_t
{
	public: 
	template <class T> 
	static bool read_type(T &value, uint32_t &position, const uint8_t* const data, const uint32_t max);
	template <class T> 
	static bool read_type_with_encoding
		(const uint8_t encoding, T &value, 
		uint32_t &position, 
		const uint8_t* const data, 
		const uint32_t max, 
		const uint64_t section_start_addr );

	static bool read_string 
		(std::string &s, 
		uint32_t & position, 
		const uint8_t* const data, 
		const uint32_t max);


	// see https://en.wikipedia.org/wiki/LEB128
	static bool read_uleb128 
		( uint64_t &result, 
		uint32_t& position, 
		const uint8_t* const data, 
		const uint32_t max);

	// see https://en.wikipedia.org/wiki/LEB128
	static bool read_sleb128 ( 
		int64_t &result, 
		uint32_t & position, 
		const uint8_t* const data, 
		const uint32_t max);
	
	static bool read_length(
		uint64_t &act_length, 
		uint32_t &position, 
		const uint8_t* const data, 
		const uint32_t max);
};

template <int ptrsize>
class eh_program_insn_t 
{
	public: 
	
	eh_program_insn_t() ;
	eh_program_insn_t(const std::string &s) ;

	void print(uint64_t &pc, int64_t caf=1) const;

	void push_byte(uint8_t c) ;

	static void print_uleb_operand(
		uint32_t pos, 
		const uint8_t* const data, 
		const uint32_t max) ;

	static void print_sleb_operand(
		uint32_t pos, 
		const uint8_t* const data, 
		const uint32_t max) ;

	bool parse_insn(
		uint8_t opcode, 
		uint32_t& pos, 
		const uint8_t* const data, 
		const uint32_t &max);

	bool isNop() const ;
	bool isRestoreState() const ;
	bool isRememberState() const ;

	bool Advance(uint64_t &cur_addr, uint64_t CAF) const ;

	const std::vector<uint8_t>& GetBytes() const ;
	std::vector<uint8_t>& GetBytes() ;

	private:

	std::vector<uint8_t> program_bytes;
};

template <int ptrsize>
bool operator<(const eh_program_insn_t<ptrsize>& a, const eh_program_insn_t<ptrsize>& b);

template <int ptrsize>
class eh_program_t
{
	public:
	void push_insn(const eh_program_insn_t<ptrsize> &i); 

	void print(const uint64_t start_addr=0) const;

	bool parse_program(
		const uint32_t& program_start_position, 
		const uint8_t* const data, 
		const uint32_t &max_program_pos);
	const std::vector<eh_program_insn_t <ptrsize> >& GetInstructions() const ;
	std::vector<eh_program_insn_t <ptrsize> >& GetInstructions() ;
	private:
	std::vector<eh_program_insn_t <ptrsize> > instructions;
};

template <int ptrsize>
bool operator<(const eh_program_t<ptrsize>& a, const eh_program_t<ptrsize>& b);

template <int ptrsize>
class cie_contents_t : eh_frame_util_t<ptrsize>
{
	private:
	uint64_t cie_position;
	uint64_t length;
	uint8_t cie_id;
	uint8_t cie_version;
	std::string augmentation;
	uint64_t code_alignment_factor;
	int64_t data_alignment_factor;
	uint64_t return_address_register_column;
	uint64_t augmentation_data_length;
	uint8_t personality_encoding;
	uint64_t personality;
	uint8_t lsda_encoding;
	uint8_t fde_encoding;
	eh_program_t<ptrsize> eh_pgm;

	public:

	cie_contents_t() ;
	
	const eh_program_t<ptrsize>& GetProgram() const ;
	uint64_t GetCAF() const ;
	int64_t GetDAF() const ;
	uint64_t GetPersonality() const ;
	uint64_t GetReturnRegister() const ;

	std::string GetAugmentation() const ;
	uint8_t GetLSDAEncoding() const ;
	uint8_t GetFDEEncoding() const ;

	bool parse_cie(
		const uint32_t &cie_position, 
		const uint8_t* const data, 
		const uint32_t max, 
		const uint64_t eh_addr);
	void print() const ;
	void build_ir(libIRDB::Instruction_t* insn) const;
};

template <int ptrsize>
class lsda_call_site_action_t : private eh_frame_util_t<ptrsize>
{
	private:
	int64_t action;

	public:
	lsda_call_site_action_t() ;
	int64_t GetAction() const ;

	bool parse_lcsa(uint32_t& pos, const uint8_t* const data, const uint64_t max, bool &end);
	void print() const;
};

template <int ptrsize>
bool operator< (const lsda_call_site_action_t <ptrsize> &lhs, const lsda_call_site_action_t <ptrsize> &rhs);

template <int ptrsize>
class lsda_type_table_entry_t: private eh_frame_util_t<ptrsize>
{
	private:
	uint64_t pointer_to_typeinfo;
	uint64_t tt_encoding;
	uint64_t tt_encoding_size;

	public:
	lsda_type_table_entry_t() ; 

	uint64_t GetTypeInfoPointer() const ;
	uint64_t GetEncoding() const ;
	uint64_t GetTTEncodingSize() const ;

	bool parse(
		const uint64_t p_tt_encoding, 	
		const uint64_t tt_pos, 	
		const uint64_t index,
		const uint8_t* const data, 
		const uint64_t max,  
		const uint64_t data_addr
		);

	void print() const;
	
};

template <int ptrsize>
class lsda_call_site_t : private eh_frame_util_t<ptrsize>
{
	private:
	uint64_t call_site_offset;
	uint64_t call_site_addr;
	uint64_t call_site_length;
	uint64_t call_site_end_addr;
	uint64_t landing_pad_offset;
	uint64_t landing_pad_addr;
	uint64_t action;
	uint64_t action_table_offset;
	uint64_t action_table_addr;

	std::vector<lsda_call_site_action_t <ptrsize> > action_table;

	public:
	lsda_call_site_t() ;

	const std::vector<lsda_call_site_action_t <ptrsize> >& GetActionTable() const { return action_table; }
	      std::vector<lsda_call_site_action_t <ptrsize> >& GetActionTable()       { return action_table; }

	uint64_t GetLandingPadAddress() const  { return landing_pad_addr ; } 

	bool parse_lcs(	
		const uint64_t action_table_start_addr, 	
		const uint64_t cs_table_start_addr, 	
		const uint8_t cs_table_encoding, 
		uint32_t &pos, 
		const uint8_t* const data, 
		const uint64_t max,  /* call site table max */
		const uint64_t data_addr, 
		const uint64_t landing_pad_base_addr,
		const uint64_t gcc_except_table_max);

	void print() const;

	bool appliesTo(const libIRDB::Instruction_t* insn) const;

	void build_ir(libIRDB::Instruction_t* insn, const std::vector<lsda_type_table_entry_t <ptrsize> > &type_table, const uint8_t& tt_encoding, const OffsetMap_t& om, libIRDB::FileIR_t* firp) const;
};


// short hand for a vector of call sites
template <int ptrsize>  using call_site_table_t = std::vector<lsda_call_site_t <ptrsize> > ;

template <int ptrsize>
class lsda_t : private eh_frame_util_t<ptrsize>
{
	private:
	uint8_t landing_pad_base_encoding;
	uint64_t landing_pad_base_addr; // often ommitted. when ommitted, filled in from FDE region start.
	uint8_t type_table_encoding;
	uint64_t type_table_offset;
	uint64_t type_table_addr;
	uint8_t cs_table_encoding;
	uint64_t cs_table_start_offset;
	uint64_t cs_table_start_addr;
	uint64_t cs_table_length;
	uint64_t cs_table_end_addr;
	uint64_t action_table_start_addr;
	call_site_table_t <ptrsize>  call_site_table;
	std::vector<lsda_type_table_entry_t <ptrsize> > type_table;

	public:

	uint8_t GetTTEncoding() const ;
	
	lsda_t() ;

	bool parse_lsda(const uint64_t lsda_addr, const libIRDB::DataScoop_t* gcc_except_scoop, const uint64_t fde_region_start);
	void print() const;
	void build_ir(libIRDB::Instruction_t* insn, const OffsetMap_t& om, libIRDB::FileIR_t* firp) const;

        const call_site_table_t<ptrsize> GetCallSites() const { return call_site_table;}

};



template <int ptrsize>
class fde_contents_t : eh_frame_util_t<ptrsize>
{
	uint32_t fde_position;
	uint32_t cie_position;
	uint64_t length;
	uint8_t id;
	uint64_t fde_start_addr;
	uint64_t fde_end_addr;
	uint64_t fde_range_len;
	uint64_t lsda_addr;


	lsda_t<ptrsize> lsda;
	eh_program_t<ptrsize> eh_pgm;
	cie_contents_t<ptrsize> cie_info;

	public:
	fde_contents_t() ;
	fde_contents_t(const uint64_t start_addr, const uint64_t end_addr)
		: 
		fde_start_addr(start_addr),
		fde_end_addr(end_addr)
	{} 

	bool appliesTo(const libIRDB::Instruction_t* insn) const;

	uint64_t GetFDEStartAddress() const { return fde_start_addr; } 
	uint64_t GetFDEEndAddress() const {return fde_end_addr; }

	const cie_contents_t<ptrsize>& GetCIE() const ;
	cie_contents_t<ptrsize>& GetCIE() ;

	const eh_program_t<ptrsize>& GetProgram() const ;
	eh_program_t<ptrsize>& GetProgram() ;

	const lsda_t<ptrsize>& GetLSDA() const { return lsda; }

	bool parse_fde(
		const uint32_t &fde_position, 
		const uint32_t &cie_position, 
		const uint8_t* const data, 
		const uint64_t max, 
		const uint64_t eh_addr,
		const libIRDB::DataScoop_t* gcc_except_scoop);

	void print() const;

	void build_ir(libIRDB::Instruction_t* insn, const OffsetMap_t &om, libIRDB::FileIR_t* firp) const;

};

template <int ptrsize>
bool operator<(const fde_contents_t<ptrsize>& a, const fde_contents_t<ptrsize>& b) { return a.GetFDEEndAddress()-1 < b.GetFDEStartAddress(); }


class split_eh_frame_t 
{
	public:

		virtual bool parse()=0;
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
	std::vector<cie_contents_t <ptrsize> > cies;
	std::set<fde_contents_t <ptrsize> > fdes;


	bool init_offset_map();

	bool iterate_fdes();

	public:

	split_eh_frame_impl_t(libIRDB::FileIR_t* p_firp);

	bool parse();

	void print() const;

	void build_ir() const;

	libIRDB::Instruction_t* find_lp(libIRDB::Instruction_t*) const ;
};

void split_eh_frame(libIRDB::FileIR_t* firp);

#endif
