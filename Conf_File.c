#include "Conf_File.h"

#define HKY_CONF_BUFFER  4096

static hky_int_t hky_conf_add_dump(hky_conf_t *cf, hky_str_t *filename);
//static hky_int_t hky_conf_handler(hky_conf_t *cf, hky_int_t last);
static hky_int_t hky_conf_read_token(hky_conf_t *cf);
static void hky_conf_flush_files(hky_origin_t *cycle);


char *
hky_conf_deprecated(hky_conf_t *cf, void *post, void *data)
{
	hky_conf_deprecated_t  *d = post;

	hky_conf_log_error(HKY_LOG_WARN, cf, 0,
		"the \"%s\" directive is deprecated, "
		"use the \"%s\" directive instead",
		d->old_name, d->new_name);

	return HKY_CONF_OK;
}


char *
hky_conf_check_num_bounds(hky_conf_t *cf, void *post, void *data)
{
	hky_conf_num_bounds_t  *bounds = post;
	hky_int_t  *np = data;

	if (bounds->high == -1) {
		if (*np >= bounds->low) {
			return HKY_CONF_OK;
		}

		hky_conf_log_error(HKY_LOG_EMERG, cf, 0,
			"value must be equal to or greater than %i",
			bounds->low);

		return HKY_CONF_ERROR;
	}

	if (*np >= bounds->low && *np <= bounds->high) {
		return HKY_CONF_OK;
	}

	hky_conf_log_error(HKY_LOG_EMERG, cf, 0,
		"value must be between %i and %i",
		bounds->low, bounds->high);

	return HKY_CONF_ERROR;
}


char *
hky_conf_param(hky_conf_t *cf)
{
	char             *rv;
	hky_str_t        *param;
	hky_buf_t         b;
	hky_conf_file_t   conf_file;

	param = &cf->cycle->conf_param;

	if (param->len == 0) {
		return HKY_CONF_OK;
	}

	hky_memzero(&conf_file, sizeof(hky_conf_file_t));

	hky_memzero(&b, sizeof(hky_buf_t));

	b.start = param->data;
	b.pos = param->data;
	b.last = param->data + param->len;
	b.end = b.last;
	b.temporary = 1;

	conf_file.file.fd = HKY_INVALID_FILE;
	conf_file.file.name.data = NULL;
	conf_file.line = 0;

	cf->conf_file = &conf_file;
	cf->conf_file->buffer = &b;

	rv = hky_conf_parse(cf, NULL);

	cf->conf_file = NULL;

	return rv;
}


char *
hky_conf_parse(hky_conf_t *cf, hky_str_t *filename)
{
	char             *rv;
	hky_fd_t          fd = 0;
	hky_int_t         rc;
	hky_buf_t         buf;
	hky_conf_file_t  *prev = 0, conf_file;
	enum {
		parse_file = 0,
		parse_block,
		parse_param
	} type;

#if (HKY_SUPPRESS_WARN)
	fd = HKY_INVALID_FILE;
	prev = NULL;
#endif

	if (filename) {

		/* open configuration file */

		fd = hky_open_file(filename->data, HKY_FILE_RDONLY, HKY_FILE_OPEN, 0);

		if (fd == HKY_INVALID_FILE) {
			hky_conf_log_error(HKY_LOG_EMERG, cf, hky_errno,
				hky_open_file_n " \"%s\" failed",
				filename->data);
			return HKY_CONF_ERROR;
		}

		prev = cf->conf_file;

		cf->conf_file = &conf_file;

		if (hky_fd_info(fd, &cf->conf_file->file.info) == HKY_FILE_ERROR) {
			hky_log_error(HKY_LOG_EMERG, cf->log, hky_errno,
				hky_fd_info_n " \"%s\" failed", filename->data);
		}

		cf->conf_file->buffer = &buf;

		buf.start = hky_alloc(HKY_CONF_BUFFER, cf->log);
		if (buf.start == NULL) {
			goto failed;
		}

		buf.pos = buf.start;
		buf.last = buf.start;
		buf.end = buf.last + HKY_CONF_BUFFER;
		buf.temporary = 1;

		cf->conf_file->file.fd = fd;
		cf->conf_file->file.name.len = filename->len;
		cf->conf_file->file.name.data = filename->data;
		cf->conf_file->file.offset = 0;
		cf->conf_file->file.log = cf->log;
		cf->conf_file->line = 1;

		type = parse_file;

		if (hky_dump_config
#if (HKY_DEBUG)
			|| 1
#endif
			)
		{
			if (hky_conf_add_dump(cf, filename) != HKY_OK) {
				goto failed;
			}

		}
		else {
			cf->conf_file->dump = NULL;
		}

	}
	else if (cf->conf_file->file.fd != HKY_INVALID_FILE) {

		type = parse_block;

	}
	else {
		type = parse_param;
	}


	for (;; ) {
		rc = hky_conf_read_token(cf);

		/*
		* hky_conf_read_token() may return
		*
		*    HKY_ERROR             there is error
		*    HKY_OK                the token terminated by ";" was found
		*    HKY_CONF_BLOCK_START  the token terminated by "{" was found
		*    HKY_CONF_BLOCK_DONE   the "}" was found
		*    HKY_CONF_FILE_DONE    the configuration file is done
		*/

		if (rc == HKY_ERROR) {
			goto done;
		}

		if (rc == HKY_CONF_BLOCK_DONE) {

			if (type != parse_block) {
				hky_conf_log_error(HKY_LOG_EMERG, cf, 0, "unexpected \"}\"");
				goto failed;
			}

			goto done;
		}

		if (rc == HKY_CONF_FILE_DONE) {

			if (type == parse_block) {
				hky_conf_log_error(HKY_LOG_EMERG, cf, 0,
					"unexpected end of file, expecting \"}\"");
				goto failed;
			}

			goto done;
		}

		if (rc == HKY_CONF_BLOCK_START) {

			if (type == parse_param) {
				hky_conf_log_error(HKY_LOG_EMERG, cf, 0,
					"block directives are not supported "
					"in -g option");
				goto failed;
			}
		}

		/* rc == HKY_OK || rc == HKY_CONF_BLOCK_START */

		if (cf->handler) {

			/*
			* the custom handler, i.e., that is used in the http's
			* "types { ... }" directive
			*/

			if (rc == HKY_CONF_BLOCK_START) {
				hky_conf_log_error(HKY_LOG_EMERG, cf, 0, "unexpected \"{\"");
				goto failed;
			}

			rv = (*cf->handler)(cf, NULL, cf->handler_conf);
			if (rv == HKY_CONF_OK) {
				continue;
			}

			if (rv == HKY_CONF_ERROR) {
				goto failed;
			}

			hky_conf_log_error(HKY_LOG_EMERG, cf, 0, rv);

			goto failed;
		}


		rc = hky_conf_handler(cf, rc);

		if (rc == HKY_ERROR) {
			goto failed;
		}
	}

failed:

	rc = HKY_ERROR;

done:

	if (filename) {
		if (cf->conf_file->buffer->start) {
			hky_free(cf->conf_file->buffer->start);
		}

		if (hky_close_file(fd) == HKY_FILE_ERROR) {
			hky_log_error(HKY_LOG_ALERT, cf->log, hky_errno,
				hky_close_file_n " %s failed",
				filename->data);
			rc = HKY_ERROR;
		}

		cf->conf_file = prev;
	}

	if (rc == HKY_ERROR) {
		return HKY_CONF_ERROR;
	}

	return HKY_CONF_OK;
}


char *
hky_conf_include(hky_conf_t *cf, hky_command_t *cmd, void *conf)
{
	char        *rv;
	hky_int_t    n;
	hky_str_t   *value, file, name;
	hky_glob_t   gl;

	value = cf->args->elts;
	file = value[1];

	hky_log_debug1(HKY_LOG_DEBUG_CORE, cf->log, 0, "include %s", file.data);

	if (hky_conf_full_name(cf->cycle, &file, 1) != HKY_OK) {
		return HKY_CONF_ERROR;
	}

	if (strpbrk((char *)file.data, "*?[") == NULL) {

		hky_log_debug1(HKY_LOG_DEBUG_CORE, cf->log, 0, "include %s", file.data);

		return hky_conf_parse(cf, &file);
	}

	hky_memzero(&gl, sizeof(hky_glob_t));

	gl.pattern = file.data;
	gl.log = cf->log;
	gl.test = 1;

	if (hky_open_glob(&gl) != HKY_OK) {
		hky_conf_log_error(HKY_LOG_EMERG, cf, hky_errno,
			hky_open_glob_n " \"%s\" failed", file.data);
		return HKY_CONF_ERROR;
	}

	rv = HKY_CONF_OK;

	for (;; ) {
		n = hky_read_glob(&gl, &name);

		if (n != HKY_OK) {
			break;
		}

		file.len = name.len++;
		file.data = hky_pstrdup(cf->pool, &name);
		if (file.data == NULL) {
			return HKY_CONF_ERROR;
		}

		hky_log_debug1(HKY_LOG_DEBUG_CORE, cf->log, 0, "include %s", file.data);

		rv = hky_conf_parse(cf, &file);

		if (rv != HKY_CONF_OK) {
			break;
		}
	}

	hky_close_glob(&gl);

	return rv;
}


hky_int_t
hky_conf_full_name(hky_origin_t *cycle, hky_str_t *name, hky_uint_t conf_prefix)
{
	hky_str_t  *prefix;

	prefix = conf_prefix ? &cycle->conf_prefix : &cycle->prefix;

	return hky_get_full_name(cycle->pool, prefix, name);
}


hky_open_file_t *
hky_conf_open_file(hky_origin_t *cycle, hky_str_t *name)
{
	hky_str_t         full;
	hky_uint_t        i;
	hky_list_part_t  *part;
	hky_open_file_t  *file;

#if (HKY_SUPPRESS_WARN)
	hky_str_null(&full);
#endif

	if (name->len) {
		full = *name;

		if (hky_conf_full_name(cycle, &full, 0) != HKY_OK) {
			return NULL;
		}

		part = &cycle->open_files.part;
		file = part->elts;

		for (i = 0; /* void */; i++) {

			if (i >= part->nelts) {
				if (part->next == NULL) {
					break;
				}
				part = part->next;
				file = part->elts;
				i = 0;
			}

			if (full.len != file[i].name.len) {
				continue;
			}

			if (hky_strcmp(full.data, file[i].name.data) == 0) {
				return &file[i];
			}
		}
	}

	file = hky_list_push(&cycle->open_files);
	if (file == NULL) {
		return NULL;
	}

	if (name->len) {
		file->fd = HKY_INVALID_FILE;
		file->name = full;

	}
	else {
		file->fd = hky_stderr;
		file->name = *name;
	}

	file->flush = NULL;
	file->data = NULL;

	return file;
}

void hky_cdecl
hky_conf_log_error(hky_uint_t level, hky_conf_t *cf, hky_err_t err,
	const char *fmt, ...)
{
	u_char   errstr[HKY_MAX_CONF_ERRSTR], *p, *last;
	va_list  args;

	last = errstr + HKY_MAX_CONF_ERRSTR;

	va_start(args, fmt);
	p = hky_vslprintf(errstr, last, fmt, args);
	va_end(args);

	if (err) {
		p = hky_log_errno(p, last, err);
	}

	if (cf->conf_file == NULL) {
		hky_log_error(level, cf->log, 0, "%*s", p - errstr, errstr);
		return;
	}

	if (cf->conf_file->file.fd == HKY_INVALID_FILE) {
		hky_log_error(level, cf->log, 0, "%*s in command line",
			p - errstr, errstr);
		return;
	}

	hky_log_error(level, cf->log, 0, "%*s in %s:%ui",
		p - errstr, errstr,
		cf->conf_file->file.name.data, cf->conf_file->line);
}

char *
hky_conf_set_flag_slot(hky_conf_t *cf, hky_command_t *cmd, void *conf)
{
	char  *p = conf;

	hky_str_t        *value;
	hky_flag_t       *fp;
	hky_conf_post_t  *post;

	fp = (hky_flag_t *)(p + cmd->offset);

	if (*fp != HKY_CONF_UNSET) {
		return "is duplicate";
	}

	value = cf->args->elts;

	if (hky_strcasecmp(value[1].data, (u_char *) "on") == 0) {
		*fp = 1;

	}
	else if (hky_strcasecmp(value[1].data, (u_char *) "off") == 0) {
		*fp = 0;

	}
	else {
		hky_conf_log_error(HKY_LOG_EMERG, cf, 0,
			"invalid value \"%s\" in \"%s\" directive, "
			"it must be \"on\" or \"off\"",
			value[1].data, cmd->name.data);
		return HKY_CONF_ERROR;
	}

	if (cmd->post) {
		post = cmd->post;
		return post->post_handler(cf, post, fp);
	}

	return HKY_CONF_OK;
}

char *
hky_conf_set_str_slot(hky_conf_t *cf, hky_command_t *cmd, void *conf)
{
	char  *p = conf;

	hky_str_t        *field, *value;
	hky_conf_post_t  *post;

	field = (hky_str_t *)(p + cmd->offset);

	if (field->data) {
		return "is duplicate";
	}

	value = cf->args->elts;

	*field = value[1];

	if (cmd->post) {
		post = cmd->post;
		return post->post_handler(cf, post, field);
	}

	return HKY_CONF_OK;
}

char *
hky_conf_set_str_array_slot(hky_conf_t *cf, hky_command_t *cmd, void *conf)
{
	char  *p = conf;

	hky_str_t         *value, *s;
	hky_array_t      **a;
	hky_conf_post_t   *post;

	a = (hky_array_t **)(p + cmd->offset);

	if (*a == HKY_CONF_UNSET_PTR) {
		*a = hky_array_create(cf->pool, 4, sizeof(hky_str_t));
		if (*a == NULL) {
			return HKY_CONF_ERROR;
		}
	}

	s = hky_array_push(*a);
	if (s == NULL) {
		return HKY_CONF_ERROR;
	}

	value = cf->args->elts;

	*s = value[1];

	if (cmd->post) {
		post = cmd->post;
		return post->post_handler(cf, post, s);
	}

	return HKY_CONF_OK;
}

char *
hky_conf_set_keyval_slot(hky_conf_t *cf, hky_command_t *cmd, void *conf)
{
	char  *p = conf;

	hky_str_t         *value;
	hky_array_t      **a;
	hky_keyval_t      *kv;
	hky_conf_post_t   *post;

	a = (hky_array_t **)(p + cmd->offset);

	if (*a == NULL) {
		*a = hky_array_create(cf->pool, 4, sizeof(hky_keyval_t));
		if (*a == NULL) {
			return HKY_CONF_ERROR;
		}
	}

	kv = hky_array_push(*a);
	if (kv == NULL) {
		return HKY_CONF_ERROR;
	}

	value = cf->args->elts;

	kv->key = value[1];
	kv->value = value[2];

	if (cmd->post) {
		post = cmd->post;
		return post->post_handler(cf, post, kv);
	}

	return HKY_CONF_OK;
}

char *
hky_conf_set_num_slot(hky_conf_t *cf, hky_command_t *cmd, void *conf)
{
	char  *p = conf;

	hky_int_t        *np;
	hky_str_t        *value;
	hky_conf_post_t  *post;


	np = (hky_int_t *)(p + cmd->offset);

	if (*np != HKY_CONF_UNSET) {
		return "is duplicate";
	}

	value = cf->args->elts;
	*np = hky_atoi(value[1].data, value[1].len);
	if (*np == HKY_ERROR) {
		return "invalid number";
	}

	if (cmd->post) {
		post = cmd->post;
		return post->post_handler(cf, post, np);
	}

	return HKY_CONF_OK;
}

char *
hky_conf_set_size_slot(hky_conf_t *cf, hky_command_t *cmd, void *conf)
{
	char  *p = conf;

	size_t           *sp;
	hky_str_t        *value;
	hky_conf_post_t  *post;


	sp = (size_t *)(p + cmd->offset);
	if (*sp != HKY_CONF_UNSET_SIZE) {
		return "is duplicate";
	}

	value = cf->args->elts;

	*sp = hky_parse_size(&value[1]);
	if (*sp == (size_t)HKY_ERROR) {
		return "invalid value";
	}

	if (cmd->post) {
		post = cmd->post;
		return post->post_handler(cf, post, sp);
	}

	return HKY_CONF_OK;
}


char *
hky_conf_set_off_slot(hky_conf_t *cf, hky_command_t *cmd, void *conf)
{
	char  *p = conf;

	off_t            *op;
	hky_str_t        *value;
	hky_conf_post_t  *post;


	op = (off_t *)(p + cmd->offset);
	if (*op != HKY_CONF_UNSET) {
		return "is duplicate";
	}

	value = cf->args->elts;

	*op = hky_parse_offset(&value[1]);
	if (*op == (off_t)HKY_ERROR) {
		return "invalid value";
	}

	if (cmd->post) {
		post = cmd->post;
		return post->post_handler(cf, post, op);
	}

	return HKY_CONF_OK;
}


char *
hky_conf_set_msec_slot(hky_conf_t *cf, hky_command_t *cmd, void *conf)
{
	char  *p = conf;

	hky_msec_t       *msp;
	hky_str_t        *value;
	hky_conf_post_t  *post;


	msp = (hky_msec_t *)(p + cmd->offset);
	if (*msp != HKY_CONF_UNSET_MSEC) {
		return "is duplicate";
	}

	value = cf->args->elts;

	*msp = hky_parse_time(&value[1], 0);
	if (*msp == (hky_msec_t)HKY_ERROR) {
		return "invalid value";
	}

	if (cmd->post) {
		post = cmd->post;
		return post->post_handler(cf, post, msp);
	}

	return HKY_CONF_OK;
}


char *
hky_conf_set_sec_slot(hky_conf_t *cf, hky_command_t *cmd, void *conf)
{
	char  *p = conf;

	time_t           *sp;
	hky_str_t        *value;
	hky_conf_post_t  *post;


	sp = (time_t *)(p + cmd->offset);
	if (*sp != HKY_CONF_UNSET) {
		return "is duplicate";
	}

	value = cf->args->elts;

	*sp = hky_parse_time(&value[1], 1);
	if (*sp == (time_t)HKY_ERROR) {
		return "invalid value";
	}

	if (cmd->post) {
		post = cmd->post;
		return post->post_handler(cf, post, sp);
	}

	return HKY_CONF_OK;
}


char *
hky_conf_set_bufs_slot(hky_conf_t *cf, hky_command_t *cmd, void *conf)
{
	char *p = conf;

	hky_str_t   *value;
	hky_bufs_t  *bufs;


	bufs = (hky_bufs_t *)(p + cmd->offset);
	if (bufs->num) {
		return "is duplicate";
	}

	value = cf->args->elts;

	bufs->num = hky_atoi(value[1].data, value[1].len);
	if (bufs->num == HKY_ERROR || bufs->num == 0) {
		return "invalid value";
	}

	bufs->size = hky_parse_size(&value[2]);
	if (bufs->size == (size_t)HKY_ERROR || bufs->size == 0) {
		return "invalid value";
	}

	return HKY_CONF_OK;
}


char *
hky_conf_set_enum_slot(hky_conf_t *cf, hky_command_t *cmd, void *conf)
{
	char  *p = conf;

	hky_uint_t       *np, i;
	hky_str_t        *value;
	hky_conf_enum_t  *e;

	np = (hky_uint_t *)(p + cmd->offset);

	if (*np != HKY_CONF_UNSET_UINT) {
		return "is duplicate";
	}

	value = cf->args->elts;
	e = cmd->post;

	for (i = 0; e[i].name.len != 0; i++) {
		if (e[i].name.len != value[1].len
			|| hky_strcasecmp(e[i].name.data, value[1].data) != 0)
		{
			continue;
		}

		*np = e[i].value;

		return HKY_CONF_OK;
	}

	hky_conf_log_error(HKY_LOG_EMERG, cf, 0,
		"invalid value \"%s\"", value[1].data);

	return HKY_CONF_ERROR;
}


char *
hky_conf_set_bitmask_slot(hky_conf_t *cf, hky_command_t *cmd, void *conf)
{
	char  *p = conf;

	hky_uint_t          *np, i, m;
	hky_str_t           *value;
	hky_conf_bitmask_t  *mask;


	np = (hky_uint_t *)(p + cmd->offset);
	value = cf->args->elts;
	mask = cmd->post;

	for (i = 1; i < cf->args->nelts; i++) {
		for (m = 0; mask[m].name.len != 0; m++) {

			if (mask[m].name.len != value[i].len
				|| hky_strcasecmp(mask[m].name.data, value[i].data) != 0)
			{
				continue;
			}

			if (*np & mask[m].mask) {
				hky_conf_log_error(HKY_LOG_WARN, cf, 0,
					"duplicate value \"%s\"", value[i].data);

			}
			else {
				*np |= mask[m].mask;
			}

			break;
		}

		if (mask[m].name.len == 0) {
			hky_conf_log_error(HKY_LOG_EMERG, cf, 0,
				"invalid value \"%s\"", value[i].data);

			return HKY_CONF_ERROR;
		}
	}

	return HKY_CONF_OK;
}


static hky_int_t
hky_conf_add_dump(hky_conf_t *cf, hky_str_t *filename)
{
	off_t             size;
	u_char           *p;
	uint32_t          hash;
	hky_buf_t        *buf;
	hky_str_node_t   *sn;
	hky_conf_dump_t  *cd;

	hash = hky_crc32_long(filename->data, filename->len);

	sn = hky_str_rbtree_lookup(&cf->cycle->config_dump_rbtree, filename, hash);

	if (sn) {
		cf->conf_file->dump = NULL;
		return HKY_OK;
	}

	p = hky_pstrdup(cf->cycle->pool, filename);
	if (p == NULL) {
		return HKY_ERROR;
	}

	cd = hky_array_push(&cf->cycle->config_dump);
	if (cd == NULL) {
		return HKY_ERROR;
	}

	size = hky_file_size(&cf->conf_file->file.info);

	buf = hky_create_temp_buf(cf->cycle->pool, (size_t)size);
	if (buf == NULL) {
		return HKY_ERROR;
	}

	cd->name.data = p;
	cd->name.len = filename->len;
	cd->buffer = buf;

	cf->conf_file->dump = buf;

	sn = hky_palloc(cf->temp_pool, sizeof(hky_str_node_t));
	if (sn == NULL) {
		return HKY_ERROR;
	}

	sn->node.key = hash;
	sn->str = cd->name;

	hky_rbtree_insert(&cf->cycle->config_dump_rbtree, &sn->node);

	return HKY_OK;
}


//static hky_int_t
//hky_conf_handler(hky_conf_t *cf, hky_int_t last)
//{
//	char           *rv;
//	void           *conf, **confp;
//	hky_uint_t      i, found;
//	hky_str_t      *name;
//	hky_command_t  *cmd;
//
//	name = cf->args->elts;
//
//	found = 0;
//
//	for (i = 0; cf->cycle->modules[i]; i++) {
//
//		cmd = cf->cycle->modules[i]->commands;
//		if (cmd == NULL) {
//			continue;
//		}
//
//		for ( /* void */; cmd->name.len; cmd++) {
//
//			if (name->len != cmd->name.len) {
//				continue;
//			}
//
//			if (hky_strcmp(name->data, cmd->name.data) != 0) {
//				continue;
//			}
//
//			found = 1;
//
//			if (cf->cycle->modules[i]->type != HKY_CONF_MODULE
//				&& cf->cycle->modules[i]->type != cf->module_type)
//			{
//				continue;
//			}
//
//			/* is the directive's location right ? */
//
//			if (!(cmd->type & cf->cmd_type)) {
//				continue;
//			}
//
//			if (!(cmd->type & HKY_CONF_BLOCK) && last != HKY_OK) {
//				hky_conf_log_error(HKY_LOG_EMERG, cf, 0,
//					"directive \"%s\" is not terminated by \";\"",
//					name->data);
//				return HKY_ERROR;
//			}
//
//			if ((cmd->type & HKY_CONF_BLOCK) && last != HKY_CONF_BLOCK_START) {
//				hky_conf_log_error(HKY_LOG_EMERG, cf, 0,
//					"directive \"%s\" has no opening \"{\"",
//					name->data);
//				return HKY_ERROR;
//			}
//
//			/* is the directive's argument count right ? */
//
//			if (!(cmd->type & HKY_CONF_ANY)) {
//
//				if (cmd->type & HKY_CONF_FLAG) {
//
//					if (cf->args->nelts != 2) {
//						goto invalid;
//					}
//
//				}
//				else if (cmd->type & HKY_CONF_1MORE) {
//
//					if (cf->args->nelts < 2) {
//						goto invalid;
//					}
//
//				}
//				else if (cmd->type & HKY_CONF_2MORE) {
//
//					if (cf->args->nelts < 3) {
//						goto invalid;
//					}
//
//				}
//				else if (cf->args->nelts > HKY_CONF_MAX_ARGS) {
//
//					goto invalid;
//
//				}
//				else if (!(cmd->type & argument_number[cf->args->nelts - 1]))
//				{
//					goto invalid;
//				}
//			}
//
//			/* set up the directive's configuration context */
//
//			conf = NULL;
//
//			if (cmd->type & HKY_DIRECT_CONF) {
//				conf = ((void **)cf->ctx)[cf->cycle->modules[i]->index];
//
//			}
//			else if (cmd->type & HKY_MAIN_CONF) {
//				conf = &(((void **)cf->ctx)[cf->cycle->modules[i]->index]);
//
//			}
//			else if (cf->ctx) {
//				confp = *(void **)((char *)cf->ctx + cmd->conf);
//
//				if (confp) {
//					conf = confp[cf->cycle->modules[i]->ctx_index];
//				}
//			}
//
//			rv = cmd->set(cf, cmd, conf);
//
//			if (rv == HKY_CONF_OK) {
//				return HKY_OK;
//			}
//
//			if (rv == HKY_CONF_ERROR) {
//				return HKY_ERROR;
//			}
//
//			hky_conf_log_error(HKY_LOG_EMERG, cf, 0,
//				"\"%s\" directive %s", name->data, rv);
//
//			return HKY_ERROR;
//		}
//	}
//
//	if (found) {
//		hky_conf_log_error(HKY_LOG_EMERG, cf, 0,
//			"\"%s\" directive is not allowed here", name->data);
//
//		return HKY_ERROR;
//	}
//
//	hky_conf_log_error(HKY_LOG_EMERG, cf, 0,
//		"unknown directive \"%s\"", name->data);
//
//	return HKY_ERROR;
//
//invalid:
//
//	hky_conf_log_error(HKY_LOG_EMERG, cf, 0,
//		"invalid number of arguments in \"%s\" directive",
//		name->data);
//
//	return HKY_ERROR;
//}


static hky_int_t
hky_conf_read_token(hky_conf_t *cf)
{
	u_char      *start, ch, *src, *dst;
	off_t        file_size;
	size_t       len;
	ssize_t      n, size;
	hky_uint_t   found, need_space, last_space, sharp_comment, variable;
	hky_uint_t   quoted, s_quoted, d_quoted, start_line;
	hky_str_t   *word;
	hky_buf_t   *b, *dump;

	found = 0;
	need_space = 0;
	last_space = 1;
	sharp_comment = 0;
	variable = 0;
	quoted = 0;
	s_quoted = 0;
	d_quoted = 0;

	cf->args->nelts = 0;
	b = cf->conf_file->buffer;
	dump = cf->conf_file->dump;
	start = b->pos;
	start_line = cf->conf_file->line;

	file_size = hky_file_size(&cf->conf_file->file.info);

	for (;; ) {

		if (b->pos >= b->last) {

			if (cf->conf_file->file.offset >= file_size) {

				if (cf->args->nelts > 0 || !last_space) {

					if (cf->conf_file->file.fd == HKY_INVALID_FILE) {
						hky_conf_log_error(HKY_LOG_EMERG, cf, 0,
							"unexpected end of parameter, "
							"expecting \";\"");
						return HKY_ERROR;
					}

					hky_conf_log_error(HKY_LOG_EMERG, cf, 0,
						"unexpected end of file, "
						"expecting \";\" or \"}\"");
					return HKY_ERROR;
				}

				return HKY_CONF_FILE_DONE;
			}

			len = b->pos - start;

			if (len == HKY_CONF_BUFFER) {
				cf->conf_file->line = start_line;

				if (d_quoted) {
					ch = '"';

				}
				else if (s_quoted) {
					ch = '\'';

				}
				else {
					hky_conf_log_error(HKY_LOG_EMERG, cf, 0,
						"too long parameter \"%*s...\" started",
						10, start);
					return HKY_ERROR;
				}

				hky_conf_log_error(HKY_LOG_EMERG, cf, 0,
					"too long parameter, probably "
					"missing terminating \"%c\" character", ch);
				return HKY_ERROR;
			}

			if (len) {
				hky_memmove(b->start, start, len);
			}

			size = (ssize_t)(file_size - cf->conf_file->file.offset);

			if (size > b->end - (b->start + len)) {
				size = b->end - (b->start + len);
			}

			n = hky_read_file(&cf->conf_file->file, b->start + len, size,
				cf->conf_file->file.offset);

			if (n == HKY_ERROR) {
				return HKY_ERROR;
			}

			if (n != size) {
				hky_conf_log_error(HKY_LOG_EMERG, cf, 0,
					hky_read_file_n " returned "
					"only %z bytes instead of %z",
					n, size);
				return HKY_ERROR;
			}

			b->pos = b->start + len;
			b->last = b->pos + n;
			start = b->start;

			if (dump) {
				dump->last = hky_cpymem(dump->last, b->pos, size);
			}
		}

		ch = *b->pos++;

		if (ch == LF) {
			cf->conf_file->line++;

			if (sharp_comment) {
				sharp_comment = 0;
			}
		}

		if (sharp_comment) {
			continue;
		}

		if (quoted) {
			quoted = 0;
			continue;
		}

		if (need_space) {
			if (ch == ' ' || ch == '\t' || ch == CR || ch == LF) {
				last_space = 1;
				need_space = 0;
				continue;
			}

			if (ch == ';') {
				return HKY_OK;
			}

			if (ch == '{') {
				return HKY_CONF_BLOCK_START;
			}

			if (ch == ')') {
				last_space = 1;
				need_space = 0;

			}
			else {
				hky_conf_log_error(HKY_LOG_EMERG, cf, 0,
					"unexpected \"%c\"", ch);
				return HKY_ERROR;
			}
		}

		if (last_space) {

			start = b->pos - 1;
			start_line = cf->conf_file->line;

			if (ch == ' ' || ch == '\t' || ch == CR || ch == LF) {
				continue;
			}

			switch (ch) {

			case ';':
			case '{':
				if (cf->args->nelts == 0) {
					hky_conf_log_error(HKY_LOG_EMERG, cf, 0,
						"unexpected \"%c\"", ch);
					return HKY_ERROR;
				}

				if (ch == '{') {
					return HKY_CONF_BLOCK_START;
				}

				return HKY_OK;

			case '}':
				if (cf->args->nelts != 0) {
					hky_conf_log_error(HKY_LOG_EMERG, cf, 0,
						"unexpected \"}\"");
					return HKY_ERROR;
				}

				return HKY_CONF_BLOCK_DONE;

			case '#':
				sharp_comment = 1;
				continue;

			case '\\':
				quoted = 1;
				last_space = 0;
				continue;

			case '"':
				start++;
				d_quoted = 1;
				last_space = 0;
				continue;

			case '\'':
				start++;
				s_quoted = 1;
				last_space = 0;
				continue;

			case '$':
				variable = 1;
				last_space = 0;
				continue;

			default:
				last_space = 0;
			}

		}
		else {
			if (ch == '{' && variable) {
				continue;
			}

			variable = 0;

			if (ch == '\\') {
				quoted = 1;
				continue;
			}

			if (ch == '$') {
				variable = 1;
				continue;
			}

			if (d_quoted) {
				if (ch == '"') {
					d_quoted = 0;
					need_space = 1;
					found = 1;
				}

			}
			else if (s_quoted) {
				if (ch == '\'') {
					s_quoted = 0;
					need_space = 1;
					found = 1;
				}

			}
			else if (ch == ' ' || ch == '\t' || ch == CR || ch == LF
				|| ch == ';' || ch == '{')
			{
				last_space = 1;
				found = 1;
			}

			if (found) {
				word = hky_array_push(cf->args);
				if (word == NULL) {
					return HKY_ERROR;
				}

				word->data = hky_pnalloc(cf->pool, b->pos - 1 - start + 1);
				if (word->data == NULL) {
					return HKY_ERROR;
				}

				for (dst = word->data, src = start, len = 0;
					src < b->pos - 1;
					len++)
				{
					if (*src == '\\') {
						switch (src[1]) {
						case '"':
						case '\'':
						case '\\':
							src++;
							break;

						case 't':
							*dst++ = '\t';
							src += 2;
							continue;

						case 'r':
							*dst++ = '\r';
							src += 2;
							continue;

						case 'n':
							*dst++ = '\n';
							src += 2;
							continue;
						}

					}
					*dst++ = *src++;
				}
				*dst = '\0';
				word->len = len;

				if (ch == ';') {
					return HKY_OK;
				}

				if (ch == '{') {
					return HKY_CONF_BLOCK_START;
				}

				found = 0;
			}
		}
	}
}


static void
hky_conf_flush_files(hky_origin_t *cycle)
{
	hky_uint_t        i;
	hky_list_part_t  *part;
	hky_open_file_t  *file;

	hky_log_debug0(HKY_LOG_DEBUG_CORE, cycle->log, 0, "flush files");

	part = &cycle->open_files.part;
	file = part->elts;

	for (i = 0; /* void */; i++) {

		if (i >= part->nelts) {
			if (part->next == NULL) {
				break;
			}
			part = part->next;
			file = part->elts;
			i = 0;
		}

		if (file[i].flush) {
			file[i].flush(&file[i], cycle->log);
		}
	}
}
