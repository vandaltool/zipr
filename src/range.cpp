#include <zipr-sdk>

using namespace Zipr_SDK;

Zipr_SDK::Range_t::Range_t(RangeAddress_t p_s, RangeAddress_t p_e) 
	: 
		m_start(p_s), 
		m_end(p_e) 
{ 
}

Zipr_SDK::Range_t::Range_t() : m_start(0), m_end(0) 
{ 
}

Zipr_SDK::Range_t::~Range_t() 
{
}

RangeAddress_t Zipr_SDK::Range_t::getStart() const 
{ 
	return m_start; 
}

RangeAddress_t Zipr_SDK::Range_t::getEnd() const 
{ 
	return m_end; 
}

void Zipr_SDK::Range_t::setStart(RangeAddress_t s) 
{ 
	m_start=s; 
}

void Zipr_SDK::Range_t::setEnd(RangeAddress_t e) 
{ 
	m_end=e; 
}

bool Zipr_SDK::Range_t::is2ByteRange() const
{
	return (m_end - m_start) == 2;
};

bool Zipr_SDK::Range_t::is5ByteRange() const
{
	return (m_end - m_start) == 5;
};

bool Zipr_SDK::Range_t::isInfiniteRange() const
{
	return m_end==(RangeAddress_t)-1;
};

// Caution!  Only works for NON-OVERLAPPING ranges.
bool Zipr_SDK::Range_tCompare::operator() (const Range_t first, const Range_t second) const
{
	return first.getEnd() < second.getStart();
}
