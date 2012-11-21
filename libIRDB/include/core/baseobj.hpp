

typedef int virtual_offset_t;
typedef int db_id_t;


// A base class for something that all objects have, for now just a DOIP.
// see .cpp file for method descriptions.
class BaseObj_t
{
    public:
        BaseObj_t(doip_t* doip);

        static void SetInterface(DBinterface_t *dbintr);
        static DBinterface_t* GetInterface() {return dbintr;}

        // get and set the ID
        db_id_t GetBaseID() const {return base_id; }
        db_id_t GetDoipID() const { return doip ? doip->GetBaseID() : NOT_IN_DATABASE; }
        void SetDoipID(doip_t *dp) { doip=dp; }
        void SetBaseID(db_id_t id) {base_id=id; }
   
        // A derived class must provide functionality to write to the database.
        virtual void WriteToDB()=0;    

	static const db_id_t NOT_IN_DATABASE;

    protected:
        static DBinterface_t *dbintr;

    private:
        doip_t* doip;
        db_id_t base_id;    // -1 means not yet in the DB.

};

