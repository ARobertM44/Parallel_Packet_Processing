// SPDX-License-Identifier: BSD-3-Clause

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>

#include "ring_buffer.h"
#include "consumer.h"
#include "producer.h"
#include "log/log.h"
#include "packet.h"
#include "utils.h"

#define SO_RING_SZ		(PKT_SZ * 1000)

pthread_mutex_t MUTEX_LOG;

void log_lock(bool lock, void *udata)
{
	pthread_mutex_t *LOCK = (pthread_mutex_t *) udata;

	if (lock)
		pthread_mutex_lock(LOCK);
	else
		pthread_mutex_unlock(LOCK);
}

void __attribute__((constructor)) init()
{
	pthread_mutex_init(&MUTEX_LOG, NULL);
	log_set_lock(log_lock, &MUTEX_LOG);
}

void __attribute__((destructor)) dest()
{
	pthread_mutex_destroy(&MUTEX_LOG);
}

int main(int argc, char **argv)
{
	shared_min_heap = malloc(sizeof(Min_Heap));
	response = malloc(sizeof(Response));
	init_min_heap(shared_min_heap, 1250);
	so_ring_buffer_t ring_buffer;
	int num_consumers, threads, rc;
	pthread_t *thread_ids = NULL;

	if (argc < 4) {
		fprintf(stderr, "Usage %s <input-file> <output-file> <num-consumers:1-32>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	rc = ring_buffer_init(&ring_buffer, SO_RING_SZ);
	DIE(rc < 0, "ring_buffer_init");

	num_consumers = strtol(argv[3], NULL, 10);

	if (num_consumers <= 0 || num_consumers > 32) {
		fprintf(stderr, "num-consumers [%d] must be in the interval [1-32]\n", num_consumers);
		exit(EXIT_FAILURE);
	}

	thread_ids = calloc(num_consumers, sizeof(pthread_t));
	DIE(thread_ids == NULL, "calloc pthread_t");

	/* create consumer threads */
	create_consumers(thread_ids, num_consumers, &ring_buffer, argv[2]);

	/* start publishing data */
	publish_data(&ring_buffer, argv[1]);

	/* TODO: wait for child threads to finish execution*/
	for (int i = 0; i < num_consumers; i++)
		pthread_join(thread_ids[i], NULL);
	free(thread_ids);
	// using a min_heap, the packets are sorted in real time
	// now we just have to write the sorted output
	while (shared_min_heap->size > 0) {
		Response pkt = extract_min(shared_min_heap);
		char buffer[256];
		int len = snprintf(buffer, 256, "%s %016lx %lu\n",
							pkt.action, pkt.hash, pkt.timestamp);
		write(out_fd, buffer, len);
}
	close(out_fd);
	min_heap_free(shared_min_heap);
	return 0;
}

