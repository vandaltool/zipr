

typedef int virtual_offset_t;
typedef int db_id_t;


// A base class for something that all objects have, for now just a DOIP.
class BaseObj_t
{
    public:
        BaseObj_t(doip_t* doip);

        static void SetInterface(DBinterface_t *dbintr);

        // get and set the ID
        db_id_t GetBaseID();
        void SetBaseID(db_id_t);
   
        // A derived class must provide functionality to write to the database.
        virtual void WriteToDB()=0;    

    private:
        doip_t* doip;
        db_id_t base_id;    // -1 means not yet in the DB.
        static DBinterface_t *dbintr;

};

