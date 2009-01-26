#include"bang-utils.h"
#include"bang-types.h"
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

BANG_node* new_BANG_node(void *data) {
	BANG_node *node = calloc(1,sizeof(BANG_node));

	node->data = data;

	return node;
}


void free_BANG_node(BANG_node *node, void(*free_data)(void*)) {
	if (node) {
		free_data(node->data);
	}
	free(node);
}

BANG_linked_list* new_BANG_linked_list() {
	/* calloc sets everything to 0 */
	BANG_linked_list *new = calloc(1,sizeof(BANG_linked_list));
	new->lck = new_BANG_rw_syncro();

	return new;
}

BANG_node* BANG_node_extract_data(BANG_node *node, void **data) {
	*data = node->data;

	if (node->prev != NULL) {
		if (node->next != NULL) {
			node->prev->next  = node->next;
			node->next->prev = node->prev;
		} else {
			node->prev->next = NULL;
		}
	}

	if (node->next != NULL) {
		if (node->prev != NULL) {
			node->next->prev = node->prev;
			node->prev->next = node->next;
		} else {
			node->next->prev = NULL;
		}
	}

	BANG_node *to_ret = (node->next != NULL) ? node->next : node->prev;

	free(node);

	return to_ret;
}

void free_BANG_linked_list(BANG_linked_list *lst, void(*free_data)(void*)) {
	if (lst == NULL) return;

	BANG_write_lock(lst->lck);

	BANG_node *node = lst->head, *temp;
	while (node) {
		temp = node->next;
		free_BANG_node(node,free_data);
		node = temp;
	}

	BANG_write_unlock(lst->lck);

	free_BANG_rw_syncro(lst->lck);
	free(lst);
}

void* BANG_linked_list_pop(BANG_linked_list *lst) {
	if (lst == NULL || lst->head == NULL || lst->tail == NULL) return NULL;

	BANG_write_lock(lst->lck);

	void *data;

	lst->head = BANG_node_extract_data(lst->head,&data);

	if (lst->head == NULL) {
		lst->tail = NULL;
	}

	lst->size--;

	BANG_write_unlock(lst->lck);

	return data;
}

void* BANG_linked_list_dequeue(BANG_linked_list *lst) {
	if (lst == NULL || lst->head == NULL || lst->tail == NULL) return NULL;

	BANG_write_lock(lst->lck);

	void *data;

	lst->tail = BANG_node_extract_data(lst->head,&data);

	if (lst->tail == NULL) {
		lst->head = NULL;
	}

	lst->size--;

	BANG_write_unlock(lst->lck);

	return data;
}

void BANG_linked_list_push(BANG_linked_list *lst, void *data) {
	if (lst == NULL) return;

	BANG_write_lock(lst->lck);

	BANG_node *node = new_BANG_node(data);

	if (lst->head && lst->tail) {
		lst->head->prev = node;
		node->next = lst->head;
		lst->tail = node;
	} else {
		lst->head = node;
		lst->tail = node;
	}

	lst->size++;

	BANG_write_unlock(lst->lck);
}

void BANG_linked_list_append(BANG_linked_list *lst, void *data) {
	if (lst == NULL) return;

	BANG_write_lock(lst->lck);

	BANG_node *node = new_BANG_node(data);

	if (lst->head && lst->tail) {
		lst->tail->next = node;
		node->prev = lst->tail;
		lst->tail = node;
	} else {
		lst->head = node;
		lst->tail = node;
	}

	lst->size++;

	BANG_write_unlock(lst->lck);
}

size_t BANG_linked_list_get_size(BANG_linked_list *lst) {
	return lst->size;
}

BANG_request* new_BANG_request(int type, void *data, int length) {
	BANG_request *new = calloc(1,sizeof(BANG_request));

	new->type = type;
	new->request = data;
	new->length = length;

	return new;
}

void free_BANG_request(void *req) {
	BANG_request *r = req;
	free(r->request);
	free(r);
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
