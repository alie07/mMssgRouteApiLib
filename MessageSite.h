#ifndef _MESSAGESITE_INCLUDE_H_
#define	_MESSAGESITE_INCLUDE_H_
#pragma once

#include <dll_export.h>

class DLL_FUNC IMessageSite
{
public:
	IMessageSite();
	virtual ~IMessageSite();

	virtual void OnCloseSite() = 0;
	virtual void OnMessageProc(LPCTSTR mssgid, LPCTSTR mssgbody) = 0;

	BOOL SendEventMssg(LPCTSTR mssgid, LPCTSTR mssgbody);

protected:
	BOOL InitInstance(LPCTSTR strAddr, UINT nPort, LPCTSTR pStrName);
	void CloseConnection();

};


#endif // !_MESSAGESITE_INCLUDE_H_

