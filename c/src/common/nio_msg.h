#ifndef _MSG_INCLUDE_H_
#define _MSG_INCLUDE_H_

#include <stdarg.h>
#include <stdio.h>
#include "nio_define.h"

#undef	USE_PRINTF_MACRO

/**
 * 当记录日志信息至日志文件时，需要调用如下的日志记录函数
 */

#ifndef	USE_PRINTF_MACRO

/**
 * 一般级别日志信息记录函数
 * @param fmt {const char*} 参数格式
 * @param ... 变参序列
 */
void PRINTF(1, 2) nio_msg_info(const char *fmt,...);

/**
 * 警告级别日志信息记录函数
 * @param fmt {const char*} 参数格式
 * @param ... 变参序列
 */
void PRINTF(1, 2) nio_msg_warn(const char *fmt,...);

/**
 * 错误级别日志信息记录函数
 * @param fmt {const char*} 参数格式
 * @param ... 变参序列
 */
void PRINTF(1, 2) nio_msg_error(const char *fmt,...);

/**
 * 致命级别日志信息记录函数
 * @param fmt {const char*} 参数格式
 * @param ... 变参序列
 */
void PRINTF(1, 2) nio_msg_fatal(const char *fmt,...);

#else

/**
 * 当记录日志信息至标准输出时，需要调用如下的日志记录函数
 */

#include <stdio.h>

#undef	nio_msg_info
#undef	nio_msg_warn
#undef	nio_msg_error
#undef	nio_msg_fatal

#define	nio_msg_info	nio_msg_printf
#define	nio_msg_warn	nio_msg_printf
#define	nio_msg_error	nio_msg_printf
#define	nio_msg_fatal	nio_msg_printf

#endif

/**
 * 输出信息至标准输出
 * @param fmt {const char*} 格式参数
 * @param ... 变参序列
 */
void PRINTF(1, 2) nio_msg_printf(const char *fmt,...);

#endif
