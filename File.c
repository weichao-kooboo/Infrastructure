#include "Config.h"

static hky_int_t hky_test_full_name(hky_str_t *name);


static hky_atomic_t   temp_number = 0;
hky_atomic_t         *hky_temp_number = &temp_number;
hky_atomic_int_t      hky_random_number = 123456;


static hky_int_t
hky_test_full_name(hky_str_t *name)
{
#if (HKY_WIN32)
	u_char  c0, c1;

	c0 = name->data[0];

	if (name->len < 2) {
		if (c0 == '/') {
			return 2;
		}

		return HKY_DECLINED;
	}

	c1 = name->data[1];

	if (c1 == ':') {
		c0 |= 0x20;

		if ((c0 >= 'a' && c0 <= 'z')) {
			return HKY_OK;
		}

		return HKY_DECLINED;
	}

	if (c1 == '/') {
		return HKY_OK;
	}

	if (c0 == '/') {
		return 2;
	}

	return HKY_DECLINED;

#else

	if (name->data[0] == '/') {
		return HKY_OK;
	}

	return HKY_DECLINED;

#endif
}


hky_int_t
hky_get_full_name(hky_pool_t *pool, hky_str_t *prefix, hky_str_t *name)
{
	size_t      len;
	u_char     *p, *n;
	hky_int_t   rc;

	rc = hky_test_full_name(name);

	if (rc == HKY_OK) {
		return rc;
	}

	len = prefix->len;

#if (HKY_WIN32)

	if (rc == 2) {
		len = rc;
	}

#endif

	n = hky_pnalloc(pool, len + name->len + 1);
	if (n == NULL) {
		return HKY_ERROR;
	}

	p = hky_cpymem(n, prefix->data, len);
	hky_cpystrn(p, name->data, name->len + 1);

	name->len += len;
	name->data = n;

	return HKY_OK;
}


ssize_t
hky_write_chain_to_temp_file(hky_temp_file_t *tf, hky_chain_t *chain)
{
	hky_int_t  rc;

	if (tf->file.fd == HKY_INVALID_FILE) {
		rc = hky_create_temp_file(&tf->file, tf->path, tf->pool,
			tf->persistent, tf->clean, tf->access);

		if (rc != HKY_OK) {
			return rc;
		}

		if (tf->log_level) {
			hky_log_error(tf->log_level, tf->file.log, 0, "%s %V",
				tf->warn, &tf->file.name);
		}
	}

#if (HKY_THREADS && HKY_HAVE_PWRITEV)

	if (tf->thread_write) {
		return hky_thread_write_chain_to_file(&tf->file, chain, tf->offset,
			tf->pool);
	}

#endif

	return hky_write_chain_to_file(&tf->file, chain, tf->offset, tf->pool);
}


hky_int_t
hky_create_temp_file(hky_file_t *file, hky_path_t *path, hky_pool_t *pool,
	hky_uint_t persistent, hky_uint_t clean, hky_uint_t access)
{
	size_t                    levels;
	u_char                   *p;
	uint32_t                  n;
	hky_err_t                 err;
	hky_str_t                 name;
	hky_uint_t                prefix;
	hky_pool_cleanup_t       *cln;
	hky_pool_cleanup_file_t  *clnf;

	if (file->name.len) {
		name = file->name;
		levels = 0;
		prefix = 1;

	}
	else {
		name = path->name;
		levels = path->len;
		prefix = 0;
	}

	file->name.len = name.len + 1 + levels + 10;

	file->name.data = hky_pnalloc(pool, file->name.len + 1);
	if (file->name.data == NULL) {
		return HKY_ERROR;
	}

#if 0
	for (i = 0; i < file->name.len; i++) {
		file->name.data[i] = 'X';
	}
#endif

	p = hky_cpymem(file->name.data, name.data, name.len);

	if (prefix) {
		*p = '.';
	}

	p += 1 + levels;

	n = (uint32_t)hky_next_temp_number(0);

	cln = hky_pool_cleanup_add(pool, sizeof(hky_pool_cleanup_file_t));
	if (cln == NULL) {
		return HKY_ERROR;
	}

	for (;; ) {
		(void)hky_sprintf(p, "%010uD%Z", n);

		if (!prefix) {
			hky_create_hashed_filename(path, file->name.data, file->name.len);
		}

		hky_log_debug1(HKY_LOG_DEBUG_CORE, file->log, 0,
			"hashed path: %s", file->name.data);

		file->fd = hky_open_tempfile(file->name.data, persistent, access);

		hky_log_debug1(HKY_LOG_DEBUG_CORE, file->log, 0,
			"temp fd:%d", file->fd);

		if (file->fd != HKY_INVALID_FILE) {

			cln->handler = clean ? hky_pool_delete_file : hky_pool_cleanup_file;
			clnf = cln->data;

			clnf->fd = file->fd;
			clnf->name = file->name.data;
			clnf->log = pool->log;

			return HKY_OK;
		}

		err = hky_errno;

		if (err == HKY_EEXIST_FILE) {
			n = (uint32_t)hky_next_temp_number(1);
			continue;
		}

		if ((path->level[0] == 0) || (err != HKY_ENOPATH)) {
			hky_log_error(HKY_LOG_CRIT, file->log, err,
				hky_open_tempfile_n " \"%s\" failed",
				file->name.data);
			return HKY_ERROR;
		}

		if (hky_create_path(file, path) == HKY_ERROR) {
			return HKY_ERROR;
		}
	}
}


void
hky_create_hashed_filename(hky_path_t *path, u_char *file, size_t len)
{
	size_t      i, level;
	hky_uint_t  n;

	i = path->name.len + 1;

	file[path->name.len + path->len] = '/';

	for (n = 0; n < HKY_MAX_PATH_LEVEL; n++) {
		level = path->level[n];

		if (level == 0) {
			break;
		}

		len -= level;
		file[i - 1] = '/';
		hky_memcpy(&file[i], &file[len], level);
		i += level + 1;
	}
}


hky_int_t
hky_create_path(hky_file_t *file, hky_path_t *path)
{
	size_t      pos;
	hky_err_t   err;
	hky_uint_t  i;

	pos = path->name.len;

	for (i = 0; i < HKY_MAX_PATH_LEVEL; i++) {
		if (path->level[i] == 0) {
			break;
		}

		pos += path->level[i] + 1;

		file->name.data[pos] = '\0';

		hky_log_debug1(HKY_LOG_DEBUG_CORE, file->log, 0,
			"temp file: \"%s\"", file->name.data);

		if (hky_create_dir(file->name.data, 0700) == HKY_FILE_ERROR) {
			err = hky_errno;
			if (err != HKY_EEXIST) {
				hky_log_error(HKY_LOG_CRIT, file->log, err,
					hky_create_dir_n " \"%s\" failed",
					file->name.data);
				return HKY_ERROR;
			}
		}

		file->name.data[pos] = '/';
	}

	return HKY_OK;
}


hky_err_t
hky_create_full_path(u_char *dir, hky_uint_t access)
{
	u_char     *p, ch;
	hky_err_t   err;

	err = 0;

#if (HKY_WIN32)
	p = dir + 3;
#else
	p = dir + 1;
#endif

	for ( /* void */; *p; p++) {
		ch = *p;

		if (ch != '/') {
			continue;
		}

		*p = '\0';

		if (hky_create_dir(dir, access) == HKY_FILE_ERROR) {
			err = hky_errno;

			switch (err) {
			case HKY_EEXIST:
				err = 0;
			case HKY_EACCES:
				break;

			default:
				return err;
			}
		}

		*p = '/';
	}

	return err;
}


hky_int_t
hky_add_path(hky_conf_t *cf, hky_path_t **slot)
{
	hky_uint_t   i, n;
	hky_path_t  *path, **p;

	path = *slot;

	p = cf->cycle->paths.elts;
	for (i = 0; i < cf->cycle->paths.nelts; i++) {
		if (p[i]->name.len == path->name.len
			&& hky_strcmp(p[i]->name.data, path->name.data) == 0)
		{
			if (p[i]->data != path->data) {
				hky_conf_log_error(HKY_LOG_EMERG, cf, 0,
					"the same path name \"%V\" "
					"used in %s:%ui and",
					&p[i]->name, p[i]->conf_file, p[i]->line);
				return HKY_ERROR;
			}

			for (n = 0; n < HKY_MAX_PATH_LEVEL; n++) {
				if (p[i]->level[n] != path->level[n]) {
					if (path->conf_file == NULL) {
						if (p[i]->conf_file == NULL) {
							hky_log_error(HKY_LOG_EMERG, cf->log, 0,
								"the default path name \"%V\" has "
								"the same name as another default path, "
								"but the different levels, you need to "
								"redefine one of them in http section",
								&p[i]->name);
							return HKY_ERROR;
						}

						hky_log_error(HKY_LOG_EMERG, cf->log, 0,
							"the path name \"%V\" in %s:%ui has "
							"the same name as default path, but "
							"the different levels, you need to "
							"define default path in http section",
							&p[i]->name, p[i]->conf_file, p[i]->line);
						return HKY_ERROR;
					}

					hky_conf_log_error(HKY_LOG_EMERG, cf, 0,
						"the same path name \"%V\" in %s:%ui "
						"has the different levels than",
						&p[i]->name, p[i]->conf_file, p[i]->line);
					return HKY_ERROR;
				}

				if (p[i]->level[n] == 0) {
					break;
				}
			}

			*slot = p[i];

			return HKY_OK;
		}
	}

	p = hky_array_push(&cf->cycle->paths);
	if (p == NULL) {
		return HKY_ERROR;
	}

	*p = path;

	return HKY_OK;
}


hky_int_t
hky_create_paths(hky_origin_t *cycle, hky_uid_t user)
{
	hky_err_t         err;
	hky_uint_t        i;
	hky_path_t      **path;

	path = cycle->paths.elts;
	for (i = 0; i < cycle->paths.nelts; i++) {

		if (hky_create_dir(path[i]->name.data, 0700) == HKY_FILE_ERROR) {
			err = hky_errno;
			if (err != HKY_EEXIST) {
				hky_log_error(HKY_LOG_EMERG, cycle->log, err,
					hky_create_dir_n " \"%s\" failed",
					path[i]->name.data);
				return HKY_ERROR;
			}
		}

		if (user == (hky_uid_t)HKY_CONF_UNSET_UINT) {
			continue;
		}

#if !(HKY_WIN32)
		{
			hky_file_info_t   fi;

			if (hky_file_info(path[i]->name.data, &fi) == HKY_FILE_ERROR) {
				hky_log_error(HKY_LOG_EMERG, cycle->log, hky_errno,
					hky_file_info_n " \"%s\" failed", path[i]->name.data);
				return HKY_ERROR;
			}

			if (fi.st_uid != user) {
				if (chown((const char *)path[i]->name.data, user, -1) == -1) {
					hky_log_error(HKY_LOG_EMERG, cycle->log, hky_errno,
						"chown(\"%s\", %d) failed",
						path[i]->name.data, user);
					return HKY_ERROR;
				}
			}

			if ((fi.st_mode & (S_IRUSR | S_IWUSR | S_IXUSR))
				!= (S_IRUSR | S_IWUSR | S_IXUSR))
			{
				fi.st_mode |= (S_IRUSR | S_IWUSR | S_IXUSR);

				if (chmod((const char *)path[i]->name.data, fi.st_mode) == -1) {
					hky_log_error(HKY_LOG_EMERG, cycle->log, hky_errno,
						"chmod() \"%s\" failed", path[i]->name.data);
					return HKY_ERROR;
				}
			}
		}
#endif
	}

	return HKY_OK;
}


hky_int_t
hky_ext_rename_file(hky_str_t *src, hky_str_t *to, hky_ext_rename_file_t *ext)
{
	u_char           *name;
	hky_err_t         err;
	hky_copy_file_t   cf;

#if !(HKY_WIN32)

	if (ext->access) {
		if (hky_change_file_access(src->data, ext->access) == HKY_FILE_ERROR) {
			hky_log_error(HKY_LOG_CRIT, ext->log, hky_errno,
				hky_change_file_access_n " \"%s\" failed", src->data);
			err = 0;
			goto failed;
		}
	}

#endif

	if (ext->time != -1) {
		if (hky_set_file_time(src->data, ext->fd, ext->time) != HKY_OK) {
			hky_log_error(HKY_LOG_CRIT, ext->log, hky_errno,
				hky_set_file_time_n " \"%s\" failed", src->data);
			err = 0;
			goto failed;
		}
	}

	if (hky_rename_file(src->data, to->data) != HKY_FILE_ERROR) {
		return HKY_OK;
	}

	err = hky_errno;

	if (err == HKY_ENOPATH) {

		if (!ext->create_path) {
			goto failed;
		}

		err = hky_create_full_path(to->data, hky_dir_access(ext->path_access));

		if (err) {
			hky_log_error(HKY_LOG_CRIT, ext->log, err,
				hky_create_dir_n " \"%s\" failed", to->data);
			err = 0;
			goto failed;
		}

		if (hky_rename_file(src->data, to->data) != HKY_FILE_ERROR) {
			return HKY_OK;
		}

		err = hky_errno;
	}

#if (HKY_WIN32)

	if (err == HKY_EEXIST || err == HKY_EEXIST_FILE) {
		err = hky_win32_rename_file(src, to, ext->log);

		if (err == 0) {
			return HKY_OK;
		}
	}

#endif

	if (err == HKY_EXDEV) {

		cf.size = -1;
		cf.buf_size = 0;
		cf.access = ext->access;
		cf.time = ext->time;
		cf.log = ext->log;

		name = hky_alloc(to->len + 1 + 10 + 1, ext->log);
		if (name == NULL) {
			return HKY_ERROR;
		}

		(void)hky_sprintf(name, "%*s.%010uD%Z", to->len, to->data,
			(uint32_t)hky_next_temp_number(0));

		if (hky_copy_file(src->data, name, &cf) == HKY_OK) {

			if (hky_rename_file(name, to->data) != HKY_FILE_ERROR) {
				hky_free(name);

				if (hky_delete_file(src->data) == HKY_FILE_ERROR) {
					hky_log_error(HKY_LOG_CRIT, ext->log, hky_errno,
						hky_delete_file_n " \"%s\" failed",
						src->data);
					return HKY_ERROR;
				}

				return HKY_OK;
			}

			hky_log_error(HKY_LOG_CRIT, ext->log, hky_errno,
				hky_rename_file_n " \"%s\" to \"%s\" failed",
				name, to->data);

			if (hky_delete_file(name) == HKY_FILE_ERROR) {
				hky_log_error(HKY_LOG_CRIT, ext->log, hky_errno,
					hky_delete_file_n " \"%s\" failed", name);

			}
		}

		hky_free(name);

		err = 0;
	}

failed:

	if (ext->delete_file) {
		if (hky_delete_file(src->data) == HKY_FILE_ERROR) {
			hky_log_error(HKY_LOG_CRIT, ext->log, hky_errno,
				hky_delete_file_n " \"%s\" failed", src->data);
		}
	}

	if (err) {
		hky_log_error(HKY_LOG_CRIT, ext->log, err,
			hky_rename_file_n " \"%s\" to \"%s\" failed",
			src->data, to->data);
	}

	return HKY_ERROR;
}


hky_int_t
hky_copy_file(u_char *from, u_char *to, hky_copy_file_t *cf)
{
	char             *buf;
	off_t             size;
	time_t            time;
	size_t            len;
	ssize_t           n;
	hky_fd_t          fd, nfd;
	hky_int_t         rc;
	hky_uint_t        access;
	hky_file_info_t   fi;

	rc = HKY_ERROR;
	buf = NULL;
	nfd = HKY_INVALID_FILE;

	fd = hky_open_file(from, HKY_FILE_RDONLY, HKY_FILE_OPEN, 0);

	if (fd == HKY_INVALID_FILE) {
		hky_log_error(HKY_LOG_CRIT, cf->log, hky_errno,
			hky_open_file_n " \"%s\" failed", from);
		goto failed;
	}

	if (cf->size != -1 && cf->access != 0 && cf->time != -1) {
		size = cf->size;
		access = cf->access;
		time = cf->time;

	}
	else {
		if (hky_fd_info(fd, &fi) == HKY_FILE_ERROR) {
			hky_log_error(HKY_LOG_ALERT, cf->log, hky_errno,
				hky_fd_info_n " \"%s\" failed", from);

			goto failed;
		}

		size = (cf->size != -1) ? cf->size : hky_file_size(&fi);
		access = cf->access ? cf->access : hky_file_access(&fi);
		time = (cf->time != -1) ? cf->time : hky_file_mtime(&fi);
	}

	len = cf->buf_size ? cf->buf_size : 65536;

	if ((off_t)len > size) {
		len = (size_t)size;
	}

	buf = hky_alloc(len, cf->log);
	if (buf == NULL) {
		goto failed;
	}

	nfd = hky_open_file(to, HKY_FILE_WRONLY, HKY_FILE_TRUNCATE, access);

	if (nfd == HKY_INVALID_FILE) {
		hky_log_error(HKY_LOG_CRIT, cf->log, hky_errno,
			hky_open_file_n " \"%s\" failed", to);
		goto failed;
	}

	while (size > 0) {

		if ((off_t)len > size) {
			len = (size_t)size;
		}

		n = hky_read_fd(fd, buf, len);

		if (n == -1) {
			hky_log_error(HKY_LOG_ALERT, cf->log, hky_errno,
				hky_read_fd_n " \"%s\" failed", from);
			goto failed;
		}

		if ((size_t)n != len) {
			hky_log_error(HKY_LOG_ALERT, cf->log, 0,
				hky_read_fd_n " has read only %z of %O from %s",
				n, size, from);
			goto failed;
		}

		n = hky_write_fd(nfd, buf, len);

		if (n == -1) {
			hky_log_error(HKY_LOG_ALERT, cf->log, hky_errno,
				hky_write_fd_n " \"%s\" failed", to);
			goto failed;
		}

		if ((size_t)n != len) {
			hky_log_error(HKY_LOG_ALERT, cf->log, 0,
				hky_write_fd_n " has written only %z of %O to %s",
				n, size, to);
			goto failed;
		}

		size -= n;
	}

	if (hky_set_file_time(to, nfd, time) != HKY_OK) {
		hky_log_error(HKY_LOG_ALERT, cf->log, hky_errno,
			hky_set_file_time_n " \"%s\" failed", to);
		goto failed;
	}

	rc = HKY_OK;

failed:

	if (nfd != HKY_INVALID_FILE) {
		if (hky_close_file(nfd) == HKY_FILE_ERROR) {
			hky_log_error(HKY_LOG_ALERT, cf->log, hky_errno,
				hky_close_file_n " \"%s\" failed", to);
		}
	}

	if (fd != HKY_INVALID_FILE) {
		if (hky_close_file(fd) == HKY_FILE_ERROR) {
			hky_log_error(HKY_LOG_ALERT, cf->log, hky_errno,
				hky_close_file_n " \"%s\" failed", from);
		}
	}

	if (buf) {
		hky_free(buf);
	}

	return rc;
}


hky_int_t
hky_walk_tree(hky_tree_ctx_t *ctx, hky_str_t *tree)
{
	void       *data, *prev;
	u_char     *p, *name;
	size_t      len;
	hky_int_t   rc;
	hky_err_t   err;
	hky_str_t   file, buf;
	hky_dir_t   dir;

	hky_str_null(&buf);

	hky_log_debug1(HKY_LOG_DEBUG_CORE, ctx->log, 0,
		"walk tree \"%V\"", tree);

	if (hky_open_dir(tree, &dir) == HKY_ERROR) {
		hky_log_error(HKY_LOG_CRIT, ctx->log, hky_errno,
			hky_open_dir_n " \"%s\" failed", tree->data);
		return HKY_ERROR;
	}

	prev = ctx->data;

	if (ctx->alloc) {
		data = hky_alloc(ctx->alloc, ctx->log);
		if (data == NULL) {
			goto failed;
		}

		if (ctx->init_handler(data, prev) == HKY_ABORT) {
			goto failed;
		}

		ctx->data = data;

	}
	else {
		data = NULL;
	}

	for (;; ) {

		hky_set_errno(0);

		if (hky_read_dir(&dir) == HKY_ERROR) {
			err = hky_errno;

			if (err == HKY_ENOMOREFILES) {
				rc = HKY_OK;

			}
			else {
				hky_log_error(HKY_LOG_CRIT, ctx->log, err,
					hky_read_dir_n " \"%s\" failed", tree->data);
				rc = HKY_ERROR;
			}

			goto done;
		}

		len = hky_de_namelen(&dir);
		name = hky_de_name(&dir);

		hky_log_debug2(HKY_LOG_DEBUG_CORE, ctx->log, 0,
			"tree name %uz:\"%s\"", len, name);

		if (len == 1 && name[0] == '.') {
			continue;
		}

		if (len == 2 && name[0] == '.' && name[1] == '.') {
			continue;
		}

		file.len = tree->len + 1 + len;

		if (file.len + HKY_DIR_MASK_LEN > buf.len) {

			if (buf.len) {
				hky_free(buf.data);
			}

			buf.len = tree->len + 1 + len + HKY_DIR_MASK_LEN;

			buf.data = hky_alloc(buf.len + 1, ctx->log);
			if (buf.data == NULL) {
				goto failed;
			}
		}

		p = hky_cpymem(buf.data, tree->data, tree->len);
		*p++ = '/';
		hky_memcpy(p, name, len + 1);

		file.data = buf.data;

		hky_log_debug1(HKY_LOG_DEBUG_CORE, ctx->log, 0,
			"tree path \"%s\"", file.data);

		if (!dir.valid_info) {
			if (hky_de_info(file.data, &dir) == HKY_FILE_ERROR) {
				hky_log_error(HKY_LOG_CRIT, ctx->log, hky_errno,
					hky_de_info_n " \"%s\" failed", file.data);
				continue;
			}
		}

		if (hky_de_is_file(&dir)) {

			hky_log_debug1(HKY_LOG_DEBUG_CORE, ctx->log, 0,
				"tree file \"%s\"", file.data);

			ctx->size = hky_de_size(&dir);
			ctx->fs_size = hky_de_fs_size(&dir);
			ctx->access = hky_de_access(&dir);
			ctx->mtime = hky_de_mtime(&dir);

			if (ctx->file_handler(ctx, &file) == HKY_ABORT) {
				goto failed;
			}

		}
		else if (hky_de_is_dir(&dir)) {

			hky_log_debug1(HKY_LOG_DEBUG_CORE, ctx->log, 0,
				"tree enter dir \"%s\"", file.data);

			ctx->access = hky_de_access(&dir);
			ctx->mtime = hky_de_mtime(&dir);

			rc = ctx->pre_tree_handler(ctx, &file);

			if (rc == HKY_ABORT) {
				goto failed;
			}

			if (rc == HKY_DECLINED) {
				hky_log_debug1(HKY_LOG_DEBUG_CORE, ctx->log, 0,
					"tree skip dir \"%s\"", file.data);
				continue;
			}

			if (hky_walk_tree(ctx, &file) == HKY_ABORT) {
				goto failed;
			}

			ctx->access = hky_de_access(&dir);
			ctx->mtime = hky_de_mtime(&dir);

			if (ctx->post_tree_handler(ctx, &file) == HKY_ABORT) {
				goto failed;
			}

		}
		else {

			hky_log_debug1(HKY_LOG_DEBUG_CORE, ctx->log, 0,
				"tree special \"%s\"", file.data);

			if (ctx->spec_handler(ctx, &file) == HKY_ABORT) {
				goto failed;
			}
		}
	}

failed:

	rc = HKY_ABORT;

done:

	if (buf.len) {
		hky_free(buf.data);
	}

	if (data) {
		hky_free(data);
		ctx->data = prev;
	}

	if (hky_close_dir(&dir) == HKY_ERROR) {
		hky_log_error(HKY_LOG_CRIT, ctx->log, hky_errno,
			hky_close_dir_n " \"%s\" failed", tree->data);
	}

	return rc;
}


hky_atomic_uint_t
hky_next_temp_number(hky_uint_t collision)
{
	hky_atomic_uint_t  n, add;

	add = collision ? hky_random_number : 1;

	n = hky_atomic_fetch_add(hky_temp_number, add);

	return n + add;
}


char *
hky_conf_set_path_slot(hky_conf_t *cf, hky_command_t *cmd, void *conf)
{
	char  *p = conf;

	ssize_t      level;
	hky_str_t   *value;
	hky_uint_t   i, n;
	hky_path_t  *path, **slot;

	slot = (hky_path_t **)(p + cmd->offset);

	if (*slot) {
		return "is duplicate";
	}

	path = hky_pcalloc(cf->pool, sizeof(hky_path_t));
	if (path == NULL) {
		return HKY_CONF_ERROR;
	}

	value = cf->args->elts;

	path->name = value[1];

	if (path->name.data[path->name.len - 1] == '/') {
		path->name.len--;
	}

	if (hky_conf_full_name(cf->cycle, &path->name, 0) != HKY_OK) {
		return HKY_CONF_ERROR;
	}

	path->conf_file = cf->conf_file->file.name.data;
	path->line = cf->conf_file->line;

	for (i = 0, n = 2; n < cf->args->nelts; i++, n++) {
		level = hky_atoi(value[n].data, value[n].len);
		if (level == HKY_ERROR || level == 0) {
			return "invalid value";
		}

		path->level[i] = level;
		path->len += level + 1;
	}

	if (path->len > 10 + i) {
		return "invalid value";
	}

	*slot = path;

	if (hky_add_path(cf, slot) == HKY_ERROR) {
		return HKY_CONF_ERROR;
	}

	return HKY_CONF_OK;
}


char *
hky_conf_merge_path_value(hky_conf_t *cf, hky_path_t **path, hky_path_t *prev,
	hky_path_init_t *init)
{
	hky_uint_t  i;

	if (*path) {
		return HKY_CONF_OK;
	}

	if (prev) {
		*path = prev;
		return HKY_CONF_OK;
	}

	*path = hky_pcalloc(cf->pool, sizeof(hky_path_t));
	if (*path == NULL) {
		return HKY_CONF_ERROR;
	}

	(*path)->name = init->name;

	if (hky_conf_full_name(cf->cycle, &(*path)->name, 0) != HKY_OK) {
		return HKY_CONF_ERROR;
	}

	for (i = 0; i < HKY_MAX_PATH_LEVEL; i++) {
		(*path)->level[i] = init->level[i];
		(*path)->len += init->level[i] + (init->level[i] ? 1 : 0);
	}

	if (hky_add_path(cf, path) != HKY_OK) {
		return HKY_CONF_ERROR;
	}

	return HKY_CONF_OK;
}


char *
hky_conf_set_access_slot(hky_conf_t *cf, hky_command_t *cmd, void *conf)
{
	char  *confp = conf;

	u_char      *p;
	hky_str_t   *value;
	hky_uint_t   i, right, shift, *access, user;

	access = (hky_uint_t *)(confp + cmd->offset);

	if (*access != HKY_CONF_UNSET_UINT) {
		return "is duplicate";
	}

	value = cf->args->elts;

	*access = 0;
	user = 0600;

	for (i = 1; i < cf->args->nelts; i++) {

		p = value[i].data;

		if (hky_strncmp(p, "user:", sizeof("user:") - 1) == 0) {
			shift = 6;
			p += sizeof("user:") - 1;
			user = 0;

		}
		else if (hky_strncmp(p, "group:", sizeof("group:") - 1) == 0) {
			shift = 3;
			p += sizeof("group:") - 1;

		}
		else if (hky_strncmp(p, "all:", sizeof("all:") - 1) == 0) {
			shift = 0;
			p += sizeof("all:") - 1;

		}
		else {
			goto invalid;
		}

		if (hky_strcmp(p, "rw") == 0) {
			right = 6;

		}
		else if (hky_strcmp(p, "r") == 0) {
			right = 4;

		}
		else {
			goto invalid;
		}

		*access |= right << shift;
	}

	*access |= user;

	return HKY_CONF_OK;

invalid:

	hky_conf_log_error(HKY_LOG_EMERG, cf, 0, "invalid value \"%V\"", &value[i]);

	return HKY_CONF_ERROR;
}


