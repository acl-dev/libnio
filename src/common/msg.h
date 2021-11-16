#ifndef _MSG_INCLUDE_H_
#define _MSG_INCLUDE_H_

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <stdio.h>
#include "define.h"

#undef	USE_PRINTF_MACRO

int last_error(void);

/**
 * 当记录日志信息至日志文件时，需要调用如下的日志记录函数
 */

#ifndef	USE_PRINTF_MACRO

/**
 * 一般级别日志信息记录函数
 * @param fmt {const char*} 参数格式
 * @param ... 变参序列
 */
void PRINTF(1, 2) msg_info(const char *fmt,...);

/**
 * 警告级别日志信息记录函数
 * @param fmt {const char*} 参数格式
 * @param ... 变参序列
 */
void PRINTF(1, 2) msg_warn(const char *fmt,...);

/**
 * 错误级别日志信息记录函数
 * @param fmt {const char*} 参数格式
 * @param ... 变参序列
 */
void PRINTF(1, 2) msg_error(const char *fmt,...);

/**
 * 致命级别日志信息记录函数
 * @param fmt {const char*} 参数格式
 * @param ... 变参序列
 */
void PRINTF(1, 2) msg_fatal(const char *fmt,...);

#else

/**
 * 当记录日志信息至标准输出时，需要调用如下的日志记录函数
 */

#include <stdio.h>

#undef	msg_info
#undef	msg_warn
#undef	msg_error
#undef	msg_fatal

#define	msg_info	msg_printf
#define	msg_warn	msg_printf
#define	msg_error	msg_printf
#define	msg_fatal	msg_printf

#endif

/**
 * 输出信息至标准输出
 * @param fmt {const char*} 格式参数
 * @param ... 变参序列
 */
void PRINTF(1, 2) msg_printf(const char *fmt,...);

#ifdef  __cplusplus
}
#endif

#endif
