#include"bang-utils.h"
#include"bang-types.h"
#include<stdio.h>
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
	if (node && free_data) {
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
		node->prev->next  = node->next;

		if (node->next != NULL) {
			node->next->prev = node->prev;
		}
	}

	if (node->next != NULL) {
		node->next->prev = node->prev;

		if (node->prev != NULL) {
			node->prev->next = node->next;
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
		lst->head = node;
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
	if (lst == NULL) return 0;
	else return lst->size;
}

void BANG_linked_list_iterate(BANG_linked_list *lst, void (*it_callback) (const void*,void*), void* data) {
	if (lst == NULL || it_callback == NULL) return;

	BANG_node *node;
	BANG_read_lock(lst->lck);

	for (node = lst->head; node != NULL; node = node->next) {
		it_callback(node->data,data);
	}

	BANG_read_unlock(lst->lck);
}

void BANG_linked_list_enumerate(BANG_linked_list *lst, void (*it_callback) (const void*,void*,int), void* data) {
	if (lst == NULL || it_callback == NULL) return;

	BANG_node *node;
	int i;
	BANG_read_lock(lst->lck);

	for (i = 0, node = lst->head; node != NULL; i++, node = node->next) {
		it_callback(node->data,data,i);
	}

	BANG_read_unlock(lst->lck);
}

int BANG_linked_list_conditional_iterate(BANG_linked_list *lst, int (*it_callback) (const void*,void*), void *data) {
	if (lst == NULL || it_callback == NULL) return 0;

#ifdef BDEBUG_1
	fprintf(stderr, "Starting a conditional iterate.\n");
#endif

	BANG_node *node;
	BANG_read_lock(lst->lck);

	for (node = lst->head; node != NULL; node = node->next) {
#ifdef BDEBUG_1
	fprintf(stderr, "Calling a callback on a node of the list.\n");
#endif
		if(it_callback(node->data,data) == 0) {
			BANG_read_unlock(lst->lck);
			return 0;
		}
	}

	BANG_read_unlock(lst->lck);

#ifdef BDEBUG_1
	fprintf(stderr, "Did not stop a conditional iterate.\n");
#endif

	return 1;
}

#define L_SHIFT 16
#define L_MASK 0x0000ffff

BANG_set* new_BANG_set() {
	BANG_set *new = malloc(sizeof(BANG_set));

	new->lck = new_BANG_rw_syncro();
	new->size = 2;
	new->current = 0;
	new->count = 0;
	new->members = calloc(new->size,sizeof(BANG_set_data));
	new->free_space = new_BANG_linked_list();

	return new;
}

/* TODO: Make this do something valid... */
void free_BANG_set(BANG_set *s) {
	free_BANG_rw_syncro(s->lck);
	free(s->members);
	free_BANG_linked_list(s->free_space,NULL);
	free(s);
}

int BANG_set_add(BANG_set *s, void *data) {
	if (s == NULL) return -1;

	int *free, key, pos;

	BANG_write_lock(s->lck);

	free = BANG_linked_list_pop(s->free_space);

	pos = (free) ? *free : s->current++;

	key = (pos << L_SHIFT) |  ++(s->count);


	if ((unsigned int) s->current > s->size) {
		s->size *= 2;
		s->members = realloc(s->members,s->size * sizeof(BANG_set_data));
	}

	s->members[pos].key = s->count;
	s->members[pos].data = data;

	BANG_write_unlock(s->lck);

	return key;
}

void* BANG_set_get(BANG_set *s, int key) {
	if (s == NULL) return NULL;;

	BANG_read_lock(s->lck);

	int pos = key >> L_SHIFT;

	if (pos > s->current || (s->members[pos].key != (key & L_MASK))) {
		BANG_read_unlock(s->lck);

		return NULL;
	}

	void *data = s->members[pos].data;

	BANG_read_unlock(s->lck);

	return data;

}

void* BANG_set_remove(BANG_set *s, int key) {
	if (s == NULL) return NULL;

	int pos = key >> L_SHIFT;
	void *data = BANG_set_get(s,key);

	if (data == NULL) return NULL;

	BANG_write_lock(s->lck);

	s->members[pos].key = -1;
	s->members[pos].data = NULL;
	s->current--;
	BANG_linked_list_push(s->free_space,&pos);

	BANG_write_unlock(s->lck);

	return data;
}

void BANG_set_iterate(BANG_set *s, void (it_callback) (void*, void*), void *data) {
	int i = 0;
	BANG_read_lock(s->lck);
	for(; i < s->current; ++i) {
		if(s->members[i].key != -1) {
			it_callback(s->members[i].data,data);
		}
	}
	BANG_read_unlock(s->lck);
}

#define HASHMAP_DEFAULT_SIZE 100

BANG_hashmap*  new_BANG_hashmap(BANG_hashcode hash_func, BANG_compare comp_func) {
	BANG_hashmap *new = malloc(sizeof(BANG_hashmap));

#ifdef BDEBUG_1
	fprintf(stderr,"Creating a new hashmap.\n");
#endif
	int i;

	new->data = calloc(HASHMAP_DEFAULT_SIZE,sizeof(BANG_linked_list*));
	new->data_size = HASHMAP_DEFAULT_SIZE;

	for (i = 0; i < new->data_size; ++i) {
		new->data[i] = new_BANG_linked_list();
	}

	new->hash_func = hash_func;
	new->compare_func = comp_func;

	return new;
}

void free_BANG_hashmap(BANG_hashmap *hashmap) {
	int i = 0;

	for (; i < hashmap->data_size; ++i) {
		free_BANG_linked_list(hashmap->data[i],NULL);
	}

	free(hashmap);
}

typedef struct {
	BANG_compare comp_func;
	BANG_hashmap_pair *pair;
} find_item_t;

static int find_item(const void *item, void *kp) {
	find_item_t *fi = kp;
	BANG_hashmap_pair *bhp = (void*) item;

#ifdef BDEBUG_1
	fprintf(stderr,"Looping over items in a hashmap trying to find an item.\n");
#endif

	if (fi->comp_func(bhp->key,fi->pair->key) == 0) {
		fi->pair->item = bhp->item;

		return 0;
	}

	return 1;
}

static int find_item_set(const void *item, void *kp) {
	find_item_t *fi = kp;
	BANG_hashmap_pair *bhp = (void*) item;

#ifdef BDEBUG_1
	fprintf(stderr,"Looping over items in a hashmap trying to set an item.\n");
#endif

	if (fi->comp_func(bhp->key, fi->pair->key) == 0) {
		bhp->item = fi->pair->item;

		return 0;
	}

	return 1;
}

void* BANG_hashmap_get(BANG_hashmap *hashmap, void *key) {
	find_item_t fi;

	int key_hash = hashmap->hash_func(key);
	fi.comp_func = hashmap->compare_func;
	fi.pair = new_BANG_hashmap_pair(key,NULL);

	int pos = key_hash % hashmap->data_size;

#ifdef BDEBUG_1
	fprintf(stderr,"Hashmap get found pos %d which is pointer %p.\n", pos, hashmap->data[pos]);
#endif

	BANG_linked_list_conditional_iterate(hashmap->data[pos],&find_item,&fi);

	return fi.pair->item;
}

void BANG_hashmap_set(BANG_hashmap *hashmap, void *key, void *item){
	int pos = hashmap->hash_func(key) % hashmap->data_size;
#ifdef BDEBUG_1
	fprintf(stderr,"Found pos %d for item %p.\n", pos, item);
#endif

	find_item_t fi;
	fi.comp_func = hashmap->compare_func;
	fi.pair = new_BANG_hashmap_pair(key,item);

	if (BANG_linked_list_conditional_iterate(hashmap->data[pos],&find_item_set,&fi)) {
#ifdef BDEBUG_1
	fprintf(stderr,"Hashmap setting an item.\n");
#endif
		BANG_linked_list_push(hashmap->data[pos],fi.pair);
	}
}

BANG_hashmap_pair* new_BANG_hashmap_pair(void *key, void *item) {
	BANG_hashmap_pair *new = malloc(sizeof(BANG_hashmap_pair));

	new->key = key;
	new->item = item;

	return new;
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
	if (++(*readers) == 1)
		pthread_mutex_lock(writers_lock);
	pthread_mutex_unlock(readers_lock);
}

void BANG_release_read_lock(int *readers, pthread_mutex_t *readers_lock, pthread_mutex_t *writers_lock) {
	pthread_mutex_lock(readers_lock);
	if (--(*readers) == 0)
		pthread_mutex_unlock(writers_lock);
	pthread_mutex_unlock(readers_lock);
}
