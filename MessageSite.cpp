#include "pch.h"


#include "dll_internal.h"

#define DECLSPEC_FUNC_EXPORT
#include "MessageSite.h"


static IMessageSite * mm_the_only_one_Message_Site_Service_in_this_library = NULL;

IMessageSite * GetSiteI()
{
	ASSERT(mm_the_only_one_Message_Site_Service_in_this_library);
	return mm_the_only_one_Message_Site_Service_in_this_library;
}




// =============================================
// Spartan Client Connection Service
// ---------------------------------------------
class CSpartanSvcSite : public CSpartanConnection
{
	typedef CSpartanConnection super;
public:
	CSpartanSvcSite() : CSpartanConnection()
	{
	}

	virtual void OnConnection()
	{
		super::RunHandler();
	}

	virtual BOOL OnReceived(LPCTSTR pStrName)
	{
		TMessageCmd * pMssg = this->ReceiveCmd(pStrName);
		ASSERT(pMssg);
		CString _mssg_body;
		pMssg->GetMessageBody(_mssg_body);
		
		GetSiteI()->OnMessageProc(pStrName, _mssg_body);

		delete pMssg;
		return TRUE;
	}


	virtual void OnClose()
	{	// Called by the handler
		GetSiteI()->OnCloseSite();
		super::OnClose();
	}

	BOOL MakeConnection(LPCTSTR strAddr, UINT nPort, LPCTSTR pStrName)
	{
		return super::ConnectToServer(strAddr, nPort, pStrName);
	}

	void KillConnection()
	{
		super::RequestOnClose();
	}

};

CSpartanSvcSite * mm_pSiteSvcSocket = NULL;


IMessageSite::IMessageSite()
{
	ASSERT(mm_the_only_one_Message_Site_Service_in_this_library == NULL);
	mm_the_only_one_Message_Site_Service_in_this_library = this;
	mm_pSiteSvcSocket = new CSpartanSvcSite;
	ASSERT(mm_pSiteSvcSocket);
}

IMessageSite::~IMessageSite()
{
	mm_the_only_one_Message_Site_Service_in_this_library = NULL;
	delete mm_pSiteSvcSocket;
}

BOOL IMessageSite::InitInstance(LPCTSTR strAddr, UINT nPort, LPCTSTR pStrName)
{
	return mm_pSiteSvcSocket->MakeConnection(strAddr, nPort, pStrName);
}


BOOL IMessageSite::SendEventMssg(LPCTSTR mssgid, LPCTSTR mssgbody)
{
	ASSERT(mm_pSiteSvcSocket);
	TMessageCmd * _pObj = new TMessageCmd(mssgid, mssgbody);
	mm_pSiteSvcSocket->SendCmd(_pObj);
	delete _pObj;
	return TRUE;
}

void IMessageSite::CloseConnection()
{
	mm_pSiteSvcSocket->KillConnection();
}


