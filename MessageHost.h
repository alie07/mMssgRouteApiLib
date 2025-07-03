#ifndef _MESSAGEHOST_INCLUDE_H_
#define _MESSAGEHOST_INCLUDE_H_
#pragma once

#include <dll_export.h>

class DLL_FUNC IMessageHost
{
public:
	IMessageHost();
	virtual ~IMessageHost();
	static IMessageHost * GetInstance();

	virtual void OnServerListen(UINT uPort) = 0;
	virtual void OnAcceptClient(LPCTSTR pStrName) = 0;
	virtual void OnCloseClient(LPCTSTR pStrName) = 0;
	virtual BOOL OnMessageRcvd(LPCTSTR pStrName, LPCTSTR pMssgName, LPCTSTR pMssgBody) = 0;		// Make sure override return a TRUE to send fan out.
	virtual void OnServiceClose() = 0;
	virtual void OnServiceInterrupt(LPCTSTR pStrMssg) = 0;

	BOOL SendEventMssg(LPCTSTR pStrName, LPCTSTR mssgid, LPCTSTR mssgbody);

protected:
	BOOL InitInstance(LPCTSTR ipname, UINT ctlrport);
	void KillServer();
};



#endif // !_MESSAGEHOST_INCLUDE_H_
