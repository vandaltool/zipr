
#include <libIRDB-core.hpp>

#include <capstone.h>
#include <x86.h>
#include <string>
#include <functional>
#include <set>
#include <algorithm>
#include <stdexcept>

using namespace libIRDB;
using namespace std;

#define ALLOF(a) begin(a),end(a)

DecodedInstructionCapstone_t::CapstoneHandle_t* DecodedInstructionCapstone_t::cs_handle=NULL ;

DecodedInstructionCapstone_t::CapstoneHandle_t::CapstoneHandle_t(FileIR_t* firp)
{

	const auto width=FileIR_t::GetArchitectureBitWidth();
	const auto mode = (width==64) ? CS_MODE_64: CS_MODE_32;
	auto err = cs_open(CS_ARCH_X86, mode,  &handle);

	if (err)
	{
		const auto s=string("Failed on cs_open() with error returned: ")+to_string(err)+"\n";
		throw std::runtime_error(s);
	}
	cs_option(handle, CS_OPT_DETAIL, CS_OPT_ON);
	cs_option(handle, CS_OPT_SYNTAX, CS_OPT_SYNTAX_INTEL);


}



typedef struct special_instruction special_instruction_t;
struct special_instruction
{
	string binary;
	string mnemonic;
	string operands;
};

vector<special_instruction_t> cs_special_instructions=
	{
		{"\xdf\xc0", "ffreep", "st0"}
	};


// static helpers 
static inline bool hasPrefix(const cs_insn* the_insn, const x86_prefix desired_pref)
{
	const auto count=count_if(ALLOF(the_insn->detail->x86.prefix), [&](const uint8_t actual_pref) { return actual_pref==desired_pref; } ) ;
	return count!=0;
}

static bool isPartOfGroup(const cs_insn* the_insn, const x86_insn_group the_grp) 
{
	const auto grp_it=find(ALLOF(the_insn->detail->groups), the_grp);
	return grp_it!=end(the_insn->detail->groups);

}

static bool isJmp(cs_insn* the_insn) 
{
	return isPartOfGroup(the_insn,X86_GRP_JUMP);
}

#if 0
class CapstoneHandle_t
{
	public:
		CapstoneHandle_t(FileIR_t* firp=NULL)
		{

			const auto width=FileIR_t::GetArchitectureBitWidth();
			const auto mode = (width==64) ? CS_MODE_64: CS_MODE_32; 
	                auto err = cs_open(CS_ARCH_X86, mode,  &handle);

			if (err) 
			{
				const auto s=string("Failed on cs_open() with error returned: ")+to_string(err)+"\n";
				throw std::runtime_error(s);
			}
                	cs_option(handle, CS_OPT_DETAIL, CS_OPT_ON);
			cs_option(handle, CS_OPT_SYNTAX, CS_OPT_SYNTAX_INTEL);


		}
		inline csh getHandle() { return handle; }

	private:
		csh handle;
};
static CapstoneHandle_t *cs_handle=NULL;
#endif

template<class type>
static inline type insnToImmedHelper(cs_insn* the_insn, csh handle)
{
        const auto count = cs_op_count(handle, the_insn, X86_OP_IMM);
	const auto x86 = &(the_insn->detail->x86);

        if (count==0) 
		return 0;	 /* no immediate is the same as an immediate of 0, i guess? */
        else if (count==1) 
	{
		const auto index = cs_op_index(handle, the_insn, X86_OP_IMM, 1);
		return x86->operands[index].imm;
	}
	else
		throw std::logic_error(string("Called ")+__FUNCTION__+" with number of immedaites not equal 1");
}



// shared code 
// constructors, destructors, operators.

void DecodedInstructionCapstone_t::Disassemble(const virtual_offset_t start_addr, const void *data, const uint32_t max_len)
{
	using namespace std::placeholders;
	auto insn=cs_malloc(cs_handle->getHandle());
	auto address=(uint64_t)start_addr;
	auto size=(size_t)max_len;
	const uint8_t* code=(uint8_t*)data;
	const auto ok = cs_disasm_iter(cs_handle->getHandle(), &code, &size, &address, insn);
	if(!ok)
		insn->size=0;


	if(insn->size==0)
	{
		const auto special_it=find_if(ALLOF(cs_special_instructions), [&](const special_instruction_t& si)
			{
				if(si.binary.length() > max_len)
					return false;
				const auto data_as_str=string((char*)data,si.binary.length());
				return si.binary==data_as_str;
			});
		const auto is_special=special_it != end(cs_special_instructions);
		if(is_special)
		{
			// if we run into more complicated stuff, we may need to extend this
			insn->size=special_it->binary.length();
			strcpy(insn->mnemonic, special_it->mnemonic.c_str());
			strcpy(insn->op_str, special_it->operands.c_str());
		}
	
	}

	if(string(insn->mnemonic)=="fcompi")
		strcpy(insn->mnemonic, "fcomip"); // bad opcode out of capstone.


	const auto cs_freer=[](cs_insn * insn) -> void 
		{  
			cs_free(insn,1); 
		} ; 
	my_insn.reset(insn,cs_freer);
}

DecodedInstructionCapstone_t::DecodedInstructionCapstone_t(const Instruction_t* i)
{
	if(!cs_handle) cs_handle=new CapstoneHandle_t(NULL);
	if(!i) throw std::invalid_argument("No instruction given to DecodedInstruction_t(Instruction_t*)");

        const auto length=i->GetDataBits().size();
	const auto data=i->GetDataBits().data();
	const auto address=i->GetAddress()->GetVirtualOffset();
        Disassemble(address,data,length);

	if(!valid()) throw std::invalid_argument("The Insruction_t::GetDataBits field is not a valid instruction.");
}

DecodedInstructionCapstone_t::DecodedInstructionCapstone_t(const virtual_offset_t start_addr, const void *data, uint32_t max_len)
{
	if(!cs_handle) cs_handle=new CapstoneHandle_t(NULL);
        const auto endptr=data+max_len;
        Disassemble(start_addr, data, max_len);
}

DecodedInstructionCapstone_t::DecodedInstructionCapstone_t(const virtual_offset_t start_addr, const void *data, const void* endptr)
{
	if(!cs_handle) cs_handle=new CapstoneHandle_t(NULL);
        const auto length=(char*)endptr-(char*)data;
        Disassemble(start_addr,data,length);
}

DecodedInstructionCapstone_t::DecodedInstructionCapstone_t(const DecodedInstructionCapstone_t& copy)
{
	*this=copy;
}

DecodedInstructionCapstone_t::DecodedInstructionCapstone_t(const shared_ptr<void> &p_my_insn)
	: my_insn(p_my_insn)
{
}

DecodedInstructionCapstone_t::~DecodedInstructionCapstone_t()
{
	// no need to cs_free(my_insn) because shared pointer will do that for us!
}

DecodedInstructionCapstone_t& DecodedInstructionCapstone_t::operator=(const DecodedInstructionCapstone_t& copy)
{
	my_insn=copy.my_insn;
	return *this;
}

// public methods

string DecodedInstructionCapstone_t::getDisassembly() const
{
	const auto myReplace=[](std::string str,
		       const std::string& oldStr,
		       const std::string& newStr) -> string
	{
		std::string::size_type pos = 0u;
		while((pos = str.find(oldStr, pos)) != std::string::npos)
		{
			str.replace(pos, oldStr.length(), newStr);
			pos += newStr.length();
		}
		return str;
	};


	// a list of special mnemonics that can't have a mem decoration because nasm sux.
	const auto no_memdec_mnemonics=set<string>(
		{
		"lea",
		"movd",
		"pinsrw",
		"psllw",
		"pslld",
		"psllq",
		"psrlw",
		"psrld",
		"psrlq",
		"psraw",
		"psrad",
		});


	// a list of prefixes and suffixes of mnemonics that can't have a mem decoration because nasm sux.
	// helps deal with things like {mov,add,sub,mul,div, cmp}*{ss,sd,pd,pq}, esp when the cmp has a 
	// {l,le,ne,eq,g,gt} specifier.
	const auto prefixes=set<string>(
		{
			"mov",
			"movl",
			"cmp",
			"movh",
			"add",
			"sub",
			"mul",
			"div",
			"ucomi",
			"cvtsi2",
			"cvtpi2",
			"shuf",
		});
	const auto suffixes=set<string>(
		{
			"sd",
			"ss",
			"ps",
			"pd"
		});

	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	const auto full_str=getMnemonic()+" "+the_insn->op_str;

	const auto noptr=myReplace(full_str," ptr ", " ");
	const auto noxword=myReplace(noptr," xword ", " tword ");
	const auto noxmmword=myReplace(noxword," xmmword ", " ");


	const auto has_prefix_it=find_if(ALLOF(prefixes),[&](const string& prefix) 
		{ const auto s=getMnemonic(); return s.substr(0,prefix.size())==prefix; } );
	const auto has_prefix = has_prefix_it != prefixes.end();
	const auto has_suffix_it=find_if(ALLOF(suffixes),[&](const string& suffix) 
		{ const auto s=getMnemonic(); return s.rfind(suffix) == (s.size() - suffix.size()); } );
	const auto has_suffix = has_suffix_it != suffixes.end();

	const auto needs_memdec_special=no_memdec_mnemonics.find(getMnemonic())==end(no_memdec_mnemonics);
	const auto needs_memdec_shape=!(has_prefix && has_suffix);
	const auto needs_memdec=needs_memdec_special && needs_memdec_shape;

	const auto no_dec1=needs_memdec ? noxmmword : (myReplace(noxmmword," qword ", " ")) ;
	const auto no_dec2=needs_memdec ? no_dec1 : (myReplace(no_dec1," dword ", " ")) ;

	const auto no_rip=myReplace(no_dec2, "rip", to_string(the_insn->size));


	return no_rip;
}

bool DecodedInstructionCapstone_t::valid() const
{
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	return the_insn->size!=0;
}

uint32_t DecodedInstructionCapstone_t::length() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	return the_insn->size;
}

bool DecodedInstructionCapstone_t::isBranch() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	return isCall() || isReturn() || isJmp(the_insn);
}

bool DecodedInstructionCapstone_t::isCall() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");

	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	return isPartOfGroup(the_insn,X86_GRP_CALL);
}

bool DecodedInstructionCapstone_t::isUnconditionalBranch() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());

	return isJmp(the_insn) && !isConditionalBranch();
}

bool DecodedInstructionCapstone_t::isConditionalBranch() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	return isJmp(the_insn) && getMnemonic()!="jmp";
}

bool DecodedInstructionCapstone_t::isReturn() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	return isPartOfGroup(the_insn,X86_GRP_RET);
}

bool DecodedInstructionCapstone_t::hasOperand(const int op_num) const
{
	if(op_num<0) throw std::logic_error(string("Called ")+ __FUNCTION__+" with opnum="+to_string(op_num));
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	const auto &x86 = (the_insn->detail->x86);
	return 0 <= op_num  && op_num < x86.op_count;
}

// 0-based.  first operand is numbered 0.
DecodedOperandCapstone_t DecodedInstructionCapstone_t::getOperand(const int op_num) const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	if(!hasOperand(op_num)) throw std::logic_error(string("Called ")+__FUNCTION__+" on without hasOperand()==true");

	return DecodedOperandCapstone_t(my_insn,(uint8_t)op_num);
	
}

DecodedOperandCapstoneVector_t DecodedInstructionCapstone_t::getOperands() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	const auto &x86 = (the_insn->detail->x86);
	const auto opcount=x86.op_count;

	auto ret_val=DecodedOperandCapstoneVector_t();
	
	for(auto i=0;i<opcount;i++)
		ret_val.push_back(getOperand(i));

	return ret_val;
}

string DecodedInstructionCapstone_t::getMnemonic() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");

	// list of opcodes to rename capstone to bea engine names
	typedef pair<string,string>  renamer_t; 
	const auto renamer=set<renamer_t>(
	{
		/* bea (right), capstone */
		{ "mov", "movabs" } ,
 		{ "fucomip", "fucompi" }, 
 //		{ "cmovnb", "cmovae" }, 
 //		{ "cmovnbe", "cmova" }, 
 //		{ "cmovnle", "cmovg" }, 
//		{ "jnbe", "ja"},
//		{ "jnc", "jae"},
//		{ "jng", "jle"},
//		{ "jnl", "jge"},
//		{ "jnle", "jg"},
 //		{ "movsd", "movsd" }, 
 //		{ "setnbe", "seta" }, 
 //		{ "setnle", "setg" }, 

	});


	// get the cs insn via casting.
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	
	// get mnemonic as a string
	auto mnemonic=string(the_insn->mnemonic);

	// remove any prefixes by finding the last space and removing anything before it.
	const auto space_pos=mnemonic.rfind(" ");
	if(space_pos!=string::npos)
		mnemonic.erase(0,space_pos+1);
		
	// check if it needs a rename.
	const auto finder_it=find_if(ALLOF(renamer), [&](const renamer_t& r) { return r.second==the_insn->mnemonic; } );
	if(finder_it!=end(renamer)) 
		mnemonic=finder_it->first;

	// return the new string.
	return mnemonic;

	
}


int64_t DecodedInstructionCapstone_t::getImmediate() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");

	const auto the_insn=static_cast<cs_insn*>(my_insn.get());

	// direct calls and jumps have a "immediate" which is really an address"  those are returned by getAddress
	if(isCall() || isJmp(the_insn))
		return 0;

	return insnToImmedHelper<int64_t>(the_insn, cs_handle->getHandle());
}


virtual_offset_t DecodedInstructionCapstone_t::getAddress() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");

	const auto the_insn=static_cast<cs_insn*>(my_insn.get());

	if(isCall() || isJmp(the_insn))
		return insnToImmedHelper<virtual_offset_t>(the_insn, cs_handle->getHandle());


        const auto count = cs_op_count(cs_handle->getHandle(), the_insn, X86_OP_MEM);
	const auto x86 = &(the_insn->detail->x86);

        if (count==0) 
		return 0;	 /* no immediate is the same as an immediate of 0, i guess? */
        else if (count==1) 
	{
		const auto index = cs_op_index(cs_handle->getHandle(), the_insn, X86_OP_MEM, 1);
		if(x86->operands[index].mem.base==X86_REG_RIP)
			return X86_REL_ADDR(*the_insn);
		if(getMnemonic()=="lea")
			return 0;
		return x86->operands[index].mem.disp;
	}
	else
		return 0;
}


bool DecodedInstructionCapstone_t::setsStackPointer() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");

	const auto the_insn=static_cast<cs_insn*>(my_insn.get());

/* slow string manip */
        if(getMnemonic()=="push")
                return true;
        if(getMnemonic()=="pop")
                return true;
        if(getMnemonic()=="call")
                return true;

	// any sp reg
	const auto sp_regs=set<x86_reg>({X86_REG_RSP, X86_REG_ESP, X86_REG_SP, X86_REG_SPL});

	// check the implicit regs.
	const auto regs_write_end=begin(the_insn->detail->regs_write)+the_insn->detail->regs_write_count;
	const auto implicit_regs_it=find_if
		(
			begin(the_insn->detail->regs_write),
			regs_write_end,
			[sp_regs](const uint8_t actual_reg) 
			{ 
				return sp_regs.find((x86_reg)actual_reg)!=sp_regs.end();
			} 
		) ;
	if(implicit_regs_it!=regs_write_end)	
		return true;


	// now check each operand
	const auto operands_begin=begin(the_insn->detail->x86.operands);
	const auto operands_end=begin(the_insn->detail->x86.operands)+the_insn->detail->x86.op_count;
	const auto rsp_it=find_if
		(
			begin(the_insn->detail->x86.operands), 
			operands_end,
			[sp_regs](const cs_x86_op& op)
			{
				return op.type==X86_OP_REG && sp_regs.find(op.reg)!=sp_regs.end();
			}
		);
	// not found in the operands, we're done.
	if(rsp_it==operands_end)	
	        return false;

	// now decide if it's a register that's read or written.

	// special check for things without a destation (as op[0])
	if(getMnemonic()=="push" || 
	   getMnemonic()=="cmp"  ||
	   getMnemonic()=="call" ||
	   getMnemonic()=="test")
		return false;

	// standard check -- is the reg op 0
	const auto is_op0=(rsp_it==operands_begin);

	return is_op0;
}

uint32_t DecodedInstructionCapstone_t::getPrefixCount() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");


	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	const auto count=count_if(ALLOF(the_insn->detail->x86.prefix), [](const uint8_t pref) { return pref!=0x0; } ) ;
	const auto count_with_rex = the_insn->detail->x86.rex == 0 ? count : count + 1;
	return count_with_rex;
}

virtual_offset_t DecodedInstructionCapstone_t::getMemoryDisplacementOffset(const DecodedOperandCapstone_t& t, const Instruction_t* insn) const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");

	const auto the_insn=static_cast<cs_insn*>(my_insn.get());

	const auto encoding_size=t.getMemoryDisplacementEncodingSize();
	const auto x86 = &(the_insn->detail->x86);
        const auto imm_count = cs_op_count(cs_handle->getHandle(), the_insn, X86_OP_IMM);
	const auto disp_size=t.getMemoryDisplacementEncodingSize();
	const auto imm=getImmediate();
	const auto disp=t.getMemoryDisplacement();

	if(imm_count==0)
		return the_insn->size - disp_size;

	// shifts can an immediate that's note encoded in the instruction
	// but never an immediate encoded in the instruction.
	if(
	   string((char*)the_insn->detail->x86.opcode)=="\xd0" || // shift left or right with an immediate that's not actually  encoded in the insn
	   string((char*)the_insn->detail->x86.opcode)=="\xd1" 
	  )
		return the_insn->size - disp_size;

	const auto possible_imm_sizes= string(the_insn->mnemonic)=="movabs" ?  set<int>({8}) : set<int>({1,2,4});

	for(const auto imm_size : possible_imm_sizes)
	{
		if(the_insn->size < disp_size + imm_size)
			continue;

		const auto disp_start=the_insn->size-imm_size-disp_size;
		const auto imm_start=the_insn->size-imm_size;
		
		const auto candidate_disp_eq = disp_size==4  ? *(int32_t*)(&insn->GetDataBits().c_str()[disp_start])==(int32_t)disp :
		                               disp_size==2  ? *(int16_t*)(&insn->GetDataBits().c_str()[disp_start])==(int16_t)disp :
		                               disp_size==1  ? *(int8_t *)(&insn->GetDataBits().c_str()[disp_start])==(int8_t )disp : (assert(0),false);

		const auto candidate_imm_eq = imm_size==8  ? *(int64_t*)(&insn->GetDataBits().c_str()[imm_start])==(int64_t)imm :
					      imm_size==4  ? *(int32_t*)(&insn->GetDataBits().c_str()[imm_start])==(int32_t)imm :
					      imm_size==2  ? *(int16_t*)(&insn->GetDataBits().c_str()[imm_start])==(int16_t)imm :
		                              imm_size==1  ? *(int8_t *)(&insn->GetDataBits().c_str()[imm_start])==(int8_t )imm : (int64_t)(assert(0),false);

		if(candidate_disp_eq && candidate_imm_eq)
			return disp_start;
		
	}

	assert(0);


}



bool DecodedInstructionCapstone_t::hasRelevantRepPrefix() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	return hasPrefix(the_insn,X86_PREFIX_REP);
}

bool DecodedInstructionCapstone_t::hasRelevantRepnePrefix() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	return hasPrefix(the_insn,X86_PREFIX_REPNE);
}

bool DecodedInstructionCapstone_t::hasRelevantOperandSizePrefix() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	return hasPrefix(the_insn,X86_PREFIX_OPSIZE);
}

bool DecodedInstructionCapstone_t::hasRexWPrefix() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	return (the_insn->detail->x86.rex & 0x8) == 0x8;
}

bool DecodedInstructionCapstone_t::hasImplicitlyModifiedRegs() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");

	const auto the_insn=static_cast<cs_insn*>(my_insn.get());


	const auto count=count_if
		(
			begin(the_insn->detail->regs_write),
			begin(the_insn->detail->regs_write)+the_insn->detail->regs_write_count,
			[](const uint8_t actual_pref) 
			{ 
				return actual_pref!=X86_REG_EFLAGS; 
			} 
		) ;
	
	return count>0;

}

