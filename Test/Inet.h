#ifndef _HKY_INET_H_INCLUDE_
#define _HKY_INET_H_INCLUDE_

#include "Config.h"

#define HKY_INET_ADDRSTRLEN   (sizeof("255.255.255.255") - 1)
#define HKY_INET6_ADDRSTRLEN                                                 \
    (sizeof("ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255") - 1)
#define HKY_UNIX_ADDRSTRLEN                                                  \
    (sizeof("unix:") - 1 +                                                   \
     sizeof(struct sockaddr_un) - offsetof(struct sockaddr_un, sun_path))

#if (HKY_HAVE_UNIX_DOMAIN)
#define HKY_SOCKADDR_STRLEN   HKY_UNIX_ADDRSTRLEN
#elif (HKY_HAVE_INET6)
#define HKY_SOCKADDR_STRLEN   (HKY_INET6_ADDRSTRLEN + sizeof("[]:65535") - 1)
#else
#define HKY_SOCKADDR_STRLEN   (HKY_INET_ADDRSTRLEN + sizeof(":65535") - 1)
#endif

/* compatibility */
#define HKY_SOCKADDRLEN       sizeof(hky_sockaddr_t)


typedef union {
	struct sockaddr           sockaddr;
	struct sockaddr_in        sockaddr_in;
#if (HKY_HAVE_INET6)
	struct sockaddr_in6       sockaddr_in6;
#endif
#if (HKY_HAVE_UNIX_DOMAIN)
	struct sockaddr_un        sockaddr_un;
#endif
} hky_sockaddr_t;


typedef struct {
	in_addr_t                 addr;
	in_addr_t                 mask;
} hky_in_cidr_t;


#if (HKY_HAVE_INET6)

typedef struct {
	struct in6_addr           addr;
	struct in6_addr           mask;
} hky_in6_cidr_t;

#endif


typedef struct {
	hky_uint_t                family;
	union {
		hky_in_cidr_t         in;
#if (HKY_HAVE_INET6)
		hky_in6_cidr_t        in6;
#endif
	} u;
} hky_cidr_t;


typedef struct {
	struct sockaddr          *sockaddr;
	socklen_t                 socklen;
	hky_str_t                 name;
} hky_addr_t;


typedef struct {
	hky_str_t                 url;
	hky_str_t                 host;
	hky_str_t                 port_text;
	hky_str_t                 uri;

	in_port_t                 port;
	in_port_t                 default_port;
	int                       family;

	unsigned                  listen : 1;
	unsigned                  uri_part : 1;
	unsigned                  no_resolve : 1;

	unsigned                  no_port : 1;
	unsigned                  wildcard : 1;

	socklen_t                 socklen;
	hky_sockaddr_t            sockaddr;

	hky_addr_t               *addrs;
	hky_uint_t                naddrs;

	char                     *err;
} hky_url_t;


in_addr_t hky_inet_addr(u_char *text, size_t len);
#if (HKY_HAVE_INET6)
hky_int_t hky_inet6_addr(u_char *p, size_t len, u_char *addr);
size_t hky_inet6_ntop(u_char *p, u_char *text, size_t len);
#endif
size_t hky_sock_ntop(struct sockaddr *sa, socklen_t socklen, u_char *text,
	size_t len, hky_uint_t port);
size_t hky_inet_ntop(int family, void *addr, u_char *text, size_t len);
hky_int_t hky_ptocidr(hky_str_t *text, hky_cidr_t *cidr);
hky_int_t hky_cidr_match(struct sockaddr *sa, hky_array_t *cidrs);
hky_int_t hky_parse_addr(hky_pool_t *pool, hky_addr_t *addr, u_char *text,
	size_t len);
hky_int_t hky_parse_addr_port(hky_pool_t *pool, hky_addr_t *addr,
	u_char *text, size_t len);
hky_int_t hky_parse_url(hky_pool_t *pool, hky_url_t *u);
hky_int_t hky_inet_resolve_host(hky_pool_t *pool, hky_url_t *u);
hky_int_t hky_cmp_sockaddr(struct sockaddr *sa1, socklen_t slen1,
	struct sockaddr *sa2, socklen_t slen2, hky_uint_t cmp_port);
in_port_t hky_inet_get_port(struct sockaddr *sa);
void hky_inet_set_port(struct sockaddr *sa, in_port_t port);


#endif // !_HKY_INET_H_INCLUDE_
