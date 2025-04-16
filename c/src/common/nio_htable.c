#include "stdafx.h"
#include "nio_memory.h"
#include "nio_msg.h"
#include "nio_htable.h"

/* nio_htable_iter_head */

static void *nio_htable_iter_head(ITER *iter, NIO_HTABLE *table)
{
	NIO_HTABLE_INFO *ptr = NULL;

	iter->dlen = -1;
	iter->klen = -1;
	iter->i = 0;
	iter->size = table->size;
	iter->ptr = NULL;

	for (; iter->i < iter->size; iter->i++) {
		if (table->data[iter->i] != NULL) {
			iter->ptr = ptr = table->data[iter->i];
			break;
		}
	}

	if (ptr) {
		iter->data = ptr->value;
		iter->key = ptr->key;
	} else {
		iter->data = NULL;
		iter->key = NULL;
	}
	return (iter->ptr);
}

/* nio_htable_iter_next */

static void *nio_htable_iter_next(ITER *iter, NIO_HTABLE *table)
{
	NIO_HTABLE_INFO *ptr;

	ptr = (NIO_HTABLE_INFO*) iter->ptr;
	if (ptr) {
		iter->ptr = ptr = ptr->next;
		if (ptr != NULL) {
			iter->data = ptr->value;
			iter->key = ptr->key;
			return (iter->ptr);
		}
	}

	for (iter->i++; iter->i < iter->size; iter->i++) {
		if (table->data[iter->i] != NULL) {
			iter->ptr = ptr = table->data[iter->i];
			break;
		}
	}

	if (ptr) {
		iter->data = ptr->value;
		iter->key = ptr->key;
	} else {
		iter->data = NULL;
		iter->key = NULL;
	}
	return (iter->ptr);
}

/* nio_htable_iter_tail */

static void *nio_htable_iter_tail(ITER *iter, NIO_HTABLE *table)
{
	NIO_HTABLE_INFO *ptr = NULL;

	iter->dlen = -1;
	iter->klen = -1;
	iter->i = table->size - 1;
	iter->size = table->size;
	iter->ptr = NULL;

	for (; iter->i >= 0; iter->i--) {
		if (table->data[iter->i] != NULL) {
			iter->ptr = ptr = table->data[iter->i];
			break;
		}
	}

	if (ptr) {
		iter->data = ptr->value;
		iter->key = ptr->key;
	} else {
		iter->data = NULL;
		iter->key = NULL;
	}
	return (iter->ptr);
}

/* nio_htable_iter_prev */

static void *nio_htable_iter_prev(ITER *iter, NIO_HTABLE *table)
{
	NIO_HTABLE_INFO *ptr;

	ptr = (NIO_HTABLE_INFO*) iter->ptr;
	if (ptr) {
		iter->ptr = ptr = ptr->next;
		if (ptr != NULL) {
			iter->data = ptr->value;
			iter->key = ptr->key;
			return (iter->ptr);
		}
	}

	for (iter->i--; iter->i >= 0; iter->i--) {
		if (table->data[iter->i] != NULL) {
			iter->ptr = ptr = table->data[iter->i];
			break;
		}
	}

	if (ptr) {
		iter->data = ptr->value;
		iter->key = ptr->key;
	} else {
		iter->data = NULL;
		iter->key = NULL;
	}
	return (iter->ptr);
}

/* nio_htable_iter_info */

static NIO_HTABLE_INFO *nio_htable_iter_info(ITER *iter, struct NIO_HTABLE *table)
{
	(void) table;
	return (iter->ptr ? (NIO_HTABLE_INFO*) iter->ptr : NULL);
}

/* __def_hash_fn - hash a string */

static unsigned __def_hash_fn(const void *buffer, size_t len)
{
        unsigned long h = 0;
        unsigned long g;
	const unsigned char* s = (const unsigned char *) buffer;

        /*
         * From the "Dragon" book by Aho, Sethi and Ullman.
         */

        while (len-- > 0) {
                h = (h << 4) + *s++;
                if ((g = (h & 0xf0000000)) != 0) {
                        h ^= (g >> 24);
                        h ^= g;
                }
        }

        return (unsigned) h;
}
/* nio_htable_link - insert element into table */

#define nio_htable_link(_table, _element, _n) { \
	NIO_HTABLE_INFO **_h = _table->data + _n; \
	_element->prev = 0; \
	if ((_element->next = *_h) != 0) \
		(*_h)->prev = _element; \
	*_h = _element; \
	_table->used++; \
}

/* nio_htable_size - allocate and initialize hash table */

static int __nio_htable_size(NIO_HTABLE *table, unsigned size)
{
	NIO_HTABLE_INFO **h;

	size |= 1;

	table->data = h = (NIO_HTABLE_INFO **) nio_mem_malloc(size * sizeof(NIO_HTABLE_INFO *));
	if(table->data == NULL) {
		return -1;
	}

	table->size = size;
	table->used = 0;

	while (size-- > 0) {
		*h++ = 0;
	}

	return 0;
}

/* nio_htable_grow - extend existing table */

static int nio_htable_grow(NIO_HTABLE *table)
{
	int ret;
	NIO_HTABLE_INFO *ht;
	NIO_HTABLE_INFO *next;
	unsigned old_size = table->size;
	NIO_HTABLE_INFO **h0 = table->data;
	NIO_HTABLE_INFO **old_entries = h0;
	unsigned n;

	ret = __nio_htable_size(table, 2 * old_size);
	if (ret < 0) {
		return -1;
	}

	while (old_size-- > 0) {
		for (ht = *h0++; ht; ht = next) {
			next = ht->next;
			n = __def_hash_fn(ht->key, strlen(ht->key)) % table->size;
			nio_htable_link(table, ht, n);
		}
	}

	nio_mem_free(old_entries);
	return 0;
}

NIO_HTABLE *nio_htable_create(int size)
{
	NIO_HTABLE *table;
	int	ret;

	table =	(NIO_HTABLE *) nio_mem_calloc(1, sizeof(NIO_HTABLE));
	if (table == NULL) {
		return NULL;
	}

	table->init_size = size;
	ret = __nio_htable_size(table, size < 13 ? 13 : size);
	if(ret < 0) {
		nio_mem_free(table);
		return NULL;
	}

	table->iter_head = nio_htable_iter_head;
	table->iter_next = nio_htable_iter_next;
	table->iter_tail = nio_htable_iter_tail;
	table->iter_prev = nio_htable_iter_prev;
	table->iter_info = nio_htable_iter_info;

	return table;
}

int nio_htable_errno(NIO_HTABLE *table)
{
	if (table == NULL) {
		return NIO_HTABLE_STAT_INVAL;
	}
	return table->status;
}

void nio_htable_set_errno(NIO_HTABLE *table, int error)
{
	if (table) {
		table->status = error;
	}
}

#define	STREQ(x,y) (x == y || (x[0] == y[0] && strcmp(x,y) == 0))

/* nio_htable_enter - enter (key, value) pair */

NIO_HTABLE_INFO *nio_htable_enter(NIO_HTABLE *table, const char *key, void *value)
{
	NIO_HTABLE_INFO *ht;
	int   ret;
	unsigned hash, n;

	table->status = NIO_HTABLE_STAT_OK;
	hash = __def_hash_fn(key, strlen(key));

	if (table->used >= table->size) {
		ret = nio_htable_grow(table);
		if(ret < 0) {
			return NULL;
		}
	}

	n = hash % table->size;

	for (ht = table->data[n]; ht; ht = ht->next) {
		if (STREQ(key, ht->key)) {
			table->status = NIO_HTABLE_STAT_DUPLEX_KEY;
			nio_msg_info("%s(%d): duplex key(%s) exist",
				__FUNCTION__, __LINE__, key);
			return ht;
		}
	}

	ht = (NIO_HTABLE_INFO *) nio_mem_malloc(sizeof(NIO_HTABLE_INFO));
	if (ht == NULL) {
		nio_msg_error("%s(%d): alloc error", __FUNCTION__, __LINE__);
		return NULL;
	}

#if defined(_WIN32) || defined(_WIN64)
	ht->key = _strdup(key);
#else
	ht->key = nio_mem_strdup(key);
#endif
	if (ht->key == NULL) {
		nio_msg_error("%s(%d): alloc error", __FUNCTION__, __LINE__);
		nio_mem_free(ht);
		return NULL;
	}
	ht->hash  = hash;
	ht->value = value;
	nio_htable_link(table, ht, n);

	return ht;
}

/* nio_htable_find - lookup value */

void *nio_htable_find(NIO_HTABLE *table, const char *key)
{
	NIO_HTABLE_INFO *ht = nio_htable_locate(table, key);

	return ht != NULL ? ht->value : NULL;
}

/* nio_htable_locate - lookup entry */

NIO_HTABLE_INFO *nio_htable_locate(NIO_HTABLE *table, const char *key)
{
	NIO_HTABLE_INFO *ht;
	unsigned     n;

	n = __def_hash_fn(key, strlen(key));
	n = n % table->size;

	for (ht = table->data[n]; ht; ht = ht->next) {
		if (STREQ(key, ht->key)) {
			return ht;
		}
	}

	return NULL;
}

void nio_htable_delete_entry(NIO_HTABLE *table, NIO_HTABLE_INFO *ht,
	void (*free_fn) (void *))
{
	unsigned n = ht->hash % table->size;
	NIO_HTABLE_INFO **h = table->data + n;

	if (ht->next)
		ht->next->prev = ht->prev;
	if (ht->prev)
		ht->prev->next = ht->next;
	else
		*h = ht->next;

	nio_mem_free(ht->key);
	if (free_fn && ht->value)
		(*free_fn) (ht->value);
	nio_mem_free(ht);
	table->used--;
}

/* nio_htable_delete - delete one entry */

int nio_htable_delete(NIO_HTABLE *table, const char *key, void (*free_fn) (void *))
{
	NIO_HTABLE_INFO *ht;
	unsigned     n;
	NIO_HTABLE_INFO **h;

	n = __def_hash_fn(key, strlen(key));
	n = n % table->size;

	h = table->data + n;
	for (ht = *h; ht; ht = ht->next) {
		if (STREQ(key, ht->key)) {
			nio_htable_delete_entry(table, ht, free_fn);
			return 0;
		}
	}
	return -1;
}

/* nio_htable_free - destroy hash table */

void nio_htable_free(NIO_HTABLE *table, void (*free_fn) (void *))
{
	unsigned i = table->size;
	NIO_HTABLE_INFO  *ht;
	NIO_HTABLE_INFO  *next;
	NIO_HTABLE_INFO **h = table->data;

	while (i-- > 0) {
		for (ht = *h++; ht; ht = next) {
			next = ht->next;
			nio_mem_free(ht->key);
			if (free_fn && ht->value)
				(*free_fn) (ht->value);
			nio_mem_free(ht);
		}
	}

	nio_mem_free(table->data);
	table->data = 0;
	nio_mem_free(table);
}

int nio_htable_reset(NIO_HTABLE *table, void (*free_fn) (void *))
{
	unsigned i = table->size;
	NIO_HTABLE_INFO *ht;
	NIO_HTABLE_INFO *next;
	NIO_HTABLE_INFO **h;
	int ret;

	h = table->data;

	while (i-- > 0) {
		for (ht = *h++; ht; ht = next) {
			next = ht->next;
			nio_mem_free(ht->key);
			if (free_fn && ht->value) {
				(*free_fn) (ht->value);
			}
			nio_mem_free(ht);
		}
	}
	nio_mem_free(table->data);
	ret = __nio_htable_size(table, table->init_size < 13 ? 13 : table->init_size);
	return ret;
}

/* nio_htable_walk - iterate over hash table */

void nio_htable_walk(NIO_HTABLE *table, void (*action)(NIO_HTABLE_INFO *, void *), void *arg)
{
	unsigned i = table->size;
	NIO_HTABLE_INFO **h = table->data;
	NIO_HTABLE_INFO *ht;

	while (i-- > 0) {
		for (ht = *h++; ht; ht = ht->next) {
			(*action) (ht, arg);
		}
	}
}

int nio_htable_size(const NIO_HTABLE *table)
{
	if (table) {
		return table->size;
	} else {
		return 0;
	}
}

int nio_htable_used(const NIO_HTABLE *table)
{
	if (table) {
		return table->used;
	} else {
		return (0);
	}
}

/*
NIO_HTABLE_INFO **nio_htable_data(NIO_HTABLE *table)
{
	return (NIO_HTABLE_INFO**) table->data;
}
*/

/* nio_htable_list - list all table members */

NIO_HTABLE_INFO **nio_htable_list(const NIO_HTABLE *table)
{
	NIO_HTABLE_INFO **list;
	NIO_HTABLE_INFO *member;
	int     count = 0;
	int     i;

	if (table != 0) {
		list = (NIO_HTABLE_INFO **) nio_mem_malloc(sizeof(*list) * (table->used + 1));
		for (i = 0; i < table->size; i++) {
			for (member = table->data[i]; member != 0;
				member = member->next) {
				list[count++] = member;
			}
		}
	} else {
		list = (NIO_HTABLE_INFO **) nio_mem_malloc(sizeof(*list));
	}
	list[count] = 0;
	return list;
}

void nio_htable_stat(const NIO_HTABLE *table)
{
	NIO_HTABLE_INFO *member;
	int	i, count;

	printf("hash stat count for each key:\n");
	for(i = 0; i < table->size; i++) {
		count = 0;
		member = table->data[i];
		for(; member != 0; member = member->next) {
			count++;
		}
		if(count > 0) {
			printf("chains[%d]: count[%d]\n", i, count);
		}
	}

	printf("hash stat all values for each key:\n");
	for(i = 0; i < table->size; i++) {
		member = table->data[i];
		if(member) {
			printf("chains[%d]: ", i);
			for(; member != 0; member = member->next)
				printf("[%s]", member->key);
			printf("\n");
		}
	}
	printf("hash table size=%d, used=%d\n", table->size, table->used);
}
