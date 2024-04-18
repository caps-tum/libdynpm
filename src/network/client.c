/*
 * Copyright (c) 2022-2024 Technical University of Munich (TUM)
 *
 * Additional copyrights may follow
 *
 */

#include "dynpm_network.h"

// TODO this has the side effect of limiting the tbon degree; move to a dynamic data structure?
dynpm_client_to_server_connection_t *server_connections[MAX_SERVER_CONNECTIONS] = {NULL};
pthread_mutex_t server_connections_mutex;
static volatile int server_connection_count = -1;
static volatile int dynpm_client_initialized = 0;

static int _dynpm_internal_send_message_to_server(dynpm_header_t **, void **, dynpm_client_to_server_connection_t *);

// a composisition of connecions and message payload
typedef struct dynpm_message_data {
	void *message;
	dynpm_client_to_server_connection_t *connection;
} dynpm_message_data_t;

static void _client_on_read(struct bufferevent *bev, void *arg) {
	//info_logger("_client_on_read; event addr: %x; arg: %x", bev, arg);
	int message_bytes_read, current_bytes_read;

	struct evbuffer *input = bufferevent_get_input(bev);
	struct evbuffer *output = bufferevent_get_output(bev);

	int total_bytes = evbuffer_get_length(input);
	if(total_bytes != DYNPM_MAX_MESSAGE_SIZE){ // only do fixed size messages for now
		info_logger("_client_on_read; total bytes: %d; exiting...", total_bytes);
		exit(-1);
	}

	dynpm_client_to_server_connection_t *connection = (dynpm_client_to_server_connection_t *)arg;
	pthread_mutex_lock(&(connection->mutex));

    void *read_buffer = malloc(sizeof(char) * DYNPM_MAX_MESSAGE_SIZE);

	// TODO the byte value per chunk must be a libevent constant
	message_bytes_read = 0;
	while ((current_bytes_read =
				bufferevent_read(bev, (read_buffer + message_bytes_read), DYNPM_MAX_MESSAGE_SIZE)) > 0) {
		message_bytes_read += current_bytes_read;

		if (message_bytes_read < DYNPM_MAX_MESSAGE_SIZE) {
			info_logger("received %d bytes (current: %d; total: %d) max: %d",
					message_bytes_read, current_bytes_read, total_bytes, DYNPM_MAX_MESSAGE_SIZE);
			continue;
		} else if (message_bytes_read == DYNPM_MAX_MESSAGE_SIZE) {
			//info_logger("full message read; processing...");
			break;
		}
	}

	(*(connection->handler))(connection, (const void*)(read_buffer));
	free(read_buffer);
	pthread_mutex_unlock(&(connection->mutex));
}

static void _client_on_write(struct bufferevent *bev, void *arg) {
	dynpm_client_to_server_connection_t *connection = (dynpm_client_to_server_connection_t *)arg;
	//info_logger("_client_on_write: at %x output buffer drained", connection);

	pthread_mutex_lock(&(connection->mutex));

	if(connection->messages_head){
		//info_logger("_client_on_write: writing at %x index %d", connection, connection->index);
		if(bufferevent_write(connection->buf_ev, connection->messages_head->buffer, DYNPM_MAX_MESSAGE_SIZE)){
			info_logger("failed to write with bufferevent write");
		}

		connection->messages_head = connection->messages_head->next;
	} else {
		//info_logger("_client_on_write: no messages at %x index %d", connection, connection->index);
	}

	pthread_mutex_unlock(&(connection->mutex));
}

static void _client_on_error(struct bufferevent *bev, short what, void *arg) {

	dynpm_client_to_server_connection_t *connection = (dynpm_client_to_server_connection_t *)arg;

	if (what & BEV_EVENT_EOF ) {
		info_logger("client connection %x (index %d) closed remotely by server",
				connection, connection->index);
		if(dynpm_disconnect_from_server(connection->index)){
			info_logger("error trying to disconnect from server");
		} else {
			info_logger("disconnected from server");
		}

		if(what & BEV_EVENT_READING){
			info_logger("BEV_EVENT_EOF while BEV_EVENT_READING");
		}
		if(what & BEV_EVENT_WRITING){
			info_logger("BEV_EVENT_EOF while EV_EVENT_WRITING");
		}
		if(!(what & BEV_EVENT_WRITING) && !(what & BEV_EVENT_READING)){
			info_logger("BEV_EVENT_EOF without writing or reading");
		}
	} else {
		if (what & BEV_EVENT_READING )  { info_logger("_client_on_error:EV_EVENT_READING"); }
		if (what & BEV_EVENT_WRITING )  { info_logger("_client_on_error:EV_EVENT_WRITING"); }
		if (what & BEV_EVENT_ERROR  )   { info_logger("_client_on_error:EV_EVENT_ERROR"); }
		if (what & BEV_EVENT_TIMEOUT )  { info_logger("_client_on_error:EV_EVENT_TIMEOUT"); }
		if (what & BEV_EVENT_CONNECTED) {
			//info_logger("_client_on_error:EV_EVENT_CONNECTED");

			pthread_mutex_lock(&(connection->mutex));

			evutil_socket_t fd = bufferevent_getfd(bev);
			int one = 1;
			setsockopt(fd, IPPROTO_TCP, TCP_NODELAY,   &one, sizeof one);
			connection->connected = 1;

			pthread_cond_signal(&(connection->initialized_cond));
			pthread_mutex_unlock(&(connection->mutex));
		}
	}
}

void _dynpm_init_server_connection(dynpm_client_to_server_connection_t **server_connection){
	(*server_connection) = malloc(sizeof(dynpm_client_to_server_connection_t));
	(*server_connection)->host = NULL;
	(*server_connection)->port = NULL;
	(*server_connection)->messages_head = NULL;
	(*server_connection)->parent_connection = NULL;
	(*server_connection)->thread = -1;
	(*server_connection)->evbase = NULL;
	(*server_connection)->buf_ev = NULL;
	(*server_connection)->dns_base = NULL;
	pthread_mutex_init(&((*server_connection)->mutex), NULL);
	pthread_mutex_init(&((*server_connection)->messages_mutex), NULL);
	pthread_cond_init(&((*server_connection)->initialized_cond), NULL);
	(*server_connection)->connected = 0;
}

int dynpm_client_init(char *info, char *error){

	//info_logger("dynpm_client_init");

	// TODO need to double check this:
	// initialization checks are not safe until initialization completes
	// this could allow for race conditions if no reasonable time is allowed
	// during any component that uses the client API

	if(dynpm_client_initialized){
		info_logger("dynpm client was already initialized; skipping");
		return 0;
	}

	//evthread_use_pthreads();

	pthread_mutex_init(&server_connections_mutex, NULL);
	pthread_mutex_lock(&server_connections_mutex);

	server_connection_count = 0;

	for(int server_conn = 0; server_conn < MAX_SERVER_CONNECTIONS; server_conn++){
		server_connections[server_conn] = NULL;
	}

	dynpm_client_initialized = 1;

	pthread_mutex_unlock(&server_connections_mutex);

	return 0;
}

static void _client_to_server_send_progress(int listener, short event, void *input){
	dynpm_client_to_server_connection_t *connection = (dynpm_client_to_server_connection_t *)input;
	//info_logger("_client_to_server_send_progress called at connection %x", connection);

	pthread_mutex_lock(&server_connections_mutex);
	if(connection != NULL){
		pthread_mutex_lock(&(connection->mutex));

		if(connection->messages_head){
			//info_logger("_client_to_server_send_progress: writing at connection %x", connection);
			if(bufferevent_write(connection->buf_ev, connection->messages_head->buffer, DYNPM_MAX_MESSAGE_SIZE)){
				info_logger("failed to write with bufferevent write");
			}

			connection->messages_head = connection->messages_head->next;
		} else {
			//info_logger("_client_to_server_send_progress: no messages");
		}

		struct timeval sleep_time;
		struct event *sleep_event;
		sleep_time.tv_sec = 0;
		sleep_time.tv_usec = 10;

		sleep_event = event_new(connection->evbase, -1, 0, _client_to_server_send_progress, (void*)connection);
		event_add(sleep_event, &sleep_time);

		pthread_mutex_unlock(&(connection->mutex));
	} else {
		info_logger("NULL connection _client_to_server_send_progress");
	}
	pthread_mutex_unlock(&server_connections_mutex);
}

static void *_server_event_connection_thread(void *input) {

	dynpm_client_to_server_connection_t *connection = (dynpm_client_to_server_connection_t *)input;
	if(connection != NULL){
		struct timeval sleep_time;
		struct event *sleep_event;
		sleep_time.tv_sec = 1;
		sleep_time.tv_usec = 0;

		sleep_event = event_new(connection->evbase, -1, 0, _client_to_server_send_progress, (void*)connection);
		event_add(sleep_event, &sleep_time);

		info_logger("client to server connection thread for connection %x, base %x started", connection, connection->evbase);
		event_base_dispatch(connection->evbase);

		info_logger("exiting %x", connection);
		event_free(sleep_event);
		bufferevent_free(connection->buf_ev);
		event_base_free(connection->evbase); // closes fd on free
		free(connection->host);
		free(connection->port);
		free(connection);
	} else {
		info_logger("NULL connection provided to _server_event_connection_thread");
	}
}

int dynpm_connect_to_server(char *host, char *port,
		dynpm_client_handler handler, dynpm_source_type source,
		dynpm_server_to_client_connection_t *parent_connection, int *server_index){

	if(!dynpm_client_initialized){
		info_logger("trying to connect to a server before dynpm client is initialized");
		return -1;
	}

	pthread_mutex_lock(&server_connections_mutex);

	int index;

	if (server_connection_count > (MAX_SERVER_CONNECTIONS - 1)){
		info_logger("server connection limit exeeded");
		goto dynpm_connect_to_server_error;
	}
	server_connection_count++;

	*server_index = -1;
	for(index = 0; index < MAX_SERVER_CONNECTIONS; index++){
		if(server_connections[index] == NULL){
			info_logger("adding dynpm server connection at index %d", index);
			_dynpm_init_server_connection(&server_connections[index]);
			server_connections[index]->host = strdup(host);
			server_connections[index]->port = strdup(port);
			server_connections[index]->parent_connection = parent_connection;
			server_connections[index]->handler = handler;
			*server_index = index;
			break;
		}
	}

	if(index >= MAX_SERVER_CONNECTIONS){
        info_logger("could not find a NULL entry in the server_connections table");
		goto dynpm_connect_to_server_error;
	}

	server_connections[index]->index = index;
	info_logger("connected to connection %x, host: %s, port: %s; parent: %x, index: %d",
			server_connections[index],
			server_connections[index]->host,
			server_connections[index]->port,
			server_connections[index]->parent_connection,
			server_connections[index]->index);

	if ((server_connections[index]->evbase = event_base_new()) == NULL) {
		info_logger("connection event_base creation failed");
		return -1;
	} else info_logger("evbase at index %d is %x", index, server_connections[index]->evbase);

	// TODO need to find out the best flags to set for the dns base
    if ((server_connections[index]->dns_base
		= evdns_base_new(server_connections[index]->evbase, 1)) == NULL){
		info_logger("connection dns_base creation failed");
		return -1;
	} else info_logger("dns_base at index %d is %x", index, server_connections[index]->dns_base);

	if((server_connections[index]->buf_ev = bufferevent_socket_new(
                    server_connections[index]->evbase,
                    -1, // the FD will be set by the connect call instead
                    //BEV_OPT_CLOSE_ON_FREE|BEV_OPT_THREADSAFE)) == NULL){
                    BEV_OPT_CLOSE_ON_FREE)) == NULL){
		info_logger("connection buf_ev creation failed");
		return -1;
	} else info_logger("buf_ev at index %d is %x", index, server_connections[index]->buf_ev);

	bufferevent_setcb(
			server_connections[index]->buf_ev,
			_client_on_read,
			_client_on_write,
			_client_on_error,
			(void*)(server_connections[index]));

	// NULL disables timeouts
	// TODO add a configuration entry to set these timeouts instead
	// set seconds timeout                                      read, write
	bufferevent_set_timeouts(server_connections[index]->buf_ev, NULL, NULL);
	bufferevent_enable(server_connections[index]->buf_ev, EV_READ|EV_WRITE|EV_PERSIST);
	bufferevent_socket_connect_hostname(server_connections[index]->buf_ev,
			 server_connections[index]->dns_base, AF_INET, host, atoi(port));

	// start the client thread for this server
	char thread_name[16]; // pthread limits this to 16
	if(pthread_create(&server_connections[index]->thread, NULL, _server_event_connection_thread,
				(void *)(server_connections[index]))) {
		info_logger("failed to create _client_handler_thread");
		goto dynpm_connect_to_server_error;
	}
	sprintf(thread_name, "dynpm_cr%d", index);
	if(pthread_setname_np(server_connections[index]->thread, thread_name)) {
		info_logger("failed to set name of listener thread");
		goto dynpm_connect_to_server_error;
	}

	pthread_mutex_lock(&(server_connections[index]->mutex));
	if(!server_connections[index]->connected){
		info_logger("waiting for client read thread startup");
		pthread_cond_wait(&(server_connections[index]->initialized_cond), &(server_connections[index]->mutex));
	} else {
		info_logger("client read thread was already connected before waiting on cond");
	}
	pthread_mutex_unlock(&(server_connections[index]->mutex));
	info_logger("client read thread started...");

	dynpm_header_t *header = malloc(sizeof(dynpm_header_t));
	header->source_type  = (dynpm_source_type)source;
	header->message_type = (dynpm_message_type)REGISTER_CONNECTION;

	dynpm_register_connection_msg_t *regmsg = malloc(sizeof(dynpm_register_connection_msg_t));
	regmsg->pid = getpid();
	char hostname[128];
	if(gethostname(hostname, 128)) { info_logger("error getting hostname"); }
	regmsg->hostname = strdup(hostname);

	if(_dynpm_internal_send_message_to_server(&header, (void**)&regmsg, server_connections[index])){
		info_logger("error sending register connection message");
		dynpm_free_register_connection_msg((dynpm_register_connection_msg_t**)&regmsg);
		dynpm_free_header(&header);
		goto dynpm_connect_to_server_error;
	}

	pthread_mutex_unlock(&server_connections_mutex);
	return 0;

dynpm_connect_to_server_error:
	pthread_mutex_unlock(&server_connections_mutex);
	return -1;
}

static void _dynpm_print_connection_enqueued_messages(dynpm_message_fifo_queue_t *messages_head){
	dynpm_message_fifo_queue_t *message = messages_head;
	while(message){
		info_logger("%s", (char*)message->buffer);
		message = message->next;
	}
}

static void _print_10_buffer_char(const void*buffer){
	info_logger("%c%c%c%c%c%c%c%c%c%c",
			((char*)(buffer))[0],
			((char*)(buffer))[1],
			((char*)(buffer))[2],
			((char*)(buffer))[3],
			((char*)(buffer))[4],
			((char*)(buffer))[5],
			((char*)(buffer))[6],
			((char*)(buffer))[7],
			((char*)(buffer))[8],
			((char*)(buffer))[9]);
}

int _dynpm_enqueue_send_buffer( const void *buffer, dynpm_message_fifo_queue_t **messages_head) {
	//info_logger("source buffer:");
	//_print_10_buffer_char(buffer);

	dynpm_message_fifo_queue_t *new_message = malloc(sizeof(dynpm_message_fifo_queue_t));
	new_message->buffer = malloc(sizeof(char)*DYNPM_MAX_MESSAGE_SIZE);
	if(new_message == NULL) { info_logger("fatal: malloc fail"); return -1; }
	memcpy((char*)new_message->buffer, (char*)buffer, DYNPM_MAX_MESSAGE_SIZE);
	new_message->next = NULL;

	//info_logger("enqueuing buffer:");
	//_print_10_buffer_char(new_message->buffer);

	if(*messages_head){
		dynpm_message_fifo_queue_t *tail = *messages_head;
		while(tail->next) tail = tail->next;
		tail->next = new_message;
	} else {
		*messages_head = new_message;
	}

	//_dynpm_print_connection_enqueued_messages(*messages_head);

	return 0;
}

static int _dynpm_internal_send_buffer_to_server(
		const void *buffer, dynpm_client_to_server_connection_t *connection) {

	pthread_mutex_lock(&(connection->messages_mutex));
	if(_dynpm_enqueue_send_buffer(buffer, &(connection->messages_head))){
		info_logger("failed to enqueue message");
	}
	pthread_mutex_unlock(&(connection->messages_mutex));

	return 0;
}

static int _dynpm_internal_send_message_to_server(
		dynpm_header_t **header, void **msg, dynpm_client_to_server_connection_t *connection) {

	pthread_mutex_lock(&(connection->messages_mutex));
	void *write_buffer = malloc(sizeof(char) * DYNPM_MAX_MESSAGE_SIZE);

	switch ((*header)->message_type) {
		case (dynpm_message_type)REGISTER_CONNECTION:
			info_logger("sending REGISTER_CONNECTION");
			dynpm_serialize_register_connection_msg(*header, *msg, &write_buffer);
			dynpm_free_register_connection_msg((dynpm_register_connection_msg_t**)msg);
			break;
		case (dynpm_message_type)DEREGISTER_CONNECTION:
			info_logger("sending DEREGISTER_CONNECTION");
			dynpm_serialize_deregister_connection_msg(*header, *msg, &write_buffer);
			dynpm_free_deregister_connection_msg((dynpm_deregister_connection_msg_t**)msg);
			break;
		case (dynpm_message_type)CREATE_TBON:
			info_logger("sending CREATE_TBON");
			dynpm_serialize_create_tbon_msg(*header, *msg, &write_buffer);
			dynpm_free_create_tbon_msg((dynpm_create_tbon_msg_t**)msg);
			break;
		case (dynpm_message_type)OFFER_RESOURCES:
			info_logger("sending OFFER_RESOURCES");
			dynpm_serialize_resource_offer_msg(*header, (dynpm_resource_offer_msg_t*)*msg, &write_buffer);
			dynpm_free_resource_offer_msg((dynpm_resource_offer_msg_t**)msg);
			break;
		case (dynpm_message_type)DYNAMIC_PM_LAUNCH:
			info_logger("sending DYNAMIC_PM_LAUNCH");
			dynpm_serialize_dynamic_pm_launch_msg(*header, (dynpm_dynamic_pm_launch_msg_t*)*msg, &write_buffer);
			dynpm_free_dynamic_pm_launch_msg((dynpm_dynamic_pm_launch_msg_t**)msg);
			break;
		case (dynpm_message_type)REGISTER_PROCESS_MANAGER:
			info_logger("sending REGISTER_PROCESS_MANAGER");
			dynpm_serialize_register_process_manager_msg(*header, (dynpm_register_process_manager_msg_t*)*msg, &write_buffer);
			dynpm_free_register_process_manager_msg((dynpm_register_process_manager_msg_t**)msg);
			break;
		case (dynpm_message_type)ALLOCATION_REQUEST:
			info_logger("sending ALLOCATION_REQUEST");
			dynpm_serialize_allocation_request_msg(*header, (dynpm_allocation_request_msg_t*)*msg, &write_buffer);
			dynpm_free_allocation_request_msg((dynpm_allocation_request_msg_t**)msg);
			break;
		default:
			info_logger("unknown message type: %d", (*header)->message_type);
			dynpm_free_header(header);
			free(*msg);
			*msg = NULL;
			free(write_buffer);
			pthread_mutex_unlock(&(connection->messages_mutex));
			return -1;
	}

	if(_dynpm_enqueue_send_buffer(write_buffer, &(connection->messages_head))){
		info_logger("failed to enqueue message");
	}
	pthread_mutex_unlock(&(connection->messages_mutex));

	dynpm_free_header(header);
	free(write_buffer);

	return 0;
}

int dynpm_send_buffer_to_server(const void *buffer, int index) {
	if(!dynpm_client_initialized){
		info_logger("trying to send a message to a server before dynpm client is initialized");
		return -1;
	}

	pthread_mutex_lock(&server_connections_mutex);
	if(server_connections[index] == NULL){
		info_logger("NULL server connecction at index: %d", index);
		pthread_mutex_unlock(&server_connections_mutex);
		return -1;
	}
	pthread_mutex_unlock(&server_connections_mutex);

	return _dynpm_internal_send_buffer_to_server(buffer, server_connections[index]);
}

int dynpm_send_message_to_server(dynpm_header_t **header, void **msg, int index) {
	dynpm_client_to_server_connection_t *server_connection;

	if(!dynpm_client_initialized){
		info_logger("trying to send a message to a server before dynpm client is initialized");
		return -1;
	}

	pthread_mutex_lock(&server_connections_mutex);
	if(server_connections[index] == NULL){
		info_logger("NULL server connecction at index: %d", index);
		pthread_mutex_unlock(&server_connections_mutex);
		return -1;
	}
	pthread_mutex_unlock(&server_connections_mutex);

	return _dynpm_internal_send_message_to_server(header, msg, server_connections[index]);
}

int dynpm_send_header_to_server_connection(dynpm_source_type source, dynpm_message_type type,
		dynpm_client_to_server_connection_t *connection) {

	dynpm_header_t *ack_header = malloc(sizeof(dynpm_header_t));
	ack_header->source_type =  source;
	ack_header->message_type = type;

	void *ack_buf = malloc(sizeof(char)*4096);
	dynpm_serialize_header(ack_header, &ack_buf);

	if(dynpm_send_buffer_to_server(ack_buf, connection->index)){
		info_logger("failed to send buffer to server");
	}

	free(ack_header);
	free(ack_buf);
}

int dynpm_send_header_to_server(dynpm_source_type source, dynpm_message_type type,
		int index) {

	int return_code = 0;

	dynpm_header_t *ack_header = malloc(sizeof(dynpm_header_t));
	ack_header->source_type =  source;
	ack_header->message_type = type;

	void *ack_buf = malloc(sizeof(char)*4096);
	dynpm_serialize_header(ack_header, &ack_buf);

	if(dynpm_send_buffer_to_server(ack_buf, index)){
		info_logger("failed to send buffer to server");
		return_code = -1;
	}

	free(ack_header);
	free(ack_buf);

	return return_code;
}

// TODO this operation only works correctly if the server connections lock is acquired
// maybe it needs to become an internal call, and not an API call
int dynpm_disconnect_from_server(int server_index){
	info_logger("disconnecting from server at index %d", server_index);
	int return_code = 0;

	if(!dynpm_client_initialized){
		info_logger("trying to disconnect from  a server before dynpm client is initialized");
		return_code = -1;
	}

	pthread_mutex_lock(&server_connections_mutex);

	if(server_connections[server_index] == NULL){
		info_logger("trying to disconnect from an invalid server");
	}

	if(server_connections[server_index]){
		// this will trigger the removal of the remote connection
		if((server_connections[server_index])->evbase)
			event_base_loopexit((server_connections[server_index])->evbase, NULL);
	} else {
		info_logger("null server at index %d", server_index);
		return_code = -1;
	}

	//return_code = pthread_join(server_connections[server_index]->thread, NULL);
	//if(return_code) {
	//	info_logger("pthread %d at index %d already ended", server_connections[server_index]->thread, server_index);
	//	return_code = 0;
	//}

	server_connections[server_index] = NULL;
	server_connection_count--;
	assert(server_connection_count >= 0);

	pthread_mutex_unlock(&server_connections_mutex);

	return return_code;
}

// WARNING: can only be called if the lock is acquired first
// maybe it should be an internal op and not par of the API
void dynpm_print_server_connections(){
	int printed_count, server_index;

	if(!dynpm_client_initialized){
		info_logger("trying to print connections before libdynpm client was initialized");
	}

	for(int index = 0; index < MAX_SERVER_CONNECTIONS; index++){
		if(server_connections[index] != NULL){
			info_logger("connection %d is set", index);
		}
	}

	return;
}
