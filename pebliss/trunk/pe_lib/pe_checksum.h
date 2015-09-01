#ifndef pebliss_pe_checksum_h
#define pebliss_pe_checksum_h
#pragma once
#include <istream>
#include "stdint_defs.h"

namespace pe_bliss
{
//Calculate checksum of image (performs no checks on PE structures)
uint32_t calculate_checksum(std::istream& file);
}
#endif
