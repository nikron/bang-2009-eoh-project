/**
 * \file bang-utils.h
 * \author Nikhil Bysani
 * \date January 19, 2008
 *
 * \brief  Small utility functions used throughout the library.
 */

#ifndef __BANG_UTILS_H
#define __BANG_UTILS_H
#include<pthread.h>
/**
 * An object to do read - writing syncing
 */
typedef struct {
	pthread_mutex_t *read;
	pthread_mutex_t *write;
	int readers;
} BANG_rw_syncro;

/**
 * A node in an BANG linked list.
 */
typedef struct _BANG_node{
	struct _BANG_node *prev;
	struct _BANG_node *next;
	void *data;
} BANG_node;

/**
 * A linked list structure, also a stack/queue.
 * This structure is supposed to be thread safe.
 */
typedef struct {
	BANG_rw_syncro *lck;
	BANG_node *head;
	BANG_node *tail;
	size_t size;
} BANG_linked_list;

typedef struct {
	int key;
	void *data;
} BANG_set_data;

typedef struct {
	BANG_rw_syncro *lck;
	size_t size;
	int current;
	int count;
	BANG_set_data *members;
	BANG_linked_list *free_space;
} BANG_set;

typedef unsigned int (*BANG_hashcode) (const void *);
typedef int (*BANG_compare) (const void*,const void*);

typedef struct {
	void *key;
	void *item;
} BANG_hashmap_pair;

typedef struct {
	BANG_linked_list **data;
	unsigned int data_size;
	BANG_hashcode hash_func;
	BANG_compare compare_func;
} BANG_hashmap;

/**
 * \param data The information to be included in the node.
 *
 * \brief Creates a new node out of some data.
 */
BANG_node* new_BANG_node(void *data);

/**
 * \param node The node whose resources to free.
 * \param free_data The function that will free the data.
 *
 * \brief Deallocates a bang node.
 */
void free_BANG_node(BANG_node *node, void(*free_data)(void*));

/**
 * \param node The node which to remove from the list.
 * \param data The place to put the data that was contained in the node.
 *
 * \return The node that is now occupying the space that node used to be
 * in.
 *
 * \brief Removes a node from the list, giving the data back, and returning
 * the next position in the list.
 */
BANG_node* BANG_node_extract_data(BANG_node *node, void **data);

/**
 * \brief Creates a linked list.
 */
BANG_linked_list* new_BANG_linked_list();

/**
 * \param lst Linked list to free.
 * \param free_data The function that will free data in the list.
 *
 * \brief Frees a linked list.
 */
void free_BANG_linked_list(BANG_linked_list *lst, void(*free_data)(void*));

/**
 * \param lst A valid linked list.
 *
 * \return The first element of the list.
 *
 * \brief Removes and returns the first element of the list.
 */
void* BANG_linked_list_pop(BANG_linked_list *lst);

/**
 * \param lst A valid linked list.
 *
 * \return The last data of the linked list.
 *
 * \brief Takes off the last node of the linked list.
 */
void* BANG_linked_list_dequeue(BANG_linked_list *lst);

/**
 * \param lst A valid linked list.
 *
 * \return The first element of the list.
 *
 * \brief Returns the first element of the list.
 */
void* BANG_linked_list_peek(BANG_linked_list *lst);

/**
 * \param lst A valid linked list.
 * \param data The data to add to the list.
 *
 * \brief Puts data on top of the linked list.
 */
void BANG_linked_list_push(BANG_linked_list *lst, void *data);

/**
 * \param lst A valid linked list.
 * \param data The data to add to the list.
 *
 * \brief Puts the data at the end of the list.
 */
void BANG_linked_list_append(BANG_linked_list *lst, void *data);

/**
 * \param lst A valid linked list.
 *
 * \return The number of elements in the linked list.
 *
 * \brief Returns the number of elements in the linked list.
 */
size_t BANG_linked_list_get_size(BANG_linked_list *lst);

/**
 * \param lst The list to iterate over.
 * \param it_callback The callback to call for every node.
 * \param The extra data to call the callback with.
 *
 * \brief Iterates over a linked calling a function back with the node data and provided
 * data, if any.
 */
void BANG_linked_list_iterate(BANG_linked_list *lst, void (*it_callback) (const void*,void*), void* data);

/**
 * \param lst The list to iterate over.
 * \param it_callback The callback to call for every node.
 * \param The extra data to call the callback with.
 *
 * \brief Iterates over a linked calling a function back with the node data, node number, and provided
 * data, if any.
 */
void BANG_linked_list_enumerate(BANG_linked_list *lst, void (*it_callback) (const void*,void*,int), void* data);

/**
 * \param lst The list to iterate over.
 * \param it_callback The callback to call for every node.
 * \param The extra data to call the callback with.
 *
 * \brief Iterates over a linked calling a function back with the node data and provided
 * data, if any.  If the it_callback returns 0, it will stop iterating, if it returns
 * 1, it will continue.
 */
int BANG_linked_list_conditional_iterate(BANG_linked_list *lst, int (*it_callback) (const void*,void*), void *data);

/**
 * \brief Allocates and returns a BANG set.
 *
 * \return A newly intialized BANG set.
 */
BANG_set* new_BANG_set();

/**
 * \param set The set to get rid of.
 *
 * \brief Dealloctes a BANG set.
 */
void free_BANG_set(BANG_set *set);

int BANG_set_add(BANG_set *s, void *data);

void* BANG_set_get(BANG_set *s, int key);

void* BANG_set_remove(BANG_set *s, int key);

void BANG_set_iterate(BANG_set *s, void (it_callback) (void*, void*), void *data);

BANG_hashmap*  new_BANG_hashmap(BANG_hashcode hash_func, BANG_compare comp_func);

void free_BANG_hashmap(BANG_hashmap *hashmap);

void* BANG_hashmap_get(BANG_hashmap *hashmap, void *key);

void BANG_hashmap_set(BANG_hashmap *hashmap, void *key, void *item);

BANG_hashmap_pair* new_BANG_hashmap_pair(void *item, void *key);

/**
 * \param v1 A bang version.
 * \param v2 Another bang version.
 *
 * \return A comparison of the two versions.
 *
 * \brief Compares the two versions, 0 if equal, - if less than, + if greater than.
 */
int BANG_version_cmp(const unsigned char *v1, const unsigned char *v2);

/**
 * \param m1 The first module name to compare.
 * \param m2 The second module name to compare.
 *
 * \return It will return a negative number for less than,
 * a positive number for greater than, and 0 for equal to.
 *
 * \brief Compares two module names similar to strcmp.
 * A module name is described as char* containing:
 * | char* name | \0 | 3 byte version
 */
int BANG_module_name_cmp(const char *m1, const char *m2);

/**
 * \param lck A lock used for reading-writing syncing.
 *
 * \brief Locks the lock for reading.
 */
void BANG_read_lock(BANG_rw_syncro *lck);

/**
 * \param lck A lock used for reading-writing syncing.
 *
 * \brief Unlocks the lock after reading.
 */
void BANG_read_unlock(BANG_rw_syncro *lck);

/**
 * \param lck A lock used for reading-writing syncing.
 *
 * \brief Locks the lock for writing.
 */
void BANG_write_lock(BANG_rw_syncro *lck);

/**
 * \param lck A lock used for reading-writing syncing.
 *
 * \brief unlocks the lock after writing.
 */
void BANG_write_unlock(BANG_rw_syncro *lck);

/**
 * \return A new BANG_rw_syncro pointer.
 *
 * \brief Allocates and sets up a BANG_rw_syncro
 * object.
 */
BANG_rw_syncro* new_BANG_rw_syncro();

/**
 * \param lck A that is not being used.
 *
 * \brief Frees and destroys a lock.
 */
void free_BANG_rw_syncro(BANG_rw_syncro *lck);

/**
 * \brief Locks a mutex for reading given the associated reading lock, and pointer to readers.
 */
void BANG_acquire_read_lock(int *readers, pthread_mutex_t *readers_lock, pthread_mutex_t *writers_lock);

/**
 * \brief Unlocks a mutex for reading given the associated reading lock, and pointer to readers.
 */
void BANG_release_read_lock(int *readers, pthread_mutex_t *readers_lock, pthread_mutex_t *writers_lock);
#endif
