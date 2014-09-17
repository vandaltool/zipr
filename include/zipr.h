#ifndef zipr_h
#define zipr_h


class Zipr_t
{
	public:
		Zipr_t(libIRDB::FileIR_t* p_firp, const Options_t &p_opts)
			: m_firp(p_firp), m_opts(p_opts) { };

		void CreateBinaryFile(std::string name);

	protected:

		libIRDB::FileIR_t* m_firp;
		const Options_t& m_opts;
};

#endif
