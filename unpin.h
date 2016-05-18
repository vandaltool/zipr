/***************************************************************************
 * Copyright (c)  2014  Zephyr Software LLC. All rights reserved.
 *
 * This software is furnished under a license and/or other restrictive
 * terms and may be used and copied only in accordance with such terms
 * and the inclusion of the above copyright notice. This software or
 * any other copies thereof may not be provided or otherwise made
 * available to any other person without the express written consent
 * of an authorized representative of Zephyr Software LCC. Title to,
 * ownership of, and all rights in the software is retained by
 * Zephyr Software LCC.
 *
 * Zephyr Software LLC. Proprietary Information
 *
 * Unless otherwise specified, the information contained in this
 * directory, following this legend, and/or referenced herein is
 * Zephyr Software LLC. (Zephyr) Proprietary Information.
 *
 * CONTACT
 *
 * For technical assistance, contact Zephyr Software LCC. at:
 *
 *
 * Zephyr Software, LLC
 * 2040 Tremont Rd
 * Charlottesville, VA 22911
 *
 * E-mail: jwd@zephyr-software.com
 **************************************************************************/

#ifndef unpin_h
#define unpin_h

#include <libIRDB-core.hpp>

class Unpin_t : public Zipr_SDK::ZiprPluginInterface_t
{
	public:
		Unpin_t( Zipr_SDK::Zipr_t* zipr_object) : zo(zipr_object) { };
		virtual void PinningBegin()
		{
			DoUnpin();
		}
		virtual void CallbackLinkingEnd()
		{
			DoUpdate();
		}

		virtual Zipr_SDK::ZiprOptionsNamespace_t *RegisterOptions(Zipr_SDK::ZiprOptionsNamespace_t *) { return NULL; }
	private:

		// workhorses 
		void DoUnpin();
		void DoUnpinForScoops();
		void DoUnpinForFixedCalls();

		void DoUpdate();
		void DoUpdateForScoops();
		void DoUpdateForInstructions();

		Zipr_SDK::Zipr_t* zo;

};

#endif
