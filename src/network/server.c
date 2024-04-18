/*
 * Copyright (c) 2022-2024 Technical University of Munich (TUM)
 *
 * Additional copyrights may follow
 *
 */

#include "dynpm_network.h"

typedef struct dynpm_client_connection_list {
	dynpm_server_to_client_connection_t *connection;
	struct dynpm_client_connection_list *next;
} dynpm_client_connection_list_t;

// a composisition of connecions and message payload
typedef struct dynpm_message_data {
	void *message;
	dynpm_server_to_client_connection_t *connection;
} dynpm_message_data_t;

pthread_t accept_thread;
int server_socket = 0;
struct sockaddr_in serv_addr;

// libevent structures
static struct event_base *evbase_server_accept;
// created in queue.c
extern dynpm_work_queue_t work_queue;
void *(*dynpm_server_handler_function)(dynpm_server_to_client_connection_t *, const void*);

// tree data
int *children_ids;
int parent_id;

LIST_HEAD(connections_head, dynpm_server_to_client_connection);
static struct connections_head head;

pthread_mutex_t registration_mutex;

// access the client connections to cascade disconnectinos when necessary
extern dynpm_client_to_server_connection_t *server_connections[];
extern pthread_mutex_t server_connections_mutex;
extern int _dynpm_enqueue_send_buffer( const void *buffer, dynpm_message_fifo_queue_t **messages_head);

static void _remove_client_connection_from_list(dynpm_server_to_client_connection_t *connection);

static int _compare_pointers(void *a, void *b){
	if(a == b) return 1;
	else return 0;
}

static int _compare_id_host(void *a, void *b){
	dynpm_server_to_client_connection_t *aa = (dynpm_server_to_client_connection_t *)a;
	dynpm_server_to_client_connection_t *bb = (dynpm_server_to_client_connection_t *)b;

	if(aa->pid == bb->pid
			&& !strcmp(aa->host, bb->host)) return 1;
	else return 0;
}

static int _print_client_connection(void *a){
	dynpm_server_to_client_connection_t *current = (dynpm_server_to_client_connection_t *)a;
	if(current){
		info_logger(" ### addr: %x; fd: %d; pid: %d; host: %s;",
				current,
				current->fd,
				current->pid,
				current->host);
	} else { info_logger("null pointer cannot be printed"); }
}

void dynpm_print_client_connections(){
	info_logger(" ###  ###  connections  ### ###");
    dynpm_server_to_client_connection_t *connection;
    LIST_FOREACH(connection, &head, connections)
        _print_client_connection(connection);
}

static void _register_client_connection(
		dynpm_server_to_client_connection_t *connection,
		dynpm_register_connection_msg_t **regmsg
		){

	pthread_mutex_lock(&(connection->mutex));

	connection->pid  = (*regmsg)->pid;
	connection->host = strdup((*regmsg)->hostname);

	info_logger("regmsg: pid: %d; host: %s; fd: %d;",
			(*regmsg)->pid, (*regmsg)->hostname,
			connection->fd);

	pthread_mutex_unlock(&(connection->mutex));

	dynpm_free_register_connection_msg(regmsg);
}

static void _deregister_client_connection(
		dynpm_server_to_client_connection_t *connection,
		dynpm_deregister_connection_msg_t **deregmsg
		){

	pthread_mutex_lock(&registration_mutex);

	_remove_client_connection_from_list(connection);

	info_logger("deregmsg: pid: %d; host: %s; ec: %d; fd: %d;",
			(*deregmsg)->pid, (*deregmsg)->hostname,
			(*deregmsg)->error_code, connection->fd);

	pthread_mutex_unlock(&registration_mutex);

	dynpm_free_deregister_connection_msg(deregmsg);
}

int dynpm_find_client_connection_in_list(char *host, int pid,
        dynpm_server_to_client_connection_t **output_connection) {

	dynpm_server_to_client_connection_t *test_connection
		= malloc(sizeof(dynpm_server_to_client_connection_t));
	test_connection->host = strdup(host);
	test_connection->pid  = pid;

    dynpm_server_to_client_connection_t *connection = NULL;
	dynpm_server_to_client_connection_t *found = NULL;
    LIST_FOREACH(connection, &head, connections)
        if(_compare_id_host(connection, test_connection)){
            found = connection;
            break;
        }

	free(test_connection->host);
	free(test_connection);

	if(found){
		*output_connection = found;
		return 1;
	} else {
		*output_connection = NULL;;
		return 0;
	}
}

static int _is_client_connection_in_list(dynpm_server_to_client_connection_t *in_connection) {

    dynpm_server_to_client_connection_t *connection;
    dynpm_server_to_client_connection_t *found = NULL;
    LIST_FOREACH(connection, &head, connections)
        if(connection == in_connection){
            found = connection;
            break;
        }

	if(found){
		return 1;
	} else {
		return 0;
	}
}

static void _remove_client_connection_from_list(dynpm_server_to_client_connection_t *in_connection) {
	if(LIST_EMPTY(&head)){
		info_logger("trying to remove connection from empty list");
		return;
	}

    dynpm_server_to_client_connection_t *connection;
    dynpm_server_to_client_connection_t *found = NULL;
    LIST_FOREACH(connection, &head, connections)
        if(connection == in_connection){
            found = connection;
            break;
        }

	if(found){
        assert(found == in_connection);
		LIST_REMOVE(in_connection, connections);
	} else {
        assert(found == NULL);
		info_logger("connection %x not found; could not remove it", in_connection);
		dynpm_print_client_connections();
	}
}

static void _dynpm_top_server_handler(void *data){
	dynpm_message_data_t *message_data = (dynpm_message_data_t *)data;
	int return_code = 0;

	dynpm_header_t *header;
	dynpm_deserialize_header(&header, (const void **)&(message_data->message));

	if(header->message_type == (dynpm_message_type)REGISTER_CONNECTION){
		dynpm_register_connection_msg_t *regmsg;
		dynpm_deserialize_register_connection_msg(&regmsg, (const void **)&(message_data->message));
		_register_client_connection(message_data->connection, &regmsg);
	} else if(header->message_type == (dynpm_message_type)DEREGISTER_CONNECTION){
		dynpm_deregister_connection_msg_t *deregmsg = malloc(sizeof(dynpm_message_data_t));
		dynpm_deserialize_deregister_connection_msg(&deregmsg, (const void **)&(message_data->message));
		_deregister_client_connection(message_data->connection, &deregmsg);
	} else {
		(*dynpm_server_handler_function)(message_data->connection, message_data->message);
		free(header);
		return;
	}
	free(header);
	free(message_data->message);
}

static void _server_on_read(struct bufferevent *bev, void *arg) {
	//info_logger("event addr: %x", bev);
	static int message_bytes_read = 0;
	int current_bytes_read;
	dynpm_work_unit_t *work_unit;
	dynpm_message_data_t *message_data;
	dynpm_server_to_client_connection_t *connection = (dynpm_server_to_client_connection_t *)arg;
    void *read_buffer = malloc(sizeof(char) * DYNPM_MAX_MESSAGE_SIZE);

	// TODO the byte value per chunk must be a libevent constant
	pthread_mutex_lock(&(connection->mutex));
	while ((current_bytes_read = bufferevent_read(bev, (read_buffer + message_bytes_read), 4096)) > 0) {
		message_bytes_read += current_bytes_read;

		if (message_bytes_read < DYNPM_MAX_MESSAGE_SIZE) {
			info_logger("received %d bytes to process; (current: %d; total: %d)",
					message_bytes_read, current_bytes_read, DYNPM_MAX_MESSAGE_SIZE);
			continue;
		} else if (message_bytes_read == DYNPM_MAX_MESSAGE_SIZE) {
			//info_logger("full message read; processing...");
		}

		if ((work_unit = malloc(sizeof(dynpm_work_unit_t))) == NULL) {
			info_logger("failed to allocate memory for reg work_unit");
			return;
		}

		if ((message_data = malloc(sizeof(dynpm_message_data_t))) == NULL) {
			info_logger("failed to allocate memory for reg work_unit");
			return;
		}

		message_data->message = read_buffer;
		message_data->connection = connection;
		work_unit->data = message_data;
		work_unit->function = _dynpm_top_server_handler;
		dynpm_work_queue_add(work_unit);

		read_buffer = malloc(sizeof(char) * DYNPM_MAX_MESSAGE_SIZE);
		message_bytes_read = 0;
	}

	pthread_mutex_unlock(&(connection->mutex));

	if(current_bytes_read <= 0){
		free(read_buffer);
	}
}

static void _server_on_write(struct bufferevent *bev, void *arg) {
	//info_logger("_server_on_write: output buffer drained");
	dynpm_server_to_client_connection_t *connection = (dynpm_server_to_client_connection_t *)arg;

	if(connection == NULL){
		info_logger("_server_on_write: NULL connection!");
		exit(-1);
	}

	pthread_mutex_lock(&(connection->mutex));
	if(connection->messages_head){
		//info_logger("_server_on_write: writing");
		if(bufferevent_write(connection->buf_ev, connection->messages_head->buffer, DYNPM_MAX_MESSAGE_SIZE)){
			info_logger("failed to write with bufferevent write");
		}

		connection->messages_head = connection->messages_head->next;
	} else {
		info_logger("_server_on_write: no messages");
	}

	pthread_mutex_unlock(&(connection->mutex));
}

static void _server_on_error(struct bufferevent *bev, short what, void *arg) {

	pthread_mutex_lock(&registration_mutex);

	dynpm_server_to_client_connection_t *in_connection = (dynpm_server_to_client_connection_t *)arg;

	if (what & BEV_EVENT_EOF ) {
		if(what & BEV_EVENT_READING){
			pthread_mutex_unlock(&(in_connection->mutex));

			// TODO remove explicit close by setting close on exit in libevent?
			if(in_connection->fd > 0){
				close(in_connection->fd);
			} else { info_logger("invalid fd while closing"); }

            dynpm_server_to_client_connection_t *connection;
            dynpm_server_to_client_connection_t *found = NULL;
            LIST_FOREACH(connection, &head, connections)
                if(connection == in_connection){
                    found = connection;
                    break;
                }

			if(found){
				//info_logger("looking for %x in the list of client connections", in_connection);
				for(int index = 0; index < MAX_SERVER_CONNECTIONS; index++){
					if(server_connections[index] != NULL){
						if(server_connections[index]->parent_connection == in_connection){
							info_logger("server conn. %x (host: %s) is child of client conn. %x (host: %s)",
									in_connection, in_connection->host,
									server_connections[index], server_connections[index]->host);

							if(dynpm_disconnect_from_server(index)){
								info_logger("error disconnecting from server");
							}
						}
					}
				}

				info_logger("removing conn. %x (host: %s; pid: %d) (BEV_EVENT_EOF, client disconnect)",
						in_connection, in_connection->host, in_connection->pid);
				_remove_client_connection_from_list(in_connection);
			} else {
				info_logger("connection %x (host %s) was NOT in list",
						in_connection, in_connection->host);
				dynpm_print_client_connections();
			}


			if(in_connection->evbase) event_base_loopexit(in_connection->evbase, NULL);
			if(in_connection->buf_ev) bufferevent_free(in_connection->buf_ev);
			if((in_connection)->host) free((in_connection)->host);

			pthread_mutex_unlock(&(in_connection->mutex));

			free(in_connection);
			in_connection = NULL;

		} else if(what & BEV_EVENT_READING){
			info_logger("unexpected BEV_EVENT_EOF while BEV_EVENT_READING");
		} else {
			info_logger("unexpected BEV_EVENT_EOF without writing or reading");
		}
	} else {
		if (what & BEV_EVENT_READING )  { info_logger("_server_on_error:EV_EVENT_READING"); }
		if (what & BEV_EVENT_WRITING )  { info_logger("_server_on_error:EV_EVENT_WRITING"); }
		if (what & BEV_EVENT_ERROR  )   { info_logger("_server_on_error:EV_EVENT_ERROR"); }
		if (what & BEV_EVENT_TIMEOUT )  { info_logger("_server_on_error:EV_EVENT_TIMEOUT"); }
		if (what & BEV_EVENT_CONNECTED) { info_logger("_server_on_error:EV_EVENT_CONNECTED"); }
	}

	pthread_mutex_unlock(&registration_mutex);
}

static void _server_to_client_send_progress(int listener, short event, void *input){
	dynpm_server_to_client_connection_t *connection = (dynpm_server_to_client_connection_t *)input;
	//info_logger("_server_to_client_send_progress called at connection %x", connection);

	pthread_mutex_lock(&registration_mutex);
	if(connection != NULL){
		pthread_mutex_lock(&(connection->mutex));

		if(connection->messages_head){
			//info_logger("_server_to_client_send_progress: writing at connection %x", connection);
			if(bufferevent_write(connection->buf_ev, connection->messages_head->buffer, DYNPM_MAX_MESSAGE_SIZE)){
				info_logger("failed to write with bufferevent write");
			}

			connection->messages_head = connection->messages_head->next;
		} else {
			//info_logger("_server_to_client_send_progress: no messages");
		}

		struct timeval sleep_time;
		struct event *sleep_event;
		sleep_time.tv_sec = 0;
		sleep_time.tv_usec = 10;

		sleep_event = event_new(connection->evbase, -1, 0, _server_to_client_send_progress, (void*)connection);
		event_add(sleep_event, &sleep_time);

		//info_logger("_server_to_client_send_progress: unlocking");
		pthread_mutex_unlock(&(connection->mutex));
	} else {
		info_logger("NULL connection in _server_to_client_send_progress");
	}
	pthread_mutex_unlock(&registration_mutex);
}

static void *_client_connection_thread(void *input) {
	pthread_detach(pthread_self());

	dynpm_server_to_client_connection_t *connection = (dynpm_server_to_client_connection_t *)input;

	if(connection != NULL){
		struct timeval sleep_time;
		struct event *sleep_event;
		sleep_time.tv_sec = 1;
		sleep_time.tv_usec = 0;

		sleep_event = event_new(connection->evbase, -1, 0, _server_to_client_send_progress, (void*)connection);
		event_add(sleep_event, &sleep_time);

		info_logger("server to client connection thread for connection %x, base %x started", connection, connection->evbase);
		event_base_dispatch(connection->evbase);
	} else {
		info_logger("NULL connection on client thread");
	}
}

static void _client_connection_setup(void *input) {

	dynpm_server_to_client_connection_t *in_connection = (dynpm_server_to_client_connection_t *)input;
	pthread_mutex_lock(&registration_mutex);
	LIST_INSERT_HEAD(&head, in_connection, connections);
	pthread_mutex_unlock(&registration_mutex);

	if (pthread_create(&(in_connection->thread), NULL,
				_client_connection_thread, (void *)input)) {
		info_logger("failed to create connection thread");
	}
}

static void _client_connection_event_handler(int fd, short ev, void *arg) {
	int receive_buffer_size;
	uint32_t reply_code = 0;
	int connection_fd;
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);
	dynpm_work_unit_t *work_unit;

	dynpm_server_to_client_connection_t *connection = malloc(sizeof(dynpm_server_to_client_connection_t));
	if(connection == NULL) {
		info_logger("could not allocate connection");
		exit(-1);
	}

	connection->messages_head = NULL;

	pthread_mutex_init(&(connection->mutex), NULL);
	pthread_mutex_lock(&(connection->mutex));
	pthread_mutex_init(&(connection->messages_mutex), NULL);

	connection_fd = accept(fd, (struct sockaddr *)&client_addr, &client_len);
	if (connection_fd < 0) {
        info_logger("accept failed in _client_connection_event_handler");
        exit(-1);
    }

	int flags = fcntl(connection_fd, F_GETFL);
	if (flags < 0) {
        info_logger("could not get server_socket flags");
        exit(-1);
    }
	flags |= O_NONBLOCK;
	if (fcntl(connection_fd, F_SETFL, flags) < 0){
		info_logger("could not set updated server_socket flag\ns");
        exit(-1);
    }

	int value = 1;
	setsockopt(connection_fd, IPPROTO_TCP, TCP_NODELAY, &value, sizeof(int));

	connection->fd = connection_fd;

	if ((connection->evbase = event_base_new()) == NULL) {
		info_logger("connection event_base creation failed");
        exit(-1);
	}

	connection->buf_ev = bufferevent_socket_new(
                    connection->evbase,
                    connection_fd,
                    //BEV_OPT_CLOSE_ON_FREE|BEV_OPT_THREADSAFE);
                    BEV_OPT_CLOSE_ON_FREE);
    if(connection->buf_ev == NULL){
		info_logger("connection bufferevent creation failed");
        exit(-1);
    }

    bufferevent_setcb(
            connection->buf_ev,
            _server_on_read,
            _server_on_write,
            _server_on_error,
            (void*)connection);

	bufferevent_set_max_single_read(connection->buf_ev, DYNPM_MAX_MESSAGE_SIZE);
	bufferevent_set_max_single_write(connection->buf_ev, DYNPM_MAX_MESSAGE_SIZE);

	// NULL disables timeouts
	// set seconds timeout                       read, write
	bufferevent_set_timeouts(connection->buf_ev, NULL, NULL);

	bufferevent_enable(connection->buf_ev, EV_READ|EV_WRITE|EV_PERSIST);

	if ((work_unit = malloc(sizeof(dynpm_work_unit_t))) == NULL) {
		info_logger("failed to allocate memory for work_unit state");
        exit(-1);
	}
	work_unit->function = _client_connection_setup;
	work_unit->data = connection;
	dynpm_work_queue_add(work_unit);

	pthread_mutex_unlock(&(connection->mutex));
}

static void *_accept_client_connection_thread() {
	pthread_detach(pthread_self());

	listen(server_socket, 10); // TODO make second parameter configurable?
	int reuseaddr_on = 1;
	setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR|TCP_NODELAY, &reuseaddr_on, sizeof(reuseaddr_on));
	int flags = fcntl(server_socket, F_GETFL);
	if (flags < 0) info_logger("could not get server_socket flags");
	flags |= O_NONBLOCK;
	if (fcntl(server_socket, F_SETFL, flags) < 0) {
		info_logger("could not set updated server_socket flags");
		pthread_exit(NULL); exit(-1);}

	if ((evbase_server_accept = event_base_new()) == NULL) {
		info_logger("unable to create event base for accept");
		pthread_exit(NULL); exit(-1);}

	struct event *ev_accept = event_new(
            evbase_server_accept,
            server_socket,
            EV_READ|EV_PERSIST,
            _client_connection_event_handler,
            (void *)&work_queue);

	event_add(ev_accept, NULL);
	event_base_dispatch(evbase_server_accept);

    event_free(ev_accept);
	event_base_free(evbase_server_accept);
	evbase_server_accept = NULL;

	close(server_socket);
}

void dynpm_lock_client_connections(){
	pthread_mutex_lock(&registration_mutex);
}

void dynpm_unlock_client_connections(){
	pthread_mutex_unlock(&registration_mutex);
}

void dynpm_clear_client_connections(){
	int lingering_connection_data;
	int return_code;

	int socket_error;
	int socket_error_len = sizeof(int);

	if(!LIST_EMPTY(&head)){
		info_logger("connections was not empty: cleaning state");

		lingering_connection_data = 0;

		dynpm_server_to_client_connection_t *connection;
        LIST_FOREACH(connection, &head, connections) {
            info_logger("removing lingering connection with fd: %d", connection->fd);

            socklen_t len = sizeof (socket_error);
            return_code =
                getsockopt(connection->fd, SOL_SOCKET, SO_ERROR,
                        &socket_error, &socket_error_len);

            if (return_code != 0) {
                info_logger("error getting socket error code: %s",
                        strerror(return_code));
            }

            if (socket_error != 0) {
                info_logger("socket_error: %s", strerror(socket_error));
            }

            if(fcntl(connection->fd, F_GETFD) != -1 || errno != EBADF){
                //info_logger("closing %d", connection->fd);
                close(connection->fd);
            }

            LIST_REMOVE(connection, connections);
            if(connection->host) free(connection->host);
            free(connection);
            lingering_connection_data++;
        }
		info_logger("%d lingering connections removed", lingering_connection_data);
	} else {
		//info_logger("connection data had no lingering connections");
	}
}

int dynpm_send_header_to_client_connection(dynpm_source_type source, dynpm_message_type type,
		dynpm_server_to_client_connection_t *connection) {

	dynpm_header_t *ack_header = malloc(sizeof(dynpm_header_t));
	ack_header->source_type =  source;
	ack_header->message_type = type;

	void *ack_buf = malloc(sizeof(char)*4096);
	dynpm_serialize_header(ack_header, &ack_buf);

	if(dynpm_send_buffer_to_client_connection(ack_buf, connection)){
		info_logger("failed to send buffer to server");
	}

	free(ack_header);
	free(ack_buf);
}

int dynpm_send_buffer_to_client_connection(const void *buffer, dynpm_server_to_client_connection_t *connection) {
	pthread_mutex_lock(&(connection->messages_mutex));
	if(_dynpm_enqueue_send_buffer(buffer, &(connection->messages_head))){
		info_logger("failed to enqueue message");
	}
	pthread_mutex_unlock(&(connection->messages_mutex));

	return 0;
}

int dynpm_send_message_to_client_connection(dynpm_header_t **header, void **msg,
		dynpm_server_to_client_connection_t *connection) {

	if(connection == NULL){
		info_logger("trying to send a message to a null connection at server");
		goto dynpm_send_message_to_client_connection_error;
	}

	void *buffer = malloc(sizeof(char)*DYNPM_MAX_MESSAGE_SIZE);
	pthread_mutex_lock(&(connection->messages_mutex));

	switch ((*header)->message_type) {
		case (dynpm_message_type)REGISTER_CONNECTION:
			info_logger("can't use send_message_to_client_connection with REGISTER_CONNECTION");
			goto dynpm_send_message_to_client_connection_error;
		case (dynpm_message_type)DEREGISTER_CONNECTION:
			info_logger("can't use send_message_to_client_connection with DEREGISTER_CONNECTION");
			goto dynpm_send_message_to_client_connection_error;
		case (dynpm_message_type)OFFER_RESOURCES:
            info_logger("sending OFFER_RESOURCES");
			dynpm_serialize_resource_offer_msg(*header, *msg, &buffer);
			dynpm_free_resource_offer_msg((dynpm_resource_offer_msg_t**)msg);
			break;
		case (dynpm_message_type)READY_TBON:
            info_logger("sending READY_TBON");
			dynpm_serialize_ready_tbon_msg(*header, *msg, &buffer);
			dynpm_free_ready_tbon_msg((dynpm_ready_tbon_msg_t**)msg);
			break;
		case (dynpm_message_type)READY_DYNAMIC_PM:
            info_logger("sending READY_DYNAMIC_PM");
			dynpm_serialize_ready_dynamic_pm_msg(*header, *msg, &buffer);
			dynpm_free_ready_dynamic_pm_msg((dynpm_ready_dynamic_pm_msg_t**)msg);
			break;
		case (dynpm_message_type)DYNAMIC_IO_OUT:
            info_logger("sending DYNAMIC_IO_OUT");
			dynpm_serialize_dynamic_io_out_msg(*header, *msg, &buffer);
			dynpm_free_dynamic_io_out_msg((dynpm_dynamic_io_out_msg_t**)msg);
			break;
		case (dynpm_message_type)PROCESSES_DONE:
            info_logger("sending PROCESSES_DONE");
			dynpm_serialize_processes_done_msg(*header, *msg, &buffer);
			dynpm_free_processes_done_msg((dynpm_processes_done_msg_t**)msg);
			break;
		case (dynpm_message_type)REPORT_PROCESS_DATA:
            info_logger("sending REPORT_PROCESS_DATA");
			dynpm_serialize_report_process_data_msg(*header, *msg, &buffer);
			dynpm_free_report_process_data_msg((dynpm_report_process_data_msg_t**)msg);
			break;
		case (dynpm_message_type)REALLOCATION_START:
            info_logger("sending REALLOCATION_START");
			dynpm_serialize_reallocation_start_msg(*header, *msg, &buffer);
			dynpm_free_reallocation_start_msg((dynpm_reallocation_start_msg_t**)msg);
			break;
		default:
			info_logger("message type %d in dynpm_send_message_to_client_connection",
					(*header)->message_type);
			pthread_mutex_unlock(&(connection->mutex));
			goto dynpm_send_message_to_client_connection_error;
	}

	if(_dynpm_enqueue_send_buffer(buffer, &(connection->messages_head))){
		info_logger("failed to enqueue message");
	}
	pthread_mutex_unlock(&(connection->messages_mutex));

	dynpm_free_header(header);
	free(buffer);
	return 0;
dynpm_send_message_to_client_connection_error:
	dynpm_free_header(header);
	free(buffer);
	free(*msg);
	return -1;
}

static int _dynpm_server_init_common(int queue_threads, dynpm_server_handler handler){
	char thread_name[16]; // pthread limits this to 16
	pthread_mutex_init(&registration_mutex, NULL);
    LIST_INIT(&head);
	dynpm_server_handler_function = handler;

	if (dynpm_work_queue_init(queue_threads)) {
		info_logger("failed to create plugin work queue");
		dynpm_work_queue_finalize();
		return -1;
	}

	if(pthread_create(&accept_thread, NULL, _accept_client_connection_thread, (void *) NULL))
	{ info_logger("failed to create accept_thread"); return -1;}

	sprintf(thread_name, "acceptthrd");
	if(pthread_setname_np(accept_thread, thread_name))
	{ info_logger("failed to set name of listener thread"); return -1;}

	return 0;
}

int dynpm_server_init(int port, int queue_threads, dynpm_server_handler handler,
		char *info, char *error){
	dynpm_init_logger(info, error);

	int error_code;

	//evthread_use_pthreads();

	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket <  0) { info_logger("Error in socket creation"); return -1; }

	memset(&serv_addr, '0', sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port);

	// without SO_REUSEADDR|SO_REUSEPORT we may fail to initialize on the
	// same port across jobs where a termination and a start are close in time
	int setting_true = 1;
	if(setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR|SO_REUSEPORT,
				&setting_true, sizeof(setting_true)) < 0) {
		info_logger("could not set server socket reuse options");
	}

	error_code = bind(server_socket, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

	if (error_code){ info_logger("could not bind server socket"); return -1;
	} else {
		return _dynpm_server_init_common(queue_threads, handler);
	}

	return 0;
}

int dynpm_server_init_port_range(int min_port, int max_port, int queue_threads,
		dynpm_server_handler handler, int *port, char *info, char *error){
	dynpm_init_logger(info, error);

	int error_code;
	int try_port;

	//evthread_use_pthreads();

	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket <  0) { info_logger("Error in socket creation"); return -1; }

	memset(&serv_addr, '0', sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	for (try_port = min_port; try_port <= max_port; try_port++) {
		serv_addr.sin_port = htons(try_port);
		error_code = bind(server_socket, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
		if(error_code < 0 && errno == EADDRINUSE){
			info_logger("port %d was in use; keep trying...", try_port);
			continue;
		} else { break; }
	}

	if (error_code){
		info_logger("could not bind server socket within port range: %d:%d",
				min_port, max_port);
		return -1;
	} else {
		*port = try_port;
		info_logger("listening in port: %d", *port);
		return _dynpm_server_init_common(queue_threads, handler);
	}
}

int dynpm_server_finalize() {
	if(close(server_socket)) {
		info_logger("error while closing fd %d at server", server_socket);
		return -1;
	}
	return 0;
}

