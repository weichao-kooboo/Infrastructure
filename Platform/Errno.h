#ifndef _HKY_ERRNO_H_INCLUDE_
#define _HKY_ERRNO_H_INCLUDE_

#include "../Config.h"

typedef DWORD                      hky_err_t;

#define hky_errno                  GetLastError()
#define hky_set_errno(err)         SetLastError(err)
#define hky_socket_errno           WSAGetLastError()
#define hky_set_socket_errno(err)  WSASetLastError(err)

#define HKY_EPERM                  ERROR_ACCESS_DENIED
#define HKY_ENOENT                 ERROR_FILE_NOT_FOUND
#define HKY_ENOPATH                ERROR_PATH_NOT_FOUND
#define HKY_ENOMEM                 ERROR_NOT_ENOUGH_MEMORY
#define HKY_EACCES                 ERROR_ACCESS_DENIED
#define HKY_EEXIST                 ERROR_ALREADY_EXISTS
#define HKY_EEXIST_FILE            ERROR_FILE_EXISTS
#define HKY_EXDEV                  ERROR_NOT_SAME_DEVICE
#define HKY_ENOTDIR                ERROR_PATH_NOT_FOUND
#define HKY_EISDIR                 ERROR_CANNOT_MAKE
#define HKY_ENOSPC                 ERROR_DISK_FULL
#define HKY_EPIPE                  EPIPE
#define HKY_EAGAIN                 WSAEWOULDBLOCK
#define HKY_EINPROGRESS            WSAEINPROGRESS
#define HKY_ENOPROTOOPT            WSAENOPROTOOPT
#define HKY_EOPNOTSUPP             WSAEOPNOTSUPP
#define HKY_EADDRINUSE             WSAEADDRINUSE
#define HKY_ECONNABORTED           WSAECONNABORTED
#define HKY_ECONNRESET             WSAECONNRESET
#define HKY_ENOTCONN               WSAENOTCONN
#define HKY_ETIMEDOUT              WSAETIMEDOUT
#define HKY_ECONNREFUSED           WSAECONNREFUSED
#define HKY_ENAMETOOLONG           ERROR_BAD_PATHNAME
#define HKY_ENETDOWN               WSAENETDOWN
#define HKY_ENETUNREACH            WSAENETUNREACH
#define HKY_EHOSTDOWN              WSAEHOSTDOWN
#define HKY_EHOSTUNREACH           WSAEHOSTUNREACH
#define HKY_ENOMOREFILES           ERROR_NO_MORE_FILES
#define HKY_EILSEQ                 ERROR_NO_UNICODE_TRANSLATION
#define HKY_ELOOP                  0
#define HKY_EBADF                  WSAEBADF

#define HKY_EALREADY               WSAEALREADY
#define HKY_EINVAL                 WSAEINVAL
#define HKY_EMFILE                 WSAEMFILE
#define HKY_ENFILE                 WSAEMFILE


u_char *hky_strerror(hky_err_t err, u_char *errstr, size_t size);
hky_int_t hky_strerror_init(void);


#endif // !_HKY_ERRNO_H_INCLUDE_
