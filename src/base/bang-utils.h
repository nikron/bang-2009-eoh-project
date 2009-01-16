#ifndef __BANG_UTILS_H
#define __BANG_UTILS_H
#include<pthread.h>
int BANG_module_name_cmp(const char *m1, const char *m2);
void BANG_acquire_read_lock(int *readers, pthread_mutex_t *readers_lock, pthread_mutex_t *writers_lock);
void BANG_release_read_lock(int *readers, pthread_mutex_t *readers_lock, pthread_mutex_t *writers_lock);
#endif 
