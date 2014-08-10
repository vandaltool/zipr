#ifndef rss_instrument_hpp
#define rss_instrument_hpp

#include "MEDS_AnnotationParser.hpp"
#include <libIRDB-core.hpp>



class RSS_Instrument
{
	public:
		RSS_Instrument(libIRDB::FileIR_t *the_firp, MEDS_Annotation::MEDS_AnnotationParser* the_meds_ap) : firp(the_firp), meds_ap(the_meds_ap) { };
		bool execute();

	private:
		libIRDB::FileIR_t* firp;
		MEDS_Annotation::MEDS_AnnotationParser* meds_ap;
};

#endif

