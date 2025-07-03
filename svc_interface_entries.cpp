#include "pch.h"
#include "TokenEx.h"
#include "HydraDefineType.h"
#include "SupportEntries.h"
#include "winsock_api_entries.h"
#include "svc_interface_entries.h"

//#include <libMTIpcComm\dll_internal.h>

UINT mm_port_used_for_this_connection = 0;

static int get_local_server_port()
{
	//return IP_SERVER_PORT;
	//return GetWindowVar(EMSG_SVC_SERVER_PORT);
	//return mm_port_used_for_this_connection;
	return GetWindowVar(EMSG_SVC_SERVER_PORT);
}

static void set_local_server_port(int port)
{
	//ASSERT( FALSE );
	//SetWindowVar(HCAS_SVC_SERVER_PORT, port, TRUE);
	//mm_port_used_for_this_connection = port;
	SetWindowVar(EMSG_SVC_SERVER_PORT, port, TRUE);
}



// =========================================
// Service Socket Send/Receive Command
// =========================================

void svc_send_cmd(SOCKET s, tsvc_cmds msgcmd)
{
	DWORD _timeout = 0;
	while (TxSocketComm::IsAvailable(s, 1, _timeout))
	{
		TxSocketComm::recvDataChar(s);
	}

	ASSERT((msgcmd >= tscmd_ack) && (msgcmd < tscmd_max_na));
	TxSocketComm::sendDataInt(s, msgcmd);
}

tsvc_cmds svc_recv_cmd(SOCKET s)
{
	try {
		tsvc_cmds msgcmd = tsvc_cmds(TxSocketComm::recvDataInt(s));
		ASSERT((msgcmd >= tscmd_ack) && (msgcmd < tscmd_max_na));
		return msgcmd;

	}
	catch (char * error) {
		TRACE("[svc_recv_cmd] rcv_message( s=%d/%s ) failed: %s\n", s, TxSocketComm::getPeerName(s), error);
		closesocket(s);
		return tscmd_invalid_socket;
	}
}

void svc_send_ack(SOCKET s)
{
	svc_send_cmd(s, tscmd_ack);
}

static BOOL svc_recv_ack(SOCKET s)
{
	return svc_recv_cmd(s) == tscmd_ack;
}

void transmit_svcmcd_to_hub(SOCKET s, tsvc_cmds command)
{
	CString _str = get_ip_address();
	svc_send_cmd(s, command);
	TxSocketComm::sendDataString(s, _str);
}


SOCKET connect_client_to_server(const char *host, int port, tsvc_cmds command, DWORD retries = INFINITE)
{
	CString _str = get_ip_address();

	do {
		SOCKET s = TxSocketComm::ConnectTo(host, port, retries);
		if (s != INVALID_SOCKET) {
			try {
				svc_send_cmd(s, command);
				TxSocketComm::sendDataString(s, _str);
				TRACE("connect_client_to_server -> Completed sending service cmd: %d\n", command);
				return s;
			}
			catch (char *error) {
				TRACE("connect_client_to_server( s=%d/%s, host=%s, port=%d ) has failed: %s\n", s, TxSocketComm::getPeerName(s), host, port, error);
				closesocket(s);
			}
		}
		if (retries != 0 && retries != INFINITE) --retries;

		if (retries != 0) Sleep(100);

	} while (retries > 0);

	return INVALID_SOCKET;
}

SOCKET connect_to_svchub(tsvc_cmds command)
{
	// Note: We must have an ability to do "Retry Connect"
	while (TRUE)
	{
		// Let's keep on trying as long as we have its Port Number registered
		// in our windows object ...
		int _port = get_local_server_port();

		if (_port == 0) return INVALID_SOCKET;

		SOCKET sockhost = connect_client_to_server(EMSG_SVC_SERVER_LOCAL_HOST, _port, command, 0);

		if (sockhost != INVALID_SOCKET) return sockhost;
	}
	// We must not reach this point ...
	return -1;
}


#define	POLL_TIMER_SERVICE_PING		(10000)		// 10 Seconds

static void service_ping(HANDLE hStop)
{
	struct local {

		static DWORD SvcThreadFunc(void * arg)
		{
			HANDLE h_stop_ev = (HANDLE)arg;

			int port = get_local_server_port();

			// Do we Retry getting server port
			while (port == 0) {
				Sleep(100);
				TRACE("service_ping has invalid port, retry ... \n");
				port = get_local_server_port();
			}

			BOOL _run_svc_ping = TRUE;
			while (_run_svc_ping) {

				DWORD _wait_stat = WaitForSingleObject(h_stop_ev, POLL_TIMER_SERVICE_PING);

				switch (_wait_stat)
				{
				case WAIT_TIMEOUT:
				{
					SOCKET sockhost = connect_client_to_server("127.0.0.1", port, tscmd_poll_clients, 0);
					svc_recv_ack(sockhost);
					closesocket(sockhost);
				}
				break;

				case WAIT_OBJECT_0:
					_run_svc_ping = FALSE;
					break;

				default:
					break;
				}
			}
			TRACE("Exiting service ping thread ...\n");
			return 0;
		}
	};

	VERIFY(CloseHandle(CreateThread(local::SvcThreadFunc, (void *)hStop)));

}

static BOOL fetch_site_info(CStringArray & rArrSites)
{
	SOCKET sockhost = connect_to_svchub(tscmd_fetch_client_id);
	if (sockhost == INVALID_SOCKET) return FALSE;

	BOOL _retval = TRUE;

	try {
		TxSocketComm::recvDataArray(sockhost, rArrSites);
	}
	catch (char *error) {
		TRACE("local->fetch_site_info( s=%d/%s ) has failed: %s\n", sockhost, TxSocketComm::getPeerName(sockhost), error);
		_retval = FALSE;
	}

	closesocket(sockhost);

	return _retval;
}

BOOL fetch_active_client_id(CStringArray & rArrSites)
{
	return fetch_site_info(rArrSites);
}

BOOL ping_local_server()
{
	return fetch_site_info(CStringArray());
}

BOOL fetch_dhcp_server_ipname(CString & ipname)
{
	SOCKET sockhost = connect_to_svchub(tscmd_fetch_dhcp_svr_ip);
	if (sockhost == INVALID_SOCKET) return FALSE;

	BOOL _retval = TRUE;

	try {
		ipname.Format("%s", TxSocketComm::recvDataString(sockhost));
	}
	catch (char *error) {
		TRACE("local->fetch_dhcp_server_ipname( s=%d/%s ) has failed: %s\n", sockhost, TxSocketComm::getPeerName(sockhost), error);
		_retval = FALSE;
	}

	closesocket(sockhost);

	return _retval;
}






// =============================================
// Invoke Socket Server to Run ...
// =============================================
SOCKET create_svc_server(int iPort, CString hostip, HANDLE hStop)
{
	int _retval = NO_ERROR;
	// HostXentral Server Port should be checked from top WinApp ...
	// This function will be called if and only if there's no Server ....

	SOCKET server = TxSocketComm::CreateServer(iPort);

	if (server != INVALID_SOCKET)
	{
		set_local_server_port(iPort);	// Let's publish the Port number so the rest of Apps can receive this same value ...
		//service_ping(hStop);
	}

	return server;
}


SOCKET wait_accept_connect(SOCKET server)
{
	SOCKET client = TxSocketComm::WaitForConnect(server);
	return client;
}


SOCKET wait_accept_client(SOCKET server, tsvc_cmds respCmd, CString & rClientName)
{
	SOCKET client = TxSocketComm::WaitForConnect(server);
	ASSERT(client != INVALID_SOCKET);

	tsvc_cmds _replycmd = svc_recv_cmd(client);

	if (_replycmd == respCmd)
	{
		rClientName = TxSocketComm::recvDataString(client);	// Receive the IP Name from Client
	}
	else {
		closesocket(client);
		client = INVALID_SOCKET;
		TRACE("[wait_accept_client] Invalid service connection \n");
	}

	return client;
}

BOOL send_tsvc_mssg_to_client(SOCKET client, tsvc_cmds cmd)
{
	BOOL _retval = TRUE;
	svc_send_cmd(client, cmd);

	return svc_recv_ack(client);
}


void destroy_svc_socket(SOCKET s)
{
	closesocket(s);
}


SOCKET connect_to_server(CString & hostip, int port)
{
	SOCKET _sockset = INVALID_SOCKET;
	_sockset = connect_client_to_server(hostip, port, tscmd_site_client_conn);
	return _sockset;
}


tsvc_cmds wait_on_tscmd(SOCKET host)
{
	return svc_recv_cmd(host);
}


void send_ack_to_host(SOCKET host)
{
	svc_send_ack(host);
}



