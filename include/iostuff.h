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
int open_limit(int limit);

int non_blocking(socket_t fd, int on);

#ifdef	__cplusplus
}
#endif

#endif
