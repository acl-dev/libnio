#ifndef	NIO_IOSTUFF_INCLUDE_H
#define	NIO_IOSTUFF_INCLUDE_H

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
 * 设定当前进程可以打开最大文件描述符值
 * @param limit {int} 设定的最大值
 * @return {int} >=0: ok; -1: error
 */
int nio_open_limit(int limit);

int nio_non_blocking(socket_t fd, int on);

void nio_tcp_nodelay(socket_t fd, int onoff);

#ifdef	__cplusplus
}
#endif

#endif
