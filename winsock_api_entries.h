#ifndef _WINSOCKAPIENTRIES_H_
#define _WINSOCKAPIENTRIES_H_
#pragma once


class TxSocketComm
{
public:
	static SOCKET CreateServer(int iPort);
	static SOCKET CheckCreateServer(int * iPort);
	static SOCKET WaitForConnect(SOCKET server);
	static SOCKET ConnectTo(LPCTSTR host, int iPort, DWORD retry);
	static int getSockPort(SOCKET s);
	static int getPeerPort(SOCKET s);
	static BOOL sendData(SOCKET s, const char *data, int len);
	static BOOL sendDataInt(SOCKET s, int i);
	static BOOL sendDataChar(SOCKET s, char c);
	static BOOL IsAvailable(SOCKET s, DWORD want, DWORD &msec);
	static BOOL recvData(SOCKET s, char *data, int len);
	static BOOL recvDataInt(SOCKET s, int *i);
	static char recvDataChar(SOCKET s);
	static BOOL recvDataChar(SOCKET s, char *c);
	static int recvDataInt(SOCKET s);
	static CString getSockName(SOCKET s);
	static CString getPeerName(SOCKET s);
	static CString getHostName();

	static BOOL sendBuffMssg(SOCKET s, int len, const char * pBuffer);
	static BOOL recvBuffMssg(SOCKET s, int bufsize, char * pBuffer, int & rcvSize);
	static BOOL readBuffMssgFromSock(SOCKET s, int readSize, char * pBuffer);

	static BOOL sendSockMssg(SOCKET s, int len, LPCTSTR pBuffer);
	static BOOL recvSockMssg(SOCKET s, CString * string);
	static BOOL sendDataString(SOCKET s, CString string);
	static BOOL recvDataString(SOCKET s, CString *string);
	static CString recvDataString(SOCKET s);
	static BOOL sendDataArray(SOCKET s, CStringArray &array);
	static BOOL recvDataArray(SOCKET s, CStringArray &array);
};




#endif
