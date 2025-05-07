#ifndef _MSG_INCLUDE_H_
#define _MSG_INCLUDE_H_

#include <stdarg.h>
#include <stdio.h>
#include "nio_define.h"

#undef	USE_PRINTF_MACRO

/**
 * ����¼��־��Ϣ����־�ļ�ʱ����Ҫ�������µ���־��¼����
 */

#ifndef	USE_PRINTF_MACRO

/**
 * һ�㼶����־��Ϣ��¼����
 * @param fmt {const char*} ������ʽ
 * @param ... �������
 */
void PRINTF(1, 2) nio_msg_info(const char *fmt,...);

/**
 * ���漶����־��Ϣ��¼����
 * @param fmt {const char*} ������ʽ
 * @param ... �������
 */
void PRINTF(1, 2) nio_msg_warn(const char *fmt,...);

/**
 * ���󼶱���־��Ϣ��¼����
 * @param fmt {const char*} ������ʽ
 * @param ... �������
 */
void PRINTF(1, 2) nio_msg_error(const char *fmt,...);

/**
 * ����������־��Ϣ��¼����
 * @param fmt {const char*} ������ʽ
 * @param ... �������
 */
void PRINTF(1, 2) nio_msg_fatal(const char *fmt,...);

#else

/**
 * ����¼��־��Ϣ����׼���ʱ����Ҫ�������µ���־��¼����
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
 * �����Ϣ����׼���
 * @param fmt {const char*} ��ʽ����
 * @param ... �������
 */
void PRINTF(1, 2) nio_msg_printf(const char *fmt,...);

#endif
