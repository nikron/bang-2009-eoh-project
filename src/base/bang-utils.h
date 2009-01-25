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
 * An object to do read - writing sycing
 */
typedef struct {
	pthread_mutex_t *read;
	pthread_mutex_t *write;
	int readers;
} BANG_rw_syncro;

typedef struct _BANG_node{
	struct _BANG_node *prev;
	struct _BANG_node *next;
	void *data;
} BANG_node;

typedef struct {
	BANG_node *head;
	BANG_node *tail;
	size_t size;
} BANG_linked_list;

/**
 * \param data The information to be included in the node.
 *
 * \brief Creates a new node out of some data.
 */
BANG_node* new_BANG_node(void *data);

/**
 * \param node The node whoose resources to free.
 * \param free_data The function that will free the data.
 *
 * \brief Deallocates a bang node.
 */
void free_BANG_node(BANG_node *node, void(*free_data)(void*));

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
 * \param lst A valid linked lst.
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
 * \param v1 A bang version.
 * \param v2 Another bang version.
 *
 * \return A comparsion of the two versions.
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
 * \brief Compares two module names similiar to strcmp.
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

void BANG_acquire_read_lock(int *readers, pthread_mutex_t *readers_lock, pthread_mutex_t *writers_lock);
void BANG_release_read_lock(int *readers, pthread_mutex_t *readers_lock, pthread_mutex_t *writers_lock);
#endif
