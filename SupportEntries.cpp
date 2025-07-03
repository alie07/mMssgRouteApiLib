#include "pch.h"


#define HD_IP_SUBNET		"127.0.0"

CString get_ip_address()
{
	WORD sockVersion;
	WSADATA wsaData;

	sockVersion = MAKEWORD(1, 1);
	//start dll
	WSAStartup(sockVersion, &wsaData);

	CString _ipaddr;
	_ipaddr.Empty();
	char hostname[255];
	if (gethostname(hostname, sizeof hostname) != SOCKET_ERROR) {

		// Find our host entry (which should always be successful if gethostname was).
		LPHOSTENT lphp = gethostbyname(hostname);
		if (lphp) {

			for (int _x = 0; _x < 10; ++_x)
			{
				// Try to convert ip address into dotted format
				char *addr = lphp->h_addr_list[_x];

				if (addr)
				{
					_ipaddr.Format("%s", inet_ntoa(*(in_addr *)addr));
					if (_ipaddr.Find(HD_IP_SUBNET) != -1)
					{
						break;
					}
					else
					{
						_ipaddr.Empty();
					}
				}
				else
				{
					break;
				}
			}
		}
	}
	else
	{
		int _error = WSAGetLastError();

		TRACE("WSAGetLastError -> %d\n", _error);

	}

	WSACleanup();

	return _ipaddr;
}

void wait_in_ms(long milliseconds)
{
	long _timeout = clock() + milliseconds;
	while (clock() < _timeout) continue;
}



HANDLE CreateThread(DWORD(*func)(void *data), void *data /* = 0 */, DWORD *id /* = 0 */, DWORD timeout /* = INFINITE */)
{
	// Calls 'func' via a __stdcall wrapper (as required by Win32's CreateThread()).
	// Alternative of casting func to LPTHREAD_START_ROUTINE works but seems dangerous.
	struct Stdcall {
		typedef DWORD(*vfunc)(void *);

		static ULONG __stdcall Call(LPVOID arg) {

			Stdcall *args = static_cast<Stdcall *>(arg);
			vfunc func = args->func;
			void *data = args->data;

			SetEvent(args->done);

			return func(data);
		}
		vfunc func;
		void *data;
		HANDLE done;
	} args = { func, data, CreateEvent(0, 0, 0, 0) };

	// Start up thread 
	DWORD ignore;
	if (id == 0) id = &ignore;
	HANDLE thread = CreateThread(0, 0, args.Call, &args, 0, id);
	if (WaitForSingleObject(args.done, timeout) != WAIT_OBJECT_0) {
		SuspendThread(thread);
		thread = 0;
		ASSERT(AfxGetApp() == 0);
	}
	VERIFY(CloseHandle(args.done));
	return thread;
}


//
// Usage: SetThreadName (-1, "MainThread");
//
typedef struct tagMssgTHREADNAME_INFO
{
	DWORD dwType; // must be 0x1000
	LPCSTR szName; // pointer to name (in user addr space)
	DWORD dwThreadID; // thread ID (-1=caller thread)
	DWORD dwFlags; // reserved for future use, must be zero
} MssgTHREADNAME_INFO;

void SetThreadName(DWORD dwThreadID, LPCSTR szThreadName)
{
	MssgTHREADNAME_INFO info;
	{
		info.dwType = 0x1000;
		info.szName = szThreadName;
		info.dwThreadID = dwThreadID;
		info.dwFlags = 0;
	}
	__try
	{
		RaiseException(0x406D1388, 0, sizeof(info) / sizeof(DWORD), (DWORD*)&info);
	}
	__except (EXCEPTION_CONTINUE_EXECUTION)
	{
	}
}








