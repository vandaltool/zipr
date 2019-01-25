/*
 * Copyright (c) 2014 - Zephyr Software LLC
 *
 * This file may be used and modified for non-commercial purposes as long as
 * all copyright, permission, and nonwarranty notices are preserved.
 * Redistribution is prohibited without prior written consent from Zephyr
 * Software.
 *
 * Please contact the authors for restrictions applying to commercial use.
 *
 * THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: Zephyr Software
 * e-mail: jwd@zephyr-software.com
 * URL   : http://www.zephyr-software.com/
 *
 */

namespace libIRDB
{


// The Digital Object Identifier for Peasoup.
class doip_t : virtual public IRDB_SDK::Doip_t 
{
	public: 
        	virtual ~doip_t(){}
        	doip_t(db_id_t did, int conf, std::string tool_name , std::string comment);

		db_id_t GetBaseID() const { return did; }
   
    private:
        db_id_t did;
        int confidence;
        std::string tool_name;
        std::string comment;
};
}
