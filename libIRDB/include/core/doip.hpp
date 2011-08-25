
// The Digital Object Identifier for Peasoup.
class doip_t
{
	public: 
        	doip_t(db_id_t did, int conf, std::string tool_name , std::string comment);

		db_id_t GetBaseID() const { return did; }

#if 0
        operator<<();        // output
        operator==();        // comparison operators
                             // compare confidence levels of two datum
        operator<();
#endif
   
    private:
        db_id_t did;
        int confidence;
        std::string tool_name;
        std::string comment;
};
