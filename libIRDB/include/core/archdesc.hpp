

class ArchitectureDescription_t
{
	public:

	ArchitectureDescription_t() : bits(0) {}

	int GetBitWidth() 		{ return bits; }	
	void SetBitWidth(int _bits) 	{ bits=_bits; }	

	private:

		int bits;
};

