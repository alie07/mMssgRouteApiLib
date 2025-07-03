#include "pch.h"

#include "HydraDefineType.h"
#include "winsock_api_entries.h"
#include "svc_interface_entries.h"

#include "dll_internal.h"

class SpartanServerData
{
	friend class CSpartanServer;

	SpartanServerData(CSpartanServer * obj) : m_objOwner(obj)
	{
	}

	~SpartanServerData()
	{

	}

private:
	CSpartanServer * m_objOwner;
	CList< CSpartanConnection*, CSpartanConnection*> m_clientInstances;

};

CSpartanServer::CSpartanServer() : CWinsockBase()
{
	m_data = new SpartanServerData(this);
	m_pHostSvcThread = NULL;
	m_thread_is_running = FALSE;
	m_listen_port = NULL;
}

CSpartanServer::~CSpartanServer()
{
	delete m_data;
}

BOOL CSpartanServer::BeginListening(LPCTSTR name, UINT uPort)
{
	BOOL _retval = FALSE;
	SOCKET _sock = create_svc_server(uPort, name);

	if (_sock != INVALID_SOCKET)
	{
		super::m_socket_handle = _sock;
		m_pHostSvcThread = AfxBeginThread(threadWaitConnect, this);
		m_listen_port = uPort;
		OnServerListen(uPort);		// Derived Object Callback
		_retval = TRUE;
	}

	return _retval;
}

void CSpartanServer::RemoveClient(CSpartanConnection * pClient)
{
	ASSERT(pClient);
	POSITION posn = m_data->m_clientInstances.Find(pClient);
	if (posn != NULL) {
		m_data->m_clientInstances.RemoveAt(posn);
	}
}

int CSpartanServer::GetClientCount()
{
	return m_data->m_clientInstances.GetCount();
}





BOOL CSpartanServer::processClientConnect(SOCKET client)
{
	BOOL _retval = FALSE;
	CString _client_ip;
	CString _client_name;

	_client_ip = TxSocketComm::recvDataString(client);	// Receive the IP Name from Client
	_client_name = TxSocketComm::recvDataString(client);	// Receive the Service Name from Client

	CSpartanConnection * pObj = OnAcceptCreate(client, _client_name);
	ASSERT(pObj);

	if (pObj != NULL)
	{
		m_data->m_clientInstances.AddTail(pObj);
		svc_send_ack(client);
		pObj->OnConnection();		// Spartan Connection Derived Object Callback
		_retval = TRUE;
	}
	else {
		svc_send_cmd(client, tscmd_nack);
	}
	return _retval;
}

void CSpartanServer::processClientOnClose(LPCTSTR sender)
{
	OnRequestSvcQuit(sender);

	while (! m_data->m_clientInstances.IsEmpty())
	{
		CSpartanConnection * pObj = m_data->m_clientInstances.RemoveHead();
		pObj->RequestOnClose();
	}
}


void CSpartanServer::beginThreadService()
{
	ASSERT(m_thread_is_running == FALSE);
	m_thread_is_running = TRUE;
}

void CSpartanServer::endThreadService()
{
	ASSERT(m_thread_is_running);
	m_thread_is_running = FALSE;
	m_listen_port = NULL;
}


UINT CSpartanServer::threadWaitConnect(LPVOID data)
{
	// We are runing inside a Worker Thread so do not implement any thread
	// function in this block ...
	//	We need an extra scoping level here because we are using objects
	//	inside this procedure which need their destructors run.  If
	//	we leave out the extra level of nesting the _endthread() call 
	//	at the bottom of the procedure bypasses all destructors.
	{
		CSpartanServer * pMe = (CSpartanServer*)data;
		BOOL _wait_on_loop = TRUE;
		CString _sender_name;
		//CString _client_name;

		ASSERT(pMe);
		ASSERT(pMe->m_socket_handle);
		pMe->beginThreadService();

		while (_wait_on_loop)
		{
			SOCKET _sock_client = wait_accept_connect(pMe->m_socket_handle);
			ASSERT(_sock_client != INVALID_SOCKET);

			tsvc_cmds _recv_cmd = svc_recv_cmd(_sock_client);

			switch (_recv_cmd)
			{
			case tscmd_site_client_conn:
				if (pMe->processClientConnect(_sock_client) == FALSE) {
					_wait_on_loop = FALSE;
					destroy_svc_socket(_sock_client);
				}
				break;

			case tscmd_site_server_quit:
				_sender_name = TxSocketComm::recvDataString(_sock_client);
				Sleep(100);	// Let the initiator to clear its call before we proceed with the Server OnClose...
				_wait_on_loop = FALSE;
				destroy_svc_socket(_sock_client);
				pMe->processClientOnClose(_sender_name);
				break;

			default:
				destroy_svc_socket(_sock_client);
				break;
			}
		}
		destroy_svc_socket(pMe->m_socket_handle);
		pMe->m_socket_handle = INVALID_SOCKET;
		pMe->OnServerExit();			// invoke derived object's Callback 
		pMe->endThreadService();
	}
	AfxEndThread(0);
	return 0;
}
