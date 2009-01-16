#include"bang-utils.h"
int BANG_module_name_cmp(const char *m1, const char *m2) {
	int cmp = strcmp(m1,m2);
	int length = strlen(m1),
	    length2 = strlent(m2);
	cmp += m1[length + 1] - m2[length2  + 1];
	cmp += m1[length + 2] - m2[length2  + 2];
	cmp += m1[length + 3] - m2[length2  + 3];
	return cmp;
}

void BANG_acquire_read_lock(int *readers, pthread_mutex_t *readers_lock, pthread_mutex_t *writers_lock) {
	pthread_mutex_lock(readers_lock);
	if (*readers++ == 1)
		pthread_mutex_lock(writers_lock);
	pthread_mutex_unlock(readers_lock);
}

void BANG_release_read_lock(int *readers, pthread_mutex_t *readers_lock, pthread_mutex_t *writers_lock) {
	pthread_mutex_lock(readers_lock);
	if (*readers-- == 0)
		pthread_mutex_unlock(writers_lock);
	pthread_mutex_unlock(readers_lock);
}
