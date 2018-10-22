#include "Config.h"

#define HKY_SYSLOG_MAX_STR                                                    \
    HKY_MAX_ERROR_STR + sizeof("<255>Jan 01 00:00:00 ") - 1                   \
    + (HKY_MAXHOSTNAMELEN - 1) + 1 /* space */                                \
    + 32 /* tag */ + 2 /* colon, space */


static char *hky_syslog_parse_args(hky_conf_t *cf, hky_syslog_peer_t *peer);
//static hky_int_t hky_syslog_init_peer(hky_syslog_peer_t *peer);
//static void hky_syslog_cleanup(void *data);


static char  *facilities[] = {
	"kern", "user", "mail", "daemon", "auth", "intern", "lpr", "news", "uucp",
	"clock", "authpriv", "ftp", "ntp", "audit", "alert", "cron", "local0",
	"local1", "local2", "local3", "local4", "local5", "local6", "local7",
	NULL
};

/* note 'error/warn' like in nginx.conf, not 'err/warning' */
static char  *severities[] = {
	"emerg", "alert", "crit", "error", "warn", "notice", "info", "debug", NULL
};

static hky_log_t    hky_syslog_dummy_log;
//static hky_event_t  hky_syslog_dummy_event;


char *
hky_syslog_process_conf(hky_conf_t *cf, hky_syslog_peer_t *peer)
{
	hky_pool_cleanup_t  *cln;

	peer->facility = HKY_CONF_UNSET_UINT;
	peer->severity = HKY_CONF_UNSET_UINT;

	if (hky_syslog_parse_args(cf, peer) != HKY_CONF_OK) {
		return HKY_CONF_ERROR;
	}

	if (peer->server.sockaddr == NULL) {
		hky_conf_log_error(HKY_LOG_EMERG, cf, 0,
			"no syslog server specified");
		return HKY_CONF_ERROR;
	}

	if (peer->facility == HKY_CONF_UNSET_UINT) {
		peer->facility = 23; /* local7 */
	}

	if (peer->severity == HKY_CONF_UNSET_UINT) {
		peer->severity = 6; /* info */
	}

	if (peer->tag.data == NULL) {
		hky_str_set(&peer->tag, "nginx");
	}

	/*peer->conn.fd = (hky_socket_t)-1;

	peer->conn.read = &hky_syslog_dummy_event;
	peer->conn.write = &hky_syslog_dummy_event;*/

	//hky_syslog_dummy_event.log = &hky_syslog_dummy_log;

	cln = hky_pool_cleanup_add(cf->pool, 0);
	if (cln == NULL) {
		return HKY_CONF_ERROR;
	}

	cln->data = peer;
	//cln->handler = hky_syslog_cleanup;

	return HKY_CONF_OK;
}


static char *
hky_syslog_parse_args(hky_conf_t *cf, hky_syslog_peer_t *peer)
{
	u_char      *p, *comma, c;
	size_t       len;
	hky_str_t   *value;
	hky_url_t    u;
	hky_uint_t   i;

	value = cf->args->elts;

	p = value[1].data + sizeof("syslog:") - 1;

	for (;; ) {
		comma = (u_char *)hky_strchr(p, ',');

		if (comma != NULL) {
			len = comma - p;
			*comma = '\0';

		}
		else {
			len = value[1].data + value[1].len - p;
		}

		if (hky_strncmp(p, "server=", 7) == 0) {

			if (peer->server.sockaddr != NULL) {
				hky_conf_log_error(HKY_LOG_EMERG, cf, 0,
					"duplicate syslog \"server\"");
				return HKY_CONF_ERROR;
			}

			hky_memzero(&u, sizeof(hky_url_t));

			u.url.data = p + 7;
			u.url.len = len - 7;
			u.default_port = 514;

			if (hky_parse_url(cf->pool, &u) != HKY_OK) {
				if (u.err) {
					hky_conf_log_error(HKY_LOG_EMERG, cf, 0,
						"%s in syslog server \"%V\"",
						u.err, &u.url);
				}

				return HKY_CONF_ERROR;
			}

			peer->server = u.addrs[0];

		}
		else if (hky_strncmp(p, "facility=", 9) == 0) {

			if (peer->facility != HKY_CONF_UNSET_UINT) {
				hky_conf_log_error(HKY_LOG_EMERG, cf, 0,
					"duplicate syslog \"facility\"");
				return HKY_CONF_ERROR;
			}

			for (i = 0; facilities[i] != NULL; i++) {

				if (hky_strcmp(p + 9, facilities[i]) == 0) {
					peer->facility = i;
					goto next;
				}
			}

			hky_conf_log_error(HKY_LOG_EMERG, cf, 0,
				"unknown syslog facility \"%s\"", p + 9);
			return HKY_CONF_ERROR;

		}
		else if (hky_strncmp(p, "severity=", 9) == 0) {

			if (peer->severity != HKY_CONF_UNSET_UINT) {
				hky_conf_log_error(HKY_LOG_EMERG, cf, 0,
					"duplicate syslog \"severity\"");
				return HKY_CONF_ERROR;
			}

			for (i = 0; severities[i] != NULL; i++) {

				if (hky_strcmp(p + 9, severities[i]) == 0) {
					peer->severity = i;
					goto next;
				}
			}

			hky_conf_log_error(HKY_LOG_EMERG, cf, 0,
				"unknown syslog severity \"%s\"", p + 9);
			return HKY_CONF_ERROR;

		}
		else if (hky_strncmp(p, "tag=", 4) == 0) {

			if (peer->tag.data != NULL) {
				hky_conf_log_error(HKY_LOG_EMERG, cf, 0,
					"duplicate syslog \"tag\"");
				return HKY_CONF_ERROR;
			}

			/*
			* RFC 3164: the TAG is a string of ABNF alphanumeric characters
			* that MUST NOT exceed 32 characters.
			*/
			if (len - 4 > 32) {
				hky_conf_log_error(HKY_LOG_EMERG, cf, 0,
					"syslog tag length exceeds 32");
				return HKY_CONF_ERROR;
			}

			for (i = 4; i < len; i++) {
				c = hky_tolower(p[i]);

				if (c < '0' || (c > '9' && c < 'a' && c != '_') || c > 'z') {
					hky_conf_log_error(HKY_LOG_EMERG, cf, 0,
						"syslog \"tag\" only allows "
						"alphanumeric characters "
						"and underscore");
					return HKY_CONF_ERROR;
				}
			}

			peer->tag.data = p + 4;
			peer->tag.len = len - 4;

		}
		else if (len == 10 && hky_strncmp(p, "nohostname", 10) == 0) {
			peer->nohostname = 1;

		}
		else {
			hky_conf_log_error(HKY_LOG_EMERG, cf, 0,
				"unknown syslog parameter \"%s\"", p);
			return HKY_CONF_ERROR;
		}

	next:

		if (comma == NULL) {
			break;
		}

		p = comma + 1;
	}

	return HKY_CONF_OK;
}


u_char *
hky_syslog_add_header(hky_syslog_peer_t *peer, u_char *buf)
{
	hky_uint_t  pri;

	pri = peer->facility * 8 + peer->severity;

	if (peer->nohostname) {
		return hky_sprintf(buf, "<%ui>%V %V: ", pri, &hky_cached_syslog_time,
			&peer->tag);
	}

	return hky_sprintf(buf, "<%ui>%V %V %V: ", pri, &hky_cached_syslog_time,
		&hky_origin->hostname, &peer->tag);
}


void
hky_syslog_writer(hky_log_t *log, hky_uint_t level, u_char *buf,
	size_t len)
{
	u_char             *p, msg[HKY_SYSLOG_MAX_STR];
	hky_uint_t          head_len;
	hky_syslog_peer_t  *peer;

	peer = log->wdata;

	if (peer->busy) {
		return;
	}

	peer->busy = 1;
	peer->severity = level - 1;

	p = hky_syslog_add_header(peer, msg);
	head_len = p - msg;

	len -= HKY_LINEFEED_SIZE;

	if (len > HKY_SYSLOG_MAX_STR - head_len) {
		len = HKY_SYSLOG_MAX_STR - head_len;
	}

	p = hky_snprintf(p, len, "%s", buf);

	//(void)hky_syslog_send(peer, msg, p - msg);

	peer->busy = 0;
}

//
//ssize_t
//hky_syslog_send(hky_syslog_peer_t *peer, u_char *buf, size_t len)
//{
//	ssize_t  n;
//
//	if (peer->conn.fd == (hky_socket_t)-1) {
//		if (hky_syslog_init_peer(peer) != HKY_OK) {
//			return HKY_ERROR;
//		}
//	}
//
//	/* log syslog socket events with valid log */
//	peer->conn.log = hky_cycle->log;
//
//	if (hky_send) {
//		n = hky_send(&peer->conn, buf, len);
//
//	}
//	else {
//		/* event module has not yet set hky_io */
//		n = hky_os_io.send(&peer->conn, buf, len);
//	}
//
//	if (n == HKY_ERROR) {
//
//		if (hky_close_socket(peer->conn.fd) == -1) {
//			hky_log_error(HKY_LOG_ALERT, hky_cycle->log, hky_socket_errno,
//				hky_close_socket_n " failed");
//		}
//
//		peer->conn.fd = (hky_socket_t)-1;
//	}
//
//	return n;
//}
//
//
//static hky_int_t
//hky_syslog_init_peer(hky_syslog_peer_t *peer)
//{
//	hky_socket_t  fd;
//
//	fd = hky_socket(peer->server.sockaddr->sa_family, SOCK_DGRAM, 0);
//	if (fd == (hky_socket_t)-1) {
//		hky_log_error(HKY_LOG_ALERT, hky_cycle->log, hky_socket_errno,
//			hky_socket_n " failed");
//		return HKY_ERROR;
//	}
//
//	if (hky_nonblocking(fd) == -1) {
//		hky_log_error(HKY_LOG_ALERT, hky_cycle->log, hky_socket_errno,
//			hky_nonblocking_n " failed");
//		goto failed;
//	}
//
//	if (connect(fd, peer->server.sockaddr, peer->server.socklen) == -1) {
//		hky_log_error(HKY_LOG_ALERT, hky_cycle->log, hky_socket_errno,
//			"connect() failed");
//		goto failed;
//	}
//
//	peer->conn.fd = fd;
//
//	/* UDP sockets are always ready to write */
//	peer->conn.write->ready = 1;
//
//	return HKY_OK;
//
//failed:
//
//	if (hky_close_socket(fd) == -1) {
//		hky_log_error(HKY_LOG_ALERT, hky_cycle->log, hky_socket_errno,
//			hky_close_socket_n " failed");
//	}
//
//	return HKY_ERROR;
//}
//
//
//static void
//hky_syslog_cleanup(void *data)
//{
//	hky_syslog_peer_t  *peer = data;
//
//	/* prevents further use of this peer */
//	peer->busy = 1;
//
//	if (peer->conn.fd == (hky_socket_t)-1) {
//		return;
//	}
//
//	if (hky_close_socket(peer->conn.fd) == -1) {
//		hky_log_error(HKY_LOG_ALERT, hky_cycle->log, hky_socket_errno,
//			hky_close_socket_n " failed");
//	}
//}
