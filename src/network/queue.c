/*
 * Copyright (c) 2022-2024 Technical University of Munich (TUM)
 *
 * Additional copyrights may follow
 *
 */

#include "dynpm_network.h"

dynpm_work_queue_t work_queue;

static void *queue_thread(void *ptr) {
	dynpm_work_unit_t *work_unit;
	worker_thread_data_t *worker_thread_data = (worker_thread_data_t *)ptr;

	char thread_name[16]; // pthread limits this to 16
	if(pthread_getname_np(worker_thread_data->thread,
				thread_name, 16)){
		error_logger("could not get thread name");
	}

	pthread_detach(pthread_self());

	while (1) {
		pthread_mutex_lock(&worker_thread_data->work_queue->mutex);

		while (worker_thread_data->work_queue->work_units == NULL) {
			if (worker_thread_data->terminate) break;
			pthread_cond_wait(&worker_thread_data->work_queue->cond,
					&worker_thread_data->work_queue->mutex);
		}

		if (worker_thread_data->terminate) {
			info_logger("breaking under terminate at worker %s", thread_name);
			pthread_mutex_unlock(&worker_thread_data->work_queue->mutex);
			break;
		}

		work_unit = worker_thread_data->work_queue->work_units;
		worker_thread_data->work_queue->work_units = work_unit->next;
		work_unit->next = NULL;

		pthread_mutex_unlock(&worker_thread_data->work_queue->mutex);

		if (work_unit == NULL){
			error_logger("work_unit was NULL at worker %s", thread_name);
			continue;
		}

		work_unit->function(work_unit->data);
	}

	free(worker_thread_data);
	pthread_exit(NULL);
}

int dynpm_work_queue_init(int thread_count) {
	int i;
	worker_thread_data_t *worker_thread_data;
	char thread_name[16]; // pthread limits this to 16

	if(thread_count < 1){
		error_logger("invalid size in dynpm_work_queue_init");
		return -1;
	}

	if(pthread_mutex_init(&(work_queue.mutex), NULL)){
		error_logger("unable to initialize work_queue mutex");
		return -1;
	}
	if(pthread_cond_init(&(work_queue.cond), NULL)){
		error_logger("unable to initialize work_queue cond");
		return -1;
	}

	work_queue.work_units = NULL;
	work_queue.thread_data = NULL;

	for (i = 0; i < thread_count; i++) {
		if ((worker_thread_data = malloc(sizeof(worker_thread_data_t))) == NULL) {
			error_logger("failed to allocate thread data");
			return 1;
		}

		worker_thread_data->work_queue = &work_queue;
		worker_thread_data->terminate = 0;

		if (pthread_create(&worker_thread_data->thread, NULL,
					queue_thread, (void *)worker_thread_data)) {
			error_logger("failed to create threads");
			free(worker_thread_data);
			return 1;
		}

		sprintf(thread_name, "queuethrd%d", i);
		if(pthread_setname_np(worker_thread_data->thread, thread_name)){
			error_logger("failed to set name of listener thread");
		}

		worker_thread_data->next = worker_thread_data->work_queue->thread_data;
		worker_thread_data->work_queue->thread_data = worker_thread_data;
	}

	return 0;
}

void dynpm_work_queue_finalize(){
	worker_thread_data_t *worker_thread_data = NULL;

	pthread_mutex_lock(&(work_queue.mutex));

	for (worker_thread_data = work_queue.thread_data;
			worker_thread_data != NULL;
			worker_thread_data = worker_thread_data->next) {
		worker_thread_data->terminate = 1;
	}

	work_queue.work_units = NULL;
	work_queue.thread_data = NULL;

	pthread_cond_broadcast(&(work_queue.cond));
	pthread_mutex_unlock(&(work_queue.mutex));
}

void dynpm_work_queue_add(dynpm_work_unit_t *work_unit){
	pthread_mutex_lock(&(work_queue.mutex));

	if(work_queue.work_units != NULL){
		dynpm_work_unit_t *current = work_queue.work_units;
		while (current->next != NULL){
			current = current->next;
		} // reach the tail and add the work unit there
		current->next = work_unit;
	} else {
		work_queue.work_units = work_unit;
	}

	work_unit->next = NULL;

	pthread_cond_signal(&(work_queue.cond));
	pthread_mutex_unlock(&(work_queue.mutex));
}
