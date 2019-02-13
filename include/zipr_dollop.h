#include <assert.h>
#include <list>

namespace zipr
{
	using namespace std;
	using namespace IRDB_SDK;

	class Dollop_t;
	class DollopManager_t;

	class Placeable_t  : virtual public Zipr_SDK::Placeable_t
	{
		public:
			Placeable_t() : m_is_placed(false), m_placed_address(0) {}
			void Place(RangeAddress_t place) {
					assert(!m_is_placed);
					m_is_placed = true;
					m_placed_address = place;
			};
			RangeAddress_t getPlace() const { return m_placed_address; }
			bool isPlaced() const { return m_is_placed; }
		protected:
			bool m_is_placed;
			RangeAddress_t m_placed_address;

		friend ostream &operator<<(ostream &, const Placeable_t &);
	};


	class DollopPatch_t : virtual public Zipr_SDK::DollopPatch_t, public Placeable_t 
	{
		public:
			DollopPatch_t(Zipr_SDK::Dollop_t *target) : m_target(target) {};
			DollopPatch_t() : m_target(NULL) { };

			Zipr_SDK::Dollop_t *getTarget() const { return m_target; }
			void setTarget(Zipr_SDK::Dollop_t *target) { m_target = target; }

			friend ostream &operator<<(ostream &, const DollopPatch_t &);
		private:
			Zipr_SDK::Dollop_t *m_target;
	};

	class DollopEntry_t : virtual public Zipr_SDK::DollopEntry_t, public Placeable_t 
	{
		public:
			DollopEntry_t(Instruction_t *insn, Zipr_SDK::Dollop_t *member_of);
			Instruction_t *getInstruction() const { return m_instruction; }
			void setTargetDollop(Zipr_SDK::Dollop_t *target) { m_target_dollop = target; }
			Zipr_SDK::Dollop_t *getTargetDollop() const { return m_target_dollop; }
			Zipr_SDK::Dollop_t *getMemberOfDollop() const { return m_member_of_dollop; }
			void MemberOfDollop(Zipr_SDK::Dollop_t *member_of) { m_member_of_dollop = member_of; }

			bool operator==(const DollopEntry_t &);
			bool operator!=(const DollopEntry_t &);
		private:
			Instruction_t *m_instruction;
			Zipr_SDK::Dollop_t *m_target_dollop, *m_member_of_dollop;

		friend ostream &operator<<(ostream &, const DollopEntry_t &);
	};

	class Dollop_t : virtual public Zipr_SDK::Dollop_t, public Placeable_t
	{
		public:
			Dollop_t(Instruction_t *start, Zipr_SDK::DollopManager_t *);
			Dollop_t() :
				m_size(0),
				m_fallthrough_dollop(NULL),
				m_fallback_dollop(NULL),
				m_fallthrough_patched(false),
				m_coalesced(false),
				m_was_truncated(false),
				m_dollop_mgr(NULL) {}

			void setDollopManager(Zipr_SDK::DollopManager_t *mgr) { m_dollop_mgr = mgr; }

			size_t getSize() const {
				return m_size;
			}
			void setSize(size_t size) {
				m_size = size;
			}

			size_t getDollopEntryCount() const {
				return size();
			}

			Zipr_SDK::Dollop_t *split(Instruction_t *split_point);
			void removeDollopEntries(Zipr_SDK::DollopEntryList_t::iterator, Zipr_SDK::DollopEntryList_t::iterator);

			void setFallbackDollop(Zipr_SDK::Dollop_t *fallback) { m_fallback_dollop = fallback; }
			Zipr_SDK::Dollop_t *getFallbackDollop(void) const { return m_fallback_dollop; }

			void setFallthroughDollop(Zipr_SDK::Dollop_t *fallthrough) { m_fallthrough_dollop = fallthrough; }
			Zipr_SDK::Dollop_t *getFallthroughDollop(void) const { return m_fallthrough_dollop; }

			bool isFallthroughPatched(void) const { return m_fallthrough_patched; }
			void setFallthroughPatched(bool patched);

			Zipr_SDK::DollopEntry_t *setFallthroughDollopEntry(Zipr_SDK::DollopEntry_t *) const;

			void wasTruncated(bool truncated) { m_was_truncated = truncated; }
			bool wasTruncated(void) const { return m_was_truncated; }

			void setCoalesced(bool coalesced);
			bool wasCoalesced(void) const { return m_coalesced; }

			void reCalculateSize();
		private:
			size_t CalculateSize();
			size_t m_size;
			Zipr_SDK::Dollop_t *m_fallthrough_dollop, *m_fallback_dollop;
			bool m_fallthrough_patched;
			bool m_coalesced;
			bool m_was_truncated;
			Zipr_SDK::DollopManager_t *m_dollop_mgr;

		friend ostream &operator<<(ostream &, const Dollop_t &);
	};
}
