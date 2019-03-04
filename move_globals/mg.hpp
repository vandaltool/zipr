#ifndef _LIBTRANSFORM_INSTRUCTIONCOUNT_H_
#define _LIBTRANSFORM_INSTRUCTIONCOUNT_H_

#include <irdb-transform>
#include <irdb-deep>
#include <memory>
#include <map>
#include <set>
#include <tuple>

#include <libStructDiv.h>
#include <exeio.h>

#include <elfio/elfio.hpp>
#include <elfio/elfio_symbols.hpp>
#include <elf.h>

using Elf_Xword = uint64_t;
using Elf_Half  = uint16_t;

template <class T_Sym, class T_Rela, class T_Rel, class T_Dyn, class Extractor>
class MoveGlobals_t : public IRDB_SDK::Transform
{
	public:
		MoveGlobals_t(IRDB_SDK::VariantID_t *p_variantID, 
			IRDB_SDK::FileIR_t*p_variantIR,
			const std::string &p_dont_move, 
			const std::string  &p_move_only,
			const int p_max_moveables,
                        const bool p_random,
			const bool p_aggrssive,
			const bool p_use_stars=false); 

		int execute(IRDB_SDK::pqxxDB_t &pqxx_interface);

	private:

		// MEDS_Annotation::MEDS_Annotations_t& getAnnotations();
		
		void ParseSyms(EXEIO::exeio * reader);
		void SetupScoopMap();
		void FilterScoops();
		void TieScoops();
		void FindInstructionReferences();
		void FindDataReferences();
		void UpdateScoopLocations();
		void FilterAndCoalesceTiedScoops();
		IRDB_SDK::Relocation_t* FindRelocationWithType(IRDB_SDK::BaseObj_t* obj, std::string type);
		void PrintStats();
		void HandleMemoryOperand(IRDB_SDK::DecodedInstruction_t &disasm, const IRDB_SDK::DecodedOperandVector_t::iterator the_arg, 
		                         IRDB_SDK::Instruction_t* insn, const IRDB_SDK::DecodedOperandVector_t &the_arg_container);
		void HandleImmediateOperand(const IRDB_SDK::DecodedInstruction_t& disasm, const IRDB_SDK::DecodedOperandVector_t::iterator the_arg, IRDB_SDK::Instruction_t* insn);
		IRDB_SDK::DataScoop_t* DetectAnnotationScoop(IRDB_SDK::Instruction_t* insn);
		IRDB_SDK::DataScoop_t* DetectProperScoop(const IRDB_SDK::DecodedInstruction_t& disasm, const IRDB_SDK::DecodedOperandVector_t::iterator the_arg, IRDB_SDK::Instruction_t* insn, 
				IRDB_SDK::VirtualOffset_t immed_addr, bool immed, const IRDB_SDK::DecodedOperandVector_t &the_arg_container);

		IRDB_SDK::DataScoop_t* DetectProperScoop_ConsiderEndOfPrev(const IRDB_SDK::DecodedInstruction_t& disasm, const IRDB_SDK::DecodedOperandVector_t::iterator the_arg, IRDB_SDK::Instruction_t* insn, 
			IRDB_SDK::VirtualOffset_t insn_addr, bool immed, IRDB_SDK::DataScoop_t* ret, const IRDB_SDK::DecodedOperandVector_t &the_arg_container);
		IRDB_SDK::DataScoop_t* DetectProperScoop_ConsiderStartOfNext(const IRDB_SDK::DecodedInstruction_t& disasm, const IRDB_SDK::DecodedOperandVector_t::iterator the_arg, IRDB_SDK::Instruction_t* insn, 
			IRDB_SDK::VirtualOffset_t insn_addr, bool immed, IRDB_SDK::DataScoop_t* cand_scoop, const IRDB_SDK::DecodedOperandVector_t &the_arg_container);

		void ApplyImmediateRelocation(IRDB_SDK::Instruction_t *insn, IRDB_SDK::DataScoop_t* to);
		void ApplyAbsoluteMemoryRelocation(IRDB_SDK::Instruction_t *insn, IRDB_SDK::DataScoop_t* to);
		void ApplyPcrelMemoryRelocation(IRDB_SDK::Instruction_t *insn, IRDB_SDK::DataScoop_t* to);
		void ApplyDataRelocation(IRDB_SDK::DataScoop_t *from, unsigned int offset, IRDB_SDK::DataScoop_t* to);
		IRDB_SDK::DataScoop_t* findScoopByAddress(const IRDB_SDK::VirtualOffset_t a) const;
		bool AreScoopsAdjacent(const IRDB_SDK::DataScoop_t *a, const IRDB_SDK::DataScoop_t *b) const;


		libStructDiv::StructuredDiversity_t *struct_div;

		std::vector<T_Sym> static_symbols;
		std::vector<T_Sym> dynamic_symbols;
		EXEIO::exeio* exe_reader;

		struct cmpByName {
			bool operator()(const IRDB_SDK::DataScoop_t* a, const IRDB_SDK::DataScoop_t* b) const {

				return (a->getName() < b->getName());
			}
		};
		std::set<IRDB_SDK::DataScoop_t*> moveable_scoops;
		std::map<IRDB_SDK::DataScoop_t*,unsigned int> reasons;

		using ScoopPair_t = std::pair<IRDB_SDK::DataScoop_t*,IRDB_SDK::DataScoop_t*>;

		std::set<ScoopPair_t> tied_scoops;

		// sets to record what insns need to be fixed later.
		struct Insn_fixup
		{ 
			IRDB_SDK::Instruction_t* from; IRDB_SDK::DataScoop_t* to;

			bool operator <(const struct Insn_fixup& rhs) const
			{
				return std::tie(from, to) < std::tie(rhs.from, rhs.to);
			}
		};
		using Insn_fixup_t = struct Insn_fixup;
		std::set<Insn_fixup_t> pcrel_refs_to_scoops, absolute_refs_to_scoops, immed_refs_to_scoops;


		// data references to scoops
		struct Scoop_fixup
		{ 
			IRDB_SDK::DataScoop_t* from; unsigned int offset; IRDB_SDK::DataScoop_t* to;

			bool operator <(const struct Scoop_fixup & rhs) const
			{
				return std::tie(from, offset, to) < std::tie(rhs.from, rhs.offset, rhs.to);
			}
		};
		using Scoop_fixup_t = struct Scoop_fixup;;
		std::set<Scoop_fixup_t> data_refs_to_scoops;


		int tied_unpinned;
		int tied_pinned;
		int tied_nochange;
		int ties_for_folded_constants;

		const std::string dont_move;
		const std::string move_only;

		using RangePair_t = std::pair<IRDB_SDK::VirtualOffset_t,IRDB_SDK::VirtualOffset_t>;
		struct cmpByRange 
		{
			bool operator()(const RangePair_t& a, const RangePair_t& b) const {

				return (a.second < b.first);
			}
		};
		std::map<RangePair_t, IRDB_SDK::DataScoop_t*, cmpByRange> scoop_map;


		const int max_moveables;
		const bool random;
		const bool aggressive;
		const bool m_use_stars;


		std::unique_ptr<IRDB_SDK::StaticGlobalStartMap_t > deep_global_static_ranges;
		std::unique_ptr<IRDB_SDK::RangeSentinelSet_t> sentinels;


};

class Extractor64_t
{
        public:
                Elf_Xword  elf_r_sym (Elf_Xword a) { return ELF64_R_SYM (a); }
                Elf_Xword  elf_r_type(Elf_Xword a) { return ELF64_R_TYPE(a); }
                unsigned char  elf_st_bind(unsigned char a) { return ELF64_ST_BIND(a); }
                unsigned char  elf_st_type(unsigned char a) { return ELF64_ST_TYPE(a); }
};
class Extractor32_t
{
        public:
                Elf32_Word  elf_r_sym (Elf32_Word a) { return ELF32_R_SYM (a); }
                Elf32_Word  elf_r_type(Elf32_Word a) { return ELF32_R_TYPE(a); }
                unsigned char  elf_st_bind(unsigned char a) { return ELF32_ST_BIND(a); }
                unsigned char  elf_st_type(unsigned char a) { return ELF32_ST_TYPE(a); }
};


const static auto elftable_names= std::set<std::string> ({".dynamic",".got",".got.plt",".dynstr",".dynsym",".rel.dyn",".rela.dyn",".rel.plt",".rela.plt", ".gnu.version", ".gnu_version_r"});
const static auto elftable_nocodeptr_names= std::set<std::string> ({".dynamic"});

using MoveGlobals32_t = class MoveGlobals_t<Elf32_Sym, Elf32_Rela, Elf32_Rel, Elf32_Dyn, Extractor32_t>;
using MoveGlobals64_t = class MoveGlobals_t<Elf64_Sym, Elf64_Rela, Elf64_Rel, Elf64_Dyn, Extractor64_t>;



#endif

