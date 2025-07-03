#ifndef _WINSOCKBASE_INCLUDE_H_
#define	_WINSOCKBASE_INCLUDE_H_
#pragma once


class CWinsockBase
{
public:
	CWinsockBase();
	virtual ~CWinsockBase();

protected:
	SOCKET m_socket_handle;
	
	CString GetPeerAddr();	//Get Peer Address
	CString GetSockAddr();	//Get Local Address
	UINT GetPeerPort();
	UINT GetSockPort();

};



#endif // !_WINSOCKBASE_INCLUDE_H_



