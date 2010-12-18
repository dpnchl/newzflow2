// Implementation of the main sq3 classes.
//
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"

#include "SQ3.h"

namespace sq3 {

	Database::Database(CString const & n)
		: m_db(0),
		  m_name(n)
	{
		Open();
	}

	Database::Database()
		: m_db(0)
	{
	}

	Database::~Database()
	{
		Close();
	}

	int Database::Close()
	{
		if(! m_db) return 0;
		int rc = sqlite3_close(m_db);
		m_db = 0; // should we only do this is close succeeds?
		return rc;
	}

	int Database::Open(CString const & name)
	{
		Close();
		m_name = name;
		return sqlite3_open16(name, &m_db);
	}

	int Database::Open()
	{
		ASSERT(!m_name.IsEmpty());
		return Open(m_name);
	}
    
	bool Database::IsOpen() const
	{
		return 0 != m_db;
	}

	CString Database::GetName() const
	{
		return m_name;
	}
	sqlite3 * Database::GetHandle() const
	{
		return m_db;
	}

	CString Database::GetErrorMessage() const
	{
		ASSERT(m_db);
		char const * s = sqlite3_errmsg(m_db);
		return s ? CString(s) : CString();
	}

	int64_t Database::GetLastInsertID() const
	{
		ASSERT(m_db);
		return sqlite3_last_insert_rowid(m_db);
	}

	void Database::SetBusyTimeout(int millis)
	{
		ASSERT(m_db);
		sqlite3_busy_timeout(m_db, millis);
	}

	int Database::Execute(char const * sql)
	{
		ASSERT(m_db);
		return sqlite3_exec(m_db, sql, NULL, NULL, 0);
	}
   
	int Database::ExecuteInt(CString const & sql, int & val)
	{
		ASSERT(m_db);
		return Statement(*this, sql).ExecuteInt(val);
	}

	int Database::ExecuteInt64(CString const & sql, int64_t & val)
	{
		ASSERT(m_db);
		return Statement(*this, sql).ExecuteInt64(val);
	}

	int Database::ExecuteDouble(CString const & sql, double & val)
	{
		ASSERT(m_db);
		return Statement(*this, sql).ExecuteDouble(val);
	}

	int Database::ExecuteString(CString const & sql, CString & val)
	{
		ASSERT(m_db);
		return Statement(*this, sql).ExecuteString(val);
	}

	void const * Database::ExecuteBlob(char const * sql, int & size)
	{
		ASSERT(m_db);
		return Statement(*this, sql).ExecuteBlob(size);
	}

	Transaction::Transaction(Database &con, bool start)
		: m_con(con),m_intrans(false)
	{
		if(start) Begin();
	}

	Transaction::~Transaction()
	{
		if(m_intrans)
		{
			__try
			{
				Rollback();
			}
			__except(EXCEPTION_EXECUTE_HANDLER)
			{
				return;
			}
		}
	}

	int Transaction::Begin()
	{
		if(m_intrans)
		{
			return SQLITE_ERROR;
		}
		int rc = m_con.Execute("begin;");
		m_intrans = (0 == rc);
		return rc;
	}

	int Transaction::Commit()
	{
		if(! m_intrans)
		{
			return SQLITE_ERROR;
		}
		int rc = m_con.Execute("commit;");
		m_intrans = (0 == rc);
		return rc;
	}

	int Transaction::Rollback()
	{
		int rc = m_con.Execute("rollback;");
		m_intrans = false;
		return rc;
	}
   
	bool Transaction::IsActive() const
	{
		return m_intrans;
	}

	Reader::Reader() : m_cmd(0)
	{

	}

	Reader::Reader(Statement * s) : m_cmd(s)
	{
		++m_cmd->m_refs;
	}

	Reader & Reader::operator=(Reader const & rhs)
	{
		if(&rhs != this)
		{
			//Close();
			m_cmd = rhs.m_cmd;
			if(m_cmd)
			{
				++m_cmd->m_refs;
			}
		}
		return *this;
	}
	Reader::Reader(Reader const & rhs)
		: m_cmd(rhs.m_cmd)
	{
		if(m_cmd)
		{
			++m_cmd->m_refs;
		}
	}


	Reader::~Reader()
	{
		Close();
	}

	int Reader::GetColCount()
	{
		return m_cmd
			? m_cmd->GetColCount()
			: -1;
	}
	int Reader::Step()
	{
		if(!m_cmd)
		{
			return SQLITE_ERROR;
		}
		return sqlite3_step(m_cmd->m_st);
	}

	int Reader::Reset()
	{
		return m_cmd
			? m_cmd->Reset()
			: SQLITE_ERROR;
	}

	int Reader::Close()
	{
		if(! m_cmd)
		{
			return 0;
		}
		int rc = SQLITE_OK;
		if(--m_cmd->m_refs==0)
		{
			rc = m_cmd->Reset();
		}
		m_cmd = 0;
		return rc;
	}

	bool Reader::IsValidIndex(int index)
	{
		return m_cmd &&
			(index < m_cmd->m_colcount);
	}

	CString Reader::GetColName(int index)
	{
		return IsValidIndex(index)
			? sqlite3_column_name(m_cmd->m_st, index)
			: CString();
	}

	bool Reader::GetInt(int index, int & val)
	{
		if(! IsValidIndex(index))
		{
			return false;
		}
		val = sqlite3_column_int(m_cmd->m_st, index);
		return true;
	}

	bool Reader::GetInt64(int index, int64_t & val)
	{
		if(! IsValidIndex(index))
		{
			return false;
		}
		val = sqlite3_column_int64(m_cmd->m_st, index);
		return true;
	}

	bool Reader::GetDouble(int index, double & val)
	{
		if(! IsValidIndex(index))
		{
			return false;
		}
		val = sqlite3_column_double(m_cmd->m_st, index);
		return true;
	}

	bool Reader::GetString(int index, CString & val)
	{
		if(! IsValidIndex(index))
		{
			return false;
		}
		val = (LPCTSTR) sqlite3_column_text16(m_cmd->m_st, index);
		return true;
	}

	void const * Reader::GetBlob(int index, int & size)
	{
		if(! IsValidIndex(index))
		{
			size = -1;
			return 0;
		}
		size = sqlite3_column_bytes(m_cmd->m_st, index);
		return sqlite3_column_blob(m_cmd->m_st, index);
	}

	Statement::Statement(Database &con, CString const & sql)
		: m_db(&con), m_st(0), m_refs(0), m_colcount(0)
	{
		void const * tail = 0;
		if(SQLITE_OK == sqlite3_prepare16(m_db->GetHandle(), sql, -1, &m_st, &tail))
		{
			m_colcount = sqlite3_column_count(m_st);
		}
	}
	Statement::~Statement()
	{
		Finalize();
	}

	int Statement::Finalize()
	{
		int rc = SQLITE_OK;
		if(m_st)
		{
			rc = sqlite3_finalize(m_st);
			m_st = 0;
		}
		return rc;
	}

	int Statement::Reset()
	{
		return m_st
			? sqlite3_reset(m_st)
			: SQLITE_ERROR;
	}

	bool Statement::IsValid() const
	{
		return 0 != m_st;
	}

	int Statement::Bind(int index)
	{
		return IsValid()
			? sqlite3_bind_null(m_st, index+1)
			: SQLITE_ERROR;
	}
	int Statement::Bind(int index, int data)
	{
		return IsValid()
			? sqlite3_bind_int(m_st, index+1, data)
			: SQLITE_ERROR;
	}
    
	int Statement::Bind(int index, int64_t data)
	{
		return IsValid()
			? sqlite3_bind_int64(m_st, index+1, data)
			: SQLITE_ERROR;
	}
    
	int Statement::Bind(int index, double data)
	{
		return IsValid()
			? sqlite3_bind_double(m_st, index+1, data)
			: SQLITE_ERROR;
	}
	int Statement::Bind(int index, CString const & data)
	{
		return IsValid()
			? sqlite3_bind_text16(m_st, index+1, data, -1, SQLITE_TRANSIENT)
			: SQLITE_ERROR;
	}
    
	int Statement::Bind(int index, char const * data, int datalen)
	{
		return IsValid()
			? sqlite3_bind_text(m_st, index+1, data, datalen, SQLITE_TRANSIENT)
			: SQLITE_ERROR;
	}
    
	int Statement::Bind(int index, void const * data, int datalen)
	{
		return IsValid()
			? sqlite3_bind_blob(m_st, index+1, data, datalen, SQLITE_TRANSIENT)
			: SQLITE_ERROR;
	}
    
	Reader Statement::ExecuteReader()
	{
		return Reader(this);
	}
	int Statement::ExecuteNonQuery()
	{
		return ExecuteReader().Step() == SQLITE_DONE ? SQLITE_OK : SQLITE_ERROR;
	}
	int Statement::ExecuteInt(int & val)
	{
		Reader r(ExecuteReader());
		if(SQLITE_ROW != r.Step())
		{
			return SQLITE_ERROR;
		}
		return r.GetInt(0, val)
			? SQLITE_OK
			: SQLITE_ERROR;
	}

	int Statement::ExecuteInt64(int64_t & val)
	{
		Reader r(ExecuteReader());
		if(SQLITE_ROW != r.Step())
		{
			return SQLITE_ERROR;
		}
		return r.GetInt64(0, val)
			? SQLITE_OK
			: SQLITE_ERROR;
	}
	int Statement::ExecuteDouble(double & val)
	{
		Reader r(ExecuteReader());
		if(SQLITE_ROW != r.Step())
		{
			return SQLITE_ERROR;
		}
		return r.GetDouble(0, val)
			? SQLITE_OK
			: SQLITE_ERROR;
	}
	int Statement::ExecuteString(CString & val)
	{
		Reader r(ExecuteReader());
		if(SQLITE_ROW != r.Step())
		{
			return SQLITE_ERROR;
		}
		return r.GetString(0, val)
			? SQLITE_OK
			: SQLITE_ERROR;
	}
	void const * Statement::ExecuteBlob(int & size)
	{
		Reader r(ExecuteReader());
		if(SQLITE_ROW != r.Step())
		{
			size = -1;
			return 0;
		}
		return r.GetBlob(0, size);
	}

	int Statement::GetColCount() const
	{
		return m_colcount;
	}
} // namespace

