#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/select.h>

#include "config.h"
#include "xni.h"
#include "xni_internal.h"


#define PROTOCOL_NAME "tcp-nlmills-20140602"
#define ALIGN(val,align) (((val)+(align)-1UL) & ~((align)-1UL))

const char *XNI_TCP_DEFAULT_CONGESTION = "";

static const size_t TCP_DATA_MESSAGE_HEADER_SIZE = 20;  // = sequence_number(8) + 
                                                        //   target_offset(8) +
                                                        //   data_length(4)

struct tcp_control_block {
  size_t num_sockets;
  char congestion[16];
  int window_size;
};

struct tcp_context {
	// inherited from struct xni_context
	struct xni_protocol *protocol;

	// added by struct tcp_context
	struct tcp_control_block control_block;
};

struct tcp_socket {
  int sockd;
  int busy;
  int eof;
};

struct tcp_connection {
    // inherited from struct xni_connection
    struct tcp_context *context;

    // added by struct tcp_connection
	// 1 = destination side, 0 = source side
    int destination;

    struct tcp_socket *sockets;
    int num_sockets;
    pthread_mutex_t socket_mutex;
    pthread_cond_t socket_cond;

	// size is equal to num_sockets
    struct tcp_target_buffer *registered_buffers;
	// count of registered buffers
	size_t num_registered;
	// these protect registered_buffers and num_registered
    pthread_mutex_t buffer_mutex;
    pthread_cond_t buffer_cond;
};

struct tcp_target_buffer {
  // inherited from xni_target_buffer
  struct tcp_connection *connection;
  void *data;
  int64_t sequence_number;
  size_t target_offset;
  int data_length;

  // added by tcp_target_buffer
  int busy;
  void *header;
};


int xni_allocate_tcp_control_block(int num_sockets, const char *congestion, int window_size, xni_control_block_t *cb_)
{
  struct tcp_control_block **cb = (struct tcp_control_block**)cb_;

  // progress would never be made...
  if (num_sockets < 1 && num_sockets != XNI_TCP_DEFAULT_NUM_SOCKETS)
    return XNI_ERR;

  // check for buffer overflow
  if (strlen(congestion) >= sizeof((*cb)->congestion))
    return XNI_ERR;

  // sanity check
  if (window_size < 1 && window_size != XNI_TCP_DEFAULT_WINDOW_SIZE)
	  return XNI_ERR;

  struct tcp_control_block *tmp = calloc(1, sizeof(*tmp));
  tmp->num_sockets = num_sockets;
  strncpy(tmp->congestion, congestion, (sizeof(tmp->congestion) - 1));
  tmp->window_size = window_size;
  *cb = tmp;
  return XNI_OK;
}

int xni_free_tcp_control_block(xni_control_block_t *cb_)
{
  struct tcp_control_block **cb = (struct tcp_control_block**)cb_;
  free(*cb);

  *cb = NULL;
  return XNI_OK;
}

static int tcp_context_create(xni_protocol_t proto, xni_control_block_t cb_, xni_context_t *ctx_)
{
  struct tcp_control_block *cb = (struct tcp_control_block*)cb_;
  struct tcp_context **ctx = (struct tcp_context**)ctx_;
  assert(strcmp(proto->name, PROTOCOL_NAME) == 0);

  // fill in a new context
  struct tcp_context *tmp = calloc(1, sizeof(*tmp));
  tmp->protocol = proto;
  tmp->control_block = *cb;
  
  // Swap in the context
  *ctx = tmp;
  return XNI_OK;
}

//TODO: do something with open connections
static int tcp_context_destroy(xni_context_t *ctx_)
{
  struct tcp_context **ctx = (struct tcp_context **)ctx_;
  free(*ctx);

  *ctx = NULL;
  return XNI_OK;
}

static int register_buffer(struct tcp_connection *conn, void* buf, size_t nbytes, size_t reserved) {
	// buffer base address
    uintptr_t beginp = (uintptr_t)buf;
	// buffer data area base address
    uintptr_t datap = (uintptr_t)buf + (uintptr_t)(reserved);
	// number of bytes available for headers
	//TODO: isn't this just the same as `reserved' ?
    size_t avail = (size_t)(datap - beginp);

	// Make sure space exists in the registered buffers array
	if (conn->context->control_block.num_sockets <= conn->num_registered)
		return XNI_ERR;
	
    // Make sure enough padding exists
    if (avail < TCP_DATA_MESSAGE_HEADER_SIZE)
        return XNI_ERR;

	// Add the buffer into the array of registered buffers
	pthread_mutex_lock(&conn->buffer_mutex);
	struct tcp_target_buffer *tb = conn->registered_buffers + conn->num_registered;
	tb->connection = conn;
	tb->data = (void*)datap;
	tb->sequence_number = 0;
	tb->target_offset = 0;
	tb->data_length = -1;
	tb->busy = 0;
	tb->header = (void*)(datap - TCP_DATA_MESSAGE_HEADER_SIZE);
	conn->num_registered++;
	pthread_mutex_unlock(&conn->buffer_mutex);

    return XNI_OK;
}

static int tcp_accept_connection(xni_context_t ctx_, struct xni_endpoint* local, xni_bufset_t *bufset, xni_connection_t* conn_)
{
	struct tcp_context *ctx = (struct tcp_context*)ctx_;
	struct tcp_connection **conn = (struct tcp_connection**)conn_;

	const int num_sockets = ctx->control_block.num_sockets;

	// listening sockets
	int servers[num_sockets];
	for (int i = 0; i < num_sockets; i++)
		servers[i] = -1;

	// connected sockets
	struct tcp_socket *clients = malloc(num_sockets*sizeof(*clients));
	for (int i = 0; i < num_sockets; i++) {
		clients[i].sockd = -1;
		clients[i].busy = 0;
		clients[i].eof = 0;
	}

	struct tcp_connection *tmpconn = calloc(1, sizeof(*tmpconn));

	// place all server sockets in the listening state
	for (int i = 0; i < num_sockets; i++) {
		if ((servers[i] = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
			perror("socket");
			goto error_out;
		}

		int optval = 1;
		setsockopt(servers[i], SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

		const char * const congestion = ctx->control_block.congestion;
		if (strcmp(congestion, XNI_TCP_DEFAULT_CONGESTION) != 0) {
#if HAVE_DECL_TCP_CONGESTION
			if (setsockopt(servers[i],
						   SOL_TCP,
						   TCP_CONGESTION,
						   congestion,
						   strlen(congestion)) == -1) {
				perror("setsockopt");
				goto error_out;
			}
#else
			// operation not supported
			goto error_out;
#endif  // HAVE_DECL_TCP_CONGESTION
		}

		// optionally set the window size
		if (ctx->control_block.window_size != XNI_TCP_DEFAULT_WINDOW_SIZE) {
			optval = ctx->control_block.window_size;
			int rc = setsockopt(servers[i],
								SOL_SOCKET,
								SO_SNDBUF,
								&optval,
								sizeof(optval));
			if (rc) {
				perror("setsockopt");
			}

			optval = ctx->control_block.window_size;
			rc = setsockopt(servers[i],
							SOL_SOCKET,
							SO_RCVBUF,
							&optval,
							sizeof(optval));
			if (rc) {
				perror("setsockopt");
			}
		}

		struct sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons((uint16_t)(local->port + i));
		addr.sin_addr.s_addr = inet_addr(local->host);
		if (bind(servers[i], (struct sockaddr*)&addr, sizeof(addr))) {
			perror("bind");
			goto error_out;
		}

		if (listen(servers[i], 1)) {
			perror("listen");
			goto error_out;
		}
	}

	// wait for all connections from the client
	for (int i = 0; i < num_sockets; i++) {
		if ((clients[i].sockd = accept(servers[i], NULL, NULL)) == -1) {
			perror("accept");
			goto error_out;
		}
		close(servers[i]);
		servers[i] = -1;
	}

	tmpconn->context = ctx;
	tmpconn->destination = 1;
	tmpconn->sockets = clients;
	tmpconn->num_sockets = num_sockets;
	pthread_mutex_init(&tmpconn->socket_mutex, NULL);
	pthread_cond_init(&tmpconn->socket_cond, NULL);
	tmpconn->registered_buffers = calloc(ctx->control_block.num_sockets,
										 sizeof(*tmpconn->registered_buffers));
	tmpconn->num_registered = 0;
	pthread_mutex_init(&tmpconn->buffer_mutex, NULL);
	pthread_cond_init(&tmpconn->buffer_cond, NULL);

	for (size_t i = 0; i < bufset->bufcount; i++) {
		int rc = register_buffer(tmpconn, bufset->bufs[i], bufset->bufsize, bufset->reserved);
		if (rc != XNI_OK) {
			goto error_out;
		}
	}

	*conn = tmpconn;
	return XNI_OK;

 error_out:
  // close any opened client sockets
  for (int i = 0; i < num_sockets; i++)
    if (clients[i].sockd != -1)
      close(clients[i].sockd);

  // close any opened server sockets
  for (int i = 0; i < num_sockets; i++)
    if (servers[i] != -1)
      close(servers[i]);

  if (tmpconn) {
	  free(tmpconn->registered_buffers);
	  free(tmpconn);
  }

  free(clients);

  return XNI_ERR;
}

static int tcp_connect(xni_context_t ctx_, struct xni_endpoint* remote, xni_bufset_t *bufset, xni_connection_t* conn_)
{
	struct tcp_context *ctx = (struct tcp_context*)ctx_;
	struct tcp_connection **conn = (struct tcp_connection**)conn_;

	const int num_sockets = ctx->control_block.num_sockets;

	// connected sockets
	struct tcp_socket *servers = malloc(num_sockets*sizeof(*servers));
	for (int i = 0; i < num_sockets; i++) {
		servers[i].sockd = -1;
		servers[i].busy = 0;
		servers[i].eof = 1;
	}
  
	struct tcp_connection *tmpconn = calloc(1, sizeof(*tmpconn));

	for (int i = 0; i < num_sockets; i++) {
		if ((servers[i].sockd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
			perror("socket");
			goto error_out;
		}

		const char * const congestion = ctx->control_block.congestion;
		if (strcmp(congestion, XNI_TCP_DEFAULT_CONGESTION) != 0) {
#if HAVE_DECL_TCP_CONGESTION
			if (setsockopt(servers[i].sockd,
						   SOL_TCP,
						   TCP_CONGESTION,
						   congestion,
						   strlen(congestion)) == -1) {
				perror("setsockopt");
				goto error_out;
			}
#else
			// operation not supported
			goto error_out;
#endif  // HAVE_DECL_TCP_CONGESTION
		}

		// optionally set the window size
		if (ctx->control_block.window_size != XNI_TCP_DEFAULT_WINDOW_SIZE) {
			int optval = ctx->control_block.window_size;
			int rc = setsockopt(servers[i].sockd,
								SOL_SOCKET,
								SO_SNDBUF,
								&optval,
								sizeof(optval));
			if (rc) {
				perror("setsockopt");
			}

			optval = ctx->control_block.window_size;
			rc = setsockopt(servers[i].sockd,
							SOL_SOCKET,
							SO_RCVBUF,
							&optval,
							sizeof(optval));
			if (rc) {
				perror("setsockopt");
			}
		}

		struct sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons((uint16_t)(remote->port + i));
		addr.sin_addr.s_addr = inet_addr(remote->host);
		if (connect(servers[i].sockd, (struct sockaddr*)&addr, sizeof(addr))) {
			perror("connect");
			goto error_out;
		}
	}

  tmpconn->context = ctx;
  tmpconn->destination = 0;
  tmpconn->sockets = servers;
  tmpconn->num_sockets = num_sockets;
  pthread_mutex_init(&tmpconn->socket_mutex, NULL);
  pthread_cond_init(&tmpconn->socket_cond, NULL);
  tmpconn->registered_buffers = calloc(ctx->control_block.num_sockets,
									   sizeof(*tmpconn->registered_buffers));
  tmpconn->num_registered = 0;
  pthread_mutex_init(&tmpconn->buffer_mutex, NULL);
  pthread_cond_init(&tmpconn->buffer_cond, NULL);
  
  for (size_t i = 0; i < bufset->bufcount; i++) {
	  int rc = register_buffer(tmpconn, bufset->bufs[i], bufset->bufsize, bufset->reserved);
	  if (rc != XNI_OK) {
		  goto error_out;
	  }
  }

  *conn = tmpconn;
  return XNI_OK;

 error_out:
  // close any opened server sockets
  for (int i = 0; i < num_sockets; i++)
    if (servers[i].sockd != -1)
      close(servers[i].sockd);

  if (tmpconn) {
	  free(tmpconn->registered_buffers);
	  free(tmpconn);
  }

  free(servers);

  return XNI_ERR;
}

//XXX: this is not going to be thread safe??
//alternatives: a flag that signals shutdown state
//and freeing the buffers as they become available on freelist
static int tcp_close_connection(xni_connection_t *conn_)
{
  struct tcp_connection **conn = (struct tcp_connection**)conn_;

  struct tcp_connection *c = *conn;

  for (int i = 0; i < c->num_sockets; i++)
    if (c->sockets[i].sockd != -1)
      close(c->sockets[i].sockd);

  pthread_mutex_destroy(&c->buffer_mutex);
  pthread_cond_destroy(&c->buffer_cond);
  free(c->registered_buffers);

  free(c);

  *conn = NULL;
  return XNI_OK;
}

static int tcp_request_target_buffer(xni_connection_t conn_, xni_target_buffer_t *targetbuf_)
{
  struct tcp_connection *conn = (struct tcp_connection*)conn_;
  struct tcp_target_buffer **targetbuf = (struct tcp_target_buffer**)targetbuf_;
  struct tcp_target_buffer *tb = NULL;
  
  pthread_mutex_lock(&conn->buffer_mutex);
  while (tb == NULL) {
	  for (size_t i = 0; i < conn->num_registered; i++) {
		  struct tcp_target_buffer *ptr = conn->registered_buffers + i;
		  if (!ptr->busy) {
			  tb = ptr;
			  tb->busy = 1;
			  break;
		  }
	  }
	  if (tb == NULL)
		  pthread_cond_wait(&conn->buffer_cond, &conn->buffer_mutex);
  }
  pthread_mutex_unlock(&conn->buffer_mutex);
    
  *targetbuf = tb;
  return XNI_OK;
}

//TODO: what happens on error? stream state is trashed
static int tcp_send_target_buffer(xni_connection_t conn_, xni_target_buffer_t *targetbuf_)
{
  struct tcp_connection *conn = (struct tcp_connection*)conn_;
  struct tcp_target_buffer **targetbuf = (struct tcp_target_buffer**)targetbuf_;
  struct tcp_target_buffer *tb = *targetbuf;

  if (tb->data_length < 1)
    return XNI_ERR;

  // encode the message header
  uint64_t tmp64 = htonll(tb->sequence_number);
  memcpy(tb->header, &tmp64, 8);
  tmp64 = htonll(tb->target_offset);
  memcpy(((char*)tb->header)+8, &tmp64, 8);
  uint32_t tmp32 = htonl(tb->data_length);
  memcpy(((char*)tb->header)+16, &tmp32, 4);

  // locate a free socket
  struct tcp_socket *socket = NULL;
  pthread_mutex_lock(&conn->socket_mutex);
  while (socket == NULL) {
    for (int i = 0; i < conn->num_sockets; i++)
      if (!conn->sockets[i].busy) {
        socket = conn->sockets+i;
        socket->busy = 1;
        break;
      }
    if (socket == NULL)
      pthread_cond_wait(&conn->socket_cond, &conn->socket_mutex);
  }
  pthread_mutex_unlock(&conn->socket_mutex);

  // send the message (header + data payload)
  const size_t total = (size_t)((char*)tb->data - (char*)tb->header) + tb->data_length;
  for (size_t sent = 0; sent < total;) {
    ssize_t cnt = send(socket->sockd, (char*)tb->header+sent, (total - sent), 0);
    //TODO: fix adding after EINTR logic
    if (cnt != -1)
      sent += cnt;
    else if (errno != EINTR) {
      perror("send");
      return XNI_ERR;
    }
  }

  // mark the socket as free
  pthread_mutex_lock(&conn->socket_mutex);
  socket->busy = 0;
  pthread_cond_signal(&conn->socket_cond);
  pthread_mutex_unlock(&conn->socket_mutex);

  // mark the buffer as free
  pthread_mutex_lock(&conn->buffer_mutex);
  tb->busy = 0;
  pthread_cond_signal(&conn->buffer_cond);
  pthread_mutex_unlock(&conn->buffer_mutex);

  *targetbuf = NULL;
  return XNI_OK;
}

static int tcp_receive_target_buffer(xni_connection_t conn_, xni_target_buffer_t *targetbuf_)
{
  struct tcp_connection *conn = (struct tcp_connection*)conn_;
  struct tcp_target_buffer **targetbuf = (struct tcp_target_buffer**)targetbuf_;
  int return_code = XNI_ERR;

  // grab a free buffer
  struct tcp_target_buffer *tb = NULL;
  pthread_mutex_lock(&conn->buffer_mutex);
  while (tb == NULL) {
	  for (size_t i = 0; i < conn->num_registered; i++) {
		  struct tcp_target_buffer *ptr = conn->registered_buffers + i;
		  if (!ptr->busy) {
			  tb = ptr;
			  tb->busy = 1;
			  break;
		  }
    }
    if (tb == NULL)
      pthread_cond_wait(&conn->buffer_cond, &conn->buffer_mutex);
  }
  pthread_mutex_unlock(&conn->buffer_mutex);

  struct tcp_socket *socket = NULL;
  while (socket == NULL) {
    // prepare to call select(2)
    fd_set outfds;
    FD_ZERO(&outfds);
    int maxsd = -1;

    pthread_mutex_lock(&conn->socket_mutex);
    for (int i = 0; i < conn->num_sockets; i++) {
      if (!conn->sockets[i].eof && conn->sockets[i].sockd != -1) {
        FD_SET(conn->sockets[i].sockd, &outfds);
        if (conn->sockets[i].sockd > maxsd)
          maxsd = conn->sockets[i].sockd;
      }
    }
    pthread_mutex_unlock(&conn->socket_mutex);
    if (maxsd == -1) {
      return_code = XNI_EOF;
      goto buffer_out;
    }

    if (select((maxsd + 1), &outfds, NULL, NULL, NULL) < 1)
      goto buffer_out;

    pthread_mutex_lock(&conn->socket_mutex);
    for (int i = 0; i < conn->num_sockets; i++)
      if (FD_ISSET(conn->sockets[i].sockd, &outfds) &&
          !conn->sockets[i].busy &&
          !conn->sockets[i].eof &&
          conn->sockets[i].sockd != -1) {
        socket = conn->sockets+i;
        socket->busy = 1;
        break;
      }
    //XXX: yes, no, maybe??
    //TODO: really only want to wait in case all sockets were busy
    //if (socket == NULL)
    //  pthread_cond_wait(&conn->socket_cond, &conn->socket_mutex);
    pthread_mutex_unlock(&conn->socket_mutex);
  }

  char *recvbuf = (char*)tb->header;
  size_t total = (size_t)((char*)tb->data - (char*)tb->header);
  for (size_t received = 0; received < total;) {
    ssize_t cnt = recv(socket->sockd, recvbuf+received, (total - received), 0);
    if (cnt == 0) {  //TODO: handle true EOF; true EOF only on first read
      return_code = XNI_EOF;
      goto socket_out;
    } else if (cnt == -1) {  //TODO: handle EINTR
      perror("recv");
      goto socket_out;
    } else
      received += cnt;
  }

  // decode the header
  uint64_t sequence_number;
  memcpy(&sequence_number, recvbuf, 8);
  sequence_number = ntohll(sequence_number);
  uint64_t target_offset;
  memcpy(&target_offset, recvbuf+8, 8);
  target_offset = ntohll(target_offset);
  uint32_t data_length;
  memcpy(&data_length, recvbuf+16, 4);
  data_length = ntohl(data_length);

  recvbuf = (char*)tb->data;
  total = data_length;
  for (size_t received = 0; received < total;) {
    ssize_t cnt = recv(socket->sockd, recvbuf+received, (total - received), 0);
    if (cnt == 0)  // failure EOF
      goto socket_out;
    else if (cnt == -1) {  //TODO: EINTR
      perror("recv");
      goto socket_out;
    } else
      received += cnt;
  }

  //TODO: sanity checks (e.g. tb->connection)
  tb->sequence_number = sequence_number;
  tb->target_offset = target_offset;
  tb->data_length = (int)data_length;
  
  *targetbuf = tb;
  return_code = XNI_OK;

 socket_out:
  // mark the socket as free
  pthread_mutex_lock(&conn->socket_mutex);
  socket->busy = 0;
  if (return_code == XNI_EOF)
    socket->eof = 1;
  //TODO: re-enable this
  //pthread_cond_signal(&conn->socket_cond);
  pthread_mutex_unlock(&conn->socket_mutex);

 buffer_out:
  if (return_code != XNI_OK) {
    // mark the buffer as free
    pthread_mutex_lock(&conn->buffer_mutex);
    tb->busy = 0;
    pthread_cond_signal(&conn->buffer_cond);
    pthread_mutex_unlock(&conn->buffer_mutex);
  }

  return return_code;
}

static int tcp_release_target_buffer(xni_target_buffer_t *targetbuf_)
{
  struct tcp_target_buffer **targetbuf = (struct tcp_target_buffer**)targetbuf_;
  struct tcp_target_buffer *tb = *targetbuf;

  tb->target_offset = 0;
  tb->data_length = -1;

  //TODO: is it really OK to access the lock protecting an object
  // through the object itself?
  pthread_mutex_lock(&tb->connection->buffer_mutex);
  tb->busy = 0;
  pthread_cond_signal(&tb->connection->buffer_cond);
  pthread_mutex_unlock(&tb->connection->buffer_mutex);
                         
  *targetbuf = NULL;
  return XNI_OK;
}


static struct xni_protocol protocol_tcp = {
  .name = PROTOCOL_NAME,
  .context_create = tcp_context_create,
  .context_destroy = tcp_context_destroy,
  .accept_connection = tcp_accept_connection,
  .connect = tcp_connect,
  .close_connection = tcp_close_connection,
  .request_target_buffer = tcp_request_target_buffer,
  .send_target_buffer = tcp_send_target_buffer,
  .receive_target_buffer = tcp_receive_target_buffer,
  .release_target_buffer = tcp_release_target_buffer,
};

struct xni_protocol *xni_protocol_tcp = &protocol_tcp;

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
