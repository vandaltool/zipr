#ifndef ElfWriter_h
#define ElfWriter_h

#include <iostream>

class EhWriter_t 
{	 
	public: 
	virtual void GenerateNewEhInfo() =0;
};


static const std::string ehframe_s_filename="ehframe.s";
static const std::string ehframe_exe_filename="ehframe.exe";

template <int ptrsize> 
class EhWriterImpl_t : public EhWriter_t
{
	private:

	class EhProgramListingManip_t : public libIRDB::EhProgramListing_t
	{
		public:
		EhProgramListingManip_t(){}
		EhProgramListingManip_t(const libIRDB::EhProgramListing_t &pgm) : libIRDB::EhProgramListing_t(pgm) { }
		bool canExtend(const EhProgramListingManip_t &other);
		void extend(const uint64_t inc_amt, const EhProgramListingManip_t &other);
		bool isAdvanceDirective(const std::string &s) const;
		private:
		int getMergeIndex(const EhProgramListingManip_t &other);

		static const int DW_CFA_advance_loc1 = 0x02;
		static const int DW_CFA_advance_loc2 = 0x03;
		static const int DW_CFA_advance_loc4 = 0x04;

	};

	class FDErepresentation_t;	// forward decl
	class CIErepresentation_t
	{
		public:
		CIErepresentation_t(libIRDB::Instruction_t*, EhWriterImpl_t<ptrsize>* ehw);
		void emitAssembly(std::ostream& out) {}
		bool canSupport(libIRDB::Instruction_t* insn) const;

		private:
			// need nothing?

		EhProgramListingManip_t pgm;
		uint64_t code_alignment_factor;
		int64_t data_alignment_factor;
		uint64_t return_reg;
		libIRDB::Relocation_t* personality_reloc;

		mutable bool has_been_output;

		friend class FDErepresentation_t;
		friend EhWriterImpl_t<ptrsize>;	
	};


	class FDErepresentation_t
	{
		private:

		class LSDArepresentation_t
		{
			public:
			LSDArepresentation_t(libIRDB::Instruction_t* insn);
			void extend(libIRDB::Instruction_t* insn);
			bool canExtend(libIRDB::Instruction_t* insn) const;
			bool exists()  const  { return callsite_table.size() > 0; }

			struct call_site_t
			{
				libIRDB::Instruction_t* cs_insn_start;
				libIRDB::Instruction_t* cs_insn_end;
				libIRDB::Instruction_t* landing_pad;
				int action_table_index;
				libIRDB::TTOrderVector_t actions;
			};
			std::vector<call_site_t> callsite_table;
			std::vector<libIRDB::TTOrderVector_t> action_table;
			std::vector<libIRDB::Relocation_t*> type_table;
			uint64_t tt_encoding;

		};

		LSDArepresentation_t lsda;
		CIErepresentation_t* cie;
		libIRDB::virtual_offset_t start_addr;
		libIRDB::virtual_offset_t end_addr;
		libIRDB::virtual_offset_t last_advance_addr;
		// the pgm for the fde
		EhProgramListingManip_t pgm;

		public:
		FDErepresentation_t(libIRDB::Instruction_t* insn, EhWriterImpl_t<ptrsize>* ehw);
		void extend(libIRDB::Instruction_t* insn, EhWriterImpl_t<ptrsize>* ehw);
		void emitAssembly(std::ostream& out);
		bool canExtend(libIRDB::Instruction_t* insn, EhWriterImpl_t<ptrsize>* ehw);
		bool hasLsda() const { return lsda.exists(); }

		friend EhWriterImpl_t<ptrsize>;	
	};

	friend class FDErepresentation_t;
	friend class CIErepresentation_t;

	ZiprImpl_t& zipr_obj;
	std::vector<FDErepresentation_t*> all_fdes;
	std::vector<CIErepresentation_t*> all_cies;

        void BuildFDEs();
        void GenerateEhOutput();
        void CompileEhOutput();
        void ScoopifyEhOutput();

	public:
	EhWriterImpl_t(ZiprImpl_t& p_zipr_obj) 
		: zipr_obj(p_zipr_obj)
	
	{ 
	}

	virtual ~EhWriterImpl_t();

	void GenerateNewEhInfo();

};

#endif
