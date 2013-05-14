/**
 * @file vector.h
 * @brief header file that implement vectors in c
 * @author Hamid Alipour <http://codingrecipes.com/implementation-of-a-vector-data-structure-in-c>
 * @version 20130409
 * @date 2013-04-09
 */
#ifndef __VECTOR_H__
#define __VECTOR_H__
#define VECTOR_INIT_SIZE    4
#define VECTOR_HASSPACE(v)  (((v)->num_elems + 1) <= (v)->num_alloc_elems)
#define VECTOR_INBOUNDS(i)	(((int) i) >= 0 && (i) < (v)->num_elems)
#define VECTOR_INDEX(i)		((char *) (v)->elems + ((v)->elem_size * (i)))
 
typedef struct _vector {
	void *elems;
	unsigned int elem_size;
	unsigned int num_elems;
	unsigned int num_alloc_elems;
    void (*free_func)(void *);
} vector;
 
void vector_init(vector *, unsigned int, unsigned int, void (*free_func)(void *));
void vector_dispose(vector *);
void vector_copy(vector *, vector *);
void vector_insert(vector *, void *, unsigned int index);
void vector_insert_at(vector *, void *, unsigned int index);
void vector_push(vector *, void *);
void vector_pop(vector *, void *);
void vector_shift(vector *, void *);
void vector_unshift(vector *, void *);
void vector_get(vector *, unsigned int, void *);
void vector_remove(vector *, unsigned int);
void vector_remove_some(vector *, unsigned int, unsigned int);
void vector_remove_all(vector *v);
void vector_transpose(vector *, unsigned int, unsigned int);
unsigned int vector_length(vector *);
unsigned int vector_size(vector *);
void vector_get_all(vector *, void *);
void vector_cmp_all(vector *, void *, int (*cmp_func)(const void *, const void *));
void vector_qsort(vector *, int (*cmp_func)(const void *, const void *));
 
#endif // __VECTOR_H__
