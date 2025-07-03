#pragma once

extern CString get_ip_address();
extern void wait_in_ms(long milliseconds);
extern HANDLE CreateThread(DWORD(*func)(void *data), void *data = 0, DWORD *id = 0, DWORD timeout = INFINITE);
extern void SetThreadName(DWORD dwThreadID, LPCSTR szThreadName);
extern BOOL SetWindowVar(LPCTSTR name, int value, BOOL runOnThread);
extern int GetWindowVar(LPCTSTR name);

