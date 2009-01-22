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
