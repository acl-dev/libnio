#include "stdafx.h"
#include "init.h"
#include "pthread_patch.h"
#include "msg.h"

#ifndef	USE_PRINTF_MACRO

int last_error(void)
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

void msg_info(const char *fmt,...)
{
	va_list ap;

	va_start (ap, fmt);

	va_end (ap);
}

void msg_warn(const char *fmt,...)
{
	va_list ap;

	va_start (ap, fmt);
	va_end (ap);
}

void msg_error(const char *fmt,...)
{
	va_list ap;

	va_start (ap, fmt);
	va_end (ap);
}

void msg_fatal(const char *fmt,...)
{
	va_list ap;

	va_start (ap, fmt);
	va_end (ap);
	abort();
}

#endif  /* USE_PRINTF_MACRO */
