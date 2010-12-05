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
