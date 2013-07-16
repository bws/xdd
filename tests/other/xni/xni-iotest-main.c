#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "xni.h"


#define BLOCK_LOW(id,p,n)  ((id)*(n)/(p))
#define BLOCK_HIGH(id,p,n) BLOCK_LOW((id)+1,p,n)
#define BLOCK_SIZE(id,p,n) \
                     (BLOCK_HIGH(id,p,n)-BLOCK_LOW(id,p,n))

#define MIN(a,b) ((a) < (b) ? (a) : (b))

#define FAIL() fail(__LINE__)
#define WORKER_FAIL() worker_fail(__LINE__)


struct worker_data {
  int buffer_size;
  xni_connection_t conn;
  int fd;
  off_t start;
  off_t end;
};


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
  struct worker_data *wd = arg;
  int return_code;
  do {
    xni_target_buffer_t tb;

    if ((return_code = xni_receive_target_buffer(wd->conn, &tb)) & XNI_ERR)
      WORKER_FAIL();

#ifdef XNI_TRACE
    if (return_code == XNI_EOF)
      printf("received EOF\n");
    else
      printf("received target_offset=%zu, data_length=%d\n",
             xni_target_buffer_target_offset(tb),
             xni_target_buffer_data_length(tb));
#endif  // XNI_TRACE

    if (return_code == XNI_OK) {
      size_t total = xni_target_buffer_data_length(tb);
      for (size_t written = 0; written < total;) {
        ssize_t cnt = pwrite(wd->fd,
                             (char*)xni_target_buffer_data(tb)+written,
                             (total - written),
                             (xni_target_buffer_target_offset(tb) + written));
        if (cnt < 0) {
          perror("pwrite");
          WORKER_FAIL();
        }
        written += cnt;
      }

      if (xni_release_target_buffer(&tb))
        WORKER_FAIL();
    }
  } while (return_code != XNI_EOF);

  return NULL;
}

static void *source_worker(void *arg)
{
  struct worker_data *wd = arg;

#ifdef XNI_TRACE
  printf("thead %p, start=%zd, end=%zd\n",
         pthread_self(),
         wd->start,
         wd->end);
#endif  // XNI_TRACE

  for (off_t file_offset = 0; file_offset < wd->end;) {
    xni_target_buffer_t tb;
    if (xni_request_target_buffer(wd->conn, &tb))
      WORKER_FAIL();

    xni_target_buffer_set_target_offset(file_offset, tb);
    size_t total = MIN(wd->buffer_size, (wd->end  - file_offset));
    xni_target_buffer_set_data_length(total, tb);

    for (size_t read = 0; read < total;) {
      ssize_t cnt = pread(wd->fd,
                          (char*)xni_target_buffer_data(tb)+read,
                          (total - read),
                          (file_offset + read));
      if (cnt < 0) {
        perror("pread");
        WORKER_FAIL();
      }
      read += cnt;
    }
    file_offset += total;

#ifdef XNI_TRACE
    printf("sending target_offset=%zu, data_length=%d\n",
           xni_target_buffer_target_offset(tb),
           xni_target_buffer_data_length(tb));
#endif  // XNI_TRACE

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
  int buffer_size = 4096;
  char *protocol = "tcp";
  char *file = NULL;
  while ((optchar = getopt(argc, argv, "sdt:n:c:p:f:")) != -1)
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
      buffer_size = atoi(optarg);
      break;
    case 'p':
      protocol = optarg;
      break;
    case 'f':
      file = optarg;
      break;
    default:
      FAIL();
    }
  if (destination == -1 || file == NULL)
    FAIL();

  if (buffer_size < 512 || buffer_size % 512 != 0)
    FAIL();

  // open the file
  const mode_t mode = S_IRUSR|S_IWUSR;
#ifdef linux
  const int flags = (destination
                     ? O_WRONLY|O_CREAT|O_EXCL|O_DIRECT
                     : O_RDONLY|O_DIRECT);
#else
  const int flags = (destination
                     ? O_WRONLY|O_CREAT|O_EXCL
                     : O_RDONLY);
#endif  // LINUX
  int fd = open(file, flags, mode);
  if (fd == -1) {
    perror("open");
    FAIL();
  }

#if defined(__APPLE__) && defined(__MACH__)
  if (fcntl(fd, F_NOCACHE, 1) == -1)
    FAIL();
#endif  // defined(__APPLE__) && defined(__MACH__)

  off_t file_size = 0;
  //TODO: when source get the file size
  if (!destination) {
    struct stat st;
    if (fstat(fd, &st))
      FAIL();
    if (!S_ISREG(st.st_mode))
      FAIL();
    file_size = st.st_size;
  }

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
    if (xni_accept_connection(ctx, &ep, num_buffers, buffer_size, &conn))
      FAIL();
  } else {
    if (xni_connect(ctx, &ep, num_buffers, buffer_size, &conn))
      FAIL();
  }

  pthread_t workers[num_threads];
  struct worker_data worker_data[num_threads];
  for (int i = 0; i < num_threads; i++) {
    worker_data[i].buffer_size = buffer_size;
    worker_data[i].conn = conn;
    worker_data[i].fd = fd;
    if (!destination) {
      worker_data[i].start = buffer_size*BLOCK_LOW(i, num_threads, file_size/buffer_size);
      worker_data[i].end = (i == num_threads - 1
                            ? file_size
                            : buffer_size*BLOCK_HIGH(i, num_threads, file_size/buffer_size));
    }
    if (pthread_create(workers+i,
                       NULL,
                       (destination ? destination_worker : source_worker),
                       worker_data+i))
      FAIL();
  }

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

  close(fd);

  puts("PASS");
  return 0;
}
