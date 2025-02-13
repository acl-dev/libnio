#ifndef _MSG_INCLUDE_H_
#define _MSG_INCLUDE_H_

#include <stdarg.h>
#include <stdio.h>
#include "net_define.h"

void net_msg_stdout(int on);

#undef	USE_PRINTF_MACRO

int net_last_error(void);

/**
 * ����¼��־��Ϣ����־�ļ�ʱ����Ҫ�������µ���־��¼����
 */

#ifndef	USE_PRINTF_MACRO

/**
 * һ�㼶����־��Ϣ��¼����
 * @param fmt {const char*} ������ʽ
 * @param ... �������
 */
void PRINTF(1, 2) net_msg_info(const char *fmt,...);

/**
 * ���漶����־��Ϣ��¼����
 * @param fmt {const char*} ������ʽ
 * @param ... �������
 */
void PRINTF(1, 2) net_msg_warn(const char *fmt,...);

/**
 * ���󼶱���־��Ϣ��¼����
 * @param fmt {const char*} ������ʽ
 * @param ... �������
 */
void PRINTF(1, 2) net_msg_error(const char *fmt,...);

/**
 * ����������־��Ϣ��¼����
 * @param fmt {const char*} ������ʽ
 * @param ... �������
 */
void PRINTF(1, 2) net_msg_fatal(const char *fmt,...);

#else

/**
 * ����¼��־��Ϣ����׼���ʱ����Ҫ�������µ���־��¼����
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
 * �����Ϣ����׼���
 * @param fmt {const char*} ��ʽ����
 * @param ... �������
 */
void PRINTF(1, 2) net_msg_printf(const char *fmt,...);

#endif
