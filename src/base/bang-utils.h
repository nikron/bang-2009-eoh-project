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

int BANG_module_name_cmp(const char *m1, const char *m2);

void BANG_read_lock(BANG_rw_syncro *lck);
void BANG_read_unlock(BANG_rw_syncro *lck);
void BANG_write_lock(BANG_rw_syncro *lck);
void BANG_write_unlock(BANG_rw_syncro *lck);
BANG_rw_syncro* new_BANG_rw_syncro();
void free_BANG_rw_syncro(BANG_rw_syncro *lck);

void BANG_acquire_read_lock(int *readers, pthread_mutex_t *readers_lock, pthread_mutex_t *writers_lock);
void BANG_release_read_lock(int *readers, pthread_mutex_t *readers_lock, pthread_mutex_t *writers_lock);
#endif
