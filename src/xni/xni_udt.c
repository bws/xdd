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


#define PROTOCOL_NAME "udt-20150413"
#define ALIGN(val,align) (((val)+(align)-1UL) & ~((align)-1UL))

const char *XNI_UDT_DEFAULT_CONGESTION = "";

static const size_t UDT_DATA_MESSAGE_HEADER_SIZE = 12;

struct udt_control_block {
  size_t num_sockets;
  char congestion[16];
};

struct udt_context {
	// inherited from struct xni_context
	struct xni_protocol *protocol;

	// added by struct udt_context
	struct udt_control_block control_block;

    // added by struct udt_connection
    struct udt_target_buffer *registered_buffers;  // NULL-terminated
	size_t num_registered;
    pthread_mutex_t buffer_mutex;
    pthread_cond_t buffer_cond;

};

struct udt_socket {
  int sockd;
  int busy;
  int eof;
};

struct udt_connection {
    // inherited from struct xni_connection
    struct udt_context *context;

    int destination;

    struct udt_socket *sockets;
    int num_sockets;
    pthread_mutex_t socket_mutex;
    pthread_cond_t socket_cond;
};

struct udt_target_buffer {
  // inherited from xni_target_buffer
  struct udt_context *context;
  void *data;
  size_t target_offset;
  int data_length;

  // added by udt_target_buffer
  int busy;
  void *header;
};


int xni_allocate_udt_control_block(int num_sockets, const char *congestion, xni_control_block_t *cb_)
{
  struct udt_control_block **cb = (struct udt_control_block**)cb_;

  // progress would never be made...
  if (num_sockets < 1 && num_sockets != XNI_UDT_DEFAULT_NUM_SOCKETS)
    return XNI_ERR;

  // check for buffer overflow
  if (strlen(congestion) >= sizeof((*cb)->congestion))
    return XNI_ERR;

  struct udt_control_block *tmp = calloc(1, sizeof(*tmp));
  tmp->num_sockets = num_sockets;
  strncpy(tmp->congestion, congestion, (sizeof(tmp->congestion) - 1));
  *cb = tmp;
  return XNI_OK;
}

int xni_free_udt_control_block(xni_control_block_t *cb_)
{
  struct udt_control_block **cb = (struct udt_control_block**)cb_;

  free(*cb);

  *cb = NULL;
  return XNI_OK;
}

static int udt_context_create(xni_protocol_t proto_, xni_control_block_t cb_, xni_context_t *ctx_)
{
  struct xni_protocol *proto = proto_;
  struct udt_control_block *cb = (struct udt_control_block*)cb_;
  struct udt_context **ctx = (struct udt_context**)ctx_;
  size_t num_buffers = cb->num_sockets;
  assert(strcmp(proto->name, PROTOCOL_NAME) == 0);

  // fill in a new context
  struct udt_context *tmp = calloc(1, sizeof(*tmp));
  tmp->protocol = proto;
  tmp->control_block = *cb;
  tmp->registered_buffers = calloc(num_buffers, sizeof(*tmp->registered_buffers));
  pthread_mutex_init(&tmp->buffer_mutex, NULL);
  pthread_cond_init(&tmp->buffer_cond, NULL);
  
  // Swap in the context
  *ctx = tmp;
  return XNI_OK;
}

//TODO: do something with open connections
static int udt_context_destroy(xni_context_t *ctx_)
{
  struct udt_context **ctx = (struct udt_context **)ctx_;
  pthread_mutex_destroy(&(*ctx)->buffer_mutex);
  pthread_cond_destroy(&(*ctx)->buffer_cond);
  free((*ctx)->registered_buffers);
  free(*ctx);

  *ctx = NULL;
  return XNI_OK;
}

static int udt_register_buffer(xni_context_t ctx_, void* buf, size_t nbytes, size_t reserved, xni_target_buffer_t* tbp) {
	struct udt_context* ctx = (struct udt_context*) ctx_;
    uintptr_t beginp = (uintptr_t)buf;
    uintptr_t datap = (uintptr_t)buf + (uintptr_t)(reserved);
    size_t avail = (size_t)(datap - beginp);

	// Make sure space exists in the registered buffers array
	if (ctx->control_block.num_sockets <= ctx->num_registered)
		return XNI_ERR;
	
    // Make sure enough padding exists
    if (avail < UDT_DATA_MESSAGE_HEADER_SIZE)
        return XNI_ERR;

	// Add the buffer into the array of registered buffers
	pthread_mutex_lock(&ctx->buffer_mutex);
	struct udt_target_buffer *tb = ctx->registered_buffers + ctx->num_registered;
	tb->context = ctx;
	tb->data = (void*)datap;
	tb->target_offset = 0;
	tb->data_length = -1;
	tb->busy = 0;
	tb->header = (void*)(datap - UDT_DATA_MESSAGE_HEADER_SIZE);
	ctx->registered_buffers[ctx->num_registered] = *tb;
	ctx->num_registered++;
	pthread_mutex_unlock(&ctx->buffer_mutex);

	// Set the user's target buffer
	*tbp = (xni_target_buffer_t)&(tb);
    return XNI_OK;
}

static int udt_unregister_buffer(xni_context_t ctx, void* buf) {
    return XNI_OK;
}

static int udt_accept_connection(xni_context_t ctx_, struct xni_endpoint* local, xni_connection_t* conn_)
{
	struct udt_context *ctx = (struct udt_context*)ctx_;
	struct udt_connection **conn = (struct udt_connection**)conn_;

	const int num_sockets = ctx->control_block.num_sockets;

	// listening sockets
	int servers[num_sockets];
	for (int i = 0; i < num_sockets; i++)
		servers[i] = -1;

	// connected sockets
	struct udt_socket *clients = malloc(num_sockets*sizeof(*clients));
	for (int i = 0; i < num_sockets; i++) {
		clients[i].sockd = -1;
		clients[i].busy = 0;
		clients[i].eof = 0;
	}

	struct udt_connection *tmpconn = calloc(1, sizeof(*tmpconn));

	// place all server sockets in the listening state
	for (int i = 0; i < num_sockets; i++) {
		if ((servers[i] = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
			perror("socket");
			goto error_out;
		}

		int optval = 1;
		setsockopt(servers[i], SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

		const char * const congestion = ctx->control_block.congestion;
		if (strcmp(congestion, XNI_UDT_DEFAULT_CONGESTION) != 0) {
#if HAVE_DECL_UDT_CONGESTION
			if (setsockopt(servers[i],
						   SOL_UDT,
						   UDT_CONGESTION,
						   congestion,
						   strlen(congestion)) == -1) {
				perror("setsockopt");
				goto error_out;
			}
#else
			// operation not supported
			goto error_out;
#endif  // HAVE_DECL_UDT_CONGESTION
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

  /*// free any allocated target buffers
  for (struct udt_target_buffer **ptr = target_buffers; *ptr; ptr++)
    ctx->control_block.free_fn(*ptr);

    free(target_buffers);*/
  free(tmpconn);
  free(clients);

  return XNI_ERR;
}

static int udt_connect(xni_context_t ctx_, struct xni_endpoint* remote, xni_connection_t* conn_)
{
	struct udt_context *ctx = (struct udt_context*)ctx_;
	struct udt_connection **conn = (struct udt_connection**)conn_;

	const int num_sockets = ctx->control_block.num_sockets;

	// connected sockets
	struct udt_socket *servers = malloc(num_sockets*sizeof(*servers));
	for (int i = 0; i < num_sockets; i++) {
		servers[i].sockd = -1;
		servers[i].busy = 0;
		servers[i].eof = 1;
	}
  
	struct udt_connection *tmpconn = calloc(1, sizeof(*tmpconn));

	for (int i = 0; i < num_sockets; i++) {
		if ((servers[i].sockd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
			perror("socket");
			goto error_out;
		}

		const char * const congestion = ctx->control_block.congestion;
		if (strcmp(congestion, XNI_UDT_DEFAULT_CONGESTION) != 0) {
#if HAVE_DECL_UDT_CONGESTION
			if (setsockopt(servers[i].sockd,
						   SOL_UDT,
						   UDT_CONGESTION,
						   congestion,
						   strlen(congestion)) == -1) {
				perror("setsockopt");
				goto error_out;
			}
#else
			// operation not supported
			goto error_out;
#endif  // HAVE_DECL_UDT_CONGESTION
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

	//TODO: allocate target buffer list and attach

  tmpconn->context = ctx;
  tmpconn->destination = 0;
  tmpconn->sockets = servers;
  tmpconn->num_sockets = num_sockets;
  pthread_mutex_init(&tmpconn->socket_mutex, NULL);
  pthread_cond_init(&tmpconn->socket_cond, NULL);

  *conn = tmpconn;
  return XNI_OK;

 error_out:
  // close any opened server sockets
  for (int i = 0; i < num_sockets; i++)
    if (servers[i].sockd != -1)
      close(servers[i].sockd);

  // free any allocated target buffers
  //for (struct udt_target_buffer **ptr = target_buffers; *ptr; ptr++)
  //  ctx->control_block.free_fn(*ptr);

  //free(target_buffers);
  free(tmpconn);
  free(servers);

  return XNI_ERR;
}

//XXX: this is not going to be thread safe??
//alternatives: a flag that signals shutdown state
//and freeing the buffers as they become available on freelist
static int udt_close_connection(xni_connection_t *conn_)
{
  struct udt_connection **conn = (struct udt_connection**)conn_;

  struct udt_connection *c = *conn;

  for (int i = 0; i < c->num_sockets; i++)
    if (c->sockets[i].sockd != -1)
      close(c->sockets[i].sockd);

  /*BWS
  struct udt_target_buffer **buffers = (c->destination ? c->receive_buffers : c->send_buffers);
  while (*buffers++)
    c->context->control_block.free_fn(*buffers);
  */
  free(c);

  *conn = NULL;
  return XNI_OK;
}

static int udt_request_target_buffer(xni_context_t ctx_, xni_target_buffer_t *targetbuf_)
{
  struct udt_context *ctx = (struct udt_context*)ctx_;
  struct udt_target_buffer **targetbuf = (struct udt_target_buffer**)targetbuf_;
  struct udt_target_buffer *tb = NULL;
  
  pthread_mutex_lock(&ctx->buffer_mutex);
  while (tb == NULL) {
	  for (size_t i = 0; i < ctx->num_registered; i++) {
		  struct udt_target_buffer *ptr = ctx->registered_buffers + i;
		  if (!ptr->busy) {
			  tb = ptr;
			  tb->busy = 1;
			  break;
		  }
	  }
	  if (tb == NULL)
		  pthread_cond_wait(&ctx->buffer_cond, &ctx->buffer_mutex);
  }
  pthread_mutex_unlock(&ctx->buffer_mutex);
    
  *targetbuf = tb;
  return XNI_OK;
}

//TODO: what happens on error? stream state is trashed
static int udt_send_target_buffer(xni_connection_t conn_, xni_target_buffer_t *targetbuf_)
{
  struct udt_connection *conn = (struct udt_connection*)conn_;
  struct udt_target_buffer **targetbuf = (struct udt_target_buffer**)targetbuf_;
  struct udt_target_buffer *tb = *targetbuf;

  if (tb->data_length < 1)
    return XNI_ERR;

  // encode the message header
  uint64_t tmp64 = tb->target_offset;
  memcpy(tb->header, &tmp64, 8);
  uint32_t tmp32 = tb->data_length;
  memcpy(((char*)tb->header)+8, &tmp32, 4);

  // locate a free socket
  struct udt_socket *socket = NULL;
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
  pthread_mutex_lock(&tb->context->buffer_mutex);
  tb->busy = 0;
  pthread_cond_signal(&tb->context->buffer_cond);
  pthread_mutex_unlock(&tb->context->buffer_mutex);

  *targetbuf = NULL;
  return XNI_OK;
}

static int udt_receive_target_buffer(xni_connection_t conn_, xni_target_buffer_t *targetbuf_)
{
  struct udt_connection *conn = (struct udt_connection*)conn_;
  struct udt_target_buffer **targetbuf = (struct udt_target_buffer**)targetbuf_;
  int return_code = XNI_ERR;

  // grab a free buffer
  struct udt_target_buffer *tb = NULL;
  pthread_mutex_lock(&conn->context->buffer_mutex);
  while (tb == NULL) {
	  for (size_t i = 0; i < conn->context->num_registered; i++) {
		  struct udt_target_buffer *ptr = conn->context->registered_buffers + i;
		  if (!ptr->busy) {
			  tb = ptr;
			  tb->busy = 1;
			  break;
		  }
    }
    if (tb == NULL)
      pthread_cond_wait(&conn->context->buffer_cond, &conn->context->buffer_mutex);
  }
  pthread_mutex_unlock(&conn->context->buffer_mutex);

  struct udt_socket *socket = NULL;
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

  uint64_t target_offset;
  memcpy(&target_offset, recvbuf, 8);
  uint32_t data_length;
  memcpy(&data_length, recvbuf+8, 4);

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
    pthread_mutex_lock(&conn->context->buffer_mutex);
    tb->busy = 0;
    pthread_cond_signal(&conn->context->buffer_cond);
    pthread_mutex_unlock(&conn->context->buffer_mutex);
  }

  return return_code;
}

static int udt_release_target_buffer(xni_target_buffer_t *targetbuf_)
{
  struct udt_target_buffer **targetbuf = (struct udt_target_buffer**)targetbuf_;
  struct udt_target_buffer *tb = *targetbuf;

  tb->target_offset = 0;
  tb->data_length = -1;

  pthread_mutex_lock(&tb->context->buffer_mutex);
  tb->busy = 0;
  pthread_cond_signal(&tb->context->buffer_cond);
  pthread_mutex_unlock(&tb->context->buffer_mutex);
                         
  *targetbuf = NULL;
  return XNI_OK;
}


static struct xni_protocol protocol_udt = {
  .name = PROTOCOL_NAME,
  .context_create = udt_context_create,
  .context_destroy = udt_context_destroy,
  .register_buffer = udt_register_buffer,
  .unregister_buffer = udt_unregister_buffer,
  .accept_connection = udt_accept_connection,
  .connect = udt_connect,
  .close_connection = udt_close_connection,
  .request_target_buffer = udt_request_target_buffer,
  .send_target_buffer = udt_send_target_buffer,
  .receive_target_buffer = udt_receive_target_buffer,
  .release_target_buffer = udt_release_target_buffer,
};

struct xni_protocol *xni_protocol_udt = &protocol_udt;

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
