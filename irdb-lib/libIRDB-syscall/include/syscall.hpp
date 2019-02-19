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

namespace libIRDB
{

	class  SyscallSite_t : public IRDB_SDK::SyscallSite_t
	{
		public:
			SyscallSite_t(IRDB_SDK::Instruction_t* p_site, IRDB_SDK::SyscallNumber_t p_num) : site(p_site), num(p_num) {}

			IRDB_SDK::Instruction_t*  getSyscallSite()   const { return site; }
			IRDB_SDK::Instruction_t*  getSite()          const { return site; }
			IRDB_SDK::SyscallNumber_t getSyscallNumber() const { return num; }

		private:
			IRDB_SDK::Instruction_t* site;
			IRDB_SDK::SyscallNumber_t num;
	};

	class Syscalls_t : public IRDB_SDK::Syscalls_t
	{
		public:
			Syscalls_t(IRDB_SDK::FileIR_t *the_firp=NULL) { if(the_firp) FindSystemCalls(the_firp); }
			virtual ~Syscalls_t();



			const IRDB_SDK::SyscallSiteSet_t& getSyscalls() {return syscalls;}
		protected:
			IRDB_SDK::SyscallNumber_t FindSystemCallNumber(IRDB_SDK::Instruction_t* insn, 
				const IRDB_SDK::InstructionPredecessors_t& preds);

			bool FindSystemCalls(const IRDB_SDK::FileIR_t* firp);
		private:
			IRDB_SDK::SyscallSiteSet_t syscalls;
	};

}
#endif

