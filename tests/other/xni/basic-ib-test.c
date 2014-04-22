//
// A basic program, that if given -s, runs a server
// -c runs a client
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "xni.h"

#define BUFFER_PREPADDING 4096
// Bohr03 is 128.219.144.20
//#define DEFAULT_HOST "127.0.0.1"
#define DEFAULT_HOST "128.219.144.20"
#define DEFAULT_IBDEV "mlx4_1"

#define FAIL() fail(__LINE__)

static int fail(int lineno)
{
  printf("FAIL (line %d)\n", lineno);
  exit(EXIT_FAILURE);
}

int start_server()
{
    int rc = 0;
#ifdef HAVE_ENABLE_IB

    // First, XNI initialization stuff
    xni_initialize();

    // Second, create the context
    xni_control_block_t xni_cb = 0;
    xni_context_t xni_ctx;

    xni_allocate_ib_control_block(DEFAULT_IBDEV, 1, &xni_cb);
    xni_context_create(xni_protocol_ib, xni_cb, &xni_ctx);

	// Third, register the memroy
	xni_target_buffer_t xtb = 0;
    void* buf = 0;
    posix_memalign(&buf, 4096, BUFFER_PREPADDING + 512);
    memset(buf, 2, BUFFER_PREPADDING + 512);
    xni_register_buffer(xni_ctx, buf, BUFFER_PREPADDING + 512, BUFFER_PREPADDING, &xtb);
    
    // Third, accept connections
    xni_endpoint_t xni_ep = {.host = DEFAULT_HOST, .port = 40000};
    xni_connection_t xni_conn;
    if (xni_accept_connection(xni_ctx, &xni_ep, &xni_conn))
		FAIL();
    
    // Now pass a little data back and forth
    if (xni_receive_target_buffer(xni_conn, &xtb))
		FAIL();
	char* payload = xni_target_buffer_data(xtb);
	size_t l = xni_target_buffer_data_length(xtb);
	printf("First 32 bits set to: %d Second 32 bits set to: %d\n", payload[0], payload[4]);
	printf("Data length: %zd\n", l);
	xni_release_target_buffer(&xtb);
	
    // XNI cleanup stuff
    xni_unregister_buffer(xni_ctx, buf);
    xni_finalize();
    free(buf);
#else
	FAIL();
#endif
	return rc;
}

int start_client()
{
    int rc = 0;
#ifdef HAVE_ENABLE_IB
	
    // First, XNI initialization stuff
    xni_initialize();

    // Second, create the context
    xni_control_block_t xni_cb = 0;
    xni_context_t xni_ctx;
	xni_allocate_ib_control_block(DEFAULT_IBDEV, 1, &xni_cb);
    xni_context_create(xni_protocol_ib, xni_cb, &xni_ctx);
 
	// Third, register the buffers (1 per socket)
	xni_target_buffer_t xtb = 0;
    void* buf = 0;
    posix_memalign(&buf, 4096, BUFFER_PREPADDING + 512);
    memset(buf, 3, BUFFER_PREPADDING + 512);
	if (xni_register_buffer(xni_ctx, buf, BUFFER_PREPADDING + 512, BUFFER_PREPADDING, &xtb))
		FAIL();
	
    // Fourth, connect to the server
	xni_endpoint_t xni_ep = {.host = DEFAULT_HOST, .port = 40000};
    xni_connection_t xni_conn;
    if (xni_connect(xni_ctx, &xni_ep, &xni_conn))
 		FAIL();
	
    // Now pass a little data back and forth
	if (xni_request_target_buffer(xni_ctx, &xtb)) FAIL();
	xni_target_buffer_set_target_offset(0, xtb);
	xni_target_buffer_set_data_length(512, xtb);
	char* payload = xni_target_buffer_data(xtb);
	memset(payload, 0, 512);
	memset(payload, 1, 4);
	printf("First 32 bits set to: %d Second 32 bits set to: %d\n", payload[0], payload[4]);
	if (xni_send_target_buffer(xni_conn, &xtb)) FAIL();
	
    // XNI cleanup stuff
    xni_finalize();
    free(buf);
#else
	FAIL();
#endif
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
