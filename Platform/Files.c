#include "../Config.h"

#define HKY_UTF16_BUFLEN  256

static hky_int_t hky_win32_check_filename(u_char *name, u_short *u,
	size_t len);
static u_short *hky_utf8_to_utf16(u_short *utf16, u_char *utf8, size_t *len);

hky_fd_t
hky_open_file(u_char *name, u_long mode, u_long create, u_long access)
{
	size_t      len;
	u_short    *u;
	hky_fd_t    fd;
	hky_err_t   err;
	u_short     utf16[HKY_UTF16_BUFLEN];

	len = HKY_UTF16_BUFLEN;
	u = hky_utf8_to_utf16(utf16, name, &len);

	if (u == NULL) {
		return INVALID_HANDLE_VALUE;
	}

	fd = INVALID_HANDLE_VALUE;

	if (create == HKY_FILE_OPEN
		&& hky_win32_check_filename(name, u, len) != HKY_OK)
	{
		goto failed;
	}

	fd = CreateFileW(u, mode,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL, create, FILE_FLAG_BACKUP_SEMANTICS, NULL);

failed:

	if (u != utf16) {
		err = hky_errno;
		hky_free(u);
		hky_set_errno(err);
	}

	return fd;
}

ssize_t
hky_read_fd(hky_fd_t fd, void *buf, size_t size)
{
	u_long  n;

	if (ReadFile(fd, buf, size, &n, NULL) != 0) {
		return (size_t)n;
	}

	return -1;
}

ssize_t
hky_write_fd(hky_fd_t fd, void *buf, size_t size)
{
	u_long  n;

	if (WriteFile(fd, buf, size, &n, NULL) != 0) {
		return (size_t)n;
	}

	return -1;
}

ssize_t
hky_write_console(hky_fd_t fd, void *buf, size_t size)
{
	u_long  n;

	(void)CharToOemBuff(buf, buf, size);

	if (WriteFile(fd, buf, size, &n, NULL) != 0) {
		return (size_t)n;
	}

	return -1;
}

hky_err_t
hky_win32_rename_file(hky_str_t *from, hky_str_t *to, hky_log_t *log)
{
	u_char             *name;
	hky_err_t           err;
	hky_uint_t          collision;
	hky_atomic_uint_t   num;

	name = hky_alloc(to->len + 1 + HKY_ATOMIC_T_LEN + 1 + sizeof("DELETE"),
		log);
	if (name == NULL) {
		return HKY_ENOMEM;
	}

	hky_memcpy(name, to->data, to->len);

	collision = 0;

	/* mutex_lock() (per cache or single ?) */

	for (;; ) {
		num = hky_next_temp_number(collision);

		hky_sprintf(name + to->len, ".%0muA.DELETE%Z", num);

		if (MoveFile((const char *)to->data, (const char *)name) != 0) {
			break;
		}

		collision = 1;

		hky_log_error(HKY_LOG_CRIT, log, hky_errno,
			"MoveFile() \"%s\" to \"%s\" failed", to->data, name);
	}

	if (MoveFile((const char *)from->data, (const char *)to->data) == 0) {
		err = hky_errno;

	}
	else {
		err = 0;
	}

	if (DeleteFile((const char *)name) == 0) {
		hky_log_error(HKY_LOG_CRIT, log, hky_errno,
			"DeleteFile() \"%s\" failed", name);
	}

	/* mutex_unlock() */

	hky_free(name);

	return err;
}

hky_int_t
hky_set_file_time(u_char *name, hky_fd_t fd, time_t s)
{
	uint64_t  intervals;
	FILETIME  ft;

	/* 116444736000000000 is commented in src/os/win32/hky_time.c */

	intervals = s * 10000000 + 116444736000000000;

	ft.dwLowDateTime = (DWORD)intervals;
	ft.dwHighDateTime = (DWORD)(intervals >> 32);

	if (SetFileTime(fd, NULL, NULL, &ft) != 0) {
		return HKY_OK;
	}

	return HKY_ERROR;
}

hky_int_t
hky_file_info(u_char *file, hky_file_info_t *sb)
{
	size_t                      len;
	long                        rc;
	u_short                    *u;
	hky_err_t                   err;
	WIN32_FILE_ATTRIBUTE_DATA   fa;
	u_short                     utf16[HKY_UTF16_BUFLEN];

	len = HKY_UTF16_BUFLEN;

	u = hky_utf8_to_utf16(utf16, file, &len);

	if (u == NULL) {
		return HKY_FILE_ERROR;
	}

	rc = HKY_FILE_ERROR;

	if (hky_win32_check_filename(file, u, len) != HKY_OK) {
		goto failed;
	}

	rc = GetFileAttributesExW(u, GetFileExInfoStandard, &fa);

	sb->dwFileAttributes = fa.dwFileAttributes;
	sb->ftCreationTime = fa.ftCreationTime;
	sb->ftLastAccessTime = fa.ftLastAccessTime;
	sb->ftLastWriteTime = fa.ftLastWriteTime;
	sb->nFileSizeHigh = fa.nFileSizeHigh;
	sb->nFileSizeLow = fa.nFileSizeLow;

failed:

	if (u != utf16) {
		err = hky_errno;
		hky_free(u);
		hky_set_errno(err);
	}

	return rc;
}

hky_int_t
hky_create_file_mapping(hky_file_mapping_t *fm)
{
	LARGE_INTEGER  size;

	fm->fd = hky_open_file(fm->name, HKY_FILE_RDWR, HKY_FILE_TRUNCATE,
		HKY_FILE_DEFAULT_ACCESS);

	if (fm->fd == HKY_INVALID_FILE) {
		hky_log_error(HKY_LOG_CRIT, fm->log, hky_errno,
			hky_open_file_n " \"%s\" failed", fm->name);
		return HKY_ERROR;
	}

	fm->handle = NULL;

	size.QuadPart = fm->size;

	if (SetFilePointerEx(fm->fd, size, NULL, FILE_BEGIN) == 0) {
		hky_log_error(HKY_LOG_CRIT, fm->log, hky_errno,
			"SetFilePointerEx(\"%s\", %uz) failed",
			fm->name, fm->size);
		goto failed;
	}

	if (SetEndOfFile(fm->fd) == 0) {
		hky_log_error(HKY_LOG_CRIT, fm->log, hky_errno,
			"SetEndOfFile() \"%s\" failed", fm->name);
		goto failed;
	}

	fm->handle = CreateFileMapping(fm->fd, NULL, PAGE_READWRITE,
		(u_long)((off_t)fm->size >> 32),
		(u_long)((off_t)fm->size & 0xffffffff),
		NULL);
	if (fm->handle == NULL) {
		hky_log_error(HKY_LOG_CRIT, fm->log, hky_errno,
			"CreateFileMapping(%s, %uz) failed",
			fm->name, fm->size);
		goto failed;
	}

	fm->addr = MapViewOfFile(fm->handle, FILE_MAP_WRITE, 0, 0, 0);

	if (fm->addr != NULL) {
		return HKY_OK;
	}

	hky_log_error(HKY_LOG_CRIT, fm->log, hky_errno,
		"MapViewOfFile(%uz) of file mapping \"%s\" failed",
		fm->size, fm->name);

failed:

	if (fm->handle) {
		if (CloseHandle(fm->handle) == 0) {
			hky_log_error(HKY_LOG_ALERT, fm->log, hky_errno,
				"CloseHandle() of file mapping \"%s\" failed",
				fm->name);
		}
	}

	if (hky_close_file(fm->fd) == HKY_FILE_ERROR) {
		hky_log_error(HKY_LOG_ALERT, fm->log, hky_errno,
			hky_close_file_n " \"%s\" failed", fm->name);
	}

	return HKY_ERROR;
}


void
hky_close_file_mapping(hky_file_mapping_t *fm)
{
	if (UnmapViewOfFile(fm->addr) == 0) {
		hky_log_error(HKY_LOG_ALERT, fm->log, hky_errno,
			"UnmapViewOfFile(%p) of file mapping \"%s\" failed",
			fm->addr, &fm->name);
	}

	if (CloseHandle(fm->handle) == 0) {
		hky_log_error(HKY_LOG_ALERT, fm->log, hky_errno,
			"CloseHandle() of file mapping \"%s\" failed",
			&fm->name);
	}

	if (hky_close_file(fm->fd) == HKY_FILE_ERROR) {
		hky_log_error(HKY_LOG_ALERT, fm->log, hky_errno,
			hky_close_file_n " \"%s\" failed", fm->name);
	}
}

u_char *
hky_realpath(u_char *path, u_char *resolved)
{
	/* STUB */
	return path;
}


hky_int_t
hky_open_dir(hky_str_t *name, hky_dir_t *dir)
{
	hky_cpystrn(name->data + name->len, HKY_DIR_MASK, HKY_DIR_MASK_LEN + 1);

	dir->dir = FindFirstFile((const char *)name->data, &dir->finddata);

	name->data[name->len] = '\0';

	if (dir->dir == INVALID_HANDLE_VALUE) {
		return HKY_ERROR;
	}

	dir->valid_info = 1;
	dir->ready = 1;

	return HKY_OK;
}


hky_int_t
hky_read_dir(hky_dir_t *dir)
{
	if (dir->ready) {
		dir->ready = 0;
		return HKY_OK;
	}

	if (FindNextFile(dir->dir, &dir->finddata) != 0) {
		dir->type = 1;
		return HKY_OK;
	}

	return HKY_ERROR;
}


hky_int_t
hky_close_dir(hky_dir_t *dir)
{
	if (FindClose(dir->dir) == 0) {
		return HKY_ERROR;
	}

	return HKY_OK;
}

hky_int_t
hky_de_info(u_char *name, hky_dir_t *dir)
{
	return HKY_OK;
}


hky_int_t
hky_de_link_info(u_char *name, hky_dir_t *dir)
{
	return HKY_OK;
}

hky_int_t
hky_open_glob(hky_glob_t *gl)
{
	u_char     *p;
	size_t      len;
	hky_err_t   err;

	gl->dir = FindFirstFile((const char *)gl->pattern, &gl->finddata);

	if (gl->dir == INVALID_HANDLE_VALUE) {

		err = hky_errno;

		if ((err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND)
			&& gl->test)
		{
			gl->no_match = 1;
			return HKY_OK;
		}

		return HKY_ERROR;
	}

	for (p = gl->pattern; *p; p++) {
		if (*p == '/') {
			gl->last = p + 1 - gl->pattern;
		}
	}

	len = hky_strlen(gl->finddata.cFileName);
	gl->name.len = gl->last + len;

	gl->name.data = hky_alloc(gl->name.len + 1, gl->log);
	if (gl->name.data == NULL) {
		return HKY_ERROR;
	}

	hky_memcpy(gl->name.data, gl->pattern, gl->last);
	hky_cpystrn(gl->name.data + gl->last, (u_char *)gl->finddata.cFileName,
		len + 1);

	gl->ready = 1;

	return HKY_OK;
}


hky_int_t
hky_read_glob(hky_glob_t *gl, hky_str_t *name)
{
	size_t     len;
	hky_err_t  err;

	if (gl->no_match) {
		return HKY_DONE;
	}

	if (gl->ready) {
		*name = gl->name;

		gl->ready = 0;
		return HKY_OK;
	}

	hky_free(gl->name.data);
	gl->name.data = NULL;

	if (FindNextFile(gl->dir, &gl->finddata) != 0) {

		len = hky_strlen(gl->finddata.cFileName);
		gl->name.len = gl->last + len;

		gl->name.data = hky_alloc(gl->name.len + 1, gl->log);
		if (gl->name.data == NULL) {
			return HKY_ERROR;
		}

		hky_memcpy(gl->name.data, gl->pattern, gl->last);
		hky_cpystrn(gl->name.data + gl->last, (u_char *)gl->finddata.cFileName,
			len + 1);

		*name = gl->name;

		return HKY_OK;
	}

	err = hky_errno;

	if (err == HKY_ENOMOREFILES) {
		return HKY_DONE;
	}

	hky_log_error(HKY_LOG_ALERT, gl->log, err,
		"FindNextFile(%s) failed", gl->pattern);

	return HKY_ERROR;
}


void
hky_close_glob(hky_glob_t *gl)
{
	if (gl->name.data) {
		hky_free(gl->name.data);
	}

	if (gl->dir == INVALID_HANDLE_VALUE) {
		return;
	}

	if (FindClose(gl->dir) == 0) {
		hky_log_error(HKY_LOG_ALERT, gl->log, hky_errno,
			"FindClose(%s) failed", gl->pattern);
	}
}


ssize_t
hky_read_file(hky_file_t *file, u_char *buf, size_t size, off_t offset)
{
	u_long      n;
	hky_err_t   err;
	OVERLAPPED  ovlp, *povlp;

	ovlp.Internal = 0;
	ovlp.InternalHigh = 0;
	ovlp.Offset = (u_long)offset;
	ovlp.OffsetHigh = (u_long)(offset >> 32);
	ovlp.hEvent = NULL;

	povlp = &ovlp;

	if (ReadFile(file->fd, buf, size, &n, povlp) == 0) {
		err = hky_errno;

		if (err == ERROR_HANDLE_EOF) {
			return 0;
		}

		hky_log_error(HKY_LOG_ERR, file->log, err,
			"ReadFile() \"%s\" failed", file->name.data);
		return HKY_ERROR;
	}

	file->offset += n;

	return n;
}


ssize_t
hky_write_file(hky_file_t *file, u_char *buf, size_t size, off_t offset)
{
	u_long      n;
	OVERLAPPED  ovlp, *povlp;

	ovlp.Internal = 0;
	ovlp.InternalHigh = 0;
	ovlp.Offset = (u_long)offset;
	ovlp.OffsetHigh = (u_long)(offset >> 32);
	ovlp.hEvent = NULL;

	povlp = &ovlp;

	if (WriteFile(file->fd, buf, size, &n, povlp) == 0) {
		hky_log_error(HKY_LOG_ERR, file->log, hky_errno,
			"WriteFile() \"%s\" failed", file->name.data);
		return HKY_ERROR;
	}

	if (n != size) {
		hky_log_error(HKY_LOG_CRIT, file->log, 0,
			"WriteFile() \"%s\" has written only %ul of %uz",
			file->name.data, n, size);
		return HKY_ERROR;
	}

	file->offset += n;

	return n;
}


ssize_t
hky_write_chain_to_file(hky_file_t *file, hky_chain_t *cl, off_t offset,
	hky_pool_t *pool)
{
	u_char   *buf, *prev;
	size_t    size;
	ssize_t   total, n;

	total = 0;

	while (cl) {
		buf = cl->buf->pos;
		prev = buf;
		size = 0;

		/* coalesce the neighbouring bufs */

		while (cl && prev == cl->buf->pos) {
			size += cl->buf->last - cl->buf->pos;
			prev = cl->buf->last;
			cl = cl->next;
		}

		n = hky_write_file(file, buf, size, offset);

		if (n == HKY_ERROR) {
			return HKY_ERROR;
		}

		total += n;
		offset += n;
	}

	return total;
}


hky_int_t
hky_read_ahead(hky_fd_t fd, size_t n)
{
	return ~HKY_FILE_ERROR;
}


hky_int_t
hky_directio_on(hky_fd_t fd)
{
	return ~HKY_FILE_ERROR;
}


hky_int_t
hky_directio_off(hky_fd_t fd)
{
	return ~HKY_FILE_ERROR;
}


size_t
hky_fs_bsize(u_char *name)
{
	u_char  root[4];
	u_long  sc, bs, nfree, ncl;

	if (name[2] == ':') {
		hky_cpystrn(root, name, 4);
		name = root;
	}

	if (GetDiskFreeSpace((const char *)name, &sc, &bs, &nfree, &ncl) == 0) {
		return 512;
	}

	return sc * bs;
}

static hky_int_t
hky_win32_check_filename(u_char *name, u_short *u, size_t len)
{
	u_char     *p, ch;
	u_long      n;
	u_short    *lu;
	hky_err_t   err;
	enum {
		sw_start = 0,
		sw_normal,
		sw_after_slash,
		sw_after_colon,
		sw_after_dot
	} state;

	/* check for NTFS streams (":"), trailing dots and spaces */

	lu = NULL;
	state = sw_start;

	for (p = name; *p; p++) {
		ch = *p;

		switch (state) {

		case sw_start:

			/*
			* skip till first "/" to allow paths starting with drive and
			* relative path, like "c:html/"
			*/

			if (ch == '/' || ch == '\\') {
				state = sw_after_slash;
			}

			break;

		case sw_normal:

			if (ch == ':') {
				state = sw_after_colon;
				break;
			}

			if (ch == '.' || ch == ' ') {
				state = sw_after_dot;
				break;
			}

			if (ch == '/' || ch == '\\') {
				state = sw_after_slash;
				break;
			}

			break;

		case sw_after_slash:

			if (ch == '/' || ch == '\\') {
				break;
			}

			if (ch == '.') {
				break;
			}

			if (ch == ':') {
				state = sw_after_colon;
				break;
			}

			state = sw_normal;
			break;

		case sw_after_colon:

			if (ch == '/' || ch == '\\') {
				state = sw_after_slash;
				break;
			}

			goto invalid;

		case sw_after_dot:

			if (ch == '/' || ch == '\\') {
				goto invalid;
			}

			if (ch == ':') {
				goto invalid;
			}

			if (ch == '.' || ch == ' ') {
				break;
			}

			state = sw_normal;
			break;
		}
	}

	if (state == sw_after_dot) {
		goto invalid;
	}

	/* check if long name match */

	lu = malloc(len * 2);
	if (lu == NULL) {
		return HKY_ERROR;
	}

	n = GetLongPathNameW(u, lu, len);

	if (n == 0) {
		goto failed;
	}

	if (n != len - 1 || _wcsicmp(u, lu) != 0) {
		goto invalid;
	}

	hky_free(lu);

	return HKY_OK;

invalid:

	hky_set_errno(HKY_ENOENT);

failed:

	if (lu) {
		err = hky_errno;
		hky_free(lu);
		hky_set_errno(err);
	}

	return HKY_ERROR;
}


static u_short *
hky_utf8_to_utf16(u_short *utf16, u_char *utf8, size_t *len)
{
	u_char    *p;
	u_short   *u, *last;
	uint32_t   n;

	p = utf8;
	u = utf16;
	last = utf16 + *len;

	while (u < last) {

		if (*p < 0x80) {
			*u++ = (u_short)*p;

			if (*p == 0) {
				*len = u - utf16;
				return utf16;
			}

			p++;

			continue;
		}

		if (u + 1 == last) {
			*len = u - utf16;
			break;
		}

		n = hky_utf8_decode(&p, 4);

		if (n > 0x10ffff) {
			hky_set_errno(HKY_EILSEQ);
			return NULL;
		}

		if (n > 0xffff) {
			n -= 0x10000;
			*u++ = (u_short)(0xd800 + (n >> 10));
			*u++ = (u_short)(0xdc00 + (n & 0x03ff));
			continue;
		}

		*u++ = (u_short)n;
	}

	/* the given buffer is not enough, allocate a new one */

	u = malloc(((p - utf8) + hky_strlen(p) + 1) * sizeof(u_short));
	if (u == NULL) {
		return NULL;
	}

	hky_memcpy(u, utf16, *len * 2);

	utf16 = u;
	u += *len;

	for (;; ) {

		if (*p < 0x80) {
			*u++ = (u_short)*p;

			if (*p == 0) {
				*len = u - utf16;
				return utf16;
			}

			p++;

			continue;
		}

		n = hky_utf8_decode(&p, 4);

		if (n > 0x10ffff) {
			hky_free(utf16);
			hky_set_errno(HKY_EILSEQ);
			return NULL;
		}

		if (n > 0xffff) {
			n -= 0x10000;
			*u++ = (u_short)(0xd800 + (n >> 10));
			*u++ = (u_short)(0xdc00 + (n & 0x03ff));
			continue;
		}

		*u++ = (u_short)n;
	}

	/* unreachable */
}
