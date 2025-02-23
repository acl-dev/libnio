#ifndef	NIO_RING_INCLUDE_H
#define	NIO_RING_INCLUDE_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <stddef.h>

typedef struct NIO_RING NIO_RING;

/**
 * ���ݻ��ṹ���Ͷ���
 */
struct NIO_RING {
	NIO_RING   *succ;           /**< successor */
	NIO_RING   *pred;           /**< predecessor */

	NIO_RING   *parent;         /**< the header of all the rings */
	int         len;            /**< the count in the ring */
};

typedef struct NIO_RING_ITER {
	NIO_RING *ptr;
} NIO_RING_ITER;

/**
 * ��ʼ�����ݻ�
 * @param ring {NIO_RING*} ���ݻ�
 */
#ifdef USE_FAST_NIO_RING
#define nio_ring_init(__ring) do { \
	NIO_RING *_ring   = __ring; \
	_ring->pred   = _ring->succ = _ring; \
	_ring->parent = _ring; \
	_ring->len    = 0; \
} while (0)
#else
void nio_ring_init(NIO_RING *ring);
#endif

/**
 * ��õ�ǰ���ݻ���Ԫ�ظ���
 * @param ring {NIO_RING*} ���ݻ�
 * @return {int} ���ݻ���Ԫ�ظ���
 */
#ifdef USE_FAST_NIO_RING
#define nio_ring_size(r) (((NIO_RING*)(r))->len)
#else
int  nio_ring_size(const NIO_RING *ring);
#endif

/**
 * ��һ����Ԫ����ӽ�����β��
 * @param ring {NIO_RING*} ���ݻ�
 * @param entry {NIO_RING*} �µ�Ԫ��
 */
#ifdef USE_FAST_NIO_RING
#define nio_ring_append(r, e) do { \
	((NIO_RING*)(e))->succ       = ((NIO_RING*)(r))->succ; \
	((NIO_RING*)(e))->pred       = (NIO_RING*)(r); \
	((NIO_RING*)(e))->parent     = ((NIO_RING*)(r))->parent; \
	((NIO_RING*)(r))->succ->pred = (NIO_RING*)(e); \
	((NIO_RING*)(r))->succ       = (NIO_RING*)(e); \
	((NIO_RING*)(r))->parent->len++; \
} while (0)

#else
void nio_ring_append(NIO_RING *ring, NIO_RING *entry);
#endif

/**
 * ��һ����Ԫ����ӽ�����ͷ��
 * @param ring {NIO_RING*} ���ݻ�
 * @param entry {NIO_RING*} �µ�Ԫ��
 */
#ifdef USE_FAST_NIO_RING
#define nio_ring_prepend(r, e) do { \
	((NIO_RING*)(e))->pred       = ((NIO_RING*)(r))->pred; \
	((NIO_RING*)(e))->succ       = (NIO_RING*)(r); \
	((NIO_RING*)(e))->parent     = ((NIO_RING*)(r))->parent; \
	((NIO_RING*)(r))->pred->succ = (NIO_RING*)(e); \
	((NIO_RING*)(r))->pred       = (NIO_RING*)(e); \
	((NIO_RING*)(r))->parent->len++; \
} while (0)
#else
void nio_ring_prepend(NIO_RING *ring, NIO_RING *entry);
#endif

/**
 * ��һ����Ԫ�ش����ݻ���ɾ��
 * @param entry {NIO_RING*} ��Ԫ��
 */
#ifdef USE_FAST_NIO_RING
#define nio_ring_detach(e) do { \
	NIO_RING *_succ, *_pred; \
	if (((NIO_RING*)(e))->parent != (NIO_RING*)(e)) { \
		_succ = ((NIO_RING*)(e))->succ; \
		_pred = ((NIO_RING*)(e))->pred; \
		if (_succ && _pred) { \
			_pred->succ = _succ; \
			_succ->pred = _pred; \
			((NIO_RING*)(e))->parent->len--; \
			((NIO_RING*)(e))->succ   = (NIO_RING*)(e); \
			((NIO_RING*)(e))->pred   = (NIO_RING*)(e); \
			((NIO_RING*)(e))->parent = (NIO_RING*)(e); \
			((NIO_RING*)(e))->len    = 0; \
		} \
	} \
} while (0)
#else
void nio_ring_detach(NIO_RING *entry);
#endif

/**
 * �ӻ��е���ͷ����Ԫ��
 * @param ring {NIO_RING*} ���ݻ�
 * @return {NIO_RING*} ͷ����Ԫ�أ�������ؿ����ʾ�����ݻ�Ϊ��
 */
#ifdef USE_FAST_NIO_RING
static inline NIO_RING *nio_ring_pop_head(NIO_RING *ring) {
	NIO_RING *succ;

	succ = ring->succ;
	if (succ == ring) {
		return NULL;
	}

	nio_ring_detach(succ);
	return succ;
}
#else
NIO_RING *nio_ring_pop_head(NIO_RING *ring);
#endif

/**
 * �ӻ��е���β����Ԫ��
 * @param ring {NIO_RING*} ���ݻ�
 * @return {NIO_RING*} β����Ԫ�أ�������ؿ����ʾ�����ݻ�Ϊ��
 */
#ifdef USE_FAST_NIO_RING
static inline NIO_RING *nio_ring_pop_tail(NIO_RING *ring) {
	NIO_RING *pred;

	pred = ring->pred;
	if (pred == ring) {
		return NULL;
	}

	nio_ring_detach(pred);
	return pred;
}
#else
NIO_RING *nio_ring_pop_tail(NIO_RING *ring);
#endif

/*--------------------  һЩ�����ݵĺ���� --------------------------------*/

/**
 * ���ص�ǰ��Ԫ�ص���һ����Ԫ��
 */
#define NIO_RING_SUCC(c) ((c)->succ)
#define	nio_ring_succ	NIO_RING_SUCC

/**
 * ���ص�ǰ��Ԫ�ص�ǰһ����Ԫ��
 */
#define NIO_RING_PRED(c) ((c)->pred)
#define	nio_ring_pred	NIO_RING_PRED

/**
 * ����Ԫ��ָ��ת����Ӧ�õ��Զ������͵�ָ���ַ
 * @param ring_ptr {NIO_RING*} ��Ԫ��ָ��
 * @param app_type Ӧ���Զ�������
 * @param ring_member {NIO_RING*} ��Ԫ����Ӧ���Զ���ṹ�еĳ�Ա����
 * @return {app_type*} Ӧ���Զ���ṹ���͵Ķ����ַ
 */
#define NIO_RING_TO_APPL(ring_ptr, app_type, ring_member) \
    ((app_type *) (((char *) (ring_ptr)) - offsetof(app_type,ring_member)))

#define	nio_ring_to_appl	NIO_RING_TO_APPL

/**
 * ��ͷ����β���������ݻ��е����л�Ԫ��
 * @param iter {RING_ITER}
 * @param head_ptr {NIO_RING*} ���ݻ���ͷָ��
 * @example:
 	typedef struct {
		char  name[32];
		NIO_RING entry;
	} DUMMY;

	void test()
	{
		NIO_RING head;
		DUMMY *dummy;
		RING_ITER iter;
		int   i;

		ring_init(&head);

		for (i = 0; i < 10; i++) {
			dummy = (DUMMY*) mycalloc(1, sizeof(DUMMY));
			snprintf(dummy->name, sizeof(dummy->name), "dummy:%d", i);
			nio_ring_append(&head, &dummy->entry);
		}

		nio_ring_foreach(iter, &head) {
			dummy = nio_ring_to_appl(iter.ptr, DUMMY, entry);
			printf("name: %s\n", dummy->name);
		}

		while (1) {
			iter.ptr = nio_ring_pop_head(&head);
			if (iter.ptr == NULL)
				break;
			dummy = nio_ring_to_appl(iter.ptr, DUMMY, entry);
			myfree(dummy);
		}
	}
 */
#define	NIO_RING_FOREACH(iter, head_ptr) \
        for ((iter).ptr = nio_ring_succ((head_ptr)); (iter).ptr != (head_ptr);  \
             (iter).ptr = nio_ring_succ((iter).ptr))

#define	nio_ring_foreach	NIO_RING_FOREACH

/**
 * ��β����ͷ���������ݻ��е����л�Ԫ��
 * @param iter {RING_ITER}
 * @param head_ptr {NIO_RING*} ���ݻ���ͷָ��
 */
#define	NIO_RING_FOREACH_REVERSE(iter, head_ptr) \
        for ((iter).ptr = nio_ring_pred((head_ptr)); (iter).ptr != (head_ptr);  \
             (iter).ptr = nio_ring_pred((iter).ptr))

#define	nio_ring_foreach_reverse	NIO_RING_FOREACH_REVERSE

/**
 * �������ݻ��е�һ����Ԫ��ָ��
 * @param head {NIO_RING*} ��ͷָ��
 * @return {NIO_RING*} NULL: ��Ϊ��
 */
#define NIO_RING_FIRST(head) \
	(nio_ring_succ(head) != (head) ? nio_ring_succ(head) : 0)

#define	nio_ring_first		NIO_RING_FIRST

/**
 * �������ݻ���ͷ��һ����Ԫ��ָ��ͬʱ����ת��Ӧ���Զ���ṹ���͵Ķ����ַ
 * @param head {NIO_RING*} ��ͷָ��
 * @param app_type Ӧ���Զ���ṹ����
 * @param ring_member {NIO_RING*} ��Ԫ����Ӧ���Զ���ṹ�еĳ�Ա����
 * @return {app_type*} Ӧ���Զ���ṹ���͵Ķ����ַ
 */
#define NIO_RING_FIRST_APPL(head, app_type, ring_member) \
	(nio_ring_succ(head) != (head) ? \
	 NIO_RING_TO_APPL(nio_ring_succ(head), app_type, ring_member) : 0)

#define	nio_ring_first_appl	NIO_RING_FIRST_APPL

/**
 * �������ݻ������һ����Ԫ��ָ��
 * @param head {NIO_RING*} ��ͷָ��
 * @return {NIO_RING*} NULL: ��Ϊ��
 */
#define NIO_RING_LAST(head) \
       (nio_ring_pred(head) != (head) ? nio_ring_pred(head) : 0)

#define	nio_ring_last		NIO_RING_LAST

/**
 * �������ݻ������һ����Ԫ��ָ��ͬʱ����ת��Ӧ���Զ���ṹ���͵Ķ����ַ
 * @param head {NIO_RING*} ��ͷָ��
 * @param app_type Ӧ���Զ���ṹ����
 * @param ring_member {NIO_RING*} ��Ԫ����Ӧ���Զ���ṹ�еĳ�Ա����
 * @return {app_type*} Ӧ���Զ���ṹ���͵Ķ����ַ
 */
#define NIO_RING_LAST_APPL(head, app_type, ring_member) \
       (nio_ring_pred(head) != (head) ? \
	NIO_RING_TO_APPL(nio_ring_pred(head), app_type, ring_member) : 0)

#define	nio_ring_last_appl	NIO_RING_LAST_APPL

/**
 * ��һ����Ԫ����ӽ�����β��
 * @param ring {NIO_RING*} ���ݻ�
 * @param entry {NIO_RING*} �µ�Ԫ��
 */
#define	NIO_RING_APPEND(ring_in, entry_in) do {  \
	NIO_RING *ring_ptr = (ring_in), *entry_ptr = (entry_in);  \
        entry_ptr->succ      = ring_ptr->succ;  \
        entry_ptr->pred      = ring_ptr;  \
        entry_ptr->parent    = ring_ptr->parent;  \
        ring_ptr->succ->pred = entry_ptr;  \
        ring_ptr->succ       = entry_ptr;  \
        ring_ptr->parent->len++;  \
} while (0)

/**
 * ��һ����Ԫ����ӽ�����ͷ��
 * @param ring {NIO_RING*} ���ݻ�
 * @param entry {NIO_RING*} �µ�Ԫ��
 */
#define	NIO_RING_PREPEND(ring_in, entry_in) do {  \
	NIO_RING *ring_ptr = (ring_in), *entry_ptr = (entry_in);  \
	entry_ptr->pred      = ring_ptr->pred;  \
	entry_ptr->succ      = ring_ptr;  \
	entry_ptr->parent    = ring_ptr->parent;  \
	ring_ptr->pred->succ = entry_ptr;  \
	ring_ptr->pred       = entry_ptr;  \
	ring_ptr->parent->len++;  \
} while (0)

/**
 * ��һ����Ԫ�ش����ݻ���ɾ��
 * @param entry {NIO_RING*} ��Ԫ��
 */
#define	NIO_RING_DETACH(entry_in) do {  \
	NIO_RING   *succ, *pred, *entry_ptr = (entry_in);  \
	succ = entry_ptr->succ;  \
	pred = entry_ptr->pred;  \
	if (succ != NULL && pred != NULL) {  \
		pred->succ = succ;  \
		succ->pred = pred;  \
		entry_ptr->parent->len--;  \
		entry_ptr->succ = entry_ptr->pred = NULL;  \
	}  \
} while (0)

#ifdef  __cplusplus
}
#endif

#endif

