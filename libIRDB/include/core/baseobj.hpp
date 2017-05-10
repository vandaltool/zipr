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


#include <stdint.h>
#include <set>

typedef uintptr_t virtual_offset_t;
typedef int db_id_t;


class Relocation_t;
typedef std::set<Relocation_t*> RelocationSet_t;



// A base class for something that all objects have, for now just a DOIP.
// see .cpp file for method descriptions.
class BaseObj_t
{
    public:
        BaseObj_t(doip_t* doip);

	virtual ~BaseObj_t() { /* management of doips is explicit */ }

        static void SetInterface(DBinterface_t *dbintr);
        static DBinterface_t* GetInterface() {return dbintr;}

        // get and set the ID
        db_id_t GetBaseID() const {return base_id; }
        db_id_t GetDoipID() const { return doip ? doip->GetBaseID() : NOT_IN_DATABASE; }
        void SetDoipID(doip_t *dp) { doip=dp; }
        void SetBaseID(db_id_t id) {base_id=id; }

	static const db_id_t NOT_IN_DATABASE;

	virtual RelocationSet_t& GetRelocations() { return relocs; }
	virtual const RelocationSet_t& GetRelocations() const { return relocs; }


    protected:
        static DBinterface_t *dbintr;

    private:
        doip_t* doip;
        db_id_t base_id;    // -1 means not yet in the DB.
        RelocationSet_t relocs;
};

