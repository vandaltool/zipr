/*
 * Copyright (c) 2014 - Zephyr Software
 *
 * This file may be used and modified for non-commercial purposes as long as
 * all copyright, permission, and nonwarranty notices are preserved.
 * Redistribution is prohibited without prior written consent from Zephyr
 * Software.
 *
 * Please contact the authors for restrictions applying to commercial use.
 *
 * THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: Zephyr Software
 * e-mail: jwd@zephyr-software.com
 * URL   : http://www.zephyr-software.com/
 *
 */

#ifndef irdb_syscall_hpp
#define irdb_syscall_hpp

typedef enum 
{
	SNT_Unknown=-1,
#ifdef CGC
       SNT_terminate=1,
       SNT_transmit=2,
       SNT_receive=3,
       SNT_fdwait=4,
       SNT_allocate=5,
       SNT_deallocate=6,
       SNT_random=7
#else
/* LINUX here? */
#endif
} SyscallNumber_t;

class  SyscallSite_t
{
	public:
		SyscallSite_t(libIRDB::Instruction_t* p_site, libIRDB::SyscallNumber_t p_num) : site(p_site), num(p_num) {}

		bool operator<(const libIRDB::SyscallSite_t& rhs) const { return this->site < rhs.site; }
		libIRDB::Instruction_t* GetSyscallSite() { return site; }
		libIRDB::Instruction_t* GetSite() const { return site; }
		libIRDB::SyscallNumber_t GetSyscallNumber() const { return num; }
	private:
		libIRDB::Instruction_t* site;
		libIRDB::SyscallNumber_t num;
};
typedef std::set<SyscallSite_t>  SyscallSiteSet_t;

class Syscalls_t
{
	public:
		Syscalls_t(FileIR_t *the_firp=NULL) { if(the_firp) FindSystemCalls(the_firp); }


		bool FindSystemCalls(const libIRDB::FileIR_t* firp);

		const libIRDB::SyscallSiteSet_t& GetSyscalls() {return syscalls;}
	protected:
		libIRDB::SyscallNumber_t FindSystemCallNumber(libIRDB::Instruction_t* insn, 
			const libIRDB::InstructionPredecessors_t& preds);

	private:
		libIRDB::SyscallSiteSet_t syscalls;
};

#endif

