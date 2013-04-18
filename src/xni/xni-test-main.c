#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "xni.h"


#define FAIL() fail(__LINE__)
#define WORKER_FAIL() worker_fail(__LINE__)


static struct {
  int buffer_size;
  int num_iters;
} g_options;


static int fail(int lineno)
{
  printf("FAIL (line %d)\n", lineno);
  exit(EXIT_FAILURE);
}

static int worker_fail(int lineno)
{
  printf("FAIL (line %d)\n", lineno);
  pthread_exit((void*)1);
}

static int control_block_from_option(const char *option, xni_control_block_t *outcb)
{
  xni_control_block_t cb;

  if (strncmp(option, "tcp", 3) == 0) {
    const char *ptr = option+3;
    int num_sockets = 0;
    if (*ptr == '\0')
      num_sockets = XNI_TCP_DEFAULT_NUM_SOCKETS;
    else if (*ptr == ',')
      num_sockets = atoi(ptr+1);
    else
      FAIL();

    if (xni_allocate_tcp_control_block(NULL, NULL, num_sockets, &cb))
      FAIL();
  } else if (strncmp(option, "ib", 2) == 0) {
    const char *device_name;
    const char *ptr = option+2;
    if (*ptr == '\0')
      device_name = NULL;
    else if (*ptr == ',')
      device_name = ptr+1;
    else
      FAIL();

    if (xni_allocate_ib_control_block(NULL, NULL, device_name, &cb))
      FAIL();
  } else
    return 1;

  *outcb = cb;
  return 0;
}

static void *destination_worker(void *arg)
{
  xni_connection_t conn = *(xni_connection_t*)arg;

  int return_code;
  do {
    xni_target_buffer_t tb;

    if ((return_code = xni_receive_target_buffer(conn, &tb)) & XNI_ERR)
      WORKER_FAIL();

    if (return_code == XNI_OK) {
      if (xni_target_buffer_target_offset(tb)%g_options.buffer_size != 0)
        WORKER_FAIL();

      if (xni_target_buffer_data_length(tb) != g_options.buffer_size)
        WORKER_FAIL();

      unsigned char *data = (unsigned char*)xni_target_buffer_data(tb);
      for (int j = 0; j < g_options.buffer_size; j++)
        if (data[j] != 0xFA)
          WORKER_FAIL();

      if (xni_release_target_buffer(&tb))
        WORKER_FAIL();
    }
  } while (return_code != XNI_EOF);

  return NULL;
}

static void *source_worker(void *arg)
{
  xni_connection_t conn = *(xni_connection_t*)arg;

  for (int i = 0; i < g_options.num_iters; i++) {
    xni_target_buffer_t tb;
    if (xni_request_target_buffer(conn, &tb))
      WORKER_FAIL();

    void *data = xni_target_buffer_data(tb);
    memset(data, 0xFA, g_options.buffer_size);
    xni_target_buffer_set_target_offset(i*g_options.buffer_size, tb);
    xni_target_buffer_set_data_length(g_options.buffer_size, tb);

    if (xni_send_target_buffer(&tb))
      WORKER_FAIL();
  }

  return NULL;
}

int main(int argc, char **argv)
{
  // parse command line options
  int optchar;
  int destination = -1;
  int num_threads = 1;
  int num_buffers = 1;
  char *protocol = "tcp";
  while ((optchar = getopt(argc, argv, "sdt:n:c:p:i:")) != -1)
    switch (optchar) {
    case 's':
      destination = false;
      break;
    case 'd':
      destination = true;
      break;
    case 't':
      num_threads = atoi(optarg);
      break;
    case 'n':
      num_buffers = atoi(optarg);
      break;
    case 'c':
      g_options.buffer_size = atoi(optarg);
      break;
    case 'p':
      protocol = optarg;
      break;
    case 'i':
      g_options.num_iters = atoi(optarg);
      break;
    default:
      FAIL();
    }
  if (destination == -1)
    FAIL();

  // parse address, port arguments
  struct xni_endpoint ep;
  if (optind >= (argc - 1))
    FAIL();
  ep.host = argv[optind++];
  ep.port = atoi(argv[optind++]);

  xni_control_block_t cb;
  if (control_block_from_option(protocol, &cb))
    FAIL();

  xni_context_t ctx;
  if (strncmp(protocol, "tcp", 3) == 0) {
    if (xni_context_create(xni_protocol_tcp, cb, &ctx))
      FAIL();
    xni_free_tcp_control_block(&cb);
  } else if (strncmp(protocol, "ib", 2) == 0) {
    if (xni_context_create(xni_protocol_ib, cb, &ctx))
      FAIL();
    xni_free_ib_control_block(&cb);
  } else
    FAIL();
  
  xni_connection_t conn;
  if (destination) {
    if (xni_accept_connection(ctx, &ep, num_buffers, g_options.buffer_size, &conn))
      FAIL();
  } else {
    if (xni_connect(ctx, &ep, num_buffers, g_options.buffer_size, &conn))
      FAIL();
  }

  pthread_t workers[num_threads];
  for (int i = 0; i < num_threads; i++)
    if (pthread_create(workers+i,
                       NULL,
                       (destination ? destination_worker : source_worker),
                       &conn))
      FAIL();

  for (int i = 0; i < num_threads; i++) {
    void *retval;
    if (pthread_join(workers[i], &retval))
      FAIL();
    if (retval)
      FAIL();
  }
  
  if (xni_close_connection(&conn))
    FAIL();

  if (xni_context_destroy(&ctx))
    FAIL();

  puts("PASS");
  return 0;
}
