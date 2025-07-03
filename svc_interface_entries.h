#ifndef _SVCINTERFACEENTRIES_INCLUDE_H_
#define	_SVCINTERFACEENTRIES_INCLUDE_H_
#pragma once

extern void svc_send_cmd(SOCKET s, tsvc_cmds msgcmd);
extern void svc_send_ack(SOCKET s);
extern tsvc_cmds svc_recv_cmd(SOCKET s);

extern SOCKET connect_to_svchub(tsvc_cmds command);
extern void transmit_svcmcd_to_hub(SOCKET s, tsvc_cmds command);

extern BOOL fetch_active_client_id(CStringArray & rArrSites);
extern BOOL ping_local_server();
extern BOOL fetch_dhcp_server_ipname(CString & ipname);


extern SOCKET create_svc_server(int iPort, CString hostip, HANDLE hStop = NULL);
extern SOCKET wait_accept_connect(SOCKET server);
extern SOCKET wait_accept_client(SOCKET server, tsvc_cmds respCmd, CString & rClientName);

extern void destroy_svc_socket(SOCKET s);
extern BOOL send_tsvc_mssg_to_client(SOCKET client, tsvc_cmds cmd);

extern SOCKET connect_to_server(CString & hostip, int port);
extern tsvc_cmds wait_on_tscmd(SOCKET host);
extern void send_ack_to_host(SOCKET host);




#endif // !_SVCINTERFACEENTRIES_INCLUDE_H_

