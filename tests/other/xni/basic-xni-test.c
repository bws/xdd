//
// A basic program, that if given -s, runs a server
// -c runs a client
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xni.h"

#define BUFFER_PREPADDING 4096
#define DEFAULT_HOST "128.219.144.20"

int start_server()
{
    int rc = 0;

    // First, XNI initialization stuff
    xni_initialize();

    // Second, create the context
    xni_endpoint_t xni_ep = {.host = "", .port = 0};
    xni_control_block_t xni_cb = 0;
    xni_context_t xni_ctx;

    xni_allocate_tcp_control_block(1, XNI_TCP_DEFAULT_CONGESTION, &xni_cb);
    xni_context_create(xni_protocol_tcp, xni_cb, &xni_ctx);

	// Third, register the memroy
	xni_target_buffer_t xtb = 0;
    void* buf = 0;
    posix_memalign(&buf, 4096, BUFFER_PREPADDING + 512);
    xni_register_buffer(xni_ctx, buf, BUFFER_PREPADDING + 512, BUFFER_PREPADDING, &xtb);
    
    // Third, accept connections
    xni_ep.host = DEFAULT_HOST;
    xni_ep.port = 40000;
    xni_connection_t xni_conn;
    xni_accept_connection(xni_ctx, &xni_ep, &xni_conn);
    
    // Now pass a little data back and forth
    xni_receive_target_buffer(xni_conn, &xtb);
	char* payload = xni_target_buffer_data(xtb);
	size_t l = xni_target_buffer_data_length(xtb);
	printf("First 32 bits set to: %d Second 32 bits set to: %d\n", payload[0], payload[4]);
	printf("Data length: %zd\n", l);
	xni_release_target_buffer(&xtb);
	
    // XNI cleanup stuff
    xni_unregister_buffer(xni_ctx, buf);
    xni_finalize();
    free(buf);
    return rc;
}

int start_client()
{
    int rc = 0;
	
    // First, XNI initialization stuff
    xni_initialize();

    // Second, create the context
	xni_endpoint_t xni_ep = {.host = "", .port = 0};
    xni_control_block_t xni_cb = 0;
    xni_context_t xni_ctx;
	xni_allocate_tcp_control_block(1, XNI_TCP_DEFAULT_CONGESTION, &xni_cb);
    xni_context_create(xni_protocol_tcp, xni_cb, &xni_ctx);
 
	// Third, register the buffers (1 per socket)
	xni_target_buffer_t xtb = 0;
    void* buf = 0;
    posix_memalign(&buf, 4096, BUFFER_PREPADDING + 512);
	xni_register_buffer(xni_ctx, buf, BUFFER_PREPADDING + 512, BUFFER_PREPADDING, &xtb);
	
    // Fourth, connect to the server
	xni_ep.host = DEFAULT_HOST;
    xni_ep.port = 40000;
    xni_connection_t xni_conn;
    xni_connect(xni_ctx, &xni_ep, &xni_conn);
	
    // Now pass a little data back and forth
	xni_request_target_buffer(xni_ctx, &xtb);
	xni_target_buffer_set_target_offset(0, xtb);
	xni_target_buffer_set_data_length(512, xtb);
	char* payload = xni_target_buffer_data(xtb);
	memset(payload, 2, 512);
	memset(payload, 1, 4);
	printf("First 32 bits set to: %d Second 32 bits set to: %d\n", payload[0], payload[4]);
	xni_send_target_buffer(xni_conn, &xtb);
	
    // XNI cleanup stuff
    xni_finalize();
    free(buf);
    
    return rc;
}

int main(int argc, char** argv)
{
    int rc = 0;

    if (2 <= argc && 0 == strcmp(argv[1], "-s")) {
        rc = start_server();
    }
    else if (2 <= argc && 0 == strcmp(argv[1], "-c")) {
        rc = start_client();
    }
    else {
        if (argc > 1)
            printf("Unknown option: >%s<\n", argv[1]);
        printf("Usage: %s [-c|-s]\n", argv[0]);
        rc = 1;
    }
    return rc;
}

/*
 * Local variables:
 *  indent-tabs-mode: t
 *  default-tab-width: 4
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=4 sts=4 sw=4 noexpandtab
 */
