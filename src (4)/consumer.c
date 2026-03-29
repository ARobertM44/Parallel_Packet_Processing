// SPDX-License-Identifier: BSD-3-Clause

#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include "consumer.h"
#include "ring_buffer.h"
#include "packet.h"
#include "utils.h"

int out_fd;
Response *response;
Min_Heap *shared_min_heap;

void init_min_heap(Min_Heap *min_heap, int initial_capacity)
{
	min_heap->data = (Response *)malloc(initial_capacity * sizeof(Response));
	min_heap->size = 0;
	min_heap->capacity = initial_capacity;
	pthread_mutex_init(&min_heap->mutex, NULL);
}

void min_heap_resize(Min_Heap *min_heap)
{
	min_heap->capacity = 2 * min_heap->capacity;
	min_heap->data = (Response *)realloc(min_heap->data, min_heap->capacity * sizeof(Response));
}

void heapify_up(Min_Heap *min_heap, int index)
{
	if (index == 0)
		return;
	int parent = (index - 1) / 2;

	if (min_heap->data[index].timestamp < min_heap->data[parent].timestamp) {
		Response temp = min_heap->data[index];

		min_heap->data[index] = min_heap->data[parent];
		min_heap->data[parent] = temp;
		heapify_up(min_heap, parent);
	}
}

void heapify_down(Min_Heap *min_heap, int index)
{
	int left_child = 2 * index + 1;
	int right_child = 2 * index + 2;
	int smallest = index;

	if (left_child < min_heap->size && min_heap->data[left_child].timestamp < min_heap->data[smallest].timestamp)
		smallest = left_child;
	if (right_child < min_heap->size && min_heap->data[right_child].timestamp < min_heap->data[smallest].timestamp)
		smallest = right_child;
	if (smallest != index) {
		Response temp = min_heap->data[index];

		min_heap->data[index] = min_heap->data[smallest];
		min_heap->data[smallest] = temp;
		heapify_down(min_heap, smallest);
	}
}

void insert_min_heap(Min_Heap *min_heap, Response pkt)
{
	pthread_mutex_lock(&min_heap->mutex);
	if (min_heap->size >= min_heap->capacity / 2)
		min_heap_resize(min_heap);
	min_heap->data[min_heap->size] = pkt;
	min_heap->size++;
	heapify_up(min_heap, min_heap->size - 1);
	pthread_mutex_unlock(&min_heap->mutex);
}

Response extract_min(Min_Heap *min_heap)
{
	pthread_mutex_lock(&min_heap->mutex);
	Response min = min_heap->data[0];

	min_heap->data[0] = min_heap->data[min_heap->size - 1];
	min_heap->size--;
	heapify_down(min_heap, 0);
	pthread_mutex_unlock(&min_heap->mutex);
	return min;
}

void min_heap_free(Min_Heap *min_heap)
{
	free(min_heap->data);
	pthread_mutex_destroy(&min_heap->mutex);
}

void consumer_thread(so_consumer_ctx_t *ctx)
{
	/* TODO: implement consumer thread */
	so_packet_t buffer[PKT_SZ];
	so_packet_t out_buffer[PKT_SZ];
	ssize_t sz;
	int k = 0;

	while (1) {
		sz = ring_buffer_dequeue(ctx->producer_rb, buffer, PKT_SZ);
		if (sz == -1)
			break;
		Response rresponse;
		so_packet_t *pkt = buffer;
		int action = process_packet(pkt);
		unsigned long hash = packet_hash(pkt);
		unsigned long timestamp = pkt->hdr.timestamp;

		if (action)
			memcpy(rresponse.action, "PASS", 4);
		else
			memcpy(rresponse.action, "DROP", 4);
		rresponse.hash = hash;
		rresponse.timestamp = timestamp;
		k++;
		insert_min_heap(shared_min_heap, rresponse);
	}
}

int create_consumers(pthread_t *tids,
					 int num_consumers,
					 struct so_ring_buffer_t *rb,
					 const char *out_filename)
{
	out_fd = open(out_filename, O_WRONLY | O_CREAT | O_APPEND, 0666);
	for (int i = 0; i < num_consumers; i++) {
		so_consumer_ctx_t *ctx = malloc(sizeof(so_consumer_ctx_t));

		ctx->producer_rb = rb;
		ctx->output_fd = out_fd;
		pthread_create(&tids[i], NULL, (void *)consumer_thread, ctx);
	}
	return num_consumers;
}
