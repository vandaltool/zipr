
#ifndef check_thunks_hpp
#define check_thunks_hpp

#include <set>
#include "libIRDB-core.hpp"

void find_all_module_starts(libIRDB::FileIR_t* firp, std::set<int>& thunk_bases);
void check_for_thunks(libIRDB::FileIR_t* firp, const std::set<int>& thunk_bases);


bool possible_target(int p, uintptr_t at=0);


#endif

