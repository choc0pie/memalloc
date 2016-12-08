#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>

struct header_t {
	size_t size;
	unsigned freemem;
	struct header_t *next;
};

struct header_t *head = NULL, *lst = NULL;
pthread_mutex_t global_malloc_lock;

struct header_t *get_free_block(size_t size)
{
	struct header_t *curr = head;
	while(curr) {
		if (curr->freemem && curr->size >= size)
			return curr;
		curr = curr->next;
	}
	return NULL;
}

void myfree(void *block)
{
	struct header_t *header, *tmp;

	if (!block)
		return;
	pthread_mutex_lock(&global_malloc_lock);
	header = (struct header_t*)block - 1;
	


	if ((char*)block + header->size == sbrk(0)) {
		if (head == lst) {
			head = lst = NULL;
		} else {
			tmp = head;
			while (tmp) {
				if(tmp->next == lst) {
					tmp->next = NULL;
					lst = tmp;
				}
				tmp = tmp->next;
			}
		}

		sbrk(0 - header->size - sizeof(struct header_t));

		pthread_mutex_unlock(&global_malloc_lock);
		return;
	}
	header->freemem = 1;
	pthread_mutex_unlock(&global_malloc_lock);
}

void *mymalloc(size_t size)
{
	size_t total_size;
	void *block;
	struct header_t *header;
	if (!size)
		return NULL;
	pthread_mutex_lock(&global_malloc_lock);
	header = get_free_block(size);
	if (header) {
		header->freemem = 0;
		pthread_mutex_unlock(&global_malloc_lock);
		return (void*)(header + 1);
	}
	total_size = sizeof(struct header_t) + size;
	block = sbrk(total_size);
	if (block == (void*) -1) {
		pthread_mutex_unlock(&global_malloc_lock);
		return NULL;
	}
	header = block;
	header->size = size;
	header->freemem = 0;
	header->next = NULL;
	if (!head)
		head = header;
	if (lst)
		lst->next = header;
	lst = header;
	pthread_mutex_unlock(&global_malloc_lock);
	return (void*)(header + 1);
}

void *mycalloc(size_t num, size_t nsize)
{
	size_t size;
	void *block;
	if (!num || !nsize)
		return NULL;
	size = num * nsize;
	if (nsize != size / num)
		return NULL;
	block = mymalloc(size);
	if (!block)
		return NULL;
	memset(block, 0, size);
	return block;
}

void *myrealloc(void *block, size_t size)
{
	struct header_t *header;
	void *ret;
	if (!block || !size)
		return mymalloc(size);
	header = (struct header_t*)block - 1;
	if (header->size >= size)
		return block;
	ret = mymalloc(size);
	if (ret) {
		memcpy(ret, block, header->size);
		myfree(block);
	}
	return ret;
}


