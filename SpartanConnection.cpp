#include "pch.h"

#include "HydraDefineType.h"
#include "winsock_api_entries.h"
#include "svc_interface_entries.h"

#define DECLSPEC_FUNC_EXPORT
#include "MessageCmd.h"

#include "dll_internal.h"

#define	_ENABLE_MSSG_TIME_SVC_

#define	_CLIENT_DATA_BUF_SIZE_		(8192)

class CSpartanClientData
{
public:
	CSpartanClientData(CSpartanConnection * pObj) : m_ObjOwner( pObj )
	{

	}

	virtual ~CSpartanClientData()
	{
		if (m_mapMessages.GetCount())
		{
			TRACE("CSpartanClientData::~CSpartanClientData encountered unprocessed message data [%d]\n", m_mapMessages.GetCount());
			m_mapMessages.RemoveAll();
		}
	}

	char * GetBuffer()
	{
		return m_dataBuf;
	}

	int ReadMessage(CString & rMessage, int iMssgSize )
	{
		m_dataBuf[iMssgSize] = NULL;	// place a Null
		rMessage.Format("%s", m_dataBuf);
		return rMessage.GetLength();
	}

	CMap<CString, LPCTSTR, CString, LPCTSTR> m_mapMessages;

private:
	char m_dataBuf[_CLIENT_DATA_BUF_SIZE_];
	CSpartanConnection * m_ObjOwner;
};



u_int64 read_systime_precise()
{
	FILETIME _strucFiletime;
	ULARGE_INTEGER _ulargeInt;
	u_int64 _timeval = 0;

	// NOTE TO DESIGNER: There is a minimum OS Requirement for this function to work.
	//                   Please refer to Microsoft Documentation.
	// What it does is it return a 64 bit value representing the number of 100-nanoseconds interval since January 1, 1601 (UTC).
	GetSystemTimePreciseAsFileTime(&_strucFiletime);
	_ulargeInt.LowPart = _strucFiletime.dwLowDateTime;
	_ulargeInt.HighPart = _strucFiletime.dwHighDateTime;

	return _ulargeInt.QuadPart;
}


double calc_precise_timestamp_diff(u_int64 & timeto, u_int64 & timefrom)
{
	double _lsbval = 0.0000001;		// 100 nano seconds;
	u_int64 _timediff = timeto - timefrom;
	double _dbldiff = _timediff * _lsbval;
	return _dbldiff;	// return time difference in seconds ...
}



CSpartanConnection::CSpartanConnection() : CWinsockBase()
{
	m_pServer = NULL;
	super::m_socket_handle = NULL;
	m_is_handler_running = FALSE;
	m_allow_auto_delete = FALSE;

	m_data = new CSpartanClientData(this);

}

CSpartanConnection::CSpartanConnection(SOCKET sockClient, LPCTSTR pStrName, CSpartanServer * pServerObj) : CWinsockBase()
{
	m_pServer = pServerObj;
	m_client_name = pStrName;
	super::m_socket_handle = sockClient;
	m_is_handler_running = FALSE;
	m_allow_auto_delete = TRUE;
	m_data = new CSpartanClientData(this);
}

CSpartanConnection::~CSpartanConnection()
{
	if (m_pServer)
	{
		m_pServer->RemoveClient(this);
		TRACE("CSpartanConnection::~CSpartanConnection destroying SocketConnectData at Server side\n");
	}
	else {
		TRACE("CSpartanConnection::~CSpartanConnection destroying SocketConnectData at Client side\n");
	}

	m_pServer = NULL;

	delete m_data;
}


BOOL CSpartanConnection::ConnectToServer(LPCTSTR strAddr, UINT nPort, LPCTSTR pStrName)
{
	ASSERT(m_pServer == NULL);		// Must a client derived object
	BOOL _retval = FALSE;
	CString _hostip;
	_hostip.Format("%s", strAddr);

	SOCKET _sock = connect_to_server(_hostip, nPort);

	if (_sock)
	{
		TxSocketComm::sendDataString(_sock, pStrName);
		if (svc_recv_cmd(_sock) == tscmd_ack)
		{
			super::m_socket_handle = _sock;
			OnConnection();		// Spartan Connection Derived Object Callback
			_retval = TRUE;
		}
		else {
			destroy_svc_socket(_sock);
		}
	}

	return _retval;
}


void CSpartanConnection::ServerQuit()
{
	// Interrupt Service Command Message = "tscmd_site_server_quit"
	BOOL _retval = FALSE;
	SOCKET _sock = INVALID_SOCKET;

	_sock = connect_to_svchub(tscmd_site_server_quit);
	if (_sock != INVALID_SOCKET ) {
		destroy_svc_socket(_sock);
	}
}


TMessageCmd * CSpartanConnection::ReceiveCmd(LPCTSTR pStrName)
{
	TMessageCmd * pObj = NULL;
	CString _strMssg;

	if (m_data->m_mapMessages.Lookup(pStrName, _strMssg))
	{
		pObj = new TMessageCmd(pStrName, _strMssg);
	}

	return pObj;
}

void CSpartanConnection::SendCmd(TMessageCmd * pCmd)
{
	char _timebuf[16];
	u_int64 _timestamp;
	int _read_size = 0;
	CString _mssg_name;

#ifdef _ENABLE_MSSG_TIME_SVC_
	_timestamp = read_systime_precise();													// 0. Mark Begin Time Stamp
	ZeroMemory(_timebuf, 16);																// 1. Prep and copy timestamp into buffer
	CopyMemory(_timebuf, &_timestamp, sizeof(u_int64));
#endif // _ENABLE_MSSG_TIME_SVC_

	_read_size = pCmd->ReadMessage(_mssg_name, _CLIENT_DATA_BUF_SIZE_, m_data->GetBuffer());

	svc_send_cmd(super::m_socket_handle, tscmd_mssg_client_data);							// 2. Send svc cmd "tscmd_mssg_client_data"
	TxSocketComm::sendDataString(super::m_socket_handle, _mssg_name);						// 3. Send Message ID or Name
#ifdef _ENABLE_MSSG_TIME_SVC_
	TxSocketComm::sendBuffMssg(super::m_socket_handle, sizeof(time_t), _timebuf);			// 4. Send Time Stamp Buffer
#endif // _ENABLE_MSSG_TIME_SVC_
	TxSocketComm::sendBuffMssg(super::m_socket_handle, _read_size, m_data->GetBuffer());	// 5. Send Message Body


	// QoS? No Ack is needed .... "Unacknowledge, Acknowledge, Assured"
}


void CSpartanConnection::OnClose()
{
	// TODO: Callback from derived class
}


void CSpartanConnection::RunHandler()
{
	ASSERT(m_is_handler_running == FALSE);

	m_thread_handle = AfxBeginThread(threadProcHandler, this);
	ASSERT(m_thread_handle);

	m_is_handler_running = TRUE;
}

CString CSpartanConnection::GetName()
{
	return m_client_name;
}

void CSpartanConnection::RequestOnClose()
{
	if (m_pServer) {
		svc_send_cmd(super::m_socket_handle, tscmd_host_client_onclose);
	}
	else {
		svc_send_cmd(super::m_socket_handle, tscmd_site_client_quit);
	}
}


int CSpartanConnection::deserializeMessage()
{
	int _retval = 0;
	CString _message_id = TxSocketComm::recvDataString(super::m_socket_handle);
	CString _message_block;
	int _mssg_byte_size;
	char _time_buf[16];
	int _time_rcvd_size;
	u_int64 _timestamp_sent = 0;
	u_int64 _timestamp_rcvd = 0;

#ifdef _ENABLE_MSSG_TIME_SVC_
	ZeroMemory(_time_buf, 16);
	if (TxSocketComm::recvBuffMssg(super::m_socket_handle, 16, _time_buf, _time_rcvd_size))
	{
		CopyMemory(&_timestamp_sent, _time_buf, sizeof(u_int64));
	}
#endif // _ENABLE_MSSG_TIME_SVC_

	if (TxSocketComm::recvBuffMssg(super::m_socket_handle, _CLIENT_DATA_BUF_SIZE_, m_data->GetBuffer(), _mssg_byte_size))
	{
		_retval = m_data->ReadMessage(_message_block, _mssg_byte_size);
		m_data->m_mapMessages.SetAt(_message_id, _message_block);
#ifdef _ENABLE_MSSG_TIME_SVC_
		_timestamp_rcvd = read_systime_precise();
		TRACE("SpartanConnection Message Transit Time: %f", calc_precise_timestamp_diff(_timestamp_rcvd, _timestamp_sent));
#endif // _ENABLE_MSSG_TIME_SVC_
		OnReceived(_message_id);
	}

	return _retval;
}




UINT CSpartanConnection::threadProcHandler(LPVOID data)
{
	// This is a static member function of this class.
	// It is just like a regular free-function and it has its won
	// local variables every time it is called.
	// We will pass a pointer to class object of the same
	// type so we can access other members of this class.
	// How about synchronization ???
	// It is a must that the handling function we will call
	// should not or must not use any of its member data other than
	// to read from it.

	CSpartanConnection * pMe = (CSpartanConnection*)data;

	pMe->handler_run_connection(pMe);

	AfxEndThread(0);

	return 0;
}

void CSpartanConnection::handler_run_connection(CSpartanConnection * pConnection)
{
	SOCKET _socket = pConnection->m_socket_handle;
	BOOL _wait_on_loop = TRUE;
	tsvc_cmds _rcvd_tcmds;

	// SetThreadName ...

	while (_wait_on_loop)
	{
		switch (_rcvd_tcmds = wait_on_tscmd(_socket))
		{
		case tscmd_host_client_onclose:
			// From HostSvc to SiteSvc
			RequestOnClose();
			_wait_on_loop = FALSE;
			break;

		case tscmd_site_client_quit:
			// From SiteSvc to HostSvc
			_wait_on_loop = FALSE;
			break;

		case tscmd_mssg_client_data:
			deserializeMessage();
			break;

		case tscmd_invalid_socket:
			_wait_on_loop = FALSE;
			break;

		default:
			break;

		}
	}

	if (pConnection->exit_instance())
	{	// m_allow_auto_delete = TRUE
		delete pConnection;				// this is where we delete or destroy this instance.
	}
}


BOOL CSpartanConnection::exit_instance()
{
	OnClose();									// Invoke OnClose Callback from derived objects
	destroy_svc_socket(super::m_socket_handle);	// close the socket handle
	return m_allow_auto_delete;
}

