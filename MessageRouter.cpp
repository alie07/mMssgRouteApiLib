#include "pch.h"

#include "dll_internal.h"

#define DECLSPEC_FUNC_EXPORT
#include "MessageCmd.h"
#include "MessageRouter.h"



CMap<CString, LPCTSTR, CSpartanConnection*, CSpartanConnection*> theMapActiveClients;

// The one and onlt Message Router Instance (Need this to be Singleton)
static IMessageRouter * theSpartanMessageRoutingInterfaceObject = NULL;
IMessageRouter * GetRouterI()
{
	ASSERT(theSpartanMessageRoutingInterfaceObject);
	return theSpartanMessageRoutingInterfaceObject;
}

CSpartanConnection * GetConnection(LPCTSTR pStrName)
{
	CSpartanConnection * _pObj = NULL;
	if (! theMapActiveClients.Lookup(pStrName, _pObj))
	{
		_pObj = NULL;
	}

	return _pObj;
}

void FanoutMessage(LPCTSTR pConnName, TMessageCmd * pMssgObj)
{
	CSpartanConnection * _pConn = NULL;
	CString _keyName;
	CString _fromName = pConnName;
	POSITION _posn = theMapActiveClients.GetStartPosition();
	while (_posn)
	{
		theMapActiveClients.GetNextAssoc(_posn, _keyName, _pConn);
		if (!(_keyName == _fromName))
		{
			_pConn->SendCmd(pMssgObj);
		}
	}
}


// =============================================
// Spartan Client Connection Service
// ---------------------------------------------
class CSpartanSvcClient : public CSpartanConnection
{
	typedef CSpartanConnection super;
public:
	CSpartanSvcClient(SOCKET sockClient, LPCTSTR pStrName, CSpartanServer * pServerObj) : CSpartanConnection(sockClient, pStrName, pServerObj)
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
		if (GetRouterI()->OnMessageRcvd(pStrName, _mssg_body) )
		{
			FanoutMessage(super::GetName(), pMssg);
		}
		delete pMssg;

		return TRUE;
	}


	virtual void OnClose()
	{	// Called by the handler
		CSpartanConnection * pObj = NULL;
		if (theMapActiveClients.Lookup(super::GetName(), pObj))
		{
			theMapActiveClients.RemoveKey(super::GetName());		// Here is where we remove this object from the Active Map
		}
		GetRouterI()->OnCloseClient(super::GetName());
		ASSERT(super::m_allow_auto_delete);	// Check and ensure object has auto_delete = TRUE
		super::OnClose();
	}
};




class CSpartanHelperClient : public CSpartanConnection
{
	typedef CSpartanConnection super;
public:

	CSpartanHelperClient() : CSpartanConnection()
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
// Spartan Server Connection Service
// ---------------------------------------------

class CSpartanHubServer : public CSpartanServer
{
	CSpartanConnection * OnAcceptCreate(SOCKET sockClient, LPCTSTR pStrName)
	{
		CSpartanSvcClient * pObj = new CSpartanSvcClient(sockClient, pStrName, static_cast<CSpartanServer*>(this));
		ASSERT(pObj);

		theMapActiveClients.SetAt(pStrName, pObj);		// Here is where we add this object into the ActiveMap
		GetRouterI()->OnAcceptClient(pStrName);

		return static_cast<CSpartanConnection*> (pObj);
	}

	void OnServerListen(UINT iPort)
	{
		m_server_port = iPort;
		GetRouterI()->OnServerListen(iPort);
	}

	void OnServerExit()
	{
		GetRouterI()->OnServiceClose();
	}

	void OnRequestSvcQuit(LPCTSTR sender)
	{
		CString _strmssg;
		_strmssg.Format("<cmd:server_quit> <sender: %s>", sender);
		GetRouterI()->OnServiceInterrupt(_strmssg);
	}


private:
	UINT m_server_port;

};


static CSpartanHubServer m_this_server_hub;








IMessageRouter::IMessageRouter()
{
	theSpartanMessageRoutingInterfaceObject = this;
}

IMessageRouter::~IMessageRouter()
{
	ASSERT(theSpartanMessageRoutingInterfaceObject == this);
	theSpartanMessageRoutingInterfaceObject = NULL;
}


IMessageRouter * IMessageRouter::GetInstance()
{
	return theSpartanMessageRoutingInterfaceObject;
}

BOOL IMessageRouter::InitInstance(CWinApp * app, LPCTSTR ipname, UINT ctlrport)
{
	BOOL _status = TRUE;
	_status &= m_this_server_hub.BeginListening(ipname, ctlrport);
	
	if (_status) {
		Sleep(10);
	}
	
	return _status;
}


void IMessageRouter::KillServer()
{
	CSpartanHelperClient * _hostsvc = new CSpartanHelperClient();
	_hostsvc->InterruptKillServer();
	delete _hostsvc;
}

