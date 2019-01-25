#ifndef Provenance_h
#define Provenance_h

#include <bitset>

class Provenance_t
{
	private:
		// ProvType enum values are explicit to show they are in bounds of the bitset
		enum class ProvType { IndJump = 0, IndCall = 1, Ret = 2 };
		std::bitset<3> prov;
	public:
		Provenance_t() {prov = std::bitset<3>();}
		virtual ~Provenance_t() {;}

		void addReturn()
		{
			prov.set((size_t) ProvType::Ret);
		}

		void addIndirectJump()
		{
			prov.set((size_t) ProvType::IndJump);
		}

		void addIndirectCall()
		{
			prov.set((size_t) ProvType::IndCall);
		}

		void addProv(const Provenance_t& other)
		{
			prov |= other.prov;
		}

		bool hasReturn() const
		{
			return prov.test((size_t) ProvType::Ret);
		}

		
		bool hasIndirectJump() const
		{
			return prov.test((size_t) ProvType::IndJump);
		}

		bool hasIndirectCall() const
                {
                        return prov.test((size_t) ProvType::IndCall);
                }
};		

#endif
