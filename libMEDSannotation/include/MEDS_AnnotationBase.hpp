#ifndef MEDS_ANNOTATION_BASE_HPP
#define MEDS_ANNOTATION_BASE_HPP


#include <assert.h>
#include "VirtualOffset.hpp"


namespace MEDS_Annotation
{


// base class for deriving objects.
class MEDS_AnnotationBase
{
	public:

		MEDS_AnnotationBase() { init(); }
		virtual ~MEDS_AnnotationBase()  { }

		/* i'd rather make this a pure virtual func, but can't figure out why it won't compile */
		virtual const std::string toString() const { assert(0); }

                // valid annotation?
                virtual bool isValid() const { return m_isValid; }
                virtual void setValid() { m_isValid = true; }

                // virtual offset
                virtual VirtualOffset getVirtualOffset() const { return m_virtualOffset; }

		virtual void init() { m_isValid=false; }




	protected:

		bool m_isValid;
                VirtualOffset  m_virtualOffset;



	
};

}

#endif
