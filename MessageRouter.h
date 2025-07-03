#ifndef _MESSAGEROUTER_INCLUDE_H_
#define	_MESSAGEROUTER_INCLUDE_H_
#pragma once

#include <dll_export.h>

class DLL_FUNC IMessageRouter
{
public:
	IMessageRouter();
	virtual ~IMessageRouter();
	static IMessageRouter * GetInstance();

	virtual void OnServerListen(UINT uPort) = 0;
	virtual void OnAcceptClient(LPCTSTR pStrName) = 0;
	virtual void OnCloseClient(LPCTSTR pStrName) = 0;
	virtual BOOL OnMessageRcvd(LPCTSTR pMssgName, LPCTSTR pMssgBody) = 0;		// Make sure override return a TRUE to send fan out.
	virtual void OnServiceClose() = 0;
	virtual void OnServiceInterrupt(LPCTSTR pStrMssg) = 0;

protected:
	BOOL InitInstance(CWinApp * app, LPCTSTR ipname, UINT ctlrport);
	void KillServer();


};



#endif // !_MESSAGEROUTER_INCLUDE_H_



