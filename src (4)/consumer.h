/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef __SO_CONSUMER_H__
#define __SO_CONSUMER_H__

#include "ring_buffer.h"
#include "packet.h"

typedef struct Response {
	char action[4];
	unsigned long hash;
	unsigned long timestamp;
} Response;

typedef struct Min_Heap {
	Response *data;
	int size;
	int capacity;
	pthread_mutex_t mutex;
} Min_Heap;

extern Response *response;

extern Min_Heap *shared_min_heap;

extern int out_fd;

typedef struct so_consumer_ctx_t {
	struct so_ring_buffer_t *producer_rb;

    /* TODO: add synchronization primitives for timestamp ordering */
	int output_fd;
} so_consumer_ctx_t;

void init_min_heap(Min_Heap *min_heap, int initial_capacity);
void insert_min_heap(Min_Heap *min_heap, Response pkt);
Response extract_min(Min_Heap *min_heap);
void min_heap_free(Min_Heap *min_heap);
void min_heap_resize(Min_Heap *min_heap);
void heapify_down(Min_Heap *min_heap, int index);
void heapify_up(Min_Heap *min_heap, int index);
void carnat(int a, int b);

int create_consumers(pthread_t *tids,
					int num_consumers,
					so_ring_buffer_t *rb,
					const char *out_filename);


#endif /* __SO_CONSUMER_H__ */
