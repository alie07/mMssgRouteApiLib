#include "pch.h"

#include <sys/timeb.h>

#include "winsock_api_entries.h"
#include "SupportEntries.h"

#pragma comment(lib, "wsock32.lib")
typedef int socklen_t;
typedef u_long in_addr_t;


static struct WSAInitObject
{
	WSAInitObject() {
		WSADATA wsaData;
		WSAStartup(MAKEWORD(1, 1), &wsaData);
	}
	~WSAInitObject() {
		WSACleanup();
	}
} mmInitWsaObject;

struct SaveWinsockError {
	SaveWinsockError() {
		error = WSAGetLastError();
	}
	~SaveWinsockError() {
		WSASetLastError(error);
	}
	operator int() {
		return error;
	}
private:
	int error;
};



static SOCKET create_socket(int(__stdcall *execf)(SOCKET s, const sockaddr *name, socklen_t len), int port, LPCTSTR host = 0)
{
	SOCKADDR_IN _sin;
	_sin.sin_family = AF_INET;
	_sin.sin_port = htons(port);	// Convert u_short to TCP/IP Network Byte Order (big-endian)
	in_addr_t &quad = _sin.sin_addr.s_addr;


	if (!host) {
		quad = INADDR_ANY;
	}
	else if ((quad = inet_addr(host)) == INADDR_NONE) {
		hostent *lphost = gethostbyname(host);
		if (lphost == NULL)
			return INVALID_SOCKET;
		memcpy(&quad, lphost->h_addr, sizeof quad);
	}

	SOCKET s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (execf(s, (sockaddr *)&_sin, sizeof _sin) == SOCKET_ERROR)
	{
		SaveWinsockError pe;
		closesocket(s);
		return INVALID_SOCKET;
	}

	return s;
}


static BOOL retry(int(*execf)(SOCKET s, char *buf, int len), SOCKET socket, char *data, int length)
{
	if (socket == INVALID_SOCKET)
		return FALSE;

	int qlen = length;
	while (length) {
		int bytes = execf(socket, data, min(length, qlen));
		if (bytes == 0)	// detect when remote socket was shut down gracefully
			return FALSE;
		if (bytes == SOCKET_ERROR) {
			switch (WSAGetLastError()) {
				//			case WSAEWOULDBLOCK:
				//				continue;
			case WSAENOBUFS:
				if (qlen >= 2) {
					qlen /= 2;
					continue;
				}
			}
			return FALSE;
		}
		data += bytes;
		length -= bytes;
	}
	return TRUE;
}





// =======================================================
//  API Function Entries 
// =======================================================


#ifdef NO_SOCKET_EXCEPTIONS
#define ERROR_EXCEPTION_RETURN( s, x )	return x( );
#else
#define ERROR_EXCEPTION_RETURN( s, x )							\
								{	SaveWinsockError sock_err;													\
									static CString strMsgErr;														\
									strMsgErr.Format( "TxSocketComm Winsock Exception Error %d on %d(%s)", sock_err, s, getPeerName( s ) );	\
									throw( (char *)(LPCTSTR) strMsgErr );											\
								}
#endif





static DWORD get_ftime()
{
	// This will wrap around, but we only subtract them from each other so it doesn't matter
	timeb now;
	ftime(&now);
	return DWORD(now.time * 1000 + now.millitm);
}





SOCKET TxSocketComm::CreateServer(int iPort)
{
	// Create Socket for server/listener...
	SOCKET server = create_socket(bind, iPort);

	if (listen(server, 5) == SOCKET_ERROR)
	{
		TRACE("[TxSocketComm::create_server] Failed at socket::listen \n");
		SaveWinsockError pe;
		closesocket(server);
		server = INVALID_SOCKET;
	}

	return server;
}

SOCKET TxSocketComm::CheckCreateServer(int * iPort)
{
	if (iPort == 0) return INVALID_SOCKET;

	SOCKET server;
	int _portnum = 5000;
	int _max_count = 100;

	for (int _portx = _portnum; _portx <= (_portnum + _max_count); ++_portx)
	{
		if ((server = CreateServer(_portx)) != INVALID_SOCKET)
		{
			(*iPort) = _portx;
			break;
		}
	}

	return server;
}



SOCKET TxSocketComm::WaitForConnect(SOCKET server)
{
	// Accept an incoming connection - blocking
	// no information about remote address is returned
	return accept(server, NULL, 0);
}

SOCKET TxSocketComm::ConnectTo(LPCTSTR host, int iPort, DWORD retry = 0)
{
	do {
		SOCKET _connsock = create_socket(connect, iPort, host);
		if (_connsock != INVALID_SOCKET) {
			TRACE("connect_to_server -> Successfull: %d\n", _connsock);
			return _connsock;
		}
		if (retry != 0 && retry != INFINITE) --retry;
		if (retry != 0) Sleep(100);
	} while (retry > 0);

	return INVALID_SOCKET;
}

int TxSocketComm::getSockPort(SOCKET s)
{
	sockaddr_in addr;
	socklen_t len = sizeof addr;
	return ::getsockname(s, (sockaddr *)&addr, &len) ? 0 : ntohs(addr.sin_port);
}

int TxSocketComm::getPeerPort(SOCKET s)
{
	sockaddr_in addr;
	socklen_t len = sizeof addr;
	return ::getpeername(s, (sockaddr *)&addr, &len) ? 0 : ntohs(addr.sin_port);
}


BOOL TxSocketComm::sendData(SOCKET s, const char *data, int len)
{
	struct stcmdlocal { static int send(SOCKET s, char *buf, int len) { return ::send(s, buf, len, 0); } };
	if (len == 0) return TRUE;
	return retry(stcmdlocal::send, s, (char *)data, len);
}

BOOL TxSocketComm::sendDataInt(SOCKET s, int i)
{
	if (sendData(s, (char *)&i, sizeof i))
		return TRUE;

	ERROR_EXCEPTION_RETURN(s, BOOL);
}

BOOL TxSocketComm::sendDataChar(SOCKET s, char c)
{
	if (sendData(s, (char*)&c, sizeof c))
		return TRUE;

	ERROR_EXCEPTION_RETURN(s, BOOL);
}





// Return TRUE if 'want' bytes are available for from 's' within a given amount of time.
BOOL TxSocketComm::IsAvailable(SOCKET s, DWORD want, DWORD &msec)
{
	DWORD start = get_ftime();
	do {
		fd_set read;
		FD_ZERO(&read);
		FD_SET(s, &read);

		timeval timeout = { msec / 1000, (msec % 1000) * 1000 };
		DWORD bytes = 0;
		if (select(s + 1, &read, 0, 0, (msec == INFINITE ? 0 : &timeout)) == 1 && ioctlsocket(s, FIONREAD, &bytes) == 0 && bytes >= want)
			return TRUE;

		DWORD now = get_ftime();
		msec -= min(msec, now - start);
		start = now;

		Sleep(max(50, msec / 4));

	} while (msec > 0);

	return FALSE;
}

BOOL TxSocketComm::recvData(SOCKET s, char *data, int len)
{
	struct stcmdlocal { static int recv(SOCKET s, char *buf, int len) { return ::recv(s, buf, len, 0); } };
	if (len == 0) return TRUE;
	return retry(stcmdlocal::recv, s, data, len);
}

BOOL TxSocketComm::recvDataInt(SOCKET s, int *i)
{
	return recvData(s, (char *)i, sizeof *i);
}

char TxSocketComm::recvDataChar(SOCKET s)
{
	char c;
	if (recvData(s, &c, sizeof c))
		return c;
	ERROR_EXCEPTION_RETURN(s, char);
}


BOOL TxSocketComm::recvDataChar(SOCKET s, char *c)
{
	return recvData(s, c, sizeof *c);
}


int TxSocketComm::recvDataInt(SOCKET s)
{
	int i;
	if (recvData(s, (char *)&i, sizeof i))
		return i;
	ERROR_EXCEPTION_RETURN(s, int);
}



CString TxSocketComm::getSockName(SOCKET s)
{
	sockaddr_in addr;
	socklen_t len = sizeof addr;
	return (::getsockname(s, (sockaddr *)&addr, &len) == SOCKET_ERROR) ? "" : inet_ntoa(addr.sin_addr);
}

CString TxSocketComm::getPeerName(SOCKET s)
{
	sockaddr_in addr;
	socklen_t len = sizeof addr;
	return (::getpeername(s, (sockaddr *)&addr, &len) == SOCKET_ERROR) ? "" : inet_ntoa(addr.sin_addr);
}

CString TxSocketComm::getHostName()
{
	char name[1024];
	VERIFY(::gethostname(name, sizeof name) == 0);
	return name;
}


BOOL TxSocketComm::sendBuffMssg(SOCKET s, int len, const char * pBuffer)
{
	if (sendDataInt(s, len) && sendData(s, pBuffer, len))
		return TRUE;
	ERROR_EXCEPTION_RETURN(s, BOOL);
}

BOOL TxSocketComm::sendSockMssg(SOCKET s, int len, LPCTSTR pBuffer)
{
	if (sendDataInt(s, len) && sendData(s, pBuffer, len))
		return TRUE;
	ERROR_EXCEPTION_RETURN(s, BOOL);
}

BOOL TxSocketComm::recvSockMssg(SOCKET s, CString * string)
{
	//Exact replica as recvDataString ...

	int _strsize;
	if (!recvDataInt(s, &_strsize)) return FALSE;
	char * _buff = (char*)alloca(_strsize);
	if (!recvData(s, _buff, _strsize)) return FALSE;
	CopyMemory(string->GetBufferSetLength(_strsize), _buff, _strsize);
	return TRUE;
}

BOOL TxSocketComm::recvBuffMssg(SOCKET s, int bufsize, char * pBuffer, int & rcvSize)
{
	if (!recvDataInt(s, &rcvSize)) {
		rcvSize = -1;
		return FALSE;
	}

	if (rcvSize > bufsize) {
		return FALSE;
	}

	if (!recvData(s, pBuffer, rcvSize)) return FALSE;

	return TRUE;
}

BOOL TxSocketComm::readBuffMssgFromSock(SOCKET s, int readSize, char * pBuffer)
{
	if (!recvData(s, pBuffer, readSize)) return FALSE;
	return TRUE;
}


BOOL TxSocketComm::sendDataString(SOCKET s, CString string)
{
	int length = string.GetLength();
	if (sendDataInt(s, length) && sendData(s, string, length))
		return TRUE;
	ERROR_EXCEPTION_RETURN(s, BOOL);
}

BOOL TxSocketComm::recvDataString(SOCKET s, CString *string)
{
	int length;
	if (!recvDataInt(s, &length))
		return FALSE;
	char *buf = (char *)alloca(length);
	if (!recvData(s, buf, length))
		return FALSE;
	CopyMemory(string->GetBufferSetLength(length), buf, length);
	return TRUE;
}

CString TxSocketComm::recvDataString(SOCKET s)
{
	int length = recvDataInt(s);
	char *buf = (char *)alloca(length);
	if (recvData(s, buf, length)) {
		// Deal with embedded 0's in the data
		CString string;
		CopyMemory(string.GetBufferSetLength(length), buf, length);
		return string;
	}
	ERROR_EXCEPTION_RETURN(s, CString);
}



BOOL TxSocketComm::sendDataArray(SOCKET s, CStringArray &array)
{
	// Send count
	BOOL res = sendDataInt(s, array.GetSize());

	// Send strings
	for (int i = 0; i < array.GetSize(); ++i)
		res &= sendDataString(s, array[i]);

	return res;
}

BOOL TxSocketComm::recvDataArray(SOCKET s, CStringArray &array)
{
	// Get count
	int size = 0;
	if (!recvDataInt(s, &size)) return FALSE;
	array.SetSize(size);

	// Get strings
	BOOL res = TRUE;
	for (int i = 0; i < size; ++i)
		res &= recvDataString(s, &array[i]);

	return res;
}




