#ifndef	NET_RING_INCLUDE_H
#define	NET_RING_INCLUDE_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <stddef.h>

typedef struct NET_RING NET_RING;

/**
 * 数据环结构类型定义
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
 * 初始化数据环
 * @param ring {NET_RING*} 数据环
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
 * 获得当前数据环内元素个数
 * @param ring {NET_RING*} 数据环
 * @return {int} 数据环内元素个数
 */
#ifdef USE_FAST_NET_RING
#define net_ring_size(r) (((NET_RING*)(r))->len)
#else
int  net_ring_size(const NET_RING *ring);
#endif

/**
 * 将一个新元素添加进环的尾部
 * @param ring {NET_RING*} 数据环
 * @param entry {NET_RING*} 新的元素
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
 * 将一个新元素添加进环的头部
 * @param ring {NET_RING*} 数据环
 * @param entry {NET_RING*} 新的元素
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
 * 将一个环元素从数据环中删除
 * @param entry {NET_RING*} 环元素
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
 * 从环中弹出头部环元素
 * @param ring {NET_RING*} 数据环
 * @return {NET_RING*} 头部环元素，如果返回空则表示该数据环为空
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
 * 从环中弹出尾部环元素
 * @param ring {NET_RING*} 数据环
 * @return {NET_RING*} 尾部环元素，如果返回空则表示该数据环为空
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

/*--------------------  一些方便快捷的宏操作 --------------------------------*/

/**
 * 返回当前环元素的下一个环元素
 */
#define NET_RING_SUCC(c) ((c)->succ)
#define	net_ring_succ	NET_RING_SUCC

/**
 * 返回当前环元素的前一个环元素
 */
#define NET_RING_PRED(c) ((c)->pred)
#define	net_ring_pred	NET_RING_PRED

/**
 * 将环元素指针转换成应用的自定义类型的指针地址
 * @param ring_ptr {NET_RING*} 环元素指针
 * @param app_type 应用自定义类型
 * @param ring_member {NET_RING*} 环元素在应用自定义结构中的成员名称
 * @return {app_type*} 应用自定义结构类型的对象地址
 */
#define NET_RING_TO_APPL(ring_ptr, app_type, ring_member) \
    ((app_type *) (((char *) (ring_ptr)) - offsetof(app_type,ring_member)))

#define	net_ring_to_appl	NET_RING_TO_APPL

/**
 * 从头部至尾部遍历数据环中的所有环元素
 * @param iter {RING_ITER}
 * @param head_ptr {NET_RING*} 数据环的头指针
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
 * 从尾部至头部遍历数据环中的所有环元素
 * @param iter {RING_ITER}
 * @param head_ptr {NET_RING*} 数据环的头指针
 */
#define	NET_RING_FOREACH_REVERSE(iter, head_ptr) \
        for ((iter).ptr = net_ring_pred((head_ptr)); (iter).ptr != (head_ptr);  \
             (iter).ptr = net_ring_pred((iter).ptr))

#define	net_ring_foreach_reverse	NET_RING_FOREACH_REVERSE

/**
 * 返回数据环中第一个环元素指针
 * @param head {NET_RING*} 环头指针
 * @return {NET_RING*} NULL: 环为空
 */
#define NET_RING_FIRST(head) \
	(net_ring_succ(head) != (head) ? net_ring_succ(head) : 0)

#define	net_ring_first		NET_RING_FIRST

/**
 * 返回数据环中头第一个环元素指针同时将其转换应用自定义结构类型的对象地址
 * @param head {NET_RING*} 环头指针
 * @param app_type 应用自定义结构类型
 * @param ring_member {NET_RING*} 环元素在应用自定义结构中的成员名称
 * @return {app_type*} 应用自定义结构类型的对象地址
 */
#define NET_RING_FIRST_APPL(head, app_type, ring_member) \
	(net_ring_succ(head) != (head) ? \
	 NET_RING_TO_APPL(net_ring_succ(head), app_type, ring_member) : 0)

#define	net_ring_first_appl	NET_RING_FIRST_APPL

/**
 * 返回数据环中最后一个环元素指针
 * @param head {NET_RING*} 环头指针
 * @return {NET_RING*} NULL: 环为空
 */
#define NET_RING_LAST(head) \
       (net_ring_pred(head) != (head) ? net_ring_pred(head) : 0)

#define	net_ring_last		NET_RING_LAST

/**
 * 返回数据环中最后一个环元素指针同时将其转换应用自定义结构类型的对象地址
 * @param head {NET_RING*} 环头指针
 * @param app_type 应用自定义结构类型
 * @param ring_member {NET_RING*} 环元素在应用自定义结构中的成员名称
 * @return {app_type*} 应用自定义结构类型的对象地址
 */
#define NET_RING_LAST_APPL(head, app_type, ring_member) \
       (net_ring_pred(head) != (head) ? \
	NET_RING_TO_APPL(net_ring_pred(head), app_type, ring_member) : 0)

#define	net_ring_last_appl	NET_RING_LAST_APPL

/**
 * 将一个新元素添加进环的尾部
 * @param ring {NET_RING*} 数据环
 * @param entry {NET_RING*} 新的元素
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
 * 将一个新元素添加进环的头部
 * @param ring {NET_RING*} 数据环
 * @param entry {NET_RING*} 新的元素
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
 * 将一个环元素从数据环中删除
 * @param entry {NET_RING*} 环元素
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

