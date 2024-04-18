/*
 * Copyright (c) 2022-2024 Technical University of Munich (TUM)
 *
 * Additional copyrights may follow
 *
 */

#ifndef _DYNPM_PROTOCOL_H
#define _DYNPM_PROTOCOL_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>

#include "event2/buffer.h"
#include "event2/bufferevent.h"
#include "event2/util.h"
#include <event2/event-config.h>
#include <event2/event.h>
#include <event2/thread.h>
#include <event2/dns.h>

#include "dynpm_common.h"

typedef enum  {
	INVALID_MESSAGE=0,
	REGISTER_CONNECTION,
	DEREGISTER_CONNECTION,
	REGISTER_PROCESS_MANAGER,
	REGISTER_PROCESS_MANAGER_ACK,
	DEREGISTER_PROCESS_MANAGER,
	OFFER_RESOURCES,
	CREATE_TBON,
	READY_TBON,
	READY_TBON_ACK,
	ALLOCATION_REQUEST,
	REALLOCATION_START,
	REALLOCATION_START_ACK,
	REALLOCATION_COMPLETE,
	DYNAMIC_PM_LAUNCH,
	READY_DYNAMIC_PM,
	DYNAMIC_IO_OUT,
	DYNAMIC_IO_OUT_ACK,
	PROCESSES_DONE,
	PROCESSES_DONE_ACK,
	POLL_PROCESS_DATA,
	REPORT_PROCESS_DATA,
} dynpm_message_type;

// message data structures

typedef struct dynpm_msg_header {
	uint32_t source_type;
	uint32_t message_type;
} dynpm_header_t;

// client to server, typically launcher to scheduler

// please note that after the launcher connects to the scheduler,
// the relationship between server and client is more elaborate,
// especially with tbon overlays

// the register and deregister connection messages and operations
// are general, and include both tbon connections as well as
// launche and scheduler connectionss

typedef struct dynpm_register_connection_msg {
	uint32_t pid;
	char *hostname;
} dynpm_register_connection_msg_t;

typedef struct dynpm_deregister_connection_msg {
	uint32_t pid;
	char *hostname;
	int32_t error_code;
} dynpm_deregister_connection_msg_t;

// the register and deregister process manager operations
// are between the launcher (representing a process manager)
// and the scheduler

typedef struct dynpm_application {
	uint32_t application_index;
	uint32_t malleability_mode;
	uint32_t node_count;
	char    *nodes;
} dynpm_application_t;

typedef struct dynpm_register_process_manager_msg {
	uint32_t pid;
	char *hostname;
	uint32_t job_index;
	uint32_t application_count;
	dynpm_application_t **applications;
} dynpm_register_process_manager_msg_t;

typedef struct dynpm_deregister_process_manager_msg {
	uint32_t pid;
	char *hostname;
	int32_t error_code;
} dynpm_deregister_process_manager_msg_t;

// allocation requests are performed representing an application
// either by a launcher or a process manager entity
// in the slurm design, this is the launcher

typedef struct dynpm_allocation_request_msg {
	uint32_t application_index;
	uint32_t partition_index;
	uint32_t type;
	uint32_t node_count;
} dynpm_allocation_request_msg_t;

typedef struct dynpm_reallocation_start_msg {
	uint32_t application_index;
	uint32_t partition_index;
	uint32_t type;
	uint32_t current_node_count;
	char    *current_nodes;
	uint32_t delta_node_count;
	char    *delta_nodes;
	uint32_t updated_node_count;
	char    *updated_nodes;
} dynpm_reallocation_start_msg_t;

typedef struct dynpm_reallocation_complete_msg {
	uint32_t application_index;
	uint32_t partition_index;
	uint32_t type;
	uint32_t node_count;
	char    *nodes;
	uint32_t status_code;
} dynpm_reallocation_complete_msg_t;

// server to client, typically a scheduler to a launcher
// but can also be a node manager to the launcher, or
// a process manager entity, such as the stepd to the launcher,
// depending on the design or the process manager
// the scheduler is always the recipient of the allocation requests
// and the scheduler is always the origin of resource offers

// resource offers originate from the scheduler, and
// are strictly informational
// the intent is that a process manager/runtime-system/application
// can use the resource availabitiy information in the offer to
// increase its chances of a successfus resource acquisition
// by adjusting its resource allocation requests based on resource
// availability
typedef struct dynpm_resource_offer_msg {
	uint32_t application_index;
	uint32_t partition_index;
	uint32_t node_count;
	time_t creation_time;
	time_t end_time;
} dynpm_resource_offer_msg_t;

typedef struct dynpm_allocation_response_msg {
	uint32_t application_index;
	uint32_t partition_index;
	uint32_t type;
	uint32_t node_count;
	char    *nodes;
} dynpm_allocation_response_msg_t;

// tbon messages

typedef struct dynpm_create_tbon_msg {
	uint32_t id;
	uint32_t degree;
	uint32_t total;
	char    *nodes;
} dynpm_create_tbon_msg_t;

typedef struct dynpm_ready_msg {
	uint32_t id;
	uint32_t subtree_ready_count;
	uint32_t error_code;
} dynpm_ready_msg_t;

typedef struct dynpm_ready_tbon_msg {
	uint32_t id;
	uint32_t subtree_ready_count;
	uint32_t error_code;
} dynpm_ready_tbon_msg_t;

typedef struct dynpm_processes_done_msg {
	uint32_t id;
	uint32_t subtree_ready_count;
	uint32_t error_code;
} dynpm_processes_done_msg_t;

typedef struct dynpm_dynamic_pm_launch_msg {
	uint32_t id;
	uint32_t type;
	uint32_t payload_bytes;
	void *payload;
} dynpm_dynamic_pm_launch_msg_t;

typedef struct dynpm_ready_dynamic_pm_msg {
	uint32_t id;
	uint32_t subtree_ready_count;
	uint32_t error_code;
} dynpm_ready_dynamic_pm_msg_t;

typedef struct dynpm_dynamic_io_out_msg {
	uint32_t id;
	char* host;
	uint32_t payload_bytes;
	void *payload;
} dynpm_dynamic_io_out_msg_t;

typedef struct dynpm_report_process_data_msg {
	uint32_t id;
	uint32_t job_index;
	uint32_t application_index;
	uint32_t malleability_mode;
	uint32_t payload_bytes;
	void *payload;
} dynpm_report_process_data_msg_t;


// non-msg data structures

typedef struct dynpm_message_fifo_queue {
	void *buffer;
	struct dynpm_message_fifo_queue *next;
} dynpm_message_fifo_queue_t;

// used by the a server to track its connection towards a client
typedef struct dynpm_server_to_client_connection {
	int pid;
	char *host;
	int fd;
	dynpm_message_fifo_queue_t *messages_head;
	pthread_mutex_t messages_mutex;
	struct event_base  *evbase;
	struct bufferevent *buf_ev;
    struct evdns_base  *dns_base;
	pthread_t thread;
	pthread_mutex_t mutex;
    LIST_ENTRY(dynpm_server_to_client_connection) connections;
} dynpm_server_to_client_connection_t;

typedef void (*dynpm_client_handler)(void *, const void*);

// used by the a client to track its connection towards a server
typedef struct dynpm_client_to_server_connection {
	char *host;
	char *port;
	dynpm_message_fifo_queue_t *messages_head;
	pthread_mutex_t messages_mutex;
	dynpm_server_to_client_connection_t *parent_connection;
	dynpm_client_handler handler;
	struct event_base  *evbase;
	struct bufferevent *buf_ev;
    struct evdns_base  *dns_base;
	pthread_t thread;
	pthread_mutex_t mutex;
	pthread_cond_t initialized_cond;
	int connected;
	int index;
} dynpm_client_to_server_connection_t;

// unit of work in the thread pool
typedef struct dynpm_work_unit {
	void (*function)(void *);
	void  *data;
	struct dynpm_work_unit *next;
} dynpm_work_unit_t;

// worker thread data for each in the thread pool
typedef struct worker_thread_data {
	pthread_t thread;
	int terminate;
	struct dynpm_work_queue *work_queue;
	struct worker_thread_data *next;
} worker_thread_data_t;

// work queue for the thread pool to process
typedef struct dynpm_work_queue {
	pthread_mutex_t       mutex;
	pthread_cond_t        cond;
	dynpm_work_unit_t     *work_units;
	worker_thread_data_t *thread_data;
} dynpm_work_queue_t;

// vertex data: children and parent data
typedef struct dynpm_tbon_vertex {
	int id;
	int degree;
	char *nodes;
	int ready_count;
	int total_nodes;
	char *local_host;
	int parent_id;
	char *parent_host;
	dynpm_server_to_client_connection_t *parent_connection;
	int total_children;
	int *children_ids;
	char **children_hosts;
	int *children_connection_indexes;
	dynpm_report_process_data_msg_t *process_data_reports;
	pthread_mutex_t mutex;
	pthread_mutex_t io_mutex;
	int initialized;
	pthread_cond_t initialized_cond;
	int children_done;
	pthread_cond_t children_done_cond;
	int finalized;
	pthread_cond_t finalized_cond;
} dynpm_tbon_vertex_t;

// types for use in the API
typedef void *dynpm_server_handler(dynpm_server_to_client_connection_t *, const void*);

// server API
int dynpm_server_init(int, int, dynpm_server_handler, char*, char*);
int dynpm_server_init_port_range(int, int, int, dynpm_server_handler, int *, char*, char*);
int dynpm_server_finalize();
void dynpm_lock_client_connections();
void dynpm_unlock_client_connections();
void dynpm_print_client_connections();
void dynpm_clear_client_connections();
int dynpm_find_client_connection_in_list(char *host, int pid, dynpm_server_to_client_connection_t **);
int dynpm_send_message_to_client_connection(dynpm_header_t **, void **, dynpm_server_to_client_connection_t *);
int dynpm_send_buffer_to_client_connection(const void *, dynpm_server_to_client_connection_t *);
int dynpm_send_header_to_client_connection(dynpm_source_type, dynpm_message_type, dynpm_server_to_client_connection_t *);
int  dynpm_work_queue_init(int);
void dynpm_work_queue_finalize(void);
void dynpm_work_queue_add(dynpm_work_unit_t *);

// TBON
void dynpm_compute_local_tbon_edges(int local_id, int degree, int total, int *parent_id, int **children_ids);
int dynpm_send_message_to_tbon_children( dynpm_server_to_client_connection_t *, const void *,
		dynpm_source_type, dynpm_message_type, dynpm_tbon_vertex_t *);
void dynpm_free_tbon_vertex(dynpm_tbon_vertex_t **);
void dynpm_initialize_tbon_vertex(dynpm_server_to_client_connection_t *,
		dynpm_create_tbon_msg_t *, char **, dynpm_tbon_vertex_t **);
void dynpm_tbon_children_done_wait_timeout(dynpm_tbon_vertex_t *tbon_vertex, int seconds, int nanoseconds);
void dynpm_tbon_initialized_wait_timeout(dynpm_tbon_vertex_t *tbon_vertex, int seconds, int nanoseconds);
void dynpm_tbon_finalized_wait_timeout(dynpm_tbon_vertex_t *tbon_vertex, int seconds, int nanoseconds);

// client API
int dynpm_client_init();
void dynpm_print_server_connections();
int dynpm_connect_to_server(char *, char *, dynpm_client_handler, dynpm_source_type, dynpm_server_to_client_connection_t *, int *);
int dynpm_disconnect_from_server(int);
int dynpm_send_message_to_server(dynpm_header_t **, void **, int);
int dynpm_send_buffer_to_server(const void *, int);
int dynpm_send_header_to_server_connection(dynpm_source_type, dynpm_message_type, dynpm_client_to_server_connection_t *);
int dynpm_send_header_to_server(dynpm_source_type, dynpm_message_type, int index);

// header
void dynpm_free_header(dynpm_header_t ** msg);
void dynpm_serialize_header(dynpm_header_t *header, void **buffer);
void dynpm_deserialize_header(dynpm_header_t ** msg_ptr, const void **buffer);

// register
void dynpm_free_register_connection_msg(dynpm_register_connection_msg_t ** msg);
void dynpm_serialize_register_connection_msg(dynpm_header_t * header, dynpm_register_connection_msg_t * msg, void **buffer);
void dynpm_deserialize_register_connection_msg(dynpm_register_connection_msg_t ** msg_ptr, const void **buffer);

// deregister connection
void dynpm_free_deregister_connection_msg(dynpm_deregister_connection_msg_t ** msg);
void dynpm_serialize_deregister_connection_msg(dynpm_header_t * header, dynpm_deregister_connection_msg_t * msg, void **buffer);
void dynpm_deserialize_deregister_connection_msg(dynpm_deregister_connection_msg_t ** msg_ptr, const void **buffer);

// resource offer
void dynpm_free_resource_offer_msg(dynpm_resource_offer_msg_t ** msg);
void dynpm_serialize_resource_offer_msg(dynpm_header_t * header, dynpm_resource_offer_msg_t * msg, void **buffer);
void dynpm_deserialize_resource_offer_msg(dynpm_resource_offer_msg_t ** msg_ptr, const void **buffer);

// create tbon
void dynpm_free_create_tbon_msg(dynpm_create_tbon_msg_t ** msg);
void dynpm_serialize_create_tbon_msg(dynpm_header_t * header, dynpm_create_tbon_msg_t * msg, void **buffer);
void dynpm_deserialize_create_tbon_msg(dynpm_create_tbon_msg_t ** msg_ptr, const void **buffer);

// ready tbon messages
void dynpm_free_ready_tbon_msg(dynpm_ready_tbon_msg_t ** msg);
void dynpm_free_ready_dynamic_pm_msg(dynpm_ready_dynamic_pm_msg_t ** msg);
void dynpm_serialize_ready_tbon_msg(dynpm_header_t * header, dynpm_ready_tbon_msg_t * msg, void **buffer);
void dynpm_serialize_ready_dynamic_pm_msg(dynpm_header_t * header, dynpm_ready_dynamic_pm_msg_t * msg, void **buffer);
void dynpm_deserialize_ready_tbon_msg(dynpm_ready_tbon_msg_t ** msg_ptr, const void **buffer);
void dynpm_deserialize_ready_dynamic_pm_msg(dynpm_ready_dynamic_pm_msg_t ** msg_ptr, const void **buffer);

// process manager registration
void dynpm_free_register_process_manager_msg(dynpm_register_process_manager_msg_t ** msg);
void dynpm_serialize_register_process_manager_msg(dynpm_header_t * header, dynpm_register_process_manager_msg_t * msg, void **buffer);
void dynpm_deserialize_register_process_manager_msg(dynpm_register_process_manager_msg_t ** msg_ptr, const void **buffer);

// process manager deregistration
void dynpm_free_deregister_process_manager_msg(dynpm_deregister_process_manager_msg_t ** msg);
void dynpm_serialize_deregister_process_manager_msg(dynpm_header_t * header, dynpm_deregister_process_manager_msg_t * msg, void **buffer);
void dynpm_deserialize_deregister_process_manager_msg(dynpm_deregister_process_manager_msg_t ** msg_ptr, const void **buffer);

// allocation request
void dynpm_free_allocation_request_msg(dynpm_allocation_request_msg_t ** msg);
void dynpm_serialize_allocation_request_msg(dynpm_header_t * header, dynpm_allocation_request_msg_t * msg, void **buffer);
void dynpm_deserialize_allocation_request_msg(dynpm_allocation_request_msg_t ** msg_ptr, const void **buffer);

// reallocation started
void dynpm_free_reallocation_start_msg(dynpm_reallocation_start_msg_t ** msg);
void dynpm_serialize_reallocation_start_msg(dynpm_header_t * header, dynpm_reallocation_start_msg_t * msg, void **buffer);
void dynpm_deserialize_reallocation_start_msg(dynpm_reallocation_start_msg_t ** msg_ptr, const void **buffer);

// reallocation complete
void dynpm_free_reallocation_complete_msg(dynpm_reallocation_complete_msg_t ** msg);
void dynpm_serialize_reallocation_complete_msg(dynpm_header_t * header, dynpm_reallocation_complete_msg_t * msg, void **buffer);
void dynpm_deserialize_reallocation_complete_msg(dynpm_reallocation_complete_msg_t ** msg_ptr, const void **buffer);

// dynamic launch
void dynpm_free_dynamic_pm_launch_msg(dynpm_dynamic_pm_launch_msg_t ** msg);
void dynpm_serialize_dynamic_pm_launch_msg(dynpm_header_t * header, dynpm_dynamic_pm_launch_msg_t * msg, void **buffer);
void dynpm_deserialize_dynamic_pm_launch_msg(dynpm_dynamic_pm_launch_msg_t ** msg_ptr, const void **buffer);

// dynamic io out
void dynpm_free_dynamic_io_out_msg(dynpm_dynamic_io_out_msg_t ** msg);
void dynpm_serialize_dynamic_io_out_msg(dynpm_header_t * header, dynpm_dynamic_io_out_msg_t * msg, void **buffer);
void dynpm_deserialize_dynamic_io_out_msg(dynpm_dynamic_io_out_msg_t ** msg_ptr, const void **buffer);

// processes done
void dynpm_free_processes_done_msg(dynpm_processes_done_msg_t ** msg);
void dynpm_serialize_processes_done_msg(dynpm_header_t * header, dynpm_processes_done_msg_t * msg, void **buffer);
void dynpm_deserialize_processes_done_msg(dynpm_processes_done_msg_t ** msg_ptr, const void **buffer);

// report process data
void dynpm_free_report_process_data_msg(dynpm_report_process_data_msg_t ** msg);
void dynpm_serialize_report_process_data_msg(dynpm_header_t * header, dynpm_report_process_data_msg_t * msg, void **buffer);
void dynpm_deserialize_report_process_data_msg(dynpm_report_process_data_msg_t ** msg_ptr, const void **buffer);


#endif // protocol header

