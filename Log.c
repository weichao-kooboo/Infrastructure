#include "Config.h"

static char *hky_error_log(hky_conf_t *cf, hky_command_t *cmd, void *conf);
static char *hky_log_set_levels(hky_conf_t *cf, hky_log_t *log);
static void hky_log_insert(hky_log_t *log, hky_log_t *new_log);

static hky_log_t        hky_log;
static hky_open_file_t  hky_log_file;
hky_uint_t              hky_use_stderr = 1;

static hky_str_t err_levels[] = {
	hky_null_string,
	hky_string("emerg"),
	hky_string("alert"),
	hky_string("crit"),
	hky_string("error"),
	hky_string("warn"),
	hky_string("notice"),
	hky_string("info"),
	hky_string("debug")
};

static const char *debug_levels[] = {
	"debug_core", "debug_alloc", "debug_mutex", "debug_event",
	"debug_http", "debug_mail", "debug_stream"
};

#if (HKY_DEBUG)

static void hky_log_memory_writer(hky_log_t *log, hky_uint_t level,
	u_char *buf, size_t len);
static void hky_log_memory_cleanup(void *data);


typedef struct {
	u_char        *start;
	u_char        *end;
	u_char        *pos;
	hky_atomic_t   written;
} hky_log_memory_buf_t;

#endif


#if (HKY_HAVE_VARIADIC_MACROS)

void
hky_log_error_core(hky_uint_t level, hky_log_t *log, hky_err_t err,
	const char *fmt, ...)

#else

void
hky_log_error_core(hky_uint_t level, hky_log_t *log, hky_err_t err,
	const char *fmt, va_list args)

#endif
{
#if (HKY_HAVE_VARIADIC_MACROS)
	va_list      args;
#endif
	u_char      *p, *last, *msg;
	ssize_t      n;
	hky_uint_t   wrote_stderr, debug_connection;
	u_char       errstr[HKY_MAX_ERROR_STR];

	last = errstr + HKY_MAX_ERROR_STR;

	p = hky_cpymem(errstr, hky_cached_err_log_time.data,
		hky_cached_err_log_time.len);

	p = hky_slprintf(p, last, " [%V] ", &err_levels[level]);

	/* pid#tid */
	p = hky_slprintf(p, last, "%P#" HKY_TID_T_FMT ": ",
		hky_log_pid, hky_log_tid);

	if (log->connection) {
		p = hky_slprintf(p, last, "*%uA ", log->connection);
	}

	msg = p;

#if (HKY_HAVE_VARIADIC_MACROS)

	va_start(args, fmt);
	p = hky_vslprintf(p, last, fmt, args);
	va_end(args);

#else

	p = hky_vslprintf(p, last, fmt, args);

#endif

	if (err) {
		p = hky_log_errno(p, last, err);
	}

	if (level != HKY_LOG_DEBUG && log->handler) {
		p = log->handler(log, p, last - p);
	}

	if (p > last - HKY_LINEFEED_SIZE) {
		p = last - HKY_LINEFEED_SIZE;
	}

	hky_linefeed(p);

	wrote_stderr = 0;
	debug_connection = (log->log_level & HKY_LOG_DEBUG_CONNECTION) != 0;

	while (log) {

		if (log->log_level < level && !debug_connection) {
			break;
		}

		if (log->writer) {
			log->writer(log, level, errstr, p - errstr);
			goto next;
		}

		if (hky_time() == log->disk_full_time) {

			/*
			* on FreeBSD writing to a full filesystem with enabled softupdates
			* may block process for much longer time than writing to non-full
			* filesystem, so we skip writing to a log for one second
			*/

			goto next;
		}

		n = hky_write_fd(log->file->fd, errstr, p - errstr);

		if (n == -1 && hky_errno == HKY_ENOSPC) {
			log->disk_full_time = hky_time();
		}

		if (log->file->fd == hky_stderr) {
			wrote_stderr = 1;
		}

	next:

		log = log->next;
	}

	if (!hky_use_stderr
		|| level > HKY_LOG_WARN
		|| wrote_stderr)
	{
		return;
	}

	msg -= (7 + err_levels[level].len + 3);

	(void)hky_sprintf(msg, "nginx: [%V] ", &err_levels[level]);

	(void)hky_write_console(hky_stderr, msg, p - msg);
}


#if !(HKY_HAVE_VARIADIC_MACROS)

void hky_cdecl
hky_log_error(hky_uint_t level, hky_log_t *log, hky_err_t err,
	const char *fmt, ...)
{
	va_list  args;

	if (log->log_level >= level) {
		va_start(args, fmt);
		hky_log_error_core(level, log, err, fmt, args);
		va_end(args);
	}
}


void hky_cdecl
hky_log_debug_core(hky_log_t *log, hky_err_t err, const char *fmt, ...)
{
	va_list  args;

	va_start(args, fmt);
	hky_log_error_core(HKY_LOG_DEBUG, log, err, fmt, args);
	va_end(args);
}

#endif


hky_log_t *
hky_log_init(u_char *prefix)
{
	u_char  *p, *name;
	size_t   nlen, plen;

	hky_log.file = &hky_log_file;
	hky_log.log_level = HKY_LOG_NOTICE;

	name = (u_char *)HKY_ERROR_LOG_PATH;

	/*
	* we use hky_strlen() here since BCC warns about
	* condition is always false and unreachable code
	*/

	nlen = hky_strlen(name);

	if (nlen == 0) {
		hky_log_file.fd = hky_stderr;
		return &hky_log;
	}

	p = NULL;

#if (HKY_WIN32)
	if (name[1] != ':') {
#else
	if (name[0] != '/') {
#endif

		if (prefix) {
			plen = hky_strlen(prefix);

		}
		else {
#ifdef HKY_PREFIX
			prefix = (u_char *)HKY_PREFIX;
			plen = hky_strlen(prefix);
#else
			plen = 0;
#endif
		}

		if (plen) {
			name = malloc(plen + nlen + 2);
			if (name == NULL) {
				return NULL;
			}

			p = hky_cpymem(name, prefix, plen);

			if (!hky_path_separator(*(p - 1))) {
				*p++ = '/';
			}

			hky_cpystrn(p, (u_char *)HKY_ERROR_LOG_PATH, nlen + 1);

			p = name;
		}
	}

	hky_log_file.fd = hky_open_file(name, HKY_FILE_APPEND,
		HKY_FILE_CREATE_OR_OPEN,
		HKY_FILE_DEFAULT_ACCESS);

	if (hky_log_file.fd == HKY_INVALID_FILE) {
		hky_log_stderr(hky_errno,
			"[alert] could not open error log file: "
			hky_open_file_n " \"%s\" failed", name);
#if (HKY_WIN32)
		hky_event_log(hky_errno,
			"could not open error log file: "
			hky_open_file_n " \"%s\" failed", name);
#endif

		hky_log_file.fd = hky_stderr;
	}

	if (p) {
		hky_free(p);
	}

	return &hky_log;
}


void hky_cdecl
hky_log_abort(hky_err_t err, const char *fmt, ...)
{
	u_char   *p;
	va_list   args;
	u_char    errstr[HKY_MAX_CONF_ERRSTR];

	va_start(args, fmt);
	p = hky_vsnprintf(errstr, sizeof(errstr) - 1, fmt, args);
	va_end(args);

	hky_log_error(HKY_LOG_ALERT, hky_origin->log, err,
		"%*s", p - errstr, errstr);
}


void hky_cdecl
hky_log_stderr(hky_err_t err, const char *fmt, ...)
{
	u_char   *p, *last;
	va_list   args;
	u_char    errstr[HKY_MAX_ERROR_STR];

	last = errstr + HKY_MAX_ERROR_STR;

	p = hky_cpymem(errstr, "nginx: ", 7);

	va_start(args, fmt);
	p = hky_vslprintf(p, last, fmt, args);
	va_end(args);

	if (err) {
		p = hky_log_errno(p, last, err);
	}

	if (p > last - HKY_LINEFEED_SIZE) {
		p = last - HKY_LINEFEED_SIZE;
	}

	hky_linefeed(p);

	(void)hky_write_console(hky_stderr, errstr, p - errstr);
}


u_char *
hky_log_errno(u_char *buf, u_char *last, hky_err_t err)
{
	if (buf > last - 50) {

		/* leave a space for an error code */

		buf = last - 50;
		*buf++ = '.';
		*buf++ = '.';
		*buf++ = '.';
	}

#if (HKY_WIN32)
	buf = hky_slprintf(buf, last, ((unsigned)err < 0x80000000)
		? " (%d: " : " (%Xd: ", err);
#else
	buf = hky_slprintf(buf, last, " (%d: ", err);
#endif

	buf = hky_strerror(err, buf, last - buf);

	if (buf < last) {
		*buf++ = ')';
	}

	return buf;
}


hky_int_t
hky_log_open_default(hky_origin_t *cycle)
{
	hky_log_t         *log;
	static hky_str_t   error_log = hky_string(HKY_ERROR_LOG_PATH);

	if (hky_log_get_file_log(&cycle->new_log) != NULL) {
		return HKY_OK;
	}

	if (cycle->new_log.log_level != 0) {
		/* there are some error logs, but no files */

		log = hky_pcalloc(cycle->pool, sizeof(hky_log_t));
		if (log == NULL) {
			return HKY_ERROR;
		}

	}
	else {
		/* no error logs at all */
		log = &cycle->new_log;
	}

	log->log_level = HKY_LOG_ERR;

	log->file = hky_conf_open_file(cycle, &error_log);
	if (log->file == NULL) {
		return HKY_ERROR;
	}

	if (log != &cycle->new_log) {
		hky_log_insert(&cycle->new_log, log);
	}

	return HKY_OK;
}


hky_int_t
hky_log_redirect_stderr(hky_origin_t *cycle)
{
	hky_fd_t  fd;

	if (cycle->log_use_stderr) {
		return HKY_OK;
	}

	/* file log always exists when we are called */
	fd = hky_log_get_file_log(cycle->log)->file->fd;

	if (fd != hky_stderr) {
		if (hky_set_stderr(fd) == HKY_FILE_ERROR) {
			hky_log_error(HKY_LOG_ALERT, cycle->log, hky_errno,
				hky_set_stderr_n " failed");

			return HKY_ERROR;
		}
	}

	return HKY_OK;
}


hky_log_t *
hky_log_get_file_log(hky_log_t *head)
{
	hky_log_t  *log;

	for (log = head; log; log = log->next) {
		if (log->file != NULL) {
			return log;
		}
	}

	return NULL;
}


char *
hky_log_set_log(hky_conf_t *cf, hky_log_t **head)
{
	hky_log_t          *new_log;
	hky_str_t          *value, name;
	hky_syslog_peer_t  *peer;

	if (*head != NULL && (*head)->log_level == 0) {
		new_log = *head;

	}
	else {

		new_log = hky_pcalloc(cf->pool, sizeof(hky_log_t));
		if (new_log == NULL) {
			return HKY_CONF_ERROR;
		}

		if (*head == NULL) {
			*head = new_log;
		}
	}

	value = cf->args->elts;

	if (hky_strcmp(value[1].data, "stderr") == 0) {
		hky_str_null(&name);
		cf->cycle->log_use_stderr = 1;

		new_log->file = hky_conf_open_file(cf->cycle, &name);
		if (new_log->file == NULL) {
			return HKY_CONF_ERROR;
		}

	}
	else if (hky_strncmp(value[1].data, "memory:", 7) == 0) {

#if (HKY_DEBUG)
		size_t                 size, needed;
		hky_pool_cleanup_t    *cln;
		hky_log_memory_buf_t  *buf;

		value[1].len -= 7;
		value[1].data += 7;

		needed = sizeof("MEMLOG  :" HKY_LINEFEED)
			+ cf->conf_file->file.name.len
			+ HKY_SIZE_T_LEN
			+ HKY_INT_T_LEN
			+ HKY_MAX_ERROR_STR;

		size = hky_parse_size(&value[1]);

		if (size == (size_t)HKY_ERROR || size < needed) {
			hky_conf_log_error(HKY_LOG_EMERG, cf, 0,
				"invalid buffer size \"%V\"", &value[1]);
			return HKY_CONF_ERROR;
		}

		buf = hky_pcalloc(cf->pool, sizeof(hky_log_memory_buf_t));
		if (buf == NULL) {
			return HKY_CONF_ERROR;
		}

		buf->start = hky_pnalloc(cf->pool, size);
		if (buf->start == NULL) {
			return HKY_CONF_ERROR;
		}

		buf->end = buf->start + size;

		buf->pos = hky_slprintf(buf->start, buf->end, "MEMLOG %uz %V:%ui%N",
			size, &cf->conf_file->file.name,
			cf->conf_file->line);

		hky_memset(buf->pos, ' ', buf->end - buf->pos);

		cln = hky_pool_cleanup_add(cf->pool, 0);
		if (cln == NULL) {
			return HKY_CONF_ERROR;
		}

		cln->data = new_log;
		cln->handler = hky_log_memory_cleanup;

		new_log->writer = hky_log_memory_writer;
		new_log->wdata = buf;

#else
		hky_conf_log_error(HKY_LOG_EMERG, cf, 0,
			"nginx was built without debug support");
		return HKY_CONF_ERROR;
#endif

	}
	else if (hky_strncmp(value[1].data, "syslog:", 7) == 0) {
		peer = hky_pcalloc(cf->pool, sizeof(hky_syslog_peer_t));
		if (peer == NULL) {
			return HKY_CONF_ERROR;
		}

		if (hky_syslog_process_conf(cf, peer) != HKY_CONF_OK) {
			return HKY_CONF_ERROR;
		}

		new_log->writer = hky_syslog_writer;
		new_log->wdata = peer;

	}
	else {
		new_log->file = hky_conf_open_file(cf->cycle, &value[1]);
		if (new_log->file == NULL) {
			return HKY_CONF_ERROR;
		}
	}

	if (hky_log_set_levels(cf, new_log) != HKY_CONF_OK) {
		return HKY_CONF_ERROR;
	}

	if (*head != new_log) {
		hky_log_insert(*head, new_log);
	}

	return HKY_CONF_OK;
}

static char *
hky_log_set_levels(hky_conf_t *cf, hky_log_t *log)
{
	hky_uint_t   i, n, d, found;
	hky_str_t   *value;

	if (cf->args->nelts == 2) {
		log->log_level = HKY_LOG_ERR;
		return HKY_CONF_OK;
	}

	value = cf->args->elts;

	for (i = 2; i < cf->args->nelts; i++) {
		found = 0;

		for (n = 1; n <= HKY_LOG_DEBUG; n++) {
			if (hky_strcmp(value[i].data, err_levels[n].data) == 0) {

				if (log->log_level != 0) {
					hky_conf_log_error(HKY_LOG_EMERG, cf, 0,
						"duplicate log level \"%V\"",
						&value[i]);
					return HKY_CONF_ERROR;
				}

				log->log_level = n;
				found = 1;
				break;
			}
		}

		for (n = 0, d = HKY_LOG_DEBUG_FIRST; d <= HKY_LOG_DEBUG_LAST; d <<= 1) {
			if (hky_strcmp(value[i].data, debug_levels[n++]) == 0) {
				if (log->log_level & ~HKY_LOG_DEBUG_ALL) {
					hky_conf_log_error(HKY_LOG_EMERG, cf, 0,
						"invalid log level \"%V\"",
						&value[i]);
					return HKY_CONF_ERROR;
				}

				log->log_level |= d;
				found = 1;
				break;
			}
		}


		if (!found) {
			hky_conf_log_error(HKY_LOG_EMERG, cf, 0,
				"invalid log level \"%V\"", &value[i]);
			return HKY_CONF_ERROR;
		}
	}

	if (log->log_level == HKY_LOG_DEBUG) {
		log->log_level = HKY_LOG_DEBUG_ALL;
	}

	return HKY_CONF_OK;
}


static char *
hky_error_log(hky_conf_t *cf, hky_command_t *cmd, void *conf)
{
	hky_log_t  *dummy;

	dummy = &cf->cycle->new_log;

	return hky_log_set_log(cf, &dummy);
}


static void
hky_log_insert(hky_log_t *log, hky_log_t *new_log)
{
	hky_log_t  tmp;

	if (new_log->log_level > log->log_level) {

		/*
		* list head address is permanent, insert new log after
		* head and swap its contents with head
		*/

		tmp = *log;
		*log = *new_log;
		*new_log = tmp;

		log->next = new_log;
		return;
	}

	while (log->next) {
		if (new_log->log_level > log->next->log_level) {
			new_log->next = log->next;
			log->next = new_log;
			return;
		}

		log = log->next;
	}

	log->next = new_log;
}

#if (HKY_DEBUG)

static void
hky_log_memory_writer(hky_log_t *log, hky_uint_t level, u_char *buf,
	size_t len)
{
	u_char                *p;
	size_t                 avail, written;
	hky_log_memory_buf_t  *mem;

	mem = log->wdata;

	if (mem == NULL) {
		return;
	}

	written = hky_atomic_fetch_add(&mem->written, len);

	p = mem->pos + written % (mem->end - mem->pos);

	avail = mem->end - p;

	if (avail >= len) {
		hky_memcpy(p, buf, len);

	}
	else {
		hky_memcpy(p, buf, avail);
		hky_memcpy(mem->pos, buf + avail, len - avail);
	}
}


static void
hky_log_memory_cleanup(void *data)
{
	hky_log_t *log = data;

	hky_log_debug0(HKY_LOG_DEBUG_CORE, log, 0, "destroy memory log buffer");

	log->wdata = NULL;
}

#endif
