#ifndef	IOSTUFF_INCLUDE_H
#define	IOSTUFF_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

#ifndef USE_SOCKET_T
#define USE_SOCKET_T

# if defined(_WIN32) || defined (_WIN64)
typedef SOCKET socket_t;
typedef int socklen_t;
# else
typedef int socket_t;
# endif
#endif

/**
 * �趨��ǰ���̿��Դ�����ļ�������ֵ
 * @param limit {int} �趨�����ֵ
 * @return {int} >=0: ok; -1: error
 */
int net_open_limit(int limit);

int net_non_blocking(socket_t fd, int on);

void net_tcp_nodelay(socket_t fd, int onoff);

#ifdef	__cplusplus
}
#endif

#endif
