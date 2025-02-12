#include "stdafx.h"
#include "net_msg.h"

#ifndef	USE_PRINTF_MACRO

int net_last_error(void)
{
#ifdef	SYS_WIN
	int   error;

	error = WSAGetLastError();
	WSASetLastError(error);
	return error;
#else
	return errno;
#endif
}

void net_msg_info(const char *fmt,...)
{
	va_list ap;

	va_start (ap, fmt);

	va_end (ap);
}

void net_msg_warn(const char *fmt,...)
{
	va_list ap;

	va_start (ap, fmt);
	va_end (ap);
}

void net_msg_error(const char *fmt,...)
{
	va_list ap;

	va_start (ap, fmt);
	va_end (ap);
}

void net_msg_fatal(const char *fmt,...)
{
	va_list ap;

	va_start (ap, fmt);
	va_end (ap);
	abort();
}

#endif  /* USE_PRINTF_MACRO */
