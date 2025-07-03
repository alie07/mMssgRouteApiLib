#ifndef _SPARTANSERVER_INCLUDE_H_
#define	_SPARTANSERVER_INCLUDE_H_
#pragma once

class CSpartanConnection;

class CSpartanServer : CWinsockBase
{
	typedef CWinsockBase super;

public:
	CSpartanServer();
	virtual ~CSpartanServer();
	BOOL BeginListening(LPCTSTR name, UINT uPort);	// Must be overidded by derived class

	void RemoveClient(CSpartanConnection * pClient);
	int GetClientCount();

protected:
	virtual CSpartanConnection * OnAcceptCreate(SOCKET sockClient, LPCTSTR pStrName) = 0;
	virtual void OnServerListen(UINT iPort) = 0;
	virtual void OnServerExit() = 0;
	virtual void OnRequestSvcQuit(LPCTSTR sender) = 0;


private:
	static UINT threadWaitConnect(LPVOID data);
	BOOL processClientConnect(SOCKET client);
	void processClientOnClose(LPCTSTR sender);
	void beginThreadService();
	void endThreadService();

	friend class SpartanServerData;
	SpartanServerData * m_data;
	CWinThread * m_pHostSvcThread;
	BOOL m_thread_is_running;
	CString m_hostname;
	UINT m_listen_port;

};




#endif // !_SPARTANSERVER_INCLUDE_H_

