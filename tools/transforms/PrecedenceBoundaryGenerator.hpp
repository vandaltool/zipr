#ifndef __PRECEDENCEBOUNDGEN
#define __PRECEDENCEBOUNDGEN
#include <libIRDB-core.hpp>
#include <libIRDB-cfg.hpp>
#include "Range.hpp"

class PrecedenceBoundaryGenerator
{
public:
    virtual ~PrecedenceBoundaryGenerator(){}
    virtual std::vector<Range> GetBoundaries(libIRDB::Function_t *func)=0;
};

#endif
