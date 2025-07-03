#include "pch.h"


#include "dll_internal.h"


TMessageCmd::TMessageCmd(LPCTSTR name, LPCTSTR mssg)
{
	m_name = name;
	m_mssg_body = mssg;
}


TMessageCmd::~TMessageCmd()
{

}


int TMessageCmd::ReadMessage(CString &rName, int bufsize, char * pBuffer)
{
	int _size = 0;
	rName = m_name;

	_size = m_mssg_body.GetLength();

	ZeroMemory(pBuffer, bufsize);	// Fill with NULLS
	CopyMemory(pBuffer, m_mssg_body.GetBuffer(), _size);

	return _size;
}

int TMessageCmd::GetMessageBody(CString & rMssgBody)
{
	rMssgBody = m_mssg_body;
	return rMssgBody.GetLength();
}
