
// The Digital Object Identifier for Peasoup.
class doip_t
{
        doip(db_id_t did, int conf, string program, string comment)
        operator<<();        // output
        operator==();        // comparison operators
                             // compare confidence levels of two datum
        operator<();
   
    private:
        db_id_t did;
        int confidence;
        string program;
        string comment;
}
