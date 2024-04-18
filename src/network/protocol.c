/*
 * Copyright (c) 2022-2024 Technical University of Munich (TUM)
 *
 * Additional copyrights may follow
 *
 */

#include "dynpm_network.h"

// header
void dynpm_free_header(dynpm_header_t **msg) {
	if (*msg) { free(*msg); }
    else { error_logger("NULL message in free_dynpm_header"); }
}

void dynpm_serialize_header(
        dynpm_header_t * header,
        void **buffer) {

    char *char_buff = (char*)(*buffer);
    sprintf(char_buff, "%d %d",
            header->source_type,
            header->message_type);
}

void dynpm_deserialize_header(dynpm_header_t ** header_ptr, const void **buffer) {
	dynpm_header_t * header;
	header = malloc(sizeof (dynpm_header_t));
	*header_ptr = header;

    char *char_buff = (char*)(*buffer);
    sscanf(char_buff, "%d %d", &(header->source_type), &(header->message_type));
}

// register
void dynpm_free_register_connection_msg( dynpm_register_connection_msg_t **msg) {
	if (*msg) {
		free((*msg)->hostname);
		free(*msg);
		// TODO deal with the applications once added
	}
    else { error_logger("NULL message in free_dynpm_register_connection_msg"); }
}

void dynpm_serialize_register_connection_msg(
        dynpm_header_t * header,
        dynpm_register_connection_msg_t * msg,
        void **buffer) {

    char *char_buff = (char*)(*buffer);
    sprintf(char_buff, "%d %d %d %s",
            header->source_type,
            header->message_type,
            msg->pid,
            msg->hostname);
}

void dynpm_deserialize_register_connection_msg(
        dynpm_register_connection_msg_t ** msg_ptr,
        const void **buffer) {

	dynpm_header_t *header = malloc(sizeof (dynpm_header_t));

	dynpm_register_connection_msg_t * msg;
	msg = malloc(sizeof(dynpm_register_connection_msg_t));
	*msg_ptr = msg;
    msg->hostname = malloc(256*sizeof(char));

    char *char_buff = (char*)(*buffer);
    sscanf(char_buff, "%d %d %d %s",
            &(header->source_type),
            &(header->message_type),
            &(msg->pid),
            (msg->hostname));

	free(header);
}

// deregister
void dynpm_free_deregister_connection_msg(dynpm_deregister_connection_msg_t **msg) {
	if (*msg) { free((*msg)->hostname); free(*msg); }
    else { error_logger("NULL message in free_dynpm_deregister_connection_msg"); }
}

void dynpm_serialize_deregister_connection_msg(
        dynpm_header_t * header,
        dynpm_deregister_connection_msg_t * msg,
        void **buffer) {

    char *char_buff = (char*)(*buffer);
    sprintf(char_buff, "%d %d %d %s %d",
            header->source_type,
            header->message_type,
            msg->pid,
            msg->hostname,
            msg->error_code);
}

void dynpm_deserialize_deregister_connection_msg(
        dynpm_deregister_connection_msg_t **msg_ptr,
        const void **buffer) {

	dynpm_header_t *header = malloc(sizeof (dynpm_header_t));

	dynpm_deregister_connection_msg_t * msg;
	msg = malloc(sizeof(dynpm_deregister_connection_msg_t));
	*msg_ptr = msg;
    msg->hostname = malloc(256);

    char *char_buff = (char*)(*buffer);
    sscanf(char_buff, "%d %d %d %s %d",
            &(header->source_type),
            &(header->message_type),
            &(msg->pid),
            (msg->hostname),
            &(msg->error_code));

	free(header);
}

// resource offer
void dynpm_free_resource_offer_msg(dynpm_resource_offer_msg_t **msg) {
	if (*msg) { free(*msg); }
    else { error_logger("NULL message in free_dynpm_resourc_offer"); }
}

void dynpm_serialize_resource_offer_msg(
        dynpm_header_t * header,
        dynpm_resource_offer_msg_t * msg,
        void **buffer) {

    char *char_buff = (char*)(*buffer);
    sprintf(char_buff, "%d %d %d %d %d %d %d",
            header->source_type,
            header->message_type,
            msg->application_index,
            msg->partition_index,
            msg->node_count,
            msg->creation_time,
            msg->end_time);
}

void dynpm_deserialize_resource_offer_msg(
        dynpm_resource_offer_msg_t ** msg_ptr,
        const void **buffer) {

	dynpm_header_t *header = malloc(sizeof (dynpm_header_t));

	dynpm_resource_offer_msg_t *msg;
	msg = malloc(sizeof(dynpm_resource_offer_msg_t));
	*msg_ptr = msg;

    char *char_buff = (char*)(*buffer);
    sscanf(char_buff, "%d %d %d %d %d %d %d",
            &(header->source_type),
            &(header->message_type),
            &(msg->application_index),
            &(msg->partition_index),
            &(msg->node_count),
            &(msg->creation_time),
            &(msg->end_time));

	free(header);
}

// create tbon
void dynpm_free_create_tbon_msg(dynpm_create_tbon_msg_t **msg) {
	if (*msg) { free((*msg)->nodes); free(*msg); }
    else { error_logger("NULL message in free_dynpm_create_tbon_msg"); }
}

void dynpm_serialize_create_tbon_msg(
        dynpm_header_t * header,
        dynpm_create_tbon_msg_t * msg,
        void **buffer) {

    char *char_buff = (char*)(*buffer);
    sprintf(char_buff, "%d %d %d %d %d %s",
            header->source_type,
            header->message_type,
            msg->id,
            msg->degree,
            msg->total,
            msg->nodes);
}

void dynpm_deserialize_create_tbon_msg(
        dynpm_create_tbon_msg_t ** msg_ptr,
        const void **buffer) {

	dynpm_header_t *header = malloc(sizeof (dynpm_header_t));

	dynpm_create_tbon_msg_t * msg;
	msg = malloc(sizeof(dynpm_create_tbon_msg_t));
	msg->nodes = malloc(sizeof(char)*256);
	*msg_ptr = msg;

    char *char_buff = (char*)(*buffer);
    sscanf(char_buff, "%d %d %d %d %d %s",
            &(header->source_type),
            &(header->message_type),
            &(msg->id),
            &(msg->degree),
            &(msg->total),
            msg->nodes);

	free(header);
}

// tbon ready types
static void _dynpm_free_ready_msg(dynpm_ready_msg_t **msg) {
	if (*msg) { free(*msg); }
    else { error_logger("NULL message in free_dynpm_ready_tbon_msg"); }
}

void dynpm_free_ready_tbon_msg(dynpm_ready_tbon_msg_t **msg) {
	_dynpm_free_ready_msg((dynpm_ready_msg_t**)msg);
}

void dynpm_free_ready_dynamic_pm_msg(dynpm_ready_dynamic_pm_msg_t **msg) {
	_dynpm_free_ready_msg((dynpm_ready_msg_t**)msg);
}

void dynpm_free_processes_done_msg(dynpm_processes_done_msg_t **msg) {
	_dynpm_free_ready_msg((dynpm_ready_msg_t**)msg);
}

static void _dynpm_serialize_ready_msg( dynpm_header_t *header, dynpm_ready_msg_t *msg, void **buffer) {
    char *char_buff = (char*)(*buffer);
    sprintf(char_buff, "%d %d %d %d %d",
            header->source_type,
            header->message_type,
            msg->id,
            msg->subtree_ready_count,
            msg->error_code);
	info_logger("%s", (char*)(*buffer));
}

void dynpm_serialize_ready_tbon_msg( dynpm_header_t * header, dynpm_ready_tbon_msg_t * msg, void **buffer) {
	_dynpm_serialize_ready_msg( header, (dynpm_ready_msg_t*)msg, buffer);
}

void dynpm_serialize_ready_dynamic_pm_msg( dynpm_header_t * header, dynpm_ready_dynamic_pm_msg_t * msg, void **buffer) {
	_dynpm_serialize_ready_msg( header, (dynpm_ready_msg_t*)msg, buffer);
}

void dynpm_serialize_processes_done_msg( dynpm_header_t * header, dynpm_processes_done_msg_t * msg, void **buffer) {
	_dynpm_serialize_ready_msg( header, (dynpm_ready_msg_t*)msg, buffer);
}

static void _dynpm_deserialize_ready_msg( dynpm_ready_msg_t ** msg_ptr, const void **buffer) {

	dynpm_header_t *header = malloc(sizeof (dynpm_header_t));

	dynpm_ready_msg_t * msg;
	msg = malloc(sizeof(dynpm_ready_msg_t));
	*msg_ptr = msg;

    char *char_buff = (char*)(*buffer);
    sscanf(char_buff, "%d %d %d %d %d",
            &(header->source_type),
            &(header->message_type),
            &(msg->id),
            &(msg->subtree_ready_count),
            &(msg->error_code));

	free(header);
}

void dynpm_deserialize_ready_tbon_msg( dynpm_ready_tbon_msg_t ** msg_ptr, const void **buffer) {
	_dynpm_deserialize_ready_msg( (dynpm_ready_msg_t **)msg_ptr, buffer);
}

void dynpm_deserialize_ready_dynamic_pm_msg( dynpm_ready_dynamic_pm_msg_t ** msg_ptr, const void **buffer) {
	_dynpm_deserialize_ready_msg( (dynpm_ready_msg_t **)msg_ptr, buffer);
}

void dynpm_deserialize_processes_done_msg( dynpm_processes_done_msg_t ** msg_ptr, const void **buffer) {
	_dynpm_deserialize_ready_msg( (dynpm_ready_msg_t **)msg_ptr, buffer);
}

// register process manager
void dynpm_free_register_process_manager_msg(dynpm_register_process_manager_msg_t **msg) {
	if (*msg) {
		if((*msg)->hostname) free((*msg)->hostname);
		for(int app = 0; app < (*msg)->application_count; app++){
			if((*msg)->applications[app]){
				if((*msg)->applications[app]->nodes){
					free((*msg)->applications[app]->nodes);
				}
				free((*msg)->applications[app]);
			}
		}
		if((*msg)->applications) free((*msg)->applications);
		free(*msg);
	}
	else { error_logger("NULL message in free_dynpm_register_process_manager_msg"); }
}

void dynpm_serialize_register_process_manager_msg(
        dynpm_header_t * header,
        dynpm_register_process_manager_msg_t * msg,
        void **buffer) {

    char *char_buff = (char*)(*buffer);

    sprintf(char_buff, "%d %d %d %s %d %d \n",
            header->source_type,
            header->message_type,
            msg->pid,
            msg->hostname,
            msg->job_index,
            msg->application_count);

	// TODO check if this is enough space
	// currently we do fixed size 1024 byte messages
	char *temp = malloc(256);
	for(int app; app < msg->application_count; app++){
		sprintf(temp, "%d %d %d %s \n",
				msg->applications[app]->application_index,
				msg->applications[app]->malleability_mode,
				msg->applications[app]->node_count,
				msg->applications[app]->nodes);
		strcat(char_buff, temp);
	}

	free(temp);
}

void dynpm_deserialize_register_process_manager_msg(
        dynpm_register_process_manager_msg_t ** msg_ptr,
        const void **buffer) {

	char *tokenized_line = NULL;
	dynpm_header_t *header = malloc(sizeof (dynpm_header_t));

	dynpm_register_process_manager_msg_t * msg;
	msg = malloc(sizeof(dynpm_register_process_manager_msg_t));
	*msg_ptr = msg;
    msg->hostname = malloc(256*sizeof(char));

	char *char_buff = (char*)(*buffer);
	tokenized_line = strtok(char_buff, "\r\n");

	sscanf(tokenized_line, "%d %d %d %s %d %d",
			&(header->source_type),
			&(header->message_type),
			&(msg->pid),
			msg->hostname,
			&(msg->job_index),
			&(msg->application_count));

	free(header);

	msg->applications = malloc(
			msg->application_count * sizeof(dynpm_application_t*));

	int app = 0;
    while (app < msg->application_count) {
		tokenized_line = strtok(NULL, "\r\n");
		msg->applications[app] = malloc(sizeof(dynpm_application_t));
		msg->applications[app]->nodes = malloc(512*sizeof(char));
		sscanf(tokenized_line, "%d %d %d %s",
				&(msg->applications[app]->application_index),
				&(msg->applications[app]->malleability_mode),
				&(msg->applications[app]->node_count),
				msg->applications[app]->nodes);
		app++;
    }
	assert(app == msg->application_count);
}

// deregister process manager
void dynpm_free_deregister_process_manager_msg(dynpm_deregister_process_manager_msg_t **msg) {
	if (*msg) { if((*msg)->hostname) free((*msg)->hostname); free(*msg); }
	else { error_logger("NULL message in free_dynpm_deregister_process_manager_msg"); }
}

void dynpm_serialize_deregister_process_manager_msg(
        dynpm_header_t * header,
        dynpm_deregister_process_manager_msg_t * msg,
        void **buffer) {

    char *char_buff = (char*)(*buffer);

    sprintf(char_buff, "%d %d %d %s %d \n",
            header->source_type,
            header->message_type,
            msg->pid,
            msg->hostname,
            msg->error_code);
}

void dynpm_deserialize_deregister_process_manager_msg(
        dynpm_deregister_process_manager_msg_t ** msg_ptr,
        const void **buffer) {

	dynpm_header_t *header = malloc(sizeof (dynpm_header_t));

	dynpm_deregister_process_manager_msg_t * msg;
	msg = malloc(sizeof(dynpm_deregister_process_manager_msg_t));
	*msg_ptr = msg;
    msg->hostname = malloc(256*sizeof(char));

	char *char_buff = (char*)(*buffer);
	sscanf(char_buff, "%d %d %d %s %d",
			&(header->source_type),
			&(header->message_type),
			&(msg->pid),
			msg->hostname,
			&(msg->error_code));

	free(header);
}

// allocation request
void dynpm_free_allocation_request_msg(dynpm_allocation_request_msg_t **msg) {
	if (*msg) { free(*msg); }
	else { error_logger("NULL message in free_dynpm_allocation_request_msg"); }
}

void dynpm_serialize_allocation_request_msg(
        dynpm_header_t * header,
        dynpm_allocation_request_msg_t * msg,
        void **buffer) {

    char *char_buff = (char*)(*buffer);
    sprintf(char_buff, "%d %d %d %d %d %d \n",
            header->source_type,
            header->message_type,
            msg->application_index,
            msg->partition_index,
            msg->type,
            msg->node_count);
}

void dynpm_deserialize_allocation_request_msg(
        dynpm_allocation_request_msg_t ** msg_ptr,
        const void **buffer) {

	dynpm_header_t *header = malloc(sizeof (dynpm_header_t));

	dynpm_allocation_request_msg_t * msg;
	msg = malloc(sizeof(dynpm_allocation_request_msg_t));
	*msg_ptr = msg;

	char *char_buff = (char*)(*buffer);
	sscanf(char_buff, "%d %d %d %d %d %d ",
			&(header->source_type),
			&(header->message_type),
			&(msg->application_index),
			&(msg->partition_index),
			&(msg->type),
			&(msg->node_count));

	free(header);
}

// reallocation start
void dynpm_free_reallocation_start_msg(dynpm_reallocation_start_msg_t **msg) {
	if (*msg) {
		if((*msg)->current_nodes) free((*msg)->current_nodes);
		if((*msg)->delta_nodes) free((*msg)->delta_nodes);
		if((*msg)->updated_nodes) free((*msg)->updated_nodes);
		free(*msg);
	} else { error_logger("NULL message in free_dynpm_reallocation_start_msg"); }
}

void dynpm_serialize_reallocation_start_msg(
        dynpm_header_t * header,
        dynpm_reallocation_start_msg_t * msg,
        void **buffer) {

    char *char_buff = (char*)(*buffer);
    sprintf(char_buff, "%d %d %d %d %d %d %s %d %s %d %s\n",
            header->source_type,
            header->message_type,
            msg->application_index,
            msg->partition_index,
            msg->type,
            msg->current_node_count,
            msg->current_nodes,
            msg->delta_node_count,
            msg->delta_nodes,
            msg->updated_node_count,
            msg->updated_nodes);
}

void dynpm_deserialize_reallocation_start_msg(
        dynpm_reallocation_start_msg_t ** msg_ptr,
        const void **buffer) {

	dynpm_header_t *header = malloc(sizeof (dynpm_header_t));

	dynpm_reallocation_start_msg_t * msg;
	msg = malloc(sizeof(dynpm_reallocation_start_msg_t));
	*msg_ptr = msg;
	msg->current_nodes = malloc(1024*sizeof(char));
	msg->delta_nodes = malloc(1024*sizeof(char));
	msg->updated_nodes = malloc(1024*sizeof(char));

	char *char_buff = (char*)(*buffer);
	sscanf(char_buff, "%d %d %d %d %d %d %s %d %s %d %s",
			&(header->source_type),
			&(header->message_type),
			&(msg->application_index),
			&(msg->partition_index),
			&(msg->type),
			&(msg->current_node_count),
			msg->current_nodes,
			&(msg->delta_node_count),
			msg->delta_nodes,
			&(msg->updated_node_count),
			msg->updated_nodes);

	free(header);
}

// reallocation complete
void dynpm_free_reallocation_complete_msg(dynpm_reallocation_complete_msg_t **msg) {
	if (*msg) { if((*msg)->nodes) free((*msg)->nodes); free(*msg); }
	else { error_logger("NULL message in free_dynpm_reallocation_complete_msg"); }
}

void dynpm_serialize_reallocation_complete_msg(
        dynpm_header_t * header,
        dynpm_reallocation_complete_msg_t * msg,
        void **buffer) {

    char *char_buff = (char*)(*buffer);
    sprintf(char_buff, "%d %d %d %d %d %d %s %d\n",
            header->source_type,
            header->message_type,
            msg->application_index,
            msg->partition_index,
            msg->type,
            msg->node_count,
            msg->nodes,
            msg->status_code);
}

void dynpm_deserialize_reallocation_complete_msg(
        dynpm_reallocation_complete_msg_t ** msg_ptr,
        const void **buffer) {

	dynpm_header_t *header = malloc(sizeof (dynpm_header_t));

	dynpm_reallocation_complete_msg_t * msg;
	msg = malloc(sizeof(dynpm_reallocation_complete_msg_t));
	*msg_ptr = msg;
	msg->nodes = malloc(256*sizeof(char));

	char *char_buff = (char*)(*buffer);
	sscanf(char_buff, "%d %d %d %d %d %d %s %d",
			&(header->source_type),
			&(header->message_type),
			&(msg->application_index),
			&(msg->partition_index),
			&(msg->type),
			&(msg->node_count),
			msg->nodes,
			&(msg->status_code));

	free(header);
}

// dynamic launch
void dynpm_free_dynamic_pm_launch_msg(dynpm_dynamic_pm_launch_msg_t **msg) {
	if (*msg) { if((*msg)->payload) free((*msg)->payload); free(*msg); }
	else { error_logger("NULL message in free_dynpm_dynamic_pm_launch_msg"); }
}

void dynpm_serialize_dynamic_pm_launch_msg(
        dynpm_header_t * header,
        dynpm_dynamic_pm_launch_msg_t * msg,
        void **buffer) {

    char *char_buff = (char*)(*buffer);
    sprintf(char_buff, "%d %d %d %d %d \n",
            header->source_type,
            header->message_type,
            msg->id,
            msg->type,
            msg->payload_bytes);

	// this value needs to be large enough that the string encoding does
	// not overflow under the max. lengths possible
	// TODO we have observer issues when these are not 64 bytes aligned
	// could be an issue with zlib or other reasons related to alignment
	//info_logger("starting memcpy at: %d", 256);
	assert(strlen(char_buff) < 256);

	if(msg->payload_bytes > 0){
		memcpy(char_buff + 256, msg->payload, msg->payload_bytes);
	}
}

void dynpm_deserialize_dynamic_pm_launch_msg(
        dynpm_dynamic_pm_launch_msg_t ** msg_ptr,
        const void **buffer) {

	char *tokenized_line = NULL;
	dynpm_header_t *header = malloc(sizeof (dynpm_header_t));

	dynpm_dynamic_pm_launch_msg_t * msg;
	msg = malloc(sizeof(dynpm_dynamic_pm_launch_msg_t));
	*msg_ptr = msg;

	char *char_buff = (char*)(*buffer);

	sscanf(char_buff, "%d %d %d %d %d",
			&(header->source_type),
			&(header->message_type),
			&(msg->id),
			&(msg->type),
			&(msg->payload_bytes));

	free(header);

	assert(strlen(char_buff) < 256);
	if(msg->payload_bytes > 0){
		msg->payload = malloc(msg->payload_bytes);
		// make sure that the offset matches the serialization
		// currently 256
		// TODO we have observer issues when these are not 64 bytes aligned
		// could be an issue with zlib or other reasons related to alignment
		memcpy(msg->payload, char_buff + 256, msg->payload_bytes);
	} else {
		msg->payload = NULL;
	}
}

// dynamic io out
void dynpm_free_dynamic_io_out_msg(dynpm_dynamic_io_out_msg_t **msg) {
	if (*msg) {
		if((*msg)->host) free((*msg)->host);
		if((*msg)->payload) free((*msg)->payload);
		free(*msg); }
	else { error_logger("NULL message in free_dynpm_dynamic_io_out_msg"); }
}

void dynpm_serialize_dynamic_io_out_msg(
        dynpm_header_t * header,
        dynpm_dynamic_io_out_msg_t * msg,
        void **buffer) {

    char *char_buff = (char*)(*buffer);
    sprintf(char_buff, "%d %d %d %s %d \n",
            header->source_type,
            header->message_type,
            msg->id,
            msg->host,
            msg->payload_bytes);

	// this value needs to be large enough that the string encoding does
	// not overflow under the max. lengths possible
	// TODO we have observer issues when these are not 64 bytes aligned
	// could be an issue with zlib or other reasons related to alignment
	//info_logger("starting memcpy at: %d", 256);
	assert(strlen(char_buff) < 256);

	if(msg->payload_bytes > 0){
		memcpy(char_buff + 256, msg->payload, msg->payload_bytes);
	}
}

void dynpm_deserialize_dynamic_io_out_msg(
        dynpm_dynamic_io_out_msg_t ** msg_ptr,
        const void **buffer) {

	dynpm_header_t *header = malloc(sizeof (dynpm_header_t));

	dynpm_dynamic_io_out_msg_t * msg;
	msg = malloc(sizeof(dynpm_dynamic_io_out_msg_t));
	*msg_ptr = msg;
	msg->host = malloc(256*sizeof(char));

	char *char_buff = (char*)(*buffer);

	sscanf(char_buff, "%d %d %d %s %d ",
			&(header->source_type),
			&(header->message_type),
			&(msg->id),
			msg->host,
			&(msg->payload_bytes));

	free(header);

	assert(strlen(char_buff) < 256);
	if(msg->payload_bytes > 0){
		msg->payload = malloc(msg->payload_bytes);
		// make sure that the offset matches the serialization
		// currently 256
		// TODO we have observer issues when these are not 64 bytes aligned
		// could be an issue with zlib or other reasons related to alignment
		memcpy(msg->payload, char_buff + 256, msg->payload_bytes);
	} else {
		msg->payload = NULL;
	}
}

// reporte process data
void dynpm_free_report_process_data_msg(dynpm_report_process_data_msg_t **msg) {
	if (*msg) {
		if((*msg)->payload) free((*msg)->payload);
		free(*msg); }
	else { error_logger("NULL message in free_dynpm_dynamic_io_out_msg"); }
}

void dynpm_serialize_report_process_data_msg(
        dynpm_header_t * header,
        dynpm_report_process_data_msg_t * msg,
        void **buffer) {

    char *char_buff = (char*)(*buffer);
    sprintf(char_buff, "%d %d %d %d %d %d %d\n",
            header->source_type,
            header->message_type,
            msg->id,
            msg->job_index,
            msg->application_index,
            msg->malleability_mode,
            msg->payload_bytes);

	assert(strlen(char_buff) < 256);

	if(msg->payload_bytes > 0){
		memcpy(char_buff + 256, msg->payload, msg->payload_bytes);
	}
}

void dynpm_deserialize_report_process_data_msg(
        dynpm_report_process_data_msg_t ** msg_ptr,
        const void **buffer) {

	dynpm_header_t *header = malloc(sizeof (dynpm_header_t));

	dynpm_report_process_data_msg_t * msg;
	msg = malloc(sizeof(dynpm_report_process_data_msg_t));
	*msg_ptr = msg;

	char *char_buff = (char*)(*buffer);

	sscanf(char_buff, "%d %d %d %d %d %d %d",
			&(header->source_type),
			&(header->message_type),
			&(msg->id),
			&(msg->job_index),
			&(msg->application_index),
			&(msg->malleability_mode),
			&(msg->payload_bytes));

	free(header);

	assert(strlen(char_buff) < 256);

	if(msg->payload_bytes > 0){
		msg->payload = malloc(msg->payload_bytes);
		memcpy(msg->payload, char_buff + 256, msg->payload_bytes);
	} else {
		msg->payload = NULL;
	}
}
