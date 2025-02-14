#ifndef	NET_RING_INCLUDE_H
#define	NET_RING_INCLUDE_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <stddef.h>

typedef struct NET_RING NET_RING;

/**
 * ���ݻ��ṹ���Ͷ���
 */
struct NET_RING {
	NET_RING   *succ;           /**< successor */
	NET_RING   *pred;           /**< predecessor */

	NET_RING   *parent;         /**< the header of all the rings */
	int         len;            /**< the count in the ring */
};

typedef struct NET_RING_ITER {
	NET_RING *ptr;
} NET_RING_ITER;

/**
 * ��ʼ�����ݻ�
 * @param ring {NET_RING*} ���ݻ�
 */
#ifdef USE_FAST_NET_RING
#define net_ring_init(__ring) do { \
	NET_RING *_ring   = __ring; \
	_ring->pred   = _ring->succ = _ring; \
	_ring->parent = _ring; \
	_ring->len    = 0; \
} while (0)
#else
void net_ring_init(NET_RING *ring);
#endif

/**
 * ��õ�ǰ���ݻ���Ԫ�ظ���
 * @param ring {NET_RING*} ���ݻ�
 * @return {int} ���ݻ���Ԫ�ظ���
 */
#ifdef USE_FAST_NET_RING
#define net_ring_size(r) (((NET_RING*)(r))->len)
#else
int  net_ring_size(const NET_RING *ring);
#endif

/**
 * ��һ����Ԫ����ӽ�����β��
 * @param ring {NET_RING*} ���ݻ�
 * @param entry {NET_RING*} �µ�Ԫ��
 */
#ifdef USE_FAST_NET_RING
#define net_ring_append(r, e) do { \
	((NET_RING*)(e))->succ       = ((NET_RING*)(r))->succ; \
	((NET_RING*)(e))->pred       = (NET_RING*)(r); \
	((NET_RING*)(e))->parent     = ((NET_RING*)(r))->parent; \
	((NET_RING*)(r))->succ->pred = (NET_RING*)(e); \
	((NET_RING*)(r))->succ       = (NET_RING*)(e); \
	((NET_RING*)(r))->parent->len++; \
} while (0)

#else
void net_ring_append(NET_RING *ring, NET_RING *entry);
#endif

/**
 * ��һ����Ԫ����ӽ�����ͷ��
 * @param ring {NET_RING*} ���ݻ�
 * @param entry {NET_RING*} �µ�Ԫ��
 */
#ifdef USE_FAST_NET_RING
#define net_ring_prepend(r, e) do { \
	((NET_RING*)(e))->pred       = ((NET_RING*)(r))->pred; \
	((NET_RING*)(e))->succ       = (NET_RING*)(r); \
	((NET_RING*)(e))->parent     = ((NET_RING*)(r))->parent; \
	((NET_RING*)(r))->pred->succ = (NET_RING*)(e); \
	((NET_RING*)(r))->pred       = (NET_RING*)(e); \
	((NET_RING*)(r))->parent->len++; \
} while (0)
#else
void net_ring_prepend(NET_RING *ring, NET_RING *entry);
#endif

/**
 * ��һ����Ԫ�ش����ݻ���ɾ��
 * @param entry {NET_RING*} ��Ԫ��
 */
#ifdef USE_FAST_NET_RING
#define net_ring_detach(e) do { \
	NET_RING *_succ, *_pred; \
	if (((NET_RING*)(e))->parent != (NET_RING*)(e)) { \
		_succ = ((NET_RING*)(e))->succ; \
		_pred = ((NET_RING*)(e))->pred; \
		if (_succ && _pred) { \
			_pred->succ = _succ; \
			_succ->pred = _pred; \
			((NET_RING*)(e))->parent->len--; \
			((NET_RING*)(e))->succ   = (NET_RING*)(e); \
			((NET_RING*)(e))->pred   = (NET_RING*)(e); \
			((NET_RING*)(e))->parent = (NET_RING*)(e); \
			((NET_RING*)(e))->len    = 0; \
		} \
	} \
} while (0)
#else
void net_ring_detach(NET_RING *entry);
#endif

/**
 * �ӻ��е���ͷ����Ԫ��
 * @param ring {NET_RING*} ���ݻ�
 * @return {NET_RING*} ͷ����Ԫ�أ�������ؿ����ʾ�����ݻ�Ϊ��
 */
#ifdef USE_FAST_NET_RING
static inline NET_RING *net_ring_pop_head(NET_RING *ring) {
	NET_RING *succ;

	succ = ring->succ;
	if (succ == ring) {
		return NULL;
	}

	net_ring_detach(succ);
	return succ;
}
#else
NET_RING *net_ring_pop_head(NET_RING *ring);
#endif

/**
 * �ӻ��е���β����Ԫ��
 * @param ring {NET_RING*} ���ݻ�
 * @return {NET_RING*} β����Ԫ�أ�������ؿ����ʾ�����ݻ�Ϊ��
 */
#ifdef USE_FAST_NET_RING
static inline NET_RING *net_ring_pop_tail(NET_RING *ring) {
	NET_RING *pred;

	pred = ring->pred;
	if (pred == ring) {
		return NULL;
	}

	net_ring_detach(pred);
	return pred;
}
#else
NET_RING *net_ring_pop_tail(NET_RING *ring);
#endif

/*--------------------  һЩ�����ݵĺ���� --------------------------------*/

/**
 * ���ص�ǰ��Ԫ�ص���һ����Ԫ��
 */
#define NET_RING_SUCC(c) ((c)->succ)
#define	net_ring_succ	NET_RING_SUCC

/**
 * ���ص�ǰ��Ԫ�ص�ǰһ����Ԫ��
 */
#define NET_RING_PRED(c) ((c)->pred)
#define	net_ring_pred	NET_RING_PRED

/**
 * ����Ԫ��ָ��ת����Ӧ�õ��Զ������͵�ָ���ַ
 * @param ring_ptr {NET_RING*} ��Ԫ��ָ��
 * @param app_type Ӧ���Զ�������
 * @param ring_member {NET_RING*} ��Ԫ����Ӧ���Զ���ṹ�еĳ�Ա����
 * @return {app_type*} Ӧ���Զ���ṹ���͵Ķ����ַ
 */
#define NET_RING_TO_APPL(ring_ptr, app_type, ring_member) \
    ((app_type *) (((char *) (ring_ptr)) - offsetof(app_type,ring_member)))

#define	net_ring_to_appl	NET_RING_TO_APPL

/**
 * ��ͷ����β���������ݻ��е����л�Ԫ��
 * @param iter {RING_ITER}
 * @param head_ptr {NET_RING*} ���ݻ���ͷָ��
 * @example:
 	typedef struct {
		char  name[32];
		NET_RING entry;
	} DUMMY;

	void test()
	{
		NET_RING head;
		DUMMY *dummy;
		RING_ITER iter;
		int   i;

		ring_init(&head);

		for (i = 0; i < 10; i++) {
			dummy = (DUMMY*) mycalloc(1, sizeof(DUMMY));
			snprintf(dummy->name, sizeof(dummy->name), "dummy:%d", i);
			net_ring_append(&head, &dummy->entry);
		}

		net_ring_foreach(iter, &head) {
			dummy = net_ring_to_appl(iter.ptr, DUMMY, entry);
			printf("name: %s\n", dummy->name);
		}

		while (1) {
			iter.ptr = net_ring_pop_head(&head);
			if (iter.ptr == NULL)
				break;
			dummy = net_ring_to_appl(iter.ptr, DUMMY, entry);
			myfree(dummy);
		}
	}
 */
#define	NET_RING_FOREACH(iter, head_ptr) \
        for ((iter).ptr = net_ring_succ((head_ptr)); (iter).ptr != (head_ptr);  \
             (iter).ptr = net_ring_succ((iter).ptr))

#define	net_ring_foreach	NET_RING_FOREACH

/**
 * ��β����ͷ���������ݻ��е����л�Ԫ��
 * @param iter {RING_ITER}
 * @param head_ptr {NET_RING*} ���ݻ���ͷָ��
 */
#define	NET_RING_FOREACH_REVERSE(iter, head_ptr) \
        for ((iter).ptr = net_ring_pred((head_ptr)); (iter).ptr != (head_ptr);  \
             (iter).ptr = net_ring_pred((iter).ptr))

#define	net_ring_foreach_reverse	NET_RING_FOREACH_REVERSE

/**
 * �������ݻ��е�һ����Ԫ��ָ��
 * @param head {NET_RING*} ��ͷָ��
 * @return {NET_RING*} NULL: ��Ϊ��
 */
#define NET_RING_FIRST(head) \
	(net_ring_succ(head) != (head) ? net_ring_succ(head) : 0)

#define	net_ring_first		NET_RING_FIRST

/**
 * �������ݻ���ͷ��һ����Ԫ��ָ��ͬʱ����ת��Ӧ���Զ���ṹ���͵Ķ����ַ
 * @param head {NET_RING*} ��ͷָ��
 * @param app_type Ӧ���Զ���ṹ����
 * @param ring_member {NET_RING*} ��Ԫ����Ӧ���Զ���ṹ�еĳ�Ա����
 * @return {app_type*} Ӧ���Զ���ṹ���͵Ķ����ַ
 */
#define NET_RING_FIRST_APPL(head, app_type, ring_member) \
	(net_ring_succ(head) != (head) ? \
	 NET_RING_TO_APPL(net_ring_succ(head), app_type, ring_member) : 0)

#define	net_ring_first_appl	NET_RING_FIRST_APPL

/**
 * �������ݻ������һ����Ԫ��ָ��
 * @param head {NET_RING*} ��ͷָ��
 * @return {NET_RING*} NULL: ��Ϊ��
 */
#define NET_RING_LAST(head) \
       (net_ring_pred(head) != (head) ? net_ring_pred(head) : 0)

#define	net_ring_last		NET_RING_LAST

/**
 * �������ݻ������һ����Ԫ��ָ��ͬʱ����ת��Ӧ���Զ���ṹ���͵Ķ����ַ
 * @param head {NET_RING*} ��ͷָ��
 * @param app_type Ӧ���Զ���ṹ����
 * @param ring_member {NET_RING*} ��Ԫ����Ӧ���Զ���ṹ�еĳ�Ա����
 * @return {app_type*} Ӧ���Զ���ṹ���͵Ķ����ַ
 */
#define NET_RING_LAST_APPL(head, app_type, ring_member) \
       (net_ring_pred(head) != (head) ? \
	NET_RING_TO_APPL(net_ring_pred(head), app_type, ring_member) : 0)

#define	net_ring_last_appl	NET_RING_LAST_APPL

/**
 * ��һ����Ԫ����ӽ�����β��
 * @param ring {NET_RING*} ���ݻ�
 * @param entry {NET_RING*} �µ�Ԫ��
 */
#define	NET_RING_APPEND(ring_in, entry_in) do {  \
	NET_RING *ring_ptr = (ring_in), *entry_ptr = (entry_in);  \
        entry_ptr->succ      = ring_ptr->succ;  \
        entry_ptr->pred      = ring_ptr;  \
        entry_ptr->parent    = ring_ptr->parent;  \
        ring_ptr->succ->pred = entry_ptr;  \
        ring_ptr->succ       = entry_ptr;  \
        ring_ptr->parent->len++;  \
} while (0)

/**
 * ��һ����Ԫ����ӽ�����ͷ��
 * @param ring {NET_RING*} ���ݻ�
 * @param entry {NET_RING*} �µ�Ԫ��
 */
#define	NET_RING_PREPEND(ring_in, entry_in) do {  \
	NET_RING *ring_ptr = (ring_in), *entry_ptr = (entry_in);  \
	entry_ptr->pred      = ring_ptr->pred;  \
	entry_ptr->succ      = ring_ptr;  \
	entry_ptr->parent    = ring_ptr->parent;  \
	ring_ptr->pred->succ = entry_ptr;  \
	ring_ptr->pred       = entry_ptr;  \
	ring_ptr->parent->len++;  \
} while (0)

/**
 * ��һ����Ԫ�ش����ݻ���ɾ��
 * @param entry {NET_RING*} ��Ԫ��
 */
#define	NET_RING_DETACH(entry_in) do {  \
	NET_RING   *succ, *pred, *entry_ptr = (entry_in);  \
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

