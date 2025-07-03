#include "pch.h"



class CWndVarBase : public CWnd
{
public:
	CWndVarBase(CString name = "", int value = 0) : m_value(value) {
		CreateEx(0, "static", name, 0, CRect(), 0, 0, 0);
	}

	~CWndVarBase() {
		DestroyWindow();
	}

	CString GetPublisherName(CString name)
	{
		CString _result;
		_result.Format("%s Publisher Windows VarAction at %d", name, m_wndmsg_action);
		return _result;
	}

	LRESULT Send(HWND hwnd, int value)
	{
		return ::SendMessage(hwnd, m_wndmsg_action, value, 0);
	}

	int m_value;

private:
	virtual int Action(int wParam) = 0;
	LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
	{
		if (message == m_wndmsg_action)
			return Action(wParam);
		else
			return CWnd::WindowProc(message, wParam, lParam);
	}

	static UINT m_wndmsg_action;
};

UINT CWndVarBase::m_wndmsg_action = RegisterWindowMessage("Windows Message Variable Name :: UINT m_wndmsg_action");



class CPublishVar : public CWndVarBase
{
	typedef CWndVarBase super;
public:
	CPublishVar(CString name, int value) : CWndVarBase(super::GetPublisherName(name), value)
	{
	}

private:
	int Action(int wParam)
	{
		return super::Send(HWND(wParam), m_value);
	}
};


class CSubscribeVar : public CWndVarBase
{
	typedef CWndVarBase super;
public:
	int Get(CString name)
	{
		Send(::FindWindowA("static", super::GetPublisherName(name)), int(m_hWnd));
		return m_value;
	}

private:
	int Action(int wParam)
	{
		m_value = wParam;
		return 1;
	}
};

class CMapHelper
{
public:
	CMapHelper()
	{
		m_map.RemoveAll();
	}

	~CMapHelper()
	{
		// Detroy Objects and Clean up Map whenever we leave this module ...
		if (!m_map.IsEmpty())
		{
			CString _key;
			CPublishVar * _pObj;
			POSITION _posn = m_map.GetStartPosition();
			while (_posn)
			{
				m_map.GetNextAssoc(_posn, _key, _pObj);
				delete _pObj;
			}
			m_map.RemoveAll();
		}
		TRACE("Destroying Publisher's CMAP ... \n");
	}

	CPublishVar * AddPublisher(CString name, int value)
	{
		CPublishVar * _pObj = new CPublishVar(name, value);
		ASSERT(_pObj);
		m_map.SetAt(name, _pObj);
		return _pObj;
	}

private:
	CMap<CString, LPCTSTR, CPublishVar*, CPublishVar* > m_map;
};

// Create a static object that will support Publisher CMap ...
static CMapHelper mm_PubMap;



static void AddPublisher(CString name, int value)
{
	ASSERT(mm_PubMap.AddPublisher(name, value));
}

int GetWindowVar(LPCTSTR name)
{
	return CSubscribeVar().Get(name);
}


BOOL SetWindowVar(LPCTSTR name, int value, BOOL runOnThread)
{
	if (value == 0) return FALSE;

	int _has_value = GetWindowVar(name);

	// Always Unique Variable Value
	if (_has_value)
	{
		TRACE("Warning: A Windows Variable %s already has value %d. (%d will be ignored).\n", name, _has_value, value);
		return FALSE;
	}

	if (!runOnThread)
	{
		AddPublisher(name, value);
		return TRUE;
	}

	struct PubThread {
		//static DWORD ThreadFunc( void * arg )
		static UINT ThreadFunc(LPVOID arg)
		{
			PubThread * args = static_cast<PubThread *>(arg);
			AddPublisher(args->name, args->value);
			delete args;
			HANDLE _handle = CEvent();

			DWORD _retval = WaitForMultipleObjects(1, &_handle, FALSE, 0);

			AfxEndThread(0);
			TRACE("Exiting PubThread ...\n");
			return _retval;
		}
		PubThread(CString name, int value) : name(name), value(value) { }
		CString name;
		int value;

	} *args = new PubThread(name, value);

	AfxBeginThread(args->ThreadFunc, args);

	BOOL _retval = TRUE;
	//_retval = CloseHandle( CreateThread( args->ThreadFunc, args ) );
	TRACE(" Leaving SetWindowVar ... \n");
	return _retval;
}



