/*
 * Copyright (c) 2022-2024 Technical University of Munich (TUM)
 *
 * Additional copyrights may follow
 *
 */

#include "dynpm_network.h"

void dynpm_compute_local_tbon_edges(int local_id, int degree, int total, int *parent_id, int **children_ids){
	int i;

	if(local_id == 0){
		*parent_id = -1;
	} else {
		*parent_id = (local_id -1)/degree;
	}
	//info_logger("local_id: %d, parent_id: %d", local_id, *parent_id);

	assert(children_ids != NULL); // needs to be allocated before the call

	int child_id;
	for(i = 0; i < degree; i++){
		child_id = (degree*local_id) + i + 1;
		if(child_id >= total){
			(*children_ids)[i] = -1;
		} else {
			(*children_ids)[i] = child_id;
		}
		//info_logger("child_id %d: %d", i, (*children_ids)[i]);
	}
}

void dynpm_free_tbon_vertex(dynpm_tbon_vertex_t **tbon_vertex){
	if(*tbon_vertex){
		if((*tbon_vertex)->nodes) free((*tbon_vertex)->nodes);
		if((*tbon_vertex)->local_host) free((*tbon_vertex)->local_host);
		if((*tbon_vertex)->children_ids) free((*tbon_vertex)->children_ids);
		if((*tbon_vertex)->children_hosts) free((*tbon_vertex)->children_hosts);
		if((*tbon_vertex)->children_connection_indexes) free((*tbon_vertex)->children_connection_indexes);
		if((*tbon_vertex)->process_data_reports){
			for(int i = 0; i < (*tbon_vertex)->total_children; i++){
				if((*tbon_vertex)->process_data_reports[i].payload){
					free((*tbon_vertex)->process_data_reports[i].payload);
				}
			}
			free((*tbon_vertex)->process_data_reports);
		}
		free((*tbon_vertex));
	} else { info_logger("attempting to free a NULL pointer"); }
}

void dynpm_initialize_tbon_vertex(dynpm_server_to_client_connection_t *connection,
		dynpm_create_tbon_msg_t *create_tbon, char **host_array,
		dynpm_tbon_vertex_t **tbon_vertex){

	(*tbon_vertex) = malloc(sizeof(dynpm_tbon_vertex_t));
	pthread_mutex_init(&((*tbon_vertex)->mutex), NULL);
	pthread_mutex_lock(&((*tbon_vertex)->mutex));

	pthread_mutex_init(&((*tbon_vertex)->io_mutex), NULL);
	pthread_cond_init(&((*tbon_vertex)->initialized_cond), NULL);
	pthread_cond_init(&((*tbon_vertex)->children_done_cond), NULL);
	pthread_cond_init(&((*tbon_vertex)->finalized_cond), NULL);

	(*tbon_vertex)->degree = create_tbon->degree;
	(*tbon_vertex)->nodes = strdup(create_tbon->nodes);
	(*tbon_vertex)->parent_connection = connection;

	(*tbon_vertex)->ready_count = 0;
	(*tbon_vertex)->children_done = 0;
	(*tbon_vertex)->total_children = 0;
	(*tbon_vertex)->initialized = 0;
	(*tbon_vertex)->finalized = 0;

	char hostname[128];
	if(gethostname(hostname, 128)) { info_logger("error getting hostname"); }
	(*tbon_vertex)->local_host = strdup(hostname);

	(*tbon_vertex)->total_nodes = create_tbon->total;

	for(int node_id = 0; node_id < create_tbon->total; node_id++){
		if(!strcmp((*tbon_vertex)->local_host, host_array[node_id])){
			//info_logger("local_id = %d (%s)", node_id, host_array[node_id]);
			(*tbon_vertex)->id = node_id;
		}
	}

	(*tbon_vertex)->children_ids   = malloc((*tbon_vertex)->degree * sizeof(int));
	(*tbon_vertex)->children_hosts = malloc((*tbon_vertex)->degree * sizeof(char *));
	(*tbon_vertex)->children_connection_indexes = malloc((*tbon_vertex)->degree * sizeof(int));
	// TODO process_data entry size should be configurable
	// TODO need to reconsider these entries once a first tool is developed
	(*tbon_vertex)->process_data_reports =
		malloc((*tbon_vertex)->degree * sizeof(dynpm_report_process_data_msg_t));
	for(int i = 0; i < (*tbon_vertex)->degree; i++){
		(*tbon_vertex)->process_data_reports[i].payload = NULL;
	}

	dynpm_compute_local_tbon_edges((*tbon_vertex)->id, (*tbon_vertex)->degree, (*tbon_vertex)->total_nodes,
			&((*tbon_vertex)->parent_id), &((*tbon_vertex)->children_ids));

	int child_index = 0;
	for(int node_id = 0; node_id < create_tbon->total; node_id++){
		if(node_id == (*tbon_vertex)->children_ids[child_index]){
			(*tbon_vertex)->children_hosts[child_index] = strdup(host_array[node_id]);
			//info_logger("child: %d; child_id: %d; host: %s",
			//		child_index, (*tbon_vertex)->children_ids[child_index],
			//		(*tbon_vertex)->children_hosts[child_index]);
			child_index++;
		} else {
			//info_logger("child id %d at index %d does not match node_id %d",
			//		(*tbon_vertex)->children_ids[child_index], child_index,
			//		node_id);
		}

		if(child_index >= (*tbon_vertex)->degree){
			//info_logger("already determined all children indexes based on a degree %d",
			//		(*tbon_vertex)->degree);
			break;
		}
	}
	(*tbon_vertex)->total_children = child_index;
	//info_logger("tbon_vertex->total_children = %d", (*tbon_vertex)->total_children);

	pthread_mutex_unlock(&((*tbon_vertex)->mutex));
}

int dynpm_send_message_to_tbon_children(
		dynpm_server_to_client_connection_t *parent_connection,
		const void *read_buffer,
		dynpm_source_type source_type,
		dynpm_message_type message_type,
		dynpm_tbon_vertex_t *tbon_vertex){

	// send the message down the tree to the children
	int messages_sent = 0;
	for(int child_id = 0; child_id < tbon_vertex->degree &&
			tbon_vertex->children_ids[child_id] != -1; child_id++){
		info_logger("sending message to child %d", child_id);

		if(dynpm_send_buffer_to_server(
					read_buffer,
					tbon_vertex->children_connection_indexes[child_id])
		  ){
			info_logger("error sending offer to child host %s",
					tbon_vertex->children_hosts[child_id]);
			return -1;
		} else {
			info_logger("sent offer to child host %s",
					tbon_vertex->children_hosts[child_id]);
		}
		messages_sent++;
	}
	info_logger("sent %d messages", messages_sent);

	return 0;
}

static void _create_time_offset(int seconds, int nanoseconds, struct timespec *tbon_wait_timeout){
		struct timeval now;
		gettimeofday(&now, NULL);
		tbon_wait_timeout->tv_sec = now.tv_sec + seconds;
		tbon_wait_timeout->tv_nsec = now.tv_usec + nanoseconds;
}

void dynpm_tbon_children_done_wait_timeout(dynpm_tbon_vertex_t *tbon_vertex, int seconds, int nanoseconds){
	if(tbon_vertex->total_children > 0){
		pthread_mutex_lock(&tbon_vertex->mutex);

		struct timespec tbon_wait_timeout;
		_create_time_offset(seconds, nanoseconds, &tbon_wait_timeout);

		if(tbon_vertex->children_done < tbon_vertex->total_children){
			info_logger("PARENT VERTEX: waiting for children done message");
			pthread_cond_timedwait(&(tbon_vertex->children_done_cond), &(tbon_vertex->mutex), &tbon_wait_timeout);
		} else {
			info_logger("PARENT VERTEX: children done was marked before waiting on cond");
		}
		pthread_mutex_unlock(&tbon_vertex->mutex);
		info_logger("done waiting for children done message");
	} else {
		info_logger("LEAF VERTEX: proceeding instead of waiting for children done");
	}
}

void dynpm_tbon_finalized_wait_timeout(dynpm_tbon_vertex_t *tbon_vertex, int seconds, int nanoseconds){
	pthread_mutex_lock(&tbon_vertex->mutex);

	struct timespec tbon_wait_timeout;
	_create_time_offset(seconds, nanoseconds, &tbon_wait_timeout);

	if(!tbon_vertex->finalized){
		info_logger("PARENT VERTEX: waiting for finalization");
		pthread_cond_timedwait(&(tbon_vertex->finalized_cond), &(tbon_vertex->mutex), &tbon_wait_timeout);
	} else {
		info_logger("PARENT VERTEX: done waiting on finalization cond");
	}

	info_logger("done waiting for finalization cond");
	pthread_mutex_unlock(&tbon_vertex->mutex);
}

void dynpm_tbon_initialized_wait_timeout(dynpm_tbon_vertex_t *tbon_vertex, int seconds, int nanoseconds){
	pthread_mutex_lock(&tbon_vertex->mutex);

	struct timespec tbon_wait_timeout;
	_create_time_offset(seconds, nanoseconds, &tbon_wait_timeout);

	if(!tbon_vertex->initialized){
		info_logger("PARENT VERTEX: waiting for initialization cond");
		pthread_cond_timedwait(&(tbon_vertex->initialized_cond), &(tbon_vertex->mutex), &tbon_wait_timeout);
	} else {
		info_logger("PARENT VERTEX: initialized before waiting on cond");
	}

	info_logger("done waiting for initialization cond");
	pthread_mutex_unlock(&tbon_vertex->mutex);
}
