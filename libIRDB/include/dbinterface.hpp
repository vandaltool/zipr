
class DatabaseError_t
{
	public: 
		enum DatabaseErrorType_t {ProgramNotInDatabase};
		DatabaseError_t(DatabaseErrorType_t the_err) : err(the_err) {}
		DatabaseErrorType_t GetErrorCode() const { return err; }
	private:
		DatabaseError_t::DatabaseErrorType_t err;
};

std::ostream& operator<<(std::ostream& output, const DatabaseError_t& p);


// an interface to a database
class DBinterface_t
{
	public:
		DBinterface_t() {};
                virtual void IssueQuery(std::string query)=0;
                virtual void MoveToNextRow()=0;
                virtual std::string GetResultColumn(std::string colname)=0;
                virtual bool IsDone()=0;
                virtual void Commit()=0;

};

