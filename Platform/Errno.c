#include "../Config.h"

u_char *
hky_strerror(hky_err_t err, u_char *errstr, size_t size)
{
	u_int          len;
	static u_long  lang = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);

	if (size == 0) {
		return errstr;
	}

	len = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, err, lang, (char *)errstr, size, NULL);

	if (len == 0 && lang && GetLastError() == ERROR_RESOURCE_LANG_NOT_FOUND) {

		lang = 0;

		len = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
			NULL, err, lang, (char *)errstr, size, NULL);
	}

	if (len == 0) {
		//return hky_snprintf(errstr, size,
		//	"FormatMessage() error:(%d)", GetLastError());
	}

	/* remove ".\r\n\0" */
	while (errstr[len] == '\0' || errstr[len] == CR
		|| errstr[len] == LF || errstr[len] == '.')
	{
		--len;
	}

	return &errstr[++len];
}


hky_int_t
hky_strerror_init(void)
{
	return HKY_OK;
}