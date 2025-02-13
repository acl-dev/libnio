#ifndef	__NET_ARRAY_INCLUDE_H__
#define	__NET_ARRAY_INCLUDE_H__

#include "iterator.h"

/**
 * ��̬�������Ͷ���
 */
typedef	struct NET_ARRAY NET_ARRAY;
struct NET_ARRAY{
	int     capacity;	/**< items ����ռ��С */
	int     count;		/**< items �к���Ԫ�صĸ��� */
	void    **items;	/**< ��̬���� */

	/* ��Ӽ����� */

	/* ������β����Ӷ�̬���� */
	void  (*push_back)(struct NET_ARRAY*, void*);
	/* ������ͷ����Ӷ�̬���� */
	void  (*push_front)(struct NET_ARRAY*, void*);
	/* ��������β����̬���� */
	void *(*pop_back)(struct NET_ARRAY*);
	/* ��������ͷ����̬���� */
	void *(*pop_front)(struct NET_ARRAY*);

	/* for iterator */

	/* ȡ������ͷ���� */
	void *(*iter_head)(ITER*, struct NET_ARRAY*);
	/* ȡ��������һ������ */
	void *(*iter_next)(ITER*, struct NET_ARRAY*);
	/* ȡ������β���� */
	void *(*iter_tail)(ITER*, struct NET_ARRAY*);
	/* ȡ��������һ������ */
	void *(*iter_prev)(ITER*, struct NET_ARRAY*);
};

/**
 * ����һ����̬����
 * @param init_size {int} ��̬����ĳ�ʼ��С
 * @return {NET_ARRAY*} ��̬����ָ��
 */
NET_ARRAY *net_array_create(int init_size);

/**
 * �ͷŵ���̬�����ڵĳ�Ա�������������ͷŶ�̬�������
 * @param a {NET_ARRAY*} ��̬����ָ��
 * @param free_fn {void (*)(void*)} �����ͷŶ�̬�����ڳ�Ա�������ͷź���ָ��
 */
void net_array_clean(NET_ARRAY *a, void (*free_fn)(void *));

/**
 * �ͷŵ���̬�����ڵĳ�Ա���������ͷŶ�̬������󣬵�������󴴽� dbuf ����
 * ʱ��������������ͷŽ������ͷ� dbuf ʱ���ͷ�
 * @param a {NET_ARRAY*} ��̬����ָ��
 * @param free_fn {void (*)(void*)} �����ͷŶ�̬�����ڳ�Ա�������ͷź���ָ��
 */
void net_array_free(NET_ARRAY *a, void (*free_fn)(void *));
#define net_array_destroy net_array_free

/**
 * ��̬����β����Ӷ�̬��Ա����
 * @param a {NET_ARRAY*} ��̬����ָ��
 * @param obj {void*} ��̬��Ա����
 * @return {int} >=0: �ɹ�, ����ֵΪ��Ԫ���������е��±�λ�ã�-1: ʧ��
 */
int net_array_append(NET_ARRAY *a, void *obj);

/**
 * ��̬����ͷ����Ӷ�̬��Ա����
 * @param a {NET_ARRAY*} ��̬����ָ��
 * @param obj {void*} ��̬��Ա����
 * @return {int} >=0: �ɹ�, ����ֵΪ��Ԫ���������е��±�λ�ã�-1: ʧ��
 */
int net_array_prepend(NET_ARRAY *a, void *obj);

/**
 * ��̬������ָ��λ��ǰ��Ӷ�̬��Ա����(�ý�㼰�Ժ����н�㶼����һ��λ��)
 * @param a {NET_ARRAY*} ��̬����ָ��
 * @param position {int} ĳ��λ�ã�����Խ��
 * @param obj {void*} ��̬��Ա����
 * @return {int} 0: �ɹ���-1: ʧ��
 */
int net_array_pred_insert(NET_ARRAY *a, int position, void *obj);

/**
 * ��̬������ָ��λ�ú���Ӷ�̬��Ա����(�ý���Ժ����н�㶼����һ��λ��)
 * @param a {NET_ARRAY*} ��̬����ָ��
 * @param position {int} ĳ��λ�ã�����Խ��
 * @param obj {void*} ��̬��Ա����
 * @return {int} 0: �ɹ���-1: ʧ��
 */
int net_array_succ_insert(NET_ARRAY *a, int position, void *obj);
#define net_array_insert net_array_succ_insert

/**
 * �Ӷ�̬�����е�ָ��λ��ɾ��ĳ����̬����, ɾ����������Ԫ�ص��Ⱥ�˳�򱣳ֲ���,
 * �����ɾ��λ�����м�ĳ��λ�ã�Ϊ�˱�֤Ԫ�ص�˳���ԣ��ڲ�����ɾ��Ԫ�غ������Ԫ��
 * ��ǰ��һ��λ��
 * @param a {NET_ARRAY*} ��̬����ָ��
 * @param position {int} ĳ��λ�ã�����Խ��
 * @param free_fn {void (*)(void*)} �����ͷŶ�̬�����ڳ�Ա�������ͷź���ָ�룬�����
 *  ָ��Ϊ�գ����ͷţ������ô˺��������ͷŶ�̬����
 * @return {int} 0: �ɹ���-1: ʧ��
 */
int net_array_delete_idx(NET_ARRAY *a, int position, void (*free_fn)(void *));

/**
 * �Ӷ�̬�����е�ָ��λ��ɾ��ĳ������ɾ����������Ԫ�ص��Ⱥ�˳���п��ܷ����˸ı�,
 * ��Ϊɾ������Զ�������������Ԫ��������λ�ô�
 * @param a {NET_ARRAY*} ��̬����ָ��
 * @param position {int} ĳ��λ�ã�����Խ��
 * @param free_fn {void (*)(void*)} �����ͷŶ�̬�����ڳ�Ա�������ͷź���ָ�룬�����
 *  ָ��Ϊ�գ����ͷţ������ô˺��������ͷŶ�̬����
 * @return {int} 0: �ɹ���-1: ʧ��
 */
int net_array_delete(NET_ARRAY *a, int position, void (*free_fn)(void*));

/**
 * �Ӷ�̬������ɾ��ָ��ָ���ַ�Ķ�̬����, ɾ����������Ԫ�ص��Ⱥ�˳�򱣳ֲ���
 * �����ɾ��λ�����м�ĳ��λ�ã�Ϊ�˱�֤Ԫ�ص�˳�����ڲ�������ɾ��Ԫ�غ������Ԫ��
 * ��ǰ��һ��λ��
 * @param a {NET_ARRAY*} ��̬����ָ��
 * @param obj {void*} ��̬����ָ���ַ
 * @param free_fn {void (*)(void*)} �����ͷŶ�̬�����ڳ�Ա�������ͷź���ָ�룬�����
 *  ָ��Ϊ�գ����ͷţ������ô˺��������ͷŶ�̬����
 * @return {int} 0: �ɹ���-1: ʧ��
 */
int net_array_delete_obj(NET_ARRAY *a, void *obj, void (*free_fn)(void *));

/**
 * �Ӷ�̬������ɾ��ĳ���±귶Χ�Ķ�̬����
 * @param a {NET_ARRAY*} ��̬����ָ��
 * @param ibegin {int} ��ʼ�±�λ��
 * @param iend {int} �����±�λ��
 * @param free_fn {void (*)(void*)} �����ͷŶ�̬�����ڳ�Ա�������ͷź���ָ�룬�����
 *  ָ��Ϊ�գ����ͷţ������ô˺��������ͷŶ�̬����
 * @return {int} 0: �ɹ���-1: ʧ��
 */
int net_array_delete_range(NET_ARRAY *a, int ibegin, int iend, void (*free_fn)(void*));

/**
 * �ƶ���̬�����еĶ���
 * @param a {NET_ARRAY*} ��̬����ָ��
 * @param ito {int} �ƶ���Ŀ���±�λ��
 * @param ifrom {int} �Ӵ��±�λ�ÿ�ʼ�ƶ�
 * @param free_fn {void (*)(void*)} �����ͷŶ�̬�����ڳ�Ա�������ͷź���ָ�룬�����
 *  ָ��Ϊ�գ����ͷţ������ô˺��������ͷŶ�̬�����ͷŵĶ�̬��������Ϊ
 *  [idx_obj_begin, idx_src_begin), Ϊһ�뿪�������
 * @return {int} 0: �ɹ���-1: ʧ��
 */
int net_array_mv_idx(NET_ARRAY *a, int ito, int ifrom, void (*free_fn)(void *) );

/**
 * Ԥ�ȱ�֤��̬����Ŀռ䳤��
 * @param a {NET_ARRAY*} ��̬����ָ��
 * @param app_count {int} ��Ҫ��̬���������� app_count ������λ��
 */
void net_array_pre_append(NET_ARRAY *a, int app_count);

/**
 * �Ӷ�̬�����е�ĳ���±�λ��ȡ����̬����
 * @param a {NET_ARRAY*} ��̬����ָ��
 * @param idx {int} �±�λ�ã�����Խ�磬���򷵻�-1
 * @return {void*} != NULL: �ɹ���== NULL: �����ڻ�ʧ��
 */
void *net_array_index(const NET_ARRAY *a, int idx);

/**
 * ��õ�ǰ��̬�����ж�̬����ĸ���
 * @param a {NET_ARRAY*} ��̬����ָ��
 * @return {int} ��̬�����ж�̬����ĸ���
 */
int net_array_size(const NET_ARRAY *a);

#endif

