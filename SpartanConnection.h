#ifndef _SPARTANCONNECTION_INCLUDE_H_
#define	_SPARTANCONNECTION_INCLUDE_H_
#pragma once

#include "WinsockBase.h"



class CSpartanConnection : public CWinsockBase
{
	typedef CWinsockBase super;
	friend class CSpartanServer;
	friend class TMessageCmd;

public:
	CSpartanConnection();																		// Object construction on the Client Process
	CSpartanConnection(SOCKET sockClient, LPCTSTR pStrName, CSpartanServer * pServerObj);		// Object construction on the Server Process
	virtual ~CSpartanConnection();

	TMessageCmd * ReceiveCmd(LPCTSTR pStrName);				// The derived class must be responsible for destroying TMessageCmd
	void SendCmd(TMessageCmd * pCmd);

protected:
	// Virtual Prototypes
	virtual void OnConnection() = 0;
	virtual BOOL OnReceived(LPCTSTR pStrName) = 0;
	
	// Virtual Overrides
	virtual void OnClose();

	// Protected functions and variables
	BOOL ConnectToServer(LPCTSTR strAddr, UINT nPort, LPCTSTR pStrName);						//At Client Process, Create and Connect to Server Socket
	void RunHandler();
	CString GetName();
	void RequestOnClose();

	BOOL m_allow_auto_delete;

	void ServerQuit();

private:
	BOOL exit_instance();
	void handler_run_connection(CSpartanConnection * pConnection);
	static UINT threadProcHandler(LPVOID data);

	int deserializeMessage();

	BOOL m_is_handler_running;
	CWinThread * m_thread_handle;

	CSpartanServer * m_pServer;
	CString m_client_name;
	
	friend class CSpartanClientData;
	CSpartanClientData * m_data;

};




#endif // !_SPARTANCONNECTION_INCLUDE_H_
