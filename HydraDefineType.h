#ifndef _HYDRADEFINETYPE_INCLUDE_H_
#define	_HYDRADEFINETYPE_INCLUDE_H_
#pragma once

#define EMSG_SVC_SERVER_LOCAL_HOST			"127.0.0.1"
#define EMSG_SVC_SERVER_PORT				"EMSG_SVC_PORT_NAME"

enum tsvc_cmds {
	tscmd_null,
	tscmd_ack,
	tscmd_nack,
	tscmd_error,
	tscmd_invalid_socket,
	tscmd_site_server_quit,

	tscmd_site_client_conn,
	tscmd_host_client_onclose,
	tscmd_site_client_quit,

	tscmd_mssg_client_data,

	tscmd_ping,
	tscmd_poll_clients,
	tscmd_fetch_client_id,
	tscmd_fetch_dhcp_svr_ip,
	tscmd_prod_mssg,
	tscmd_prod_mssg_rdy,
	tscmd_prod_mssg_bad,
	tscmd_begin_prod_load,
	tscmd_unload_prod_tools,
	tscmd_unload_pgm,
	tscmd_close_txui,

	tscmd_max_na
};

enum prod_mssg {
	pm_operator,
	pm_access_level,
	pm_prgn,
	pm_lot,
	pm_device,
	pm_testmode,
	pm_temp,
	pm_hndlid,
	pm_ldbrd,
	pm_dlname,
	pm_dlogi,
	pm_dlstart,
	pm_start,
	pm_dlgen,
	pm_dlend,
	pm_eol,
	pm_max_na
};



#endif // !_HYDRADEFINETYPE_INCLUDE_H_
