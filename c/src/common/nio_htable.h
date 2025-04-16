#ifndef NIO_HTABLE_INCLUDE_H
#define NIO_HTABLE_INCLUDE_H

#include "iterator.h"

/*--------------------------------------------------------------------------*/
typedef struct NIO_HTABLE		NIO_HTABLE;
typedef struct NIO_HTABLE_INFO 	NIO_HTABLE_INFO;

/**
 * ��ϣ�����ṹ���
 */
struct NIO_HTABLE {
	int     size;                   /* length of entries array */
	int     init_size;              /* length of initial entryies array */
	int     used;                   /* number of entries in table */
	NIO_HTABLE_INFO **data;             /* entries array, auto-resized */
	int     status;                 /* the operator's status on the htable */

	/* for iterator */

	/* ȡ������ͷ���� */
	void *(*iter_head)(ITER*, struct NIO_HTABLE*);
	/* ȡ��������һ������ */
	void *(*iter_next)(ITER*, struct NIO_HTABLE*);
	/* ȡ������β���� */
	void *(*iter_tail)(ITER*, struct NIO_HTABLE*);
	/* ȡ��������һ������ */
	void *(*iter_prev)(ITER*, struct NIO_HTABLE*);
	/* ȡ�����������ĵ�ǰ������Ա�ṹ���� */
	NIO_HTABLE_INFO *(*iter_info)(ITER*, struct NIO_HTABLE*);
};

/**
 * ��ϣ����ÿһ����ϣ��Ĵ洢��Ϣ����
 */
struct NIO_HTABLE_INFO {
	char *key;
	void *value;			/**< associated value */
	unsigned hash;			/**< store the key's hash value */
	struct NIO_HTABLE_INFO *next;	/**< colliding entry */
	struct NIO_HTABLE_INFO *prev;	/**< colliding entry */
};

/**
 * ������ϣ��
 * @param size ��ϣ����
 * @return ������ϣ���ͷָ���Ϊ��(��ʱ��ʾ�������صĴ���, ��Ҫ���ڴ��������)
 */
NIO_HTABLE *nio_htable_create(int size);

/**
 * �����һ�ι�ϣ��������ϣ���״̬
 * @param table ��ϣ��ָ��
 * @return {int} ������ϣ����״̬, �μ����µ� NIO_HTABLE_STAT_XXX
 */
int nio_htable_errno(NIO_HTABLE *table);
#define	NIO_HTABLE_STAT_OK          0  /**< ״̬���� */
#define	NIO_HTABLE_STAT_INVAL       1  /**< ��Ч���� */
#define	NIO_HTABLE_STAT_DUPLEX_KEY  2  /**< �ظ��� */

/**
 * ���ù�ϣ��ĵ�ǰ״̬, error ȡֵ NIO_HTABLE_STAT_XXX
 * @param table ��ϣ��ָ��
 * @param error ���ù�ϣ��Ĵ���״̬
 */
void nio_htable_set_errno(NIO_HTABLE *table, int error);

/**
 * ����ϣ��������µ���
 * @param table ��ϣ��ָ��
 * @param key ��, �ں����ڲ��Ḵ�ƴ� key ��
 * @param value �û��Լ����ض�������(����������Ӳת������, ���Ǵ����������
 *  ���ܶ�ջ����)
 * @return ������Ĺ�ϣ�����ָ��, == NULL: ��ʾ�ڲ������ڴ����, Ϊ���صĴ���
 *  ע����������ʱ�ù�ϣ�������ڣ��򷵻��Ѿ����ڵĹ�ϣ�ʹ����Ӧ��ͨ������
 *  nio_htable_last_errno() ���鿴�Ƿ��ظ����ͬһ����ֵ(NIO_HTABLE_STAT_DUPLEX_KEY)
 */
NIO_HTABLE_INFO *nio_htable_enter(NIO_HTABLE *table, const char *key, void *value);

/**
 * �������� key ����Ѱĳһ�ض���ϣ��
 * @param table ��ϣ��ָ��
 * @param key ��
 * @return ��Ϊ��ָ��: ��ʾ�鵽�˶�Ӧ�� key ���Ĺ�ϣ��
 *         Ϊ��: ��ʾδ�鵽��Ӧ�� key ���Ĺ�ϣ��
 */
NIO_HTABLE_INFO *nio_htable_locate(NIO_HTABLE *table, const char *key);

/**
 * �������� key ����Ѱ�û���������
 * @param table ��ϣ��ָ��
 * @param key ��
 * @return ��Ϊ��: ��ʾ�鵽�˶�Ӧ�� key ����������, �û����Ը����û��Լ���
 *  �������ͽ���ת��; Ϊ��: ��ʾδ�鵽��Ӧ�� key ����������
 */
void *nio_htable_find(NIO_HTABLE *table, const char *key);

/**
 * ���������� key ��ɾ��ĳһ��ϣ��
 * @param table ��ϣ��ָ��
 * @param key ��
 * @param free_fn ����ú���ָ�벻Ϊ�ղ����ҵ��˶�Ӧ�� key ����������, ����
 *  �����û����ṩ������������һЩ��β����, Ȼ�����ͷŸù�ϣ��
 * @return 0: �ɹ�;  -1: δ�ҵ��� key ��
 */
int nio_htable_delete(NIO_HTABLE *table, const char *key, void (*free_fn) (void *));

/**
 * ֱ�Ӹ��� nio_htable_locate ���صķǿն���ӹ�ϣ����ɾ���ö���
 * @param table ��ϣ��ָ��
 * @param ht {NIO_HTABLE_INFO*} �洢�ڹ�ϣ���е��ڲ��ṹ����
 * @param free_fn ����ú���ָ�벻Ϊ�ղ����ҵ��˶�Ӧ�� key ����������, ����
 *  �����û����ṩ������������һЩ��β����, Ȼ�����ͷŸù�ϣ��
 */
void nio_htable_delete_entry(NIO_HTABLE *table, NIO_HTABLE_INFO *ht, void (*free_fn) (void *));

/**
 * �ͷ�������ϣ��
 * @param table ��ϣ��ָ��
 * @param free_fn �����ָ�벻Ϊ����Թ�ϣ���е�ÿһ���ϣ�����øú�����
 *  ��β����, Ȼ�����ͷ�
 */
void nio_htable_free(NIO_HTABLE *table, void (*free_fn) (void *));

/**
 * ���ù�ϣ��, �ú������ͷŹ�ϣ���е�����������, �����³�ʼ��
 * @param table ��ϣ��ָ��
 * @param free_fn �����ָ�벻Ϊ����Թ�ϣ���е�ÿһ���ϣ�����øú�����
 *  ��β����, Ȼ�����ͷ�
 * @return �Ƿ����óɹ�. 0: OK; < 0: error.
 */
int nio_htable_reset(NIO_HTABLE *table, void (*free_fn) (void *));

/**
 * �Թ�ϣ���е�ÿһ���ϣ����д���
 * @param table ��ϣ��ָ��
 * @param walk_fn ����ÿһ���ϣ��ĺ���ָ��, ����Ϊ��
 * @param arg �û��Լ����͵�����
 */
void nio_htable_walk(NIO_HTABLE *table, void (*walk_fn) (NIO_HTABLE_INFO *, void *), void *arg);

/**
 * ���ع�ϣ��ǰ�������ռ��С
 * @param table ��ϣ��ָ��
 * @return ��ϣ��������ռ��С
 */
int nio_htable_size(const NIO_HTABLE *table);

/**
 * ���ع�ϣ��ǰ�Ĵ���������Ԫ�ظ���
 * @param table ��ϣ��ָ��
 * @return ��ϣ����Ԫ�ظ���
 */
int nio_htable_used(const NIO_HTABLE *table);

/**
 * ����ϣ�������������ϳ�һ������
 * @param table ��ϣ��
 * @return ��Ϊ��: ����ָ��; Ϊ��: ��ʾ�ù�ϣ����û�й�ϣ��
 */
NIO_HTABLE_INFO **nio_htable_list(const NIO_HTABLE *table);

/**
 * ��ʾ��ϣ���� key ���ķֲ�״̬
 * @param table ��ϣ��ָ��
 */
void nio_htable_stat(const NIO_HTABLE *table);
#define	nio_htable_stat_r	nio_htable_stat

/*--------------------  һЩ�����ݵĺ���� --------------------------------*/

#define	NIO_HTABLE_ITER_KEY(iter)	((iter).ptr->key.c_key)
#define	nio_htable_iter_key		NIO_HTABLE_ITER_KEY

#define	NIO_HTABLE_ITER_VALUE(iter)	((iter).ptr->value)
#define	nio_htable_iter_value	NIO_HTABLE_ITER_VALUE

#endif
