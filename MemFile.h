#pragma once

class CMemFile
{
public:
	CMemFile()
	{
		m_buffer = NULL;
		m_bufferFull = m_bufferSize = m_bufferGrow = 0;
	}
	CMemFile(int bufferSize, int bufferGrow) 
	{
		m_buffer = NULL;
		Create(bufferSize, bufferGrow);
	}
	CMemFile(void* buffer, int bufferSize)
	{
		m_buffer = NULL;
		Attach(buffer, bufferSize);
	}
	~CMemFile()
	{
		delete m_buffer;
	}
	void Create(int bufferSize, int bufferGrow) 
	{
		delete m_buffer;
		m_buffer = new char [bufferSize];
		m_bufferSize = bufferSize;
		m_bufferGrow = bufferGrow;
		m_bufferFull = 0;
	}
	void Attach(void* buffer, int bufferSize)
	{
		delete m_buffer;
		m_buffer = buffer;
		m_bufferSize = bufferSize;
		m_bufferFull = 0;
		m_bufferGrow = 0;
	}
	template <class T>
	void Write(const T& data)
	{
		WriteBinary(&data, sizeof(T));
	}
	void WriteString(const CString& str)
	{
		WriteBinary((const TCHAR*)str, str.GetLength() * sizeof(TCHAR));
		Write<TCHAR>(0);
	}
	void WriteBinary(const void* data, int size)
	{
		if(m_bufferSize - m_bufferFull < size)
			Grow();
		memcpy((char*)m_buffer + m_bufferFull, data, size);
		m_bufferFull += size;
	}
	template <class T>
	T Read()
	{
		T data;
		ReadBinary(&data, sizeof(T));
		return data;
	}
	CString ReadString()
	{
		TCHAR* str = (TCHAR*)((char*)m_buffer + m_bufferFull);
		int start = m_bufferFull;
		while(*(TCHAR*)((char*)m_buffer + m_bufferFull) != 0 && m_bufferFull < m_bufferSize) {
			m_bufferFull += sizeof(TCHAR);
		}
		if(*(TCHAR*)((char*)m_buffer + m_bufferFull) == 0) {
			m_bufferFull += sizeof(TCHAR);
			return CString(str, (m_bufferFull - start) / sizeof(TCHAR) - 1);
		} else
			return _T(""); // error
	}
	void ReadBinary(void* data, int size)
	{
		memcpy(data, (char*)m_buffer + m_bufferFull, min(size, m_bufferSize - m_bufferFull));
		m_bufferFull += size;
	}
	void* GetBuffer()
	{
		return m_buffer;
	}
	int GetSize()
	{
		return m_bufferFull;
	}

protected:
	void Grow() 
	{
		ASSERT(m_buffer);
		ASSERT(m_bufferGrow > 0);
		m_bufferSize += m_bufferGrow;
		void* newBuffer = new char [m_bufferSize];
		memcpy(newBuffer, m_buffer, m_bufferFull);
		delete m_buffer;
		m_buffer = newBuffer;
	}

protected:
	void* m_buffer;
	int m_bufferFull;
	int m_bufferSize;
	int m_bufferGrow;
};

class CTextFile
{
public:
	enum EOpenMode {
		kRead,
		kWrite
	};

	bool Open(LPCTSTR fileName, EOpenMode openMode)
	{
		if(openMode == kRead) {
			if(!m_file.Open(fileName))
				return false;
		} else {
			if(!m_file.Open(fileName, GENERIC_WRITE, 0, CREATE_ALWAYS))
				return false;
		}
		return true;
	}
	bool Read(CString& outString)
	{
		ASSERT(m_file.IsOpen());
		__int64 size64 = m_file.GetSize();
		if(size64 > 32*1024*1024) // limit to 32MB
			return false;
		int size = (int)size64;
		unsigned char bom[2] = { 0, 0 };
		if(!m_file.Read(bom, 2))
			return false;
		if(bom[0] == 0xff && bom[1] == 0xfe) {
			CStringW wString;
			int strLen = (size-2)/sizeof(wchar_t);
			m_file.Read(wString.GetBuffer(strLen), size-2);
			wString.ReleaseBuffer(strLen);
			outString = wString;
			return true;
		} else {
			CStringA aString;
			m_file.Seek(0, FILE_BEGIN);
			int strLen = size;
			m_file.Read(aString.GetBuffer(strLen), size);
			aString.ReleaseBuffer(strLen);
			outString = aString;
			return true;
		}
	}
	void Write(const CString& inString)
	{
		ASSERT(m_file.IsOpen());
		if(sizeof(TCHAR) == 2) {
			unsigned char bom[2] = { 0xff, 0xfe };
			m_file.Write(bom, 2);
		}
		m_file.Write((LPCTSTR)inString, inString.GetLength() * sizeof(TCHAR));
	}

protected:
	CFile m_file;
};