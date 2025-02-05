#include "stdafx.h"
#include "memory.h"
#include "net_msg.h"
#include "net_array.h"

static void net_array_push_back(struct NET_ARRAY *a, void *obj)
{
	net_array_append(a, obj);
}

static void net_array_push_front(struct NET_ARRAY *a, void *obj)
{
	net_array_prepend(a, obj);
}

static void *net_array_pop_back(struct NET_ARRAY *a)
{
	void *obj;
	if (a->count <= 0) {
		return NULL;
	}
	a->count--;
	obj = a->items[a->count];
	return obj;
}

static void *net_array_pop_front(struct NET_ARRAY *a)
{
	void *obj;
	int   i;

	if (a->count <= 0) {
		return NULL;
	}
	obj = a->items[0];
	a->count--;
	for (i = 0; i < a->count; i++) {
		a->items[i] = a->items[i + 1];
	}

	return obj;
}

/* net_array_iter_head - get the head of the array */

static void *net_array_iter_head(ITER *iter, struct NET_ARRAY *a)
{
	iter->dlen = -1;
	iter->key = NULL;
	iter->klen = 0;
	iter->i = 0;
	iter->size = a->count;
	if (a->items == NULL || a->count <= 0) {
		iter->ptr = iter->data = 0;
	} else {
		iter->ptr = iter->data = a->items[0];
	}

	return iter->ptr;
}

/* net_array_iter_next - get the next of the array */

static void *net_array_iter_next(ITER *iter, struct NET_ARRAY *a)
{
	iter->i++;
	if (iter->i >= a->count) {
		iter->data = iter->ptr = 0;
	} else {
		iter->data = iter->ptr = a->items[iter->i];
	}
	return iter->ptr;
}
 
/* net_array_iter_tail - get the tail of the array */

static void *net_array_iter_tail(ITER *iter, struct NET_ARRAY *a)
{
	iter->dlen = -1;
	iter->key = NULL;
	iter->klen = 0;
	iter->i = a->count - 1;
	iter->size = a->count;
	if (a->items == NULL || iter->i < 0) {
		iter->ptr = iter->data = 0;
	} else {
		iter->data = iter->ptr = a->items[iter->i];
	}
	return iter->ptr;
}

/* net_array_iter_prev - get the prev of the array */

static void *net_array_iter_prev(ITER *iter, struct NET_ARRAY *a)
{
	iter->i--;
	if (iter->i < 0) {
		iter->data = iter->ptr = 0;
	} else {
		iter->data = iter->ptr = a->items[iter->i];
	}
	return iter->ptr;
}

/* grows internal buffer to satisfy required minimal capacity */
static void net_array_grow(NET_ARRAY *a, int min_capacity)
{
	int min_delta = 16;
	int delta;

	/* don't need to grow the capacity of the array */
	if (a->capacity >= min_capacity) {
		return;
	}

	delta = min_capacity;
	/* make delta a multiple of min_delta */
	delta += min_delta - 1;
	delta /= min_delta;
	delta *= min_delta;
	/* actual grow */
	if (delta <= 0) {
		return;
	}

	a->capacity += delta;

	if (a->items == NULL) {
		a->items = (void**) mem_malloc(a->capacity * sizeof(void*));
	} else {
		a->items = (void**) mem_realloc(a->items, a->capacity * sizeof(void*));
	}

	/* reset, just in case */
	memset(a->items + a->count, 0,
		(a->capacity - a->count) * sizeof(void *));
}

NET_ARRAY *net_array_create(int init_size)
{
	NET_ARRAY *a;

	a = (NET_ARRAY *) mem_calloc(1, sizeof(NET_ARRAY));

	a->push_back  = net_array_push_back;
	a->push_front = net_array_push_front;
	a->pop_back   = net_array_pop_back;
	a->pop_front  = net_array_pop_front;
	a->iter_head  = net_array_iter_head;
	a->iter_next  = net_array_iter_next;
	a->iter_tail  = net_array_iter_tail;
	a->iter_prev  = net_array_iter_prev;

	if(init_size <= 0) {
		init_size = 100;
	}

	net_array_pre_append(a, init_size);

	return a;
}

void net_array_clean(NET_ARRAY *a, void (*free_fn)(void *))
{
	int	idx;

	for(idx = 0; idx < a->count; idx++) {
		if(free_fn != NULL && a->items[idx] != NULL) {
			free_fn(a->items[idx]);
		}
		a->items[idx] = NULL;	/* sanity set to be null */
	}
	a->count = 0;
}

void net_array_free(NET_ARRAY *a, void (*free_fn)(void *))
{
	net_array_clean(a, free_fn);
	if (a->items) {
		mem_free(a->items);
	}
	mem_free(a);
}

int net_array_append(NET_ARRAY *a, void *obj)
{
	if (a->count >= a->capacity) {
		net_array_grow(a, a->count + 16);
	}
	a->items[a->count++] = obj;
	return a->count - 1;
}

int net_array_pred_insert(NET_ARRAY *a, int position, void *obj)
{
	int	idx;

	/*
	 * a->items[count - 1] should be the last valid item node
	 * position should: positioin >= 0 && position <= a->count - 1
	 */
	if(position < 0 || position >= a->count) {
		return -1;
	}

	if(a->count >= a->capacity) {
		net_array_grow(a, a->count + 1);
	}

	/* NOTICE: the C's index begin with 0
	 * when position == 0, just prepend one new node before the first node
	 * of the array
	 */
	for(idx = a->count; idx > position && idx > 0; idx--) {
		/* if idx == 0 then we has arrived
		 * at the beginning of the array
		 */
		a->items[idx] = a->items[idx - 1];
	}
	a->items[position] = obj;
	a->count++;
	return position;
}

int net_array_succ_insert(NET_ARRAY *a, int position, void *obj)
{
	int	idx, position_succ;

	/*
	 * a->items[count - 1] should be the last valid item node
	 * position should: position >= 0 && position <= a->count - 1
	 */
	if (position < 0 || position >= a->count) {
		return -1;
	}

	if (a->count >= a->capacity) {
		net_array_grow(a, a->count + 1);
	}

	position_succ = position + 1;

	/*
	 * position_succ should:
	 * position_succ > 0 (position >= 0 and position_succ = position + 1)
	 * and position_succ <= a->count (when position == a->count - 1,
	 * position == a->count, and just append one new node after the
	 * last node)
	 * NOTICE: the C's index begin with 0
	 */
	for (idx = a->count; idx > position_succ; idx--) {
		a->items[idx] = a->items[idx - 1];
	}
	a->items[position_succ] = obj;
	a->count++;
	return position_succ;
}

int net_array_prepend(NET_ARRAY *a, void *obj)
{
	return net_array_pred_insert(a, 0, obj);
}

int net_array_delete_idx(NET_ARRAY *a, int position, void (*free_fn)(void *))
{
	int	idx;

	if (position < 0 || position >= a->count) {
		return -1;
	}
	if (free_fn != NULL && a->items[position] != NULL) {
		free_fn(a->items[position]);
	}
	a->items[position] = NULL;   /* sanity set to be null */

	for (idx = position; idx < a->count - 1; idx++) {
		a->items[idx] = a->items[idx + 1];
	}
	a->count--;
	return 0;
}

int net_array_delete(NET_ARRAY *a, int idx, void (*free_fn)(void*))
{
	if (idx < 0 || idx >= a->count) {
		return  -1;
	}
	if (free_fn != NULL && a->items[idx] != NULL) {
		free_fn(a->items[idx]);
	}
	a->count--;
	if (a->count > 0) {
		a->items[idx] = a->items[a->count];
	}
	return 0;
}

int net_array_delete_obj(NET_ARRAY *a, void *obj, void (*free_fn)(void *))
{
	int   idx, position, ret;

	position = -1;
	for (idx = 0; idx < a->count; idx++) {
		if (a->items[idx] == obj) {
			position = idx;
			break;
		}
	}

	if (free_fn != NULL && obj != NULL) {
		free_fn(obj);
	}
	if (position == -1) { /* not found */
		return -1;
	}

	/* don't need to free the obj in net_array_delete_idx */
	a->items[idx] = NULL;
	ret = net_array_delete_idx(a, position, NULL);
	if (ret < 0) {
		return -1;
	}
	return ret;
}

int net_array_delete_range(NET_ARRAY *a, int ibegin, int iend,
	void (*free_fn)(void*))
{
	int   i, imax;

	if (ibegin < 0 || iend < 0 || a->count <= 0) {
		return -1;
	}
	if (ibegin > iend) {
		return -1;
	}

	imax = a->count - 1;
	if (iend > imax) {
		iend = imax;
	}

	if (free_fn != NULL) {
		for (i = ibegin; i <= iend; i++) {
			if (a->items[i] != NULL) {
				free_fn(a->items[i]);
			}
			a->items[i] = NULL;
		}
	}

	a->count -= iend - ibegin + 1;

	for (iend++; iend <= imax;) {
		a->items[ibegin++] = a->items[iend++];
	}

	return 0;
}

int net_array_mv_idx(NET_ARRAY *a, int ito, int ifrom, void (*free_fn)(void *))
{
	int   i, i_obj, i_src, i_max;

	if (ito < 0 || ifrom < 0 || a->count < 0) {
		return -1;
	}

	if (a->count == 0 || ito >= ifrom || ifrom >= a->count) {
		return 0;
	}

	i_obj = ito;
	i_src = ifrom;
	i_max = a->count - 1;

	if (free_fn != NULL) {
		for (i = i_obj; i < i_src; i++) {
			if (a->items[i] != NULL) {
				free_fn(a->items[i]);
			}
			a->items[i] = NULL;
		}
	}
	for (; i_src <= i_max; i_src++) {
		a->items[i_obj] = a->items[i_src];
		i_obj++;
	}

	a->count -= ifrom - ito;
	if (a->count < 0) { /* imposible, sanity check */
		return -1;
	}
	return 0;
}

/* if you are going to append a known and large number of items,
 * call this first
 */
void net_array_pre_append(NET_ARRAY *a, int app_count)
{
	const char *name = "net_array_pre_append";

	if (app_count <= 0) {
		net_msg_fatal("%s(%d)->%s: invalid input", __FILE__, __LINE__, name);
	}

	if (a->count + app_count > a->capacity) {
		net_array_grow(a, a->count + app_count);
	}
}

void *net_array_index(const NET_ARRAY *a, int idx)
{
	if (idx < 0 || idx > a->count - 1) {
		return NULL;
	}

	return a->items[idx];
}

int net_array_size(const NET_ARRAY *a)
{
	return a->count;
}
