#include "pch.h"

#include "winsock_api_entries.h"

#include "WinsockBase.h"


CWinsockBase::CWinsockBase()
{
	m_socket_handle = INVALID_SOCKET;
}

CWinsockBase::~CWinsockBase()
{

}

CString CWinsockBase::GetPeerAddr()
{
	ASSERT(m_socket_handle);
	CString _retval;
	_retval = TxSocketComm::getPeerName(m_socket_handle);
	return _retval;
}

CString CWinsockBase::GetSockAddr()
{
	ASSERT(m_socket_handle);
	CString _retval;
	_retval = TxSocketComm::getSockName(m_socket_handle);
	return _retval;
}

UINT CWinsockBase::GetPeerPort()
{
	ASSERT(m_socket_handle);
	UINT _retval = (UINT)TxSocketComm::getPeerPort(m_socket_handle);
	return _retval;
}

UINT CWinsockBase::GetSockPort()
{
	ASSERT(m_socket_handle);
	UINT _retval = (UINT)TxSocketComm::getSockPort(m_socket_handle);
	return _retval;
}




