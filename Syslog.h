#ifndef _HKY_SYSLOG_H_INCLUDE_
#define _HKY_SYSLOG_H_INCLUDE_
#include "Config.h"

typedef struct {
	hky_uint_t        facility;
	hky_uint_t        severity;
	hky_str_t         tag;

	hky_addr_t        server;
	//hky_connection_t  conn;
	unsigned          busy : 1;
	unsigned          nohostname : 1;
} hky_syslog_peer_t;


char *hky_syslog_process_conf(hky_conf_t *cf, hky_syslog_peer_t *peer);
u_char *hky_syslog_add_header(hky_syslog_peer_t *peer, u_char *buf);
void hky_syslog_writer(hky_log_t *log, hky_uint_t level, u_char *buf,
	size_t len);
ssize_t hky_syslog_send(hky_syslog_peer_t *peer, u_char *buf, size_t len);


#endif // !_HKY_SYSLOG_H_INCLUDE_
