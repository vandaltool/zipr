// An ELF file as represented by the DB
class File_t : public BaseObj_t
{
    public:
        // create new item.
        File_t(db_id_t file_id, std::string url, std::string hash, std::string arch, int elfoid, db_id_t doipid);

        File_t(db_id_t file_id) : BaseObj_t(NULL) { assert(0);}          // read from DB       
        void WriteToDB() { assert(0); }   // writes to DB ID is not -1.

	int GetELFOID() { return elfoid; };

    private:
        std::string url;
        std::string hash;
        std::string arch;       
	int elfoid;
};
