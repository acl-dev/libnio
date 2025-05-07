#include "stdafx.h"
#include "nio_msg.h"
#include "nio/nio_event.h"

static int stdout_on = 0;
static NIO_MSG_WRITE_FN __write_fn = NULL;
static void *__msg_ctx = NULL;

void nio_msg_register(NIO_MSG_WRITE_FN write_fn, void *ctx) {
    if (write_fn != NULL) {
        __write_fn = write_fn;
        __msg_ctx  = ctx;
    }
}

void nio_msg_unregister(void) {
    __write_fn = NULL;
    __msg_ctx  = NULL;
}

void nio_msg_stdout(int on) {
	stdout_on = on;
}

#ifndef	USE_PRINTF_MACRO

int nio_last_error(void) {
#ifdef	SYS_WIN
	int   error;

	error = WSAGetLastError();
	WSASetLastError(error);
	return error;
#else
	return errno;
#endif
}

static void fmt_printf(const char *tag, const char *fmt, va_list ap) {
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

void nio_msg_info(const char *fmt,...) {
	va_list ap;

	va_start (ap, fmt);
	if (stdout_on) {
		fmt_printf(__FUNCTION__, fmt, ap);
	}
    if (__write_fn != NULL) {
        __write_fn(fmt, ap);
    }
	va_end (ap);
}

void nio_msg_warn(const char *fmt,...) {
	va_list ap;

	va_start (ap, fmt);
	if (stdout_on) {
		fmt_printf(__FUNCTION__, fmt, ap);
	}
    if (__write_fn != NULL) {
        __write_fn(fmt, ap);
    }
	va_end (ap);
}

void nio_msg_error(const char *fmt,...) {
	va_list ap;

	va_start (ap, fmt);
	if (stdout_on) {
		fmt_printf(__FUNCTION__, fmt, ap);
	}
    if (__write_fn != NULL) {
        __write_fn(fmt, ap);
    }
	va_end (ap);
}

void nio_msg_fatal(const char *fmt,...) {
	va_list ap;

	va_start (ap, fmt);
	if (stdout_on) {
		fmt_printf(__FUNCTION__, fmt, ap);
	}
    if (__write_fn != NULL) {
        __write_fn(fmt, ap);
    }
	va_end (ap);
	abort();
}

#endif  /* USE_PRINTF_MACRO */
