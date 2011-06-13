
// The Digital Object Identifier for Peasoup.
class doip_t
{
        doip_t(db_id_t did, int conf, std::string program, std::string comment);

#if 0
        operator<<();        // output
        operator==();        // comparison operators
                             // compare confidence levels of two datum
        operator<();
#endif
   
    private:
        db_id_t did;
        int confidence;
        std::string program;
        std::string comment;
};
