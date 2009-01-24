#include"bang-utils.h"
#include<stdlib.h>
#include<string.h>

void BANG_read_lock(BANG_rw_syncro *lck) {
	BANG_acquire_read_lock(&(lck->readers),lck->read,lck->write);
}

void BANG_read_unlock(BANG_rw_syncro *lck) {
	BANG_release_read_lock(&(lck->readers),lck->read,lck->write);
}

void BANG_write_lock(BANG_rw_syncro *lck) {
	pthread_mutex_lock(lck->write);
}

void BANG_write_unlock(BANG_rw_syncro *lck) {
	pthread_mutex_unlock(lck->write);
}

BANG_rw_syncro* new_BANG_rw_syncro() {
	BANG_rw_syncro *new = malloc(sizeof(BANG_rw_syncro));
	new->read = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(new->read,NULL);
	new->write = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(new->write,NULL);
	new->readers = 0;

	return new;
}
void free_BANG_rw_syncro(BANG_rw_syncro *lck) {
	pthread_mutex_unlock(lck->read);
	pthread_mutex_destroy(lck->read);
	free(lck->read);
	pthread_mutex_unlock(lck->write);
	pthread_mutex_destroy(lck->write);
	free(lck->write);
	free(lck);
}

int BANG_version_cmp(const unsigned char *v1, const unsigned char *v2) {
	int cmp = 0;

	cmp += v1[0] - v2[0];
	cmp += v1[1] - v2[1];
	cmp += v1[2] - v2[2];

	return cmp;
}

int BANG_module_name_cmp(const char *m1, const char *m2) {
	int cmp = strcmp(m1,m2);
	int length = strlen(m1) + 1,
	    length2 = strlen(m2) + 1;
	cmp += BANG_version_cmp(((unsigned char*)m1) + length, ((unsigned char*)m2) + length2);
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
