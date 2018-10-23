#ifndef _HKY_FILES_H_INCLUDE_
#define _HKY_FILES_H_INCLUDE_

#include "Config.h"

typedef HANDLE                      hky_fd_t;
typedef BY_HANDLE_FILE_INFORMATION  hky_file_info_t;
typedef uint64_t                    hky_file_uniq_t;


typedef struct {
	u_char                         *name;
	size_t                          size;
	void                           *addr;
	hky_fd_t                        fd;
	HANDLE                          handle;
	hky_log_t                      *log;
} hky_file_mapping_t;


typedef struct {
	HANDLE                          dir;
	WIN32_FIND_DATA                 finddata;

	unsigned                        valid_info : 1;
	unsigned                        type : 1;
	unsigned                        ready : 1;
} hky_dir_t;


typedef struct {
	HANDLE                          dir;
	WIN32_FIND_DATA                 finddata;

	unsigned                        ready : 1;
	unsigned                        test : 1;
	unsigned                        no_match : 1;

	u_char                         *pattern;
	hky_str_t                       name;
	size_t                          last;
	hky_log_t                      *log;
} hky_glob_t;



/* INVALID_FILE_ATTRIBUTES is specified but not defined at least in MSVC6SP2 */
#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES     0xffffffff
#endif

/* INVALID_SET_FILE_POINTER is not defined at least in MSVC6SP2 */
#ifndef INVALID_SET_FILE_POINTER
#define INVALID_SET_FILE_POINTER    0xffffffff
#endif


#define HKY_INVALID_FILE            INVALID_HANDLE_VALUE
#define HKY_FILE_ERROR              0


hky_fd_t hky_open_file(u_char *name, u_long mode, u_long create, u_long access);
#define hky_open_file_n             "CreateFile()"

#define HKY_FILE_RDONLY             GENERIC_READ
#define HKY_FILE_WRONLY             GENERIC_WRITE
#define HKY_FILE_RDWR               GENERIC_READ|GENERIC_WRITE
#define HKY_FILE_APPEND             FILE_APPEND_DATA|SYNCHRONIZE
#define HKY_FILE_NONBLOCK           0

#define HKY_FILE_CREATE_OR_OPEN     OPEN_ALWAYS
#define HKY_FILE_OPEN               OPEN_EXISTING
#define HKY_FILE_TRUNCATE           CREATE_ALWAYS

#define HKY_FILE_DEFAULT_ACCESS     0
#define HKY_FILE_OWNER_ACCESS       0


#define hky_open_tempfile(name, persistent, access)                          \
    CreateFile((const char *) name,                                          \
               GENERIC_READ|GENERIC_WRITE,                                   \
               FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,           \
               NULL,                                                         \
               CREATE_NEW,                                                   \
               persistent ? 0:                                               \
                   FILE_ATTRIBUTE_TEMPORARY|FILE_FLAG_DELETE_ON_CLOSE,       \
               NULL);

#define hky_open_tempfile_n         "CreateFile()"


#define hky_close_file              CloseHandle
#define hky_close_file_n            "CloseHandle()"


ssize_t hky_read_fd(hky_fd_t fd, void *buf, size_t size);
#define hky_read_fd_n               "ReadFile()"


ssize_t hky_write_fd(hky_fd_t fd, void *buf, size_t size);
#define hky_write_fd_n              "WriteFile()"


ssize_t hky_write_console(hky_fd_t fd, void *buf, size_t size);


#define hky_linefeed(p)             *p++ = CR; *p++ = LF;
#define HKY_LINEFEED_SIZE           2
#define HKY_LINEFEED                CRLF


#define hky_delete_file(name)       DeleteFile((const char *) name)
#define hky_delete_file_n           "DeleteFile()"


#define hky_rename_file(o, n)       MoveFile((const char *) o, (const char *) n)
#define hky_rename_file_n           "MoveFile()"
hky_err_t hky_win32_rename_file(hky_str_t *from, hky_str_t *to, hky_log_t *log);



hky_int_t hky_set_file_time(u_char *name, hky_fd_t fd, time_t s);
#define hky_set_file_time_n         "SetFileTime()"


hky_int_t hky_file_info(u_char *filename, hky_file_info_t *fi);
#define hky_file_info_n             "GetFileAttributesEx()"


#define hky_fd_info(fd, fi)         GetFileInformationByHandle(fd, fi)
#define hky_fd_info_n               "GetFileInformationByHandle()"


#define hky_link_info(name, fi)     hky_file_info(name, fi)
#define hky_link_info_n             "GetFileAttributesEx()"


#define hky_is_dir(fi)                                                       \
    (((fi)->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
#define hky_is_file(fi)                                                      \
    (((fi)->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
#define hky_is_link(fi)     0
#define hky_is_exec(fi)     0

#define hky_file_access(fi) 0

#define hky_file_size(fi)                                                    \
    (((off_t) (fi)->nFileSizeHigh << 32) | (fi)->nFileSizeLow)
#define hky_file_fs_size(fi)        hky_file_size(fi)

#define hky_file_uniq(fi)   (*(hky_file_uniq_t *) &(fi)->nFileIndexHigh)


/* 116444736000000000 is commented in src/os/win32/hky_time.c */

#define hky_file_mtime(fi)                                                   \
 (time_t) (((((unsigned __int64) (fi)->ftLastWriteTime.dwHighDateTime << 32) \
                               | (fi)->ftLastWriteTime.dwLowDateTime)        \
                                          - 116444736000000000) / 10000000)

hky_int_t hky_create_file_mapping(hky_file_mapping_t *fm);
void hky_close_file_mapping(hky_file_mapping_t *fm);


u_char *hky_realpath(u_char *path, u_char *resolved);
#define hky_realpath_n              ""
#define hky_getcwd(buf, size)       GetCurrentDirectory(size, (char *) buf)
#define hky_getcwd_n                "GetCurrentDirectory()"
#define hky_path_separator(c)       ((c) == '/' || (c) == '\\')

#define HKY_HAVE_MAX_PATH           1
#define HKY_MAX_PATH                MAX_PATH

#define HKY_DIR_MASK                (u_char *) "/*"
#define HKY_DIR_MASK_LEN            2


hky_int_t hky_open_dir(hky_str_t *name, hky_dir_t *dir);
#define hky_open_dir_n              "FindFirstFile()"


hky_int_t hky_read_dir(hky_dir_t *dir);
#define hky_read_dir_n              "FindNextFile()"


hky_int_t hky_close_dir(hky_dir_t *dir);
#define hky_close_dir_n             "FindClose()"


#define hky_create_dir(name, access) CreateDirectory((const char *) name, NULL)
#define hky_create_dir_n            "CreateDirectory()"


#define hky_delete_dir(name)        RemoveDirectory((const char *) name)
#define hky_delete_dir_n            "RemoveDirectory()"


#define hky_dir_access(a)           (a)


#define hky_de_name(dir)            ((u_char *) (dir)->finddata.cFileName)
#define hky_de_namelen(dir)         hky_strlen((dir)->finddata.cFileName)

hky_int_t hky_de_info(u_char *name, hky_dir_t *dir);
#define hky_de_info_n               "dummy()"

hky_int_t hky_de_link_info(u_char *name, hky_dir_t *dir);
#define hky_de_link_info_n          "dummy()"

#define hky_de_is_dir(dir)                                                   \
    (((dir)->finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
#define hky_de_is_file(dir)                                                  \
    (((dir)->finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
#define hky_de_is_link(dir)         0
#define hky_de_access(dir)          0
#define hky_de_size(dir)                                                     \
  (((off_t) (dir)->finddata.nFileSizeHigh << 32) | (dir)->finddata.nFileSizeLow)
#define hky_de_fs_size(dir)         hky_de_size(dir)

/* 116444736000000000 is commented in src/os/win32/hky_time.c */

#define hky_de_mtime(dir)                                                    \
    (time_t) (((((unsigned __int64)                                          \
                     (dir)->finddata.ftLastWriteTime.dwHighDateTime << 32)   \
                      | (dir)->finddata.ftLastWriteTime.dwLowDateTime)       \
                                          - 116444736000000000) / 10000000)


hky_int_t hky_open_glob(hky_glob_t *gl);
#define hky_open_glob_n             "FindFirstFile()"

hky_int_t hky_read_glob(hky_glob_t *gl, hky_str_t *name);
void hky_close_glob(hky_glob_t *gl);


ssize_t hky_read_file(hky_file_t *file, u_char *buf, size_t size, off_t offset);
#define hky_read_file_n             "ReadFile()"

ssize_t hky_write_file(hky_file_t *file, u_char *buf, size_t size,
	off_t offset);

ssize_t hky_write_chain_to_file(hky_file_t *file, hky_chain_t *ce,
	off_t offset, hky_pool_t *pool);

hky_int_t hky_read_ahead(hky_fd_t fd, size_t n);
#define hky_read_ahead_n            "hky_read_ahead_n"

hky_int_t hky_directio_on(hky_fd_t fd);
#define hky_directio_on_n           "hky_directio_on_n"

hky_int_t hky_directio_off(hky_fd_t fd);
#define hky_directio_off_n          "hky_directio_off_n"

size_t hky_fs_bsize(u_char *name);


#define hky_stdout               GetStdHandle(STD_OUTPUT_HANDLE)
#define hky_stderr               GetStdHandle(STD_ERROR_HANDLE)
#define hky_set_stderr(fd)       SetStdHandle(STD_ERROR_HANDLE, fd)
#define hky_set_stderr_n         "SetStdHandle(STD_ERROR_HANDLE)"


#endif // !_HKY_FILES_H_INCLUDE_
