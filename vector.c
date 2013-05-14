/**
 * @file vector.c
 * @brief implement vectors in c
 * @author Hamid Alipour <http://codingrecipes.com/implementation-of-a-vector-data-structure-in-c>, bug fixed by Po-Hsien Tseng <steve13814@gmail.com>
 * @version 20130409
 * @date 2013-04-09
 */
#include "vector.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
 
static void vector_swap(void *elemp1, void *elemp2, unsigned int elem_size)
{
	void *tmp = malloc(elem_size);
 
	memcpy(tmp, elemp1, elem_size);
	memcpy(elemp1, elemp2, elem_size);
	memcpy(elemp2, tmp, elem_size);
 
             free(tmp); /* Thanks to gromit */
}

static void vector_grow(vector *v, unsigned int size)
{
	if (size > v->num_alloc_elems)
		v->num_alloc_elems = size;
	else
		v->num_alloc_elems *= 2;
 
	v->elems = realloc(v->elems, v->elem_size * v->num_alloc_elems);
	assert(v->elems != NULL);
}

void vector_init(vector *v, unsigned int elem_size, unsigned int init_size, void (*free_func)(void *))
{
	v->elem_size = elem_size;
	v->num_alloc_elems = (int) init_size > 0 ? init_size : VECTOR_INIT_SIZE;
	v->num_elems = 0;
	v->elems = malloc(elem_size * v->num_alloc_elems);
	assert(v->elems != NULL);
	v->free_func = free_func != NULL ? free_func : NULL;
}
 
void vector_dispose(vector *v)
{
	unsigned int i;
 
	if (v->free_func != NULL) {
		for (i = 0; i < v->num_elems; i++) {
			v->free_func(VECTOR_INDEX(i));
		}
	}
 
	free(v->elems);
}
 
void vector_copy(vector *v1, vector *v2)
{
	v2->num_elems = v1->num_elems;
	v2->num_alloc_elems = v1->num_alloc_elems;
	v2->elem_size = v1->elem_size;
 
	v2->elems = realloc(v2->elems, v2->num_alloc_elems * v2->elem_size);
	assert(v2->elems != NULL);
 
	memcpy(v2->elems, v1->elems, v2->num_elems * v2->elem_size);
}
 
void vector_insert(vector *v, void *elem, unsigned int index)
{
	void *target;
 
	if ((int) index > -1) {
		if (!VECTOR_INBOUNDS(index))
			return;
		target = VECTOR_INDEX(index);
	} else {
		if (!VECTOR_HASSPACE(v))
			vector_grow(v, 0);
		target = VECTOR_INDEX(v->num_elems);
		v->num_elems++; /* Only grow when adding a new item not when inserting in a spec indx */
	}
 
	memcpy(target, elem, v->elem_size);
}
 
void vector_insert_at(vector *v, void *elem, unsigned int index)
{
	if ((int) index < 0)
		return;
 
	if (!VECTOR_HASSPACE(v))
		vector_grow(v, 0);
 
	if (index < v->num_elems)
		memmove(VECTOR_INDEX(index + 1), VECTOR_INDEX(index), (v->num_elems - index) * v->elem_size);
 
	/* 1: we are passing index so insert won't increment this 2: insert checks INBONDS... */
	v->num_elems++;
 
	vector_insert(v, elem, index);
}
 
void vector_push(vector *v, void *elem)
{
	vector_insert(v, elem, -1);
}
 
void vector_pop(vector *v, void *elem)
{
	memcpy(elem, VECTOR_INDEX(v->num_elems - 1), v->elem_size);
	v->num_elems--;
}
 
void vector_shift(vector *v, void *elem)
{
	memcpy(elem, v->elems, v->elem_size);
	v->num_elems--;
	memmove(VECTOR_INDEX(0), VECTOR_INDEX(1), v->num_elems * v->elem_size);
}
 
void vector_unshift(vector *v, void *elem)
{
	if (!VECTOR_HASSPACE(v))
		vector_grow(v, v->num_elems + 1);
 
	memmove(VECTOR_INDEX(1), v->elems, v->num_elems * v->elem_size);
	memcpy(v->elems, elem, v->elem_size);
 
	v->num_elems++;
}
 
void vector_transpose(vector *v, unsigned int index1, unsigned int index2)
{
	vector_swap(VECTOR_INDEX(index1), VECTOR_INDEX(index2), v->elem_size);
}
 
void vector_get(vector *v, unsigned int index, void *elem)
{
	assert((int) index >= 0);
 
	if (!VECTOR_INBOUNDS(index)) {
		elem = NULL;
		return;
	}
 
	memcpy(elem, VECTOR_INDEX(index), v->elem_size);
}
 
void vector_get_all(vector *v, void *elems)
{
	memcpy(elems, v->elems, v->num_elems * v->elem_size);
}
 
void vector_remove(vector *v, unsigned int index)
{
	assert((int) index >= 0);
 
	if (!VECTOR_INBOUNDS(index))
		return;
 
	memmove(VECTOR_INDEX(index), VECTOR_INDEX(index + 1), v->elem_size * (v->num_elems - index - 1));
	v->num_elems--;
}
 
void vector_remove_some(vector *v, unsigned int from, unsigned int to)
{
	assert((int) from >= 0);
	assert((int) to >= 0);
	assert(from <= to);

	if (!VECTOR_INBOUNDS(from))
		return;
	if (!VECTOR_INBOUNDS(to))
		return;
	if (from == to)
		return;

	memmove(VECTOR_INDEX(from), VECTOR_INDEX(to), v->elem_size * (v->num_elems - to));
	v->num_elems -= to - from;
}

void vector_remove_all(vector *v)
{
	v->num_elems = 0;
	v->num_alloc_elems = VECTOR_INIT_SIZE;
	v->elems = realloc(v->elems, v->elem_size * v->num_alloc_elems);
	assert(v->elems != NULL);
}
 
unsigned int vector_length(vector *v)
{
	return v->num_elems;
}
 
unsigned int vector_size(vector *v)
{
	return v->num_elems * v->elem_size;
}
 
void vector_cmp_all(vector *v, void *elem, int (*cmp_func)(const void *, const void *))
{
	unsigned int i;
	void *best_match = VECTOR_INDEX(0);
 
	for (i = 1; i < v->num_elems; i++)
		if (cmp_func(VECTOR_INDEX(i), best_match) > 0)
			best_match = VECTOR_INDEX(i);
 
	memcpy(elem, best_match, v->elem_size);
}
 
void vector_qsort(vector *v, int (*cmp_func)(const void *, const void *))
{
	qsort(v->elems, v->num_elems, v->elem_size, cmp_func);
}
 
