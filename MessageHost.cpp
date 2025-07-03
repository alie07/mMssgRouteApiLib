#include "pch.h"

#include "dll_internal.h"

#define DECLSPEC_FUNC_EXPORT
#include "MessageHost.h"



CMap<CString, LPCTSTR, CSpartanConnection*, CSpartanConnection*> theHostActiveClients;

// The one and onlt Message Router Instance (Need this to be Singleton)
static IMessageHost * theSpartanMessageHostingInterfaceObject = NULL;
IMessageHost * GetHostI()
{
	ASSERT(theSpartanMessageHostingInterfaceObject);
	return theSpartanMessageHostingInterfaceObject;
}

CSpartanConnection * GetHostConnection(LPCTSTR pStrName)
{
	CSpartanConnection * _pObj = NULL;
	if (!theHostActiveClients.Lookup(pStrName, _pObj))
	{
		_pObj = NULL;
	}

	return _pObj;
}



// =============================================
// Client Connection Service
// ---------------------------------------------
class CHostSvcClient : public CSpartanConnection
{
	typedef CSpartanConnection super;
public:
	CHostSvcClient(SOCKET sockClient, LPCTSTR pStrName, CSpartanServer * pServerObj) : CSpartanConnection(sockClient, pStrName, pServerObj)
	{
	}

	virtual void OnConnection()
	{
		super::m_allow_auto_delete = TRUE;	// Ensure we auto delete inside the handler
		super::RunHandler();
	}

	virtual BOOL OnReceived(LPCTSTR pStrName)
	{
		TMessageCmd * pMssg = this->ReceiveCmd(pStrName);
		ASSERT(pMssg);
		CString _mssg_body;
		pMssg->GetMessageBody(_mssg_body);
		BOOL _retval = GetHostI()->OnMessageRcvd(super::GetName(), pStrName, _mssg_body);
		delete pMssg;
		return _retval;
	}


	virtual void OnClose()
	{	// Called by the handler
		CSpartanConnection * pObj = NULL;
		if (theHostActiveClients.Lookup(super::GetName(), pObj))
		{
			theHostActiveClients.RemoveKey(super::GetName());		// Here is where we remove this object from the Active Map
		}
		GetHostI()->OnCloseClient(super::GetName());
		ASSERT(super::m_allow_auto_delete);	// Check and ensure object has auto_delete = TRUE
		super::OnClose();
	}
};



// =============================================
// Helper Client Connection Service
// ---------------------------------------------
class CHostHelperClient : public CSpartanConnection
{
	typedef CSpartanConnection super;
public:

	CHostHelperClient() : CSpartanConnection()
	{

	}

	virtual void OnConnection()
	{
	}

	virtual BOOL OnReceived(LPCTSTR pStrName)
	{
		return TRUE;
	}


	virtual void OnClose()
	{	// Called by the handler
		super::OnClose();
	}

	void InterruptKillServer()
	{
		super::ServerQuit();
	}

};




// =============================================
// Host Server Connection Service
// ---------------------------------------------

class CHostMssgServer : public CSpartanServer
{
	CSpartanConnection * OnAcceptCreate(SOCKET sockClient, LPCTSTR pStrName)
	{
		CHostSvcClient * pObj = new CHostSvcClient(sockClient, pStrName, static_cast<CSpartanServer*>(this));
		ASSERT(pObj);

		theHostActiveClients.SetAt(pStrName, pObj);		// Here is where we add this object into the ActiveMap
		GetHostI()->OnAcceptClient(pStrName);

		return static_cast<CSpartanConnection*> (pObj);
	}

	void OnServerListen(UINT iPort)
	{
		m_server_port = iPort;
		GetHostI()->OnServerListen(iPort);
	}

	void OnServerExit()
	{
		GetHostI()->OnServiceClose();
	}

	void OnRequestSvcQuit(LPCTSTR sender)
	{
		CString _strmssg;
		_strmssg.Format("<cmd:server_quit> <sender: %s>", sender);
		GetHostI()->OnServiceInterrupt(_strmssg);
	}

private:
	UINT m_server_port;
};


static CHostMssgServer * m_pHostServerSocket;








IMessageHost::IMessageHost()
{
	theSpartanMessageHostingInterfaceObject = this;
	m_pHostServerSocket = new CHostMssgServer;
}

IMessageHost::~IMessageHost()
{
	ASSERT(theSpartanMessageHostingInterfaceObject == this);
	theSpartanMessageHostingInterfaceObject = NULL;
	delete m_pHostServerSocket;
}


IMessageHost * IMessageHost::GetInstance()
{
	return theSpartanMessageHostingInterfaceObject;
}

BOOL IMessageHost::InitInstance(LPCTSTR ipname, UINT ctlrport)
{
	BOOL _status = TRUE;
	_status &= m_pHostServerSocket->BeginListening(ipname, ctlrport);

	if (_status) {
		Sleep(10);
	}

	return _status;
}


void IMessageHost::KillServer()
{
	CHostHelperClient * _hostsvc = new CHostHelperClient();
	_hostsvc->InterruptKillServer();
	delete _hostsvc;
}


BOOL IMessageHost::SendEventMssg(LPCTSTR pStrName, LPCTSTR mssgid, LPCTSTR mssgbody)
{
	CSpartanConnection * _pConn = GetHostConnection(pStrName);
	ASSERT(_pConn);

	TMessageCmd * _pObj = new TMessageCmd(mssgid, mssgbody);
	_pConn->SendCmd(_pObj);

	delete _pObj;
	return TRUE;
}





