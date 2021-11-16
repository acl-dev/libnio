#ifndef	IOSTUFF_INCLUDE_H
#define	IOSTUFF_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

/**
 * �趨��ǰ���̿��Դ�����ļ�������ֵ
 * @param limit {int} �趨�����ֵ
 * @return {int} >=0: ok; -1: error
 */
int open_limit(int limit);

int non_blocking(socket_t fd, int on);

#ifdef	__cplusplus
}
#endif

#endif
