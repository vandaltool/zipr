// An ELF file as represented by the DB
class File_t : public BaseObj_t
{
    public:
        // create new item.
        File_t(db_id_t file_id, std::string url, std::string hash, std::string arch);
        File_t(db_id_t file_id);         // read from DB       
        void WriteToDB(DBinterface_t);   // writes to DB ID is not -1.

    private:
        db_id_t file_id;    /* -1 means not yet in DB */
        std::string url;
        std::string hash;
        std::string arch;       
};
