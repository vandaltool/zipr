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
using virtual_offset_t = IRDB_SDK::VirtualOffset_t;
using db_id_t          = IRDB_SDK::DatabaseID_t;
using RelocationSet_t  = IRDB_SDK::RelocationSet_t;


// A base class for something that all objects have, for now just a DOIP.
// see .cpp file for method descriptions.
class BaseObj_t : virtual public IRDB_SDK::BaseObj_t
{
    public:
	virtual ~BaseObj_t() {}
        BaseObj_t(doip_t* doip);

        // static void SetInterface(DBinterface_t *dbintr);
        static DBinterface_t* GetInterface() {return dbintr;}

        // get and set the ID
        db_id_t getBaseID() const {return base_id; }
        db_id_t getDoipID() const { return doip ? doip->GetBaseID() : NOT_IN_DATABASE; }
        void setDoipID(IRDB_SDK::Doip_t *dp)  { doip=dynamic_cast<doip_t*>(dp); if(dp) assert(doip);  }
        void setBaseID(db_id_t id) {base_id=id; }

	virtual RelocationSet_t& GetRelocations() { return relocs; }
	virtual const RelocationSet_t& getRelocations() const { return relocs; }
	virtual void setRelocations(const RelocationSet_t& rels) { relocs=rels; }


    protected:
        static DBinterface_t *dbintr;

    private:
        doip_t* doip;
        db_id_t base_id;    // -1 means not yet in the DB.
        RelocationSet_t relocs;

	friend class IRDB_SDK::BaseObj_t;
};

}
