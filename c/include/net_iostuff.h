#ifndef	IOSTUFF_INCLUDE_H
#define	IOSTUFF_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

/**
 * 设定当前进程可以打开最大文件描述符值
 * @param limit {int} 设定的最大值
 * @return {int} >=0: ok; -1: error
 */
int net_open_limit(int limit);

int net_non_blocking(socket_t fd, int on);

void net_tcp_nodelay(socket_t fd, int onoff);

#ifdef	__cplusplus
}
#endif

#endif
