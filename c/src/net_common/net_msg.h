#ifndef _MSG_INCLUDE_H_
#define _MSG_INCLUDE_H_

#include <stdarg.h>
#include <stdio.h>
#include "net_define.h"

void net_msg_stdout(int on);

#undef	USE_PRINTF_MACRO

int net_last_error(void);

/**
 * 当记录日志信息至日志文件时，需要调用如下的日志记录函数
 */

#ifndef	USE_PRINTF_MACRO

/**
 * 一般级别日志信息记录函数
 * @param fmt {const char*} 参数格式
 * @param ... 变参序列
 */
void PRINTF(1, 2) net_msg_info(const char *fmt,...);

/**
 * 警告级别日志信息记录函数
 * @param fmt {const char*} 参数格式
 * @param ... 变参序列
 */
void PRINTF(1, 2) net_msg_warn(const char *fmt,...);

/**
 * 错误级别日志信息记录函数
 * @param fmt {const char*} 参数格式
 * @param ... 变参序列
 */
void PRINTF(1, 2) net_msg_error(const char *fmt,...);

/**
 * 致命级别日志信息记录函数
 * @param fmt {const char*} 参数格式
 * @param ... 变参序列
 */
void PRINTF(1, 2) net_msg_fatal(const char *fmt,...);

#else

/**
 * 当记录日志信息至标准输出时，需要调用如下的日志记录函数
 */

#include <stdio.h>

#undef	net_msg_info
#undef	net_msg_warn
#undef	net_msg_error
#undef	net_msg_fatal

#define	net_msg_info	net_msg_printf
#define	net_msg_warn	net_msg_printf
#define	net_msg_error	net_msg_printf
#define	net_msg_fatal	net_msg_printf

#endif

/**
 * 输出信息至标准输出
 * @param fmt {const char*} 格式参数
 * @param ... 变参序列
 */
void PRINTF(1, 2) net_msg_printf(const char *fmt,...);

#endif
