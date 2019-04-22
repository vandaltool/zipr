#ifndef EhWriter_h
#define EhWriter_h

#include <iostream>
#include <irdb-core>

namespace EhWriter
{

	using namespace std;
	using namespace IRDB_SDK;

	class EhWriter_t 
	{	 
		protected:
			ZiprImpl_t& zipr_obj;
			VirtualOffset_t eh_addr=0;

			EhWriter_t(ZiprImpl_t& p_zipr_obj)
				: 
					zipr_obj(p_zipr_obj)
			{

			}

			void SetEhAddr()
			{

				auto page_round_up=[](const uintptr_t x) -> uintptr_t
				{
					auto page_size=(uintptr_t)PAGE_SIZE;
					return  ( (((uintptr_t)(x)) + page_size-1)  & (~(page_size-1)) );
				};

				// find maximum used scoop address.
				const auto max_used_addr=max_element(
					zipr_obj.getFileIR()->getDataScoops().begin(),
					zipr_obj.getFileIR()->getDataScoops().end(),
					[&](const DataScoop_t* a, const DataScoop_t* b)
					{
						assert(a && b && a->getEnd() && b->getEnd()) ;
						return a->getEnd()->getVirtualOffset() < b->getEnd()->getVirtualOffset();
					}
					);

				// round it up and stringify it.
				eh_addr=page_round_up((*max_used_addr)->getEnd()->getVirtualOffset());
			}


		public: 
			virtual ~EhWriter_t() {}
			virtual void GenerateNewEhInfo() =0;

		static unique_ptr<EhWriter_t> factory(ZiprImpl_t& p_zipr_obj);
	};

	class PE_unwind_info_t
	{
		private:
			VirtualOffset_t start_rva = 0;
			VirtualOffset_t end_rva   = 0;
			union
			{
				struct
				{
					uint8_t	version:3;
					uint8_t	flags:5;
				} parts;
				uint8_t whole;
			} ver_and_flags = {};
			uint8_t	prolog_sz = 0;
			union
			{
				struct
				{
					uint8_t	frame_reg:4;
					uint8_t	frame_reg_offset:4;
				} parts;
				uint8_t whole;
			} frame_reg_and_offset = {};
			vector<uint16_t> unwind_array;

			struct 
			{
				uint32_t         handler_rva;
				vector<uint8_t>  user_data;
			} handle = {};

			ZiprImpl_t& zipr_obj;

		public:
			PE_unwind_info_t(Instruction_t* insns, ZiprImpl_t& zipr_obj);

			bool canExtend(Instruction_t* insn) const
			{
				const auto o = PE_unwind_info_t(insn, zipr_obj);
				return tie(  ver_and_flags.whole,   prolog_sz,   frame_reg_and_offset.whole,   unwind_array,   handle.handler_rva,   handle.user_data) ==
				       tie(o.ver_and_flags.whole, o.prolog_sz, o.frame_reg_and_offset.whole, o.unwind_array, o.handle.handler_rva, o.handle.user_data) ;
			}

			void extend(Instruction_t* insn)
			{
				const auto &loc_map = *zipr_obj.getLocationMap();
				end_rva             = loc_map.at(insn) - zipr_obj.getFileIR()->getArchitecture()->getFileBase() + insn->getDataBits().size();
			}

			void extend(const VirtualOffset_t to_addr)
			{
				end_rva = to_addr - zipr_obj.getFileIR()->getArchitecture()->getFileBase();
			}

			string get_raw_bytes() const
			{
				const auto unwind_sz = static_cast<uint8_t>(unwind_array.size());

				auto ret = string();
				ret += string(reinterpret_cast<const char*>(&ver_and_flags.whole)       , 1) ; 
				ret += string(reinterpret_cast<const char*>(&prolog_sz)                 , 1) ;
				ret += string(reinterpret_cast<const char*>(&unwind_sz)                 , 1) ; 
				ret += string(reinterpret_cast<const char*>(&frame_reg_and_offset.whole), 1) ;
				ret += string(reinterpret_cast<const char*>(unwind_array.data())        , 2*unwind_array.size());

				// may need to pad the unwind array.
				if( (unwind_sz & 0b1) == 0xb1 )
					ret += string(2, '\0');


				ret += string(reinterpret_cast<const char*>(&handle.handler_rva)    , 4);
				ret += string(reinterpret_cast<const char*>(handle.user_data.data()), handle.user_data.size());

				return move(ret);

			}

			VirtualOffset_t getStartRVA() const { return start_rva; }
			VirtualOffset_t getEndRVA()   const { return end_rva; }


	};

	template <int ptrsize> 
	class PEEhWriter_t : public EhWriter_t
	{
		private:
			using SehVector_t = vector<unique_ptr<PE_unwind_info_t> > ;

			SehVector_t BuildSehVector();
			void LayoutSehVector (const SehVector_t& seh_vector);

		public:
			PEEhWriter_t(ZiprImpl_t& p_zipr_obj) 
				: EhWriter_t(p_zipr_obj)
			
			{ 
			}

			virtual ~PEEhWriter_t() {}

			void GenerateNewEhInfo();

	};

	template <int ptrsize> 
	class ElfEhWriter_t : public EhWriter_t
	{

		private:

			const string ehframe_s_filename="ehframe.s";
			const string ehframe_exe_filename="ehframe.exe";

			class EhProgramListingManip_t : public EhProgramListing_t
			{
				public:
				EhProgramListingManip_t(){}
				EhProgramListingManip_t(const EhProgramListing_t &pgm) : EhProgramListing_t(pgm) { }
				bool canExtend(const EhProgramListingManip_t &other);
				void extend(const uint64_t inc_amt, const EhProgramListingManip_t &other);
				bool isAdvanceDirective(const string &s) const;
				string getPrintableString(const string &s) const;
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
				CIErepresentation_t(Instruction_t*, ElfEhWriter_t<ptrsize>* ehw);
				bool canSupport(Instruction_t* insn) const;
				Relocation_t* GetPersonalityReloc() const { return personality_reloc;}

				private:
					// need nothing?

				EhProgramListingManip_t pgm;
				uint64_t code_alignment_factor;
				int64_t data_alignment_factor;
				uint64_t return_reg;
				Relocation_t* personality_reloc;

				mutable bool has_been_output;

				friend class FDErepresentation_t;
				friend ElfEhWriter_t<ptrsize>;	
			};

			static void print_pers(Instruction_t* insn, CIErepresentation_t *cie);


			class FDErepresentation_t
			{
				private:

				class LSDArepresentation_t
				{
					public:
					LSDArepresentation_t(Instruction_t* insn);
					void extend(Instruction_t* insn);
					bool canExtend(Instruction_t* insn) const;
					bool exists()  const  { return callsite_table.size() > 0; }

					struct call_site_t
					{
						Instruction_t* cs_insn_start;
						Instruction_t* cs_insn_end;
						Instruction_t* landing_pad;
						int action_table_index;
						TTOrderVector_t actions;
					};
					vector<call_site_t> callsite_table;
					vector<TTOrderVector_t> action_table;
					vector<Relocation_t*> type_table;
					uint64_t tt_encoding;

				};

				LSDArepresentation_t lsda;
				CIErepresentation_t* cie;
				VirtualOffset_t start_addr;
				VirtualOffset_t end_addr;
				VirtualOffset_t last_advance_addr;
				// the pgm for the fde
				EhProgramListingManip_t pgm;

				public:
				FDErepresentation_t(Instruction_t* insn, ElfEhWriter_t<ptrsize>* ehw);
				void extend(Instruction_t* insn, ElfEhWriter_t<ptrsize>* ehw);
				bool canExtend(Instruction_t* insn, ElfEhWriter_t<ptrsize>* ehw);
				bool hasLsda() const { return lsda.exists(); }

				friend ElfEhWriter_t<ptrsize>;	
			};

			friend class FDErepresentation_t;
			friend class CIErepresentation_t;

			vector<FDErepresentation_t*> all_fdes;
			vector<CIErepresentation_t*> all_cies;

			VirtualOffset_t eh_frame_hdr_addr; // where the eh frame hdr will land.
			string asm_comment;

			void BuildFDEs();
			void GenerateEhOutput();
			void CompileEhOutput();
			void ScoopifyEhOutput();

		public:
			ElfEhWriter_t(ZiprImpl_t& p_zipr_obj) 
				: EhWriter_t(p_zipr_obj)
			
			{ 
				asm_comment = p_zipr_obj.getFileIR()->getArchitecture()->getMachineType()==admtAarch64 ?  " // " : " # " ;
			}

			virtual ~ElfEhWriter_t();

			void GenerateNewEhInfo();

	};
}

#endif
