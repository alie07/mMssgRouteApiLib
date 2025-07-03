#ifndef _MESSAGECMD_INCLUDE_H_
#define	_MESSAGECMD_INCLUDE_H_
#pragma once


class TMessageCmd
{
public:
	TMessageCmd(LPCTSTR name, LPCTSTR mssg);
	virtual ~TMessageCmd();

	int ReadMessage(CString &rName, int bufsize, char * pBuffer);
	int GetMessageBody(CString & rMssgBody);

private:
	CString m_name;
	CString m_mssg_body;
};



#endif // !_MESSAGECMD_INCLUDE_H_


