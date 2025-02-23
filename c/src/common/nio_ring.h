#ifndef	NIO_RING_INCLUDE_H
#define	NIO_RING_INCLUDE_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <stddef.h>

typedef struct NIO_RING NIO_RING;

/**
 * 数据环结构类型定义
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
 * 初始化数据环
 * @param ring {NIO_RING*} 数据环
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
 * 获得当前数据环内元素个数
 * @param ring {NIO_RING*} 数据环
 * @return {int} 数据环内元素个数
 */
#ifdef USE_FAST_NIO_RING
#define nio_ring_size(r) (((NIO_RING*)(r))->len)
#else
int  nio_ring_size(const NIO_RING *ring);
#endif

/**
 * 将一个新元素添加进环的尾部
 * @param ring {NIO_RING*} 数据环
 * @param entry {NIO_RING*} 新的元素
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
 * 将一个新元素添加进环的头部
 * @param ring {NIO_RING*} 数据环
 * @param entry {NIO_RING*} 新的元素
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
 * 将一个环元素从数据环中删除
 * @param entry {NIO_RING*} 环元素
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
 * 从环中弹出头部环元素
 * @param ring {NIO_RING*} 数据环
 * @return {NIO_RING*} 头部环元素，如果返回空则表示该数据环为空
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
 * 从环中弹出尾部环元素
 * @param ring {NIO_RING*} 数据环
 * @return {NIO_RING*} 尾部环元素，如果返回空则表示该数据环为空
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

/*--------------------  一些方便快捷的宏操作 --------------------------------*/

/**
 * 返回当前环元素的下一个环元素
 */
#define NIO_RING_SUCC(c) ((c)->succ)
#define	nio_ring_succ	NIO_RING_SUCC

/**
 * 返回当前环元素的前一个环元素
 */
#define NIO_RING_PRED(c) ((c)->pred)
#define	nio_ring_pred	NIO_RING_PRED

/**
 * 将环元素指针转换成应用的自定义类型的指针地址
 * @param ring_ptr {NIO_RING*} 环元素指针
 * @param app_type 应用自定义类型
 * @param ring_member {NIO_RING*} 环元素在应用自定义结构中的成员名称
 * @return {app_type*} 应用自定义结构类型的对象地址
 */
#define NIO_RING_TO_APPL(ring_ptr, app_type, ring_member) \
    ((app_type *) (((char *) (ring_ptr)) - offsetof(app_type,ring_member)))

#define	nio_ring_to_appl	NIO_RING_TO_APPL

/**
 * 从头部至尾部遍历数据环中的所有环元素
 * @param iter {RING_ITER}
 * @param head_ptr {NIO_RING*} 数据环的头指针
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
 * 从尾部至头部遍历数据环中的所有环元素
 * @param iter {RING_ITER}
 * @param head_ptr {NIO_RING*} 数据环的头指针
 */
#define	NIO_RING_FOREACH_REVERSE(iter, head_ptr) \
        for ((iter).ptr = nio_ring_pred((head_ptr)); (iter).ptr != (head_ptr);  \
             (iter).ptr = nio_ring_pred((iter).ptr))

#define	nio_ring_foreach_reverse	NIO_RING_FOREACH_REVERSE

/**
 * 返回数据环中第一个环元素指针
 * @param head {NIO_RING*} 环头指针
 * @return {NIO_RING*} NULL: 环为空
 */
#define NIO_RING_FIRST(head) \
	(nio_ring_succ(head) != (head) ? nio_ring_succ(head) : 0)

#define	nio_ring_first		NIO_RING_FIRST

/**
 * 返回数据环中头第一个环元素指针同时将其转换应用自定义结构类型的对象地址
 * @param head {NIO_RING*} 环头指针
 * @param app_type 应用自定义结构类型
 * @param ring_member {NIO_RING*} 环元素在应用自定义结构中的成员名称
 * @return {app_type*} 应用自定义结构类型的对象地址
 */
#define NIO_RING_FIRST_APPL(head, app_type, ring_member) \
	(nio_ring_succ(head) != (head) ? \
	 NIO_RING_TO_APPL(nio_ring_succ(head), app_type, ring_member) : 0)

#define	nio_ring_first_appl	NIO_RING_FIRST_APPL

/**
 * 返回数据环中最后一个环元素指针
 * @param head {NIO_RING*} 环头指针
 * @return {NIO_RING*} NULL: 环为空
 */
#define NIO_RING_LAST(head) \
       (nio_ring_pred(head) != (head) ? nio_ring_pred(head) : 0)

#define	nio_ring_last		NIO_RING_LAST

/**
 * 返回数据环中最后一个环元素指针同时将其转换应用自定义结构类型的对象地址
 * @param head {NIO_RING*} 环头指针
 * @param app_type 应用自定义结构类型
 * @param ring_member {NIO_RING*} 环元素在应用自定义结构中的成员名称
 * @return {app_type*} 应用自定义结构类型的对象地址
 */
#define NIO_RING_LAST_APPL(head, app_type, ring_member) \
       (nio_ring_pred(head) != (head) ? \
	NIO_RING_TO_APPL(nio_ring_pred(head), app_type, ring_member) : 0)

#define	nio_ring_last_appl	NIO_RING_LAST_APPL

/**
 * 将一个新元素添加进环的尾部
 * @param ring {NIO_RING*} 数据环
 * @param entry {NIO_RING*} 新的元素
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
 * 将一个新元素添加进环的头部
 * @param ring {NIO_RING*} 数据环
 * @param entry {NIO_RING*} 新的元素
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
 * 将一个环元素从数据环中删除
 * @param entry {NIO_RING*} 环元素
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

