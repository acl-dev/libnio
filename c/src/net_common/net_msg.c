#include "stdafx.h"
#include "net_msg.h"

static int stdout_on = 0;

void net_msg_stdout(int on)
{
	stdout_on = on;
}

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

static void fmt_printf(const char *tag, const char *fmt, va_list ap)
{
#if (defined(_WIN32) || defined(_WIN64)) && _MSC_VER < 1900
	printf("%s->pid(%d), ", tag, GETPID());
	vprintf(fmt, ap);
#else
	va_list ap_tmp;
	va_copy(ap_tmp, ap);
	printf("%s->pid(%d), ", tag, GETPID());
	vprintf(fmt, ap_tmp);
#endif
	printf("\r\n");
}

void net_msg_info(const char *fmt,...)
{
	va_list ap;

	va_start (ap, fmt);
	if (stdout_on) {
		fmt_printf(__FUNCTION__, fmt, ap);
	}
	va_end (ap);
}

void net_msg_warn(const char *fmt,...)
{
	va_list ap;

	va_start (ap, fmt);
	if (stdout_on) {
		fmt_printf(__FUNCTION__, fmt, ap);
	}
	va_end (ap);
}

void net_msg_error(const char *fmt,...)
{
	va_list ap;

	va_start (ap, fmt);
	if (stdout_on) {
		fmt_printf(__FUNCTION__, fmt, ap);
	}
	va_end (ap);
}

void net_msg_fatal(const char *fmt,...)
{
	va_list ap;

	va_start (ap, fmt);
	if (stdout_on) {
		fmt_printf(__FUNCTION__, fmt, ap);
	}
	va_end (ap);
	abort();
}

#endif  /* USE_PRINTF_MACRO */
