#ifndef	__NIO_ARRAY_INCLUDE_H__
#define	__NIO_ARRAY_INCLUDE_H__

#include "iterator.h"

/**
 * ��̬�������Ͷ���
 */
typedef	struct NIO_ARRAY NIO_ARRAY;
struct NIO_ARRAY{
	int     capacity;	/**< items ����ռ��С */
	int     count;		/**< items �к���Ԫ�صĸ��� */
	void    **items;	/**< ��̬���� */

	/* ��Ӽ����� */

	/* ������β����Ӷ�̬���� */
	void  (*push_back)(struct NIO_ARRAY*, void*);
	/* ������ͷ����Ӷ�̬���� */
	void  (*push_front)(struct NIO_ARRAY*, void*);
	/* ��������β����̬���� */
	void *(*pop_back)(struct NIO_ARRAY*);
	/* ��������ͷ����̬���� */
	void *(*pop_front)(struct NIO_ARRAY*);

	/* for iterator */

	/* ȡ������ͷ���� */
	void *(*iter_head)(ITER*, struct NIO_ARRAY*);
	/* ȡ��������һ������ */
	void *(*iter_next)(ITER*, struct NIO_ARRAY*);
	/* ȡ������β���� */
	void *(*iter_tail)(ITER*, struct NIO_ARRAY*);
	/* ȡ��������һ������ */
	void *(*iter_prev)(ITER*, struct NIO_ARRAY*);
};

/**
 * ����һ����̬����
 * @param init_size {int} ��̬����ĳ�ʼ��С
 * @return {NIO_ARRAY*} ��̬����ָ��
 */
NIO_ARRAY *nio_array_create(int init_size);

/**
 * �ͷŵ���̬�����ڵĳ�Ա�������������ͷŶ�̬�������
 * @param a {NIO_ARRAY*} ��̬����ָ��
 * @param free_fn {void (*)(void*)} �����ͷŶ�̬�����ڳ�Ա�������ͷź���ָ��
 */
void nio_array_clean(NIO_ARRAY *a, void (*free_fn)(void *));

/**
 * �ͷŵ���̬�����ڵĳ�Ա���������ͷŶ�̬������󣬵�������󴴽� dbuf ����
 * ʱ��������������ͷŽ������ͷ� dbuf ʱ���ͷ�
 * @param a {NIO_ARRAY*} ��̬����ָ��
 * @param free_fn {void (*)(void*)} �����ͷŶ�̬�����ڳ�Ա�������ͷź���ָ��
 */
void nio_array_free(NIO_ARRAY *a, void (*free_fn)(void *));
#define nio_array_destroy nio_array_free

/**
 * ��̬����β����Ӷ�̬��Ա����
 * @param a {NIO_ARRAY*} ��̬����ָ��
 * @param obj {void*} ��̬��Ա����
 * @return {int} >=0: �ɹ�, ����ֵΪ��Ԫ���������е��±�λ�ã�-1: ʧ��
 */
int nio_array_append(NIO_ARRAY *a, void *obj);

/**
 * ��̬����ͷ����Ӷ�̬��Ա����
 * @param a {NIO_ARRAY*} ��̬����ָ��
 * @param obj {void*} ��̬��Ա����
 * @return {int} >=0: �ɹ�, ����ֵΪ��Ԫ���������е��±�λ�ã�-1: ʧ��
 */
int nio_array_prepend(NIO_ARRAY *a, void *obj);

/**
 * ��̬������ָ��λ��ǰ��Ӷ�̬��Ա����(�ý�㼰�Ժ����н�㶼����һ��λ��)
 * @param a {NIO_ARRAY*} ��̬����ָ��
 * @param position {int} ĳ��λ�ã�����Խ��
 * @param obj {void*} ��̬��Ա����
 * @return {int} 0: �ɹ���-1: ʧ��
 */
int nio_array_pred_insert(NIO_ARRAY *a, int position, void *obj);

/**
 * ��̬������ָ��λ�ú���Ӷ�̬��Ա����(�ý���Ժ����н�㶼����һ��λ��)
 * @param a {NIO_ARRAY*} ��̬����ָ��
 * @param position {int} ĳ��λ�ã�����Խ��
 * @param obj {void*} ��̬��Ա����
 * @return {int} 0: �ɹ���-1: ʧ��
 */
int nio_array_succ_insert(NIO_ARRAY *a, int position, void *obj);
#define nio_array_insert nio_array_succ_insert

/**
 * �Ӷ�̬�����е�ָ��λ��ɾ��ĳ����̬����, ɾ����������Ԫ�ص��Ⱥ�˳�򱣳ֲ���,
 * �����ɾ��λ�����м�ĳ��λ�ã�Ϊ�˱�֤Ԫ�ص�˳���ԣ��ڲ�����ɾ��Ԫ�غ������Ԫ��
 * ��ǰ��һ��λ��
 * @param a {NIO_ARRAY*} ��̬����ָ��
 * @param position {int} ĳ��λ�ã�����Խ��
 * @param free_fn {void (*)(void*)} �����ͷŶ�̬�����ڳ�Ա�������ͷź���ָ�룬�����
 *  ָ��Ϊ�գ����ͷţ������ô˺��������ͷŶ�̬����
 * @return {int} 0: �ɹ���-1: ʧ��
 */
int nio_array_delete_idx(NIO_ARRAY *a, int position, void (*free_fn)(void *));

/**
 * �Ӷ�̬�����е�ָ��λ��ɾ��ĳ������ɾ����������Ԫ�ص��Ⱥ�˳���п��ܷ����˸ı�,
 * ��Ϊɾ������Զ�������������Ԫ��������λ�ô�
 * @param a {NIO_ARRAY*} ��̬����ָ��
 * @param position {int} ĳ��λ�ã�����Խ��
 * @param free_fn {void (*)(void*)} �����ͷŶ�̬�����ڳ�Ա�������ͷź���ָ�룬�����
 *  ָ��Ϊ�գ����ͷţ������ô˺��������ͷŶ�̬����
 * @return {int} 0: �ɹ���-1: ʧ��
 */
int nio_array_delete(NIO_ARRAY *a, int position, void (*free_fn)(void*));

/**
 * �Ӷ�̬������ɾ��ָ��ָ���ַ�Ķ�̬����, ɾ����������Ԫ�ص��Ⱥ�˳�򱣳ֲ���
 * �����ɾ��λ�����м�ĳ��λ�ã�Ϊ�˱�֤Ԫ�ص�˳�����ڲ�������ɾ��Ԫ�غ������Ԫ��
 * ��ǰ��һ��λ��
 * @param a {NIO_ARRAY*} ��̬����ָ��
 * @param obj {void*} ��̬����ָ���ַ
 * @param free_fn {void (*)(void*)} �����ͷŶ�̬�����ڳ�Ա�������ͷź���ָ�룬�����
 *  ָ��Ϊ�գ����ͷţ������ô˺��������ͷŶ�̬����
 * @return {int} 0: �ɹ���-1: ʧ��
 */
int nio_array_delete_obj(NIO_ARRAY *a, void *obj, void (*free_fn)(void *));

/**
 * �Ӷ�̬������ɾ��ĳ���±귶Χ�Ķ�̬����
 * @param a {NIO_ARRAY*} ��̬����ָ��
 * @param ibegin {int} ��ʼ�±�λ��
 * @param iend {int} �����±�λ��
 * @param free_fn {void (*)(void*)} �����ͷŶ�̬�����ڳ�Ա�������ͷź���ָ�룬�����
 *  ָ��Ϊ�գ����ͷţ������ô˺��������ͷŶ�̬����
 * @return {int} 0: �ɹ���-1: ʧ��
 */
int nio_array_delete_range(NIO_ARRAY *a, int ibegin, int iend, void (*free_fn)(void*));

/**
 * �ƶ���̬�����еĶ���
 * @param a {NIO_ARRAY*} ��̬����ָ��
 * @param ito {int} �ƶ���Ŀ���±�λ��
 * @param ifrom {int} �Ӵ��±�λ�ÿ�ʼ�ƶ�
 * @param free_fn {void (*)(void*)} �����ͷŶ�̬�����ڳ�Ա�������ͷź���ָ�룬�����
 *  ָ��Ϊ�գ����ͷţ������ô˺��������ͷŶ�̬�����ͷŵĶ�̬��������Ϊ
 *  [idx_obj_begin, idx_src_begin), Ϊһ�뿪�������
 * @return {int} 0: �ɹ���-1: ʧ��
 */
int nio_array_mv_idx(NIO_ARRAY *a, int ito, int ifrom, void (*free_fn)(void *) );

/**
 * Ԥ�ȱ�֤��̬����Ŀռ䳤��
 * @param a {NIO_ARRAY*} ��̬����ָ��
 * @param app_count {int} ��Ҫ��̬���������� app_count ������λ��
 */
void nio_array_pre_append(NIO_ARRAY *a, int app_count);

/**
 * �Ӷ�̬�����е�ĳ���±�λ��ȡ����̬����
 * @param a {NIO_ARRAY*} ��̬����ָ��
 * @param idx {int} �±�λ�ã�����Խ�磬���򷵻�-1
 * @return {void*} != NULL: �ɹ���== NULL: �����ڻ�ʧ��
 */
void *nio_array_index(const NIO_ARRAY *a, int idx);

/**
 * ��õ�ǰ��̬�����ж�̬����ĸ���
 * @param a {NIO_ARRAY*} ��̬����ָ��
 * @return {int} ��̬�����ж�̬����ĸ���
 */
int nio_array_size(const NIO_ARRAY *a);

#endif

