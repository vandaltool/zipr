#ifndef range_h
#define range_h


typedef uintptr_t RangeAddress_t;

class Range_t
{
	public:
		Range_t(RangeAddress_t p_s, RangeAddress_t p_e) : m_start(p_s), m_end(p_e) { }

		RangeAddress_t GetStart() { return m_start; }
		RangeAddress_t GetEnd() { return m_end; }

	protected:

		RangeAddress_t m_start, m_end;
};

#endif
