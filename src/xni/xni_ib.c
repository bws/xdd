#include "config.h"
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
#include <arpa/inet.h>

#ifdef HAVE_INFINIBAND_VERBS_H
#include <infiniband/verbs.h>

#include "xni.h"
#include "xni_internal.h"

//#define XNI_TRACE 1
#define PROTOCOL_NAME "ib-nlmills-20120809"
#define ALIGN(val,align) (((val)+(align)-1UL) & ~((align)-1UL))


struct ib_control_block {
	char device_name[IBV_SYSFS_NAME_MAX];
	size_t num_buffers;
};

struct ib_context {
	// inherited from struct xni_context
	struct xni_protocol *protocol;

	// added by struct ib_context
	struct ib_control_block control_block;
	struct ibv_context *verbs_context;
	struct ibv_pd *domain;

	// target buffer registration data
	struct ib_target_buffer *target_buffers;
	size_t num_registered;

	// locks
	pthread_mutex_t target_buffers_mutex;
	pthread_cond_t target_buffers_cond;
	pthread_mutex_t busy_flag_mutex;
	pthread_cond_t busy_flag_cond;

};

struct ib_connection {
	// inherited from struct xni_connection
	struct ib_context *context;

	//
	// added by struct ib_connection
	//
	struct ib_credit_buffer **credit_buffers;  // NULL-terminated
	int credits;
	int eof;

	// locks
	pthread_mutex_t send_state_mutex;
	pthread_mutex_t credit_mutex;  

	int destination;  // 0 = source, otherwise destination
	
	struct ibv_cq *send_cq;
	struct ibv_cq *receive_cq;
	struct ibv_qp *queue_pair;

	uint32_t remote_qpnum;
	uint16_t remote_lid;
};

#define IB_DATA_MESSAGE_HEADER_SIZE 12 // = tag(4) + target_offset(8)
enum send_state {
  QUEUED,
  SENT,
  ERROR,
};
struct ib_target_buffer {
	// inherited from xni_target_buffer
	struct ib_context *context;
	void *data;
	size_t target_offset;
	int data_length;

	// added by struct ib_target_buffer
	size_t buffer_size;
	int busy;
	enum send_state send_state;
	struct ibv_mr *memory_region;
	void *header;
	struct ib_connection *connection;
};

#define IB_CREDIT_MESSAGE_SIZE 8  // = tag(4) + credits(4)
struct ib_credit_buffer {
  struct ibv_mr *memory_region;
  int busy;
  char msgbuf[IB_CREDIT_MESSAGE_SIZE];
};


static const int TAG_LENGTH = 4;
static const char * const DATA_MESSAGE_TAG = "DATA";
static const char * const CREDIT_MESSAGE_TAG = "CRED";
static const char * const EOF_MESSAGE_TAG = "EOF ";


int xni_allocate_ib_control_block(const char *device_name, size_t num_buffers, xni_control_block_t *cb_)
{
  struct ib_control_block **cb = (struct ib_control_block**)cb_;

  struct ib_control_block *tmp = calloc(1, sizeof(*tmp));
  if (device_name == NULL)
    strcpy(tmp->device_name, "");
  else
    strncpy(tmp->device_name, device_name, IBV_SYSFS_NAME_MAX);
  tmp->num_buffers = num_buffers;
  *cb = tmp;
  return XNI_OK;
}

int xni_free_ib_control_block(xni_control_block_t *cb_)
{
  struct ib_control_block **cb = (struct ib_control_block**)cb_;
  free(*cb);

  *cb = NULL;
  return XNI_OK;
}

static int ib_context_create(xni_protocol_t proto_, xni_control_block_t cb_, xni_context_t *ctx_)
{
  struct xni_protocol *proto = proto_;
  struct ib_control_block *cb = (struct ib_control_block*)cb_;
  struct ib_context **ctx = (struct ib_context**)ctx_;

  assert(strcmp(proto->name, PROTOCOL_NAME) == 0);

  struct ibv_device **devlist = ibv_get_device_list(NULL);
  if (devlist == NULL) {
    fputs("No InfiniBand devices found.\n", stderr);
    return XNI_ERR;
  }

  struct ibv_device **deviceptr;
  for (deviceptr = devlist; *deviceptr != NULL; deviceptr++) {
	  if (*cb->device_name == '\0' ||
		  strcmp(cb->device_name, ibv_get_device_name(*deviceptr)) == 0)
		  break;
  }
  if (*deviceptr == NULL) {
    fputs("No matching InfiniBand device found.\n", stderr);
    ibv_free_device_list(devlist);
    return XNI_ERR;
  }
  
  struct ibv_context *verbsctx = ibv_open_device(*deviceptr);
  ibv_free_device_list(devlist);
  if (verbsctx == NULL) {
    fputs("Unable to open InfiniBand device.\n", stderr);
    return XNI_ERR;
  }

  struct ibv_pd *pd = ibv_alloc_pd(verbsctx);
  if (pd == NULL) {
    fputs("Unable to allocate protection domain.\n", stderr);
    (void)ibv_close_device(verbsctx);
    return XNI_ERR;
  }

  struct ib_context *tmp = calloc(1, sizeof(*tmp));
  tmp->protocol = proto;
  tmp->control_block = *cb;
  tmp->verbs_context = verbsctx;
  tmp->domain = pd;
  tmp->target_buffers = calloc(cb->num_buffers, sizeof(*tmp->target_buffers));
  tmp->num_registered = 0;
  pthread_mutex_init(&tmp->target_buffers_mutex, NULL);
  pthread_cond_init(&tmp->target_buffers_cond, NULL);
  pthread_mutex_init(&tmp->busy_flag_mutex, NULL);
  pthread_cond_init(&tmp->busy_flag_cond, NULL);

  *ctx = tmp;
  return XNI_OK;
}

static int ib_context_destroy(xni_context_t *ctx_)
{
  struct ib_context **ctx = (struct ib_context**)ctx_;

  (void)ibv_dealloc_pd((*ctx)->domain);
  (void)ibv_close_device((*ctx)->verbs_context);

  free(*ctx);

  *ctx = NULL;
  return XNI_OK;
}

static int ib_register_buffer(xni_context_t ctx_, void* buf, size_t nbytes, size_t reserved,
							  xni_target_buffer_t* xtb)
{
	struct ib_context* ctx = (struct ib_context*)ctx_;
	uintptr_t beginp = (uintptr_t)buf;
	uintptr_t datap = (uintptr_t)buf + (uintptr_t)(reserved);
	size_t avail = (size_t)(datap - beginp);

	// Make sure space exists in the registered buffer array
	if (ctx->control_block.num_buffers <= ctx->num_registered)
		return XNI_ERR;

	// Make sure enough padding exists
	if (avail < IB_DATA_MESSAGE_HEADER_SIZE)
		return XNI_ERR;

	// Register the memory with verbs
	struct ibv_mr *mr = ibv_reg_mr(ctx->domain, buf, nbytes,
								   IBV_ACCESS_LOCAL_WRITE);
	if (NULL == mr)
		return XNI_ERR;

	// Add the buffer into the array of registered buffers
	pthread_mutex_lock(&ctx->target_buffers_mutex);
	struct ib_target_buffer* tb = ctx->target_buffers + ctx->num_registered;
	tb->context = ctx;
	tb->data = (void*)datap;
	tb->data_length = -1;
	tb->buffer_size = nbytes - reserved;
	tb->busy = 0;
	tb->send_state = 0;
	tb->memory_region = mr;
	tb->header = (void*)(datap - IB_DATA_MESSAGE_HEADER_SIZE);
	tb->connection = NULL;
	ctx->num_registered++;
	pthread_mutex_unlock(&ctx->target_buffers_mutex);

	// Set the outbound target buffer
	*xtb= (xni_target_buffer_t)&tb;
	return XNI_OK;
}

static int ib_unregister_buffer(xni_context_t ctx_, void* buf)
{
	struct ib_context* ctx = (struct ib_context*)ctx_;
	pthread_mutex_lock(&ctx->target_buffers_mutex);
	pthread_mutex_unlock(&ctx->target_buffers_mutex);
    return XNI_OK;
}

static struct ib_credit_buffer **allocate_credit_buffers(struct ib_context *ctx, int nbuf)
{
	struct ib_credit_buffer **credit_buffers = calloc((nbuf + 1), sizeof(*credit_buffers));

	for (int i = 0; i < nbuf; i++) {
		struct ib_credit_buffer *cb = calloc(1, sizeof(*cb));
		if (cb == NULL)
			goto error_out;

		struct ibv_mr *mr = ibv_reg_mr(ctx->domain, cb, sizeof(*cb), IBV_ACCESS_LOCAL_WRITE);
		if (mr == NULL)
			goto error_out;

		cb->memory_region = mr;
		cb->busy = 0;

		credit_buffers[i] = cb;
	}

	return credit_buffers;

  error_out:
	for (struct ib_credit_buffer **ptr = credit_buffers; *ptr != NULL; ptr++) {
		if ((*ptr)->memory_region != NULL)
			(void)ibv_dereg_mr((*ptr)->memory_region);
		free(*ptr);
	}
	free(credit_buffers);

	return NULL;
}

static void free_credit_buffers(struct ib_credit_buffer **credit_buffers)
{
	if (credit_buffers == NULL)
		return;

	for (struct ib_credit_buffer **ptr = credit_buffers; *ptr != NULL; ptr++) {
		if ((*ptr)->memory_region != NULL)
			(void)ibv_dereg_mr((*ptr)->memory_region);
		free(*ptr);
	}

	free(credit_buffers);
}

static struct ibv_qp *create_queue_pair(struct ib_context *ctx, struct ibv_cq *sendcq, struct ibv_cq *recvcq, int nbuf)
{
	struct ibv_qp_init_attr initattr;
	memset(&initattr, 0, sizeof(initattr));
	initattr.qp_context = ctx->verbs_context;
	initattr.send_cq = sendcq;
	initattr.recv_cq = recvcq;
	initattr.cap.max_send_wr = nbuf;
	initattr.cap.max_recv_wr = nbuf;
	initattr.cap.max_send_sge = 1;
	initattr.cap.max_recv_sge = 1;
	initattr.cap.max_inline_data = 0;
	initattr.qp_type = IBV_QPT_RC;
	initattr.sq_sig_all = 1;

	return ibv_create_qp(ctx->domain, &initattr);
}

static int move_qp_to_init(struct ibv_qp *qp)
{
	struct ibv_qp_attr attr;
	memset(&attr, 0, sizeof(attr)); 
	attr.qp_state = IBV_QPS_INIT;
	attr.pkey_index = 0;
	attr.port_num = 1;
	attr.qp_access_flags = IBV_ACCESS_LOCAL_WRITE;

	return ibv_modify_qp(qp, &attr, (IBV_QP_STATE|IBV_QP_PKEY_INDEX|
									 IBV_QP_PORT|IBV_QP_ACCESS_FLAGS));
}

static int move_qp_to_rtr(struct ibv_qp *qp, uint32_t remote_qpnum, uint16_t remote_lid, int nbuf)
{
	struct ibv_qp_attr attr;
	memset(&attr, 0, sizeof(attr)); 
	attr.qp_state = IBV_QPS_RTR;
	attr.ah_attr.dlid = remote_lid;
	attr.ah_attr.sl = 0;
	attr.ah_attr.static_rate = IBV_RATE_MAX;
	attr.ah_attr.is_global = 0;
	attr.ah_attr.port_num = 1;
	attr.path_mtu = IBV_MTU_2048;
	attr.dest_qp_num = remote_qpnum;
	attr.rq_psn = 1;
	attr.max_dest_rd_atomic = nbuf;
	attr.min_rnr_timer = 18;  // about 1.07 seconds (recommended value)

	return ibv_modify_qp(qp, &attr, (IBV_QP_STATE|IBV_QP_AV|IBV_QP_PATH_MTU|
									 IBV_QP_DEST_QPN|IBV_QP_MAX_DEST_RD_ATOMIC|
									 IBV_QP_RQ_PSN|IBV_QP_MIN_RNR_TIMER));
}

static int move_qp_to_rts(struct ibv_qp *qp, int nbuf)
{
	struct ibv_qp_attr attr;
	memset(&attr, 0, sizeof(attr)); 
	attr.qp_state = IBV_QPS_RTS;
	attr.sq_psn = 1;
	attr.max_rd_atomic = nbuf;
	attr.retry_cnt = 7;  // unlimited retries
	attr.rnr_retry = 7;  // unlimited RNR retries
	attr.timeout = 14;  // recommended value

	return ibv_modify_qp(qp, &attr, (IBV_QP_STATE|IBV_QP_SQ_PSN|
									 IBV_QP_MAX_QP_RD_ATOMIC|
									 IBV_QP_RETRY_CNT|IBV_QP_RNR_RETRY|
									 IBV_QP_TIMEOUT));
}

static int post_receive(struct ibv_qp *qp, struct ibv_mr *mr, void *buf, int bufsiz, uint64_t id)
{
	struct ibv_sge sge;
	memset(&sge, 0, sizeof(sge)); 
	sge.addr = (uintptr_t)buf;
	sge.length = bufsiz;
	sge.lkey = mr->lkey;

	struct ibv_recv_wr wr, *badwr;
	memset(&wr, 0, sizeof(wr)); 
	wr.wr_id = id;
	wr.next = NULL;
	wr.sg_list = &sge;
	wr.num_sge = 1;
  
	return ibv_post_recv(qp, &wr, &badwr);
}

static int send_credits(struct ib_connection *conn, int ncredits)
{
	//
	// grab a free send buffer
	//
	struct ib_credit_buffer *cb = NULL;

#ifdef XNI_TRACE
	puts("Searching for credit.");
#endif
	
	pthread_mutex_lock(&conn->credit_mutex);
	while (cb == NULL) {
		// first try to find a free one
		for (struct ib_credit_buffer **cbptr = conn->credit_buffers; *cbptr != NULL; cbptr++)
			if (!(*cbptr)->busy) {
				cb = *cbptr;
				cb->busy = 1;
				break;
			}

		// otherwise mark any that have become free
		if (cb == NULL) {
			size_t num_bufs = conn->context->num_registered;
			struct ibv_wc wc[num_bufs];
			int completed = ibv_poll_cq(conn->send_cq, num_bufs, wc);
			if (completed < 0) {
				pthread_mutex_unlock(&conn->credit_mutex);
				return 1;
			}
			for (int i = 0; i < completed; i++) {
				struct ib_credit_buffer *tmp = (struct ib_credit_buffer*)wc[i].wr_id;
				tmp->busy = 0;
			}
		}
	}
	pthread_mutex_unlock(&conn->credit_mutex);

#ifdef XNI_TRACE
	puts("Found a credit");
#endif
	
	//
	// encode and send the credit message
	//
	//TODO: NBO?
	memcpy(cb->msgbuf, CREDIT_MESSAGE_TAG, TAG_LENGTH);
	uint32_t tmp32 = (uint32_t)ncredits;
	memcpy(cb->msgbuf+TAG_LENGTH, &tmp32, 4);

	struct ibv_sge sge;
	memset(&sge, 0, sizeof(sge)); 
	sge.addr = (uintptr_t)cb->msgbuf;
	sge.length = IB_CREDIT_MESSAGE_SIZE;
	sge.lkey = cb->memory_region->lkey;

	struct ibv_send_wr wr, *badwr;
	memset(&wr, 0, sizeof(wr)); 
	wr.wr_id = (uintptr_t)cb;
	wr.next = NULL;
	wr.sg_list = &sge;
	wr.num_sge = 1;
	wr.opcode = IBV_WR_SEND;
	wr.send_flags = 0;

#ifdef XNI_TRACE
	puts("Sending credit.");
#endif
	

	if (ibv_post_send(conn->queue_pair, &wr, &badwr)) {
		pthread_mutex_lock(&conn->credit_mutex);
		cb->busy = 0;
		pthread_mutex_unlock(&conn->credit_mutex);
		return 1;
	}

	//TODO: need to make sure the completion event isn't for a data message
	struct ibv_wc wc;
	memset(&wc, 0, sizeof(wc));
	int completed = ibv_poll_cq(conn->send_cq, 1, &wc);
	if (completed < 0 || wc.status != IBV_WC_SUCCESS) {
		return XNI_ERR;
	}

	return XNI_OK;
}

static int consume_credit(struct ib_connection *conn)
{
  pthread_mutex_lock(&conn->credit_mutex);
  while (conn->credits < 1) {
    struct ibv_wc wc;
    memset(&wc, 0, sizeof(wc)); 
    int completed = ibv_poll_cq(conn->receive_cq, 1, &wc);
    if (completed < 0 || wc.status != IBV_WC_SUCCESS) {
      pthread_mutex_unlock(&conn->credit_mutex);
      return 1;
    }
    if (completed > 0) {
      struct ib_credit_buffer *cb = (struct ib_credit_buffer*)wc.wr_id;
      uint32_t tmp32 = 0;
      //TODO: NBO?
      memcpy(&tmp32, cb->msgbuf+TAG_LENGTH, 4);
      //TODO: check for deadlock if receive can't be posted
      post_receive(conn->queue_pair, cb->memory_region, cb->msgbuf,
                   IB_CREDIT_MESSAGE_SIZE, (uintptr_t)cb);
      conn->credits += tmp32;
    }
  }
  conn->credits -= 1;
  pthread_mutex_unlock(&conn->credit_mutex);

  return 0;
}

//XXX: not thread safe; all other ops must be finished
static int send_eof(struct ib_connection *conn)
{
  //TODO: use a better buffer
  //XXX: for now, just hijack the first target bufffer
  struct ib_target_buffer *tb = conn->context->target_buffers;
  // encode the message
  memcpy(tb->header, EOF_MESSAGE_TAG, TAG_LENGTH);

  // send the message
  struct ibv_sge sge;
  memset(&sge, 0, sizeof(sge)); 
  sge.addr = (uintptr_t)tb->header;
  sge.length = TAG_LENGTH;
  sge.lkey = tb->memory_region->lkey;

  struct ibv_send_wr wr, *badwr;
  memset(&wr, 0, sizeof(wr)); 
  wr.wr_id = (uintptr_t)tb;
  wr.next = NULL;
  wr.sg_list = &sge;
  wr.num_sge = 1;
  wr.opcode = IBV_WR_SEND;
  wr.send_flags = 0;
  if (consume_credit(conn))
    return 1;
  if (ibv_post_send(conn->queue_pair, &wr, &badwr))
    return 1;

  // wait for send completion
  struct ibv_wc wc;
  memset(&wc, 0, sizeof(wc)); 
  int completed = 0;
  do {
    completed = ibv_poll_cq(conn->send_cq, 1, &wc);
    if (completed < 0)
      return 1;
    if (completed > 0 && wc.status != IBV_WC_SUCCESS)
      return 1;
  } while (!completed);
  assert(wc.wr_id == (uintptr_t)tb);

  return 0;
}

static int ib_accept_connection(xni_context_t ctx_, struct xni_endpoint* local, xni_connection_t* conn_)
{
	struct ib_context *ctx = (struct ib_context*)ctx_;
	struct ib_connection **conn = (struct ib_connection**)conn_;

	struct ib_connection *tmpconn = NULL;
	struct ib_credit_buffer **credit_buffers = NULL;
	struct ibv_cq *sendcq=NULL, *recvcq=NULL;
	struct ibv_qp *qp = NULL;
	int server=-1, client=-1;

	// Ensure a registered buffer exists
	if (ctx->num_registered < 1)
		return XNI_ERR;

	tmpconn = calloc(1, sizeof(*tmpconn));
	tmpconn->context = ctx;
	
	credit_buffers = allocate_credit_buffers(ctx, ctx->num_registered);
	if (credit_buffers == NULL)
		goto error_out;

	if ((sendcq = ibv_create_cq(ctx->verbs_context, ctx->num_registered, NULL, NULL, 0)) == NULL)
		goto error_out;
	if ((recvcq = ibv_create_cq(ctx->verbs_context, ctx->num_registered, NULL, NULL, 0)) == NULL)
		goto error_out;
	
	if ((qp = create_queue_pair(ctx, sendcq, recvcq, ctx->num_registered)) == NULL)
		goto error_out;
  
	// start listening for a client
	server = socket(AF_INET, SOCK_STREAM, 0);
	if (server == -1) {
		perror("socket");
		goto error_out;
	}

	int optval = 1;
	setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr)); 
	addr.sin_family = AF_INET;
	addr.sin_port = htons((uint16_t)local->port);
	addr.sin_addr.s_addr = inet_addr(local->host);
	if (bind(server, (struct sockaddr*)&addr, sizeof(addr))) {
		perror("bind");
		goto error_out;
	}

	if (listen(server, 1)) {
		perror("listen");
		goto error_out;
	}

	// connect to the client
	client = accept(server, NULL, NULL);
	if (client == -1) {
		perror("accept");
		goto error_out;
	}
	close(server);
	server = -1;

	// exchange QPN, LID with remote side
	char msgbuf[6] = { 0 };
	uint32_t remote_qpnum;
	uint16_t remote_lid;

	if (recv(client, msgbuf, 6, MSG_WAITALL) < 6)
		goto error_out;

	memcpy(&remote_qpnum, msgbuf, 4);
	memcpy(&remote_lid, msgbuf+4, 2);

#ifdef XNI_TRACE
	printf("Remote QPN=%u, LID=%u\n",
		   (unsigned int)remote_qpnum,
		   (unsigned int)remote_lid);
#endif  // XNI_TRACE

	uint32_t tmp32 = qp->qp_num;
	memcpy(msgbuf, &tmp32, 4);
	struct ibv_port_attr portattr;
	memset(&portattr, 0, sizeof(portattr)); 
	if (ibv_query_port(ctx->verbs_context, 1, &portattr))
		goto error_out;
	uint16_t tmp16 = portattr.lid;
	memcpy(msgbuf+4, &tmp16, 2);

#ifdef XNI_TRACE
	printf("Local QPN=%u, LID=%u\n",
		   (unsigned int)qp->qp_num,
		   (unsigned int)portattr.lid);
#endif  // XNI_TRACE

	if (send(client, msgbuf, 6, 0) < 6)
		goto error_out;
	close(client);
	client = -1;

	// prepare the QP for sending
	if (move_qp_to_init(qp))
		goto error_out;

	for (size_t i =0; i < ctx->num_registered; i++) {
		struct ib_target_buffer* tbp = ctx->target_buffers + i;
		tbp->connection = tmpconn;
		size_t bufsiz = tbp->buffer_size;
		if (post_receive(qp, tbp->memory_region, tbp->header,
						 (int)((char*)(tbp->data) - (char*)(tbp->header) + bufsiz),
						 (uintptr_t)tbp))
			goto error_out;
	}
	if (move_qp_to_rtr(qp, remote_qpnum, remote_lid, ctx->num_registered) ||
		move_qp_to_rts(qp, ctx->num_registered))
		goto error_out;

#ifdef XNI_TRACE
	puts("Connected.");
#endif  // XNI_TRACE

	tmpconn->context = ctx;
	tmpconn->credit_buffers = credit_buffers;
	tmpconn->eof = 0;
	pthread_mutex_init(&tmpconn->credit_mutex, NULL);
	tmpconn->destination = 1;
	tmpconn->send_cq = sendcq;
	tmpconn->receive_cq = recvcq;
	tmpconn->queue_pair = qp;
	tmpconn->remote_qpnum = remote_qpnum;
	tmpconn->remote_lid = remote_lid;

	// send the initial credits
	if (send_credits(tmpconn, ctx->num_registered))
		goto error_out;

	*conn = tmpconn;
	return XNI_OK;
	
  error_out:
	if (client != -1)
		close(client);
	
	if (server != -1)
		close(server);

	if (qp != NULL)
		(void)ibv_destroy_qp(qp);

	if (recvcq != NULL)
		(void)ibv_destroy_cq(recvcq);
	if (sendcq != NULL)
		(void)ibv_destroy_cq(sendcq);

	free_credit_buffers(credit_buffers);
	free(tmpconn);
	return XNI_ERR;
}

static int ib_connect(xni_context_t ctx_, struct xni_endpoint* remote, xni_connection_t* conn_)
{
	struct ib_context *ctx = (struct ib_context*)ctx_;
	struct ib_connection **conn = (struct ib_connection**)conn_;

	struct ib_connection *tmpconn = NULL;
	struct ib_credit_buffer **credit_buffers = NULL;
	struct ibv_cq *sendcq=NULL, *recvcq=NULL;
	struct ibv_qp *qp = NULL;
	int server=-1, client=-1;

	// Ensure a registered buffer exists
	if (ctx->num_registered < 1)
		return XNI_ERR;

#ifdef XNI_TRACE
	puts("2");
#endif  // XNI_TRACE
	// fill out a temporary connection object
	tmpconn = calloc(1, sizeof(*tmpconn));
	tmpconn->context = ctx;


#ifdef XNI_TRACE
	puts("3");
#endif  // XNI_TRACE
	credit_buffers = allocate_credit_buffers(ctx, ctx->num_registered);
	if (credit_buffers == NULL)
		goto error_out;
#ifdef XNI_TRACE
	puts("4");
#endif  // XNI_TRACE

	if ((sendcq = ibv_create_cq(ctx->verbs_context, ctx->num_registered, NULL, NULL, 0)) == NULL)
		goto error_out;
	if ((recvcq = ibv_create_cq(ctx->verbs_context, ctx->num_registered, NULL, NULL, 0)) == NULL)
		goto error_out;

	if ((qp = create_queue_pair(ctx, sendcq, recvcq, ctx->num_registered)) == NULL)
		goto error_out;
#ifdef XNI_TRACE
	puts("Create qps");
#endif  // XNI_TRACE
  
	// connect to the server
	server = socket(AF_INET, SOCK_STREAM, 0);
	if (server == -1) {
		perror("socket");
		goto error_out;
	}

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr)); 
	addr.sin_family = AF_INET;
	addr.sin_port = htons((uint16_t)remote->port);
	addr.sin_addr.s_addr = inet_addr(remote->host);
	if (connect(server, (struct sockaddr*)&addr, sizeof(addr))) {
		perror("connect");
		goto error_out;
	}

	// exchange QPN, LID with remote side
	char msgbuf[6] = { 0 };
	uint32_t remote_qpnum;
	uint16_t remote_lid;

	uint32_t tmp32 = qp->qp_num;
	memcpy(msgbuf, &tmp32, 4);
	struct ibv_port_attr portattr;
	memset(&portattr, 0, sizeof(portattr)); 
	if (ibv_query_port(ctx->verbs_context, 1, &portattr))
		goto error_out;
	uint16_t tmp16 = portattr.lid;
	memcpy(msgbuf+4, &tmp16, 2);

#ifdef XNI_TRACE
	printf("Local QPN=%u, LID=%u\n",
		   (unsigned int)qp->qp_num,
		   (unsigned int)portattr.lid);
#endif  // XNI_TRACE

	if (send(server, msgbuf, 6, 0) < 6)
		goto error_out;

	if (recv(server, msgbuf, 6, MSG_WAITALL) < 6)
		goto error_out;
	close(server);
	client = -1;

	memcpy(&remote_qpnum, msgbuf, 4);
	memcpy(&remote_lid, msgbuf+4, 2);

#ifdef XNI_TRACE
	printf("Remote QPN=%u, LID=%u\n",
		   (unsigned int)remote_qpnum,
		   (unsigned int)remote_lid);
#endif  // XNI_TRACE

	// prepare the QP for sending
	if (move_qp_to_init(qp))
		goto error_out;
	
	for (struct ib_credit_buffer **cbptr = credit_buffers; *cbptr != NULL; cbptr++)
		if (post_receive(qp, (*cbptr)->memory_region, (*cbptr)->msgbuf,
						 IB_CREDIT_MESSAGE_SIZE, (uintptr_t)*cbptr))
			goto error_out;
  
	if (move_qp_to_rtr(qp, remote_qpnum, remote_lid, ctx->num_registered) ||
		move_qp_to_rts(qp, ctx->num_registered))
		goto error_out;

	tmpconn->context = ctx;
	tmpconn->credit_buffers = credit_buffers;
	tmpconn->credits = 0;
	pthread_mutex_init(&tmpconn->send_state_mutex, NULL);
	pthread_mutex_init(&tmpconn->credit_mutex, NULL);
	tmpconn->destination = 0;
	tmpconn->send_cq = sendcq;
	tmpconn->receive_cq = recvcq;
	tmpconn->queue_pair = qp;
	tmpconn->remote_qpnum = remote_qpnum;
	tmpconn->remote_lid = remote_lid;

	// Add the connection to the registered buffers
	for (size_t i = 0; i < ctx->num_registered; i++)
		ctx->target_buffers[i].connection = tmpconn;
	
#ifdef XNI_TRACE
	puts("Connected.");
#endif  // XNI_TRACE

	*conn = tmpconn;
  return XNI_OK;
  
 error_out:
  if (server != -1)
	  close(server);

  if (qp != NULL)
	  (void)ibv_destroy_qp(qp);

  if (recvcq != NULL)
	  (void)ibv_destroy_cq(recvcq);
  if (sendcq != NULL)
	  (void)ibv_destroy_cq(sendcq);

  free_credit_buffers(credit_buffers);
  free(tmpconn);

  return XNI_ERR;
}

//XXX: all other sends, receives must be finished first!
//XXX: this is not going to be thread safe??
//alternatives: a flag that signals shutdown state
//and freeing the buffers as they become available on freelist
static int ib_close_connection(xni_connection_t *conn_)
{
  struct ib_connection **conn = (struct ib_connection**)conn_;

  struct ib_connection *c = *conn;

  if (!c->destination)
    (void)send_eof(c);

  (void)ibv_destroy_qp(c->queue_pair);
  (void)ibv_destroy_cq(c->receive_cq);
  (void)ibv_destroy_cq(c->send_cq);

  free_credit_buffers(c->credit_buffers);

  free(c);

  *conn = NULL;
  return XNI_OK;
}

static int ib_request_target_buffer(xni_context_t ctx_, xni_target_buffer_t *targetbuf_)
{
	struct ib_context *ctx = (struct ib_context*)ctx_;
	struct ib_target_buffer **targetbuf = (struct ib_target_buffer**)targetbuf_;

	struct ib_target_buffer *tb = NULL;
	pthread_mutex_lock(&ctx->busy_flag_mutex);
	while (tb == NULL) {
		for (size_t i = 0; i < ctx->num_registered; i++) {
			struct ib_target_buffer* ptr = ctx->target_buffers + i;

			if (!ptr->busy) {
				tb = ptr;
				tb->busy = 1;
				break;
			}
		}
		if (tb == NULL)
			pthread_cond_wait(&ctx->busy_flag_cond,
							  &ctx->busy_flag_mutex);
	}
	pthread_mutex_unlock(&ctx->busy_flag_mutex);
    
	*targetbuf = tb;
	return XNI_OK;
}

//TODO: what happens on error? QP is trashed
//TODO: always mark the buffer as free before exit
static int ib_send_target_buffer(xni_connection_t conn_, xni_target_buffer_t *targetbuf_)
{
	struct ib_connection* conn = (struct ib_connection*) conn_;
	struct ib_target_buffer **targetbuf = (struct ib_target_buffer**)targetbuf_;
	struct ib_target_buffer *tb = *targetbuf;

	int return_code = XNI_ERR;
	
	if (conn->destination || tb->data_length < 1)
		goto free_out;

	// encode the message
	//TODO: NBO?
	memcpy(tb->header, DATA_MESSAGE_TAG, TAG_LENGTH);
	uint64_t tmp64 = tb->target_offset;
	memcpy(((char*)tb->header)+TAG_LENGTH, &tmp64, 8);

	// send the message
	struct ibv_sge sge;
	memset(&sge, 0, sizeof(sge)); 
	sge.addr = (uintptr_t)tb->header;
	sge.length = (int)((char*)tb->data - (char*)tb->header) + tb->data_length;
	sge.lkey = tb->memory_region->lkey;

	struct ibv_send_wr wr, *badwr;
	memset(&wr, 0, sizeof(wr)); 
	wr.wr_id = (uintptr_t)tb;
	wr.next = NULL;
	wr.sg_list = &sge;
	wr.num_sge = 1;
	wr.opcode = IBV_WR_SEND;
	wr.send_flags = 0;
	if (consume_credit(conn))
		goto free_out;
	tb->send_state = QUEUED;
	if (ibv_post_send(conn->queue_pair, &wr, &badwr))
		goto free_out;

	// wait for send completion
	pthread_mutex_lock(&conn->send_state_mutex);
	while (tb->send_state == QUEUED) {
		size_t num_bufs = conn->context->num_registered;
		struct ibv_wc wc[num_bufs];
		int completed = ibv_poll_cq(conn->send_cq, num_bufs, wc);
		if (completed < 0) {
			//TODO: better error handling
			pthread_mutex_unlock(&conn->send_state_mutex);
			goto free_out;
		}

		for (int i = 0; i < completed; i++) {
			struct ib_target_buffer *tb = (struct ib_target_buffer*)wc[i].wr_id;
			tb->send_state = (wc[i].status == IBV_WC_SUCCESS
							  ? SENT
							  : ERROR);
		}
	}
	pthread_mutex_unlock(&conn->send_state_mutex);

	return_code = (tb->send_state == SENT
				   ? XNI_OK
				   : XNI_ERR);
    
  free_out:
	// mark the buffer as free
	pthread_mutex_lock(&conn->context->busy_flag_mutex);
	tb->busy = 0;
	pthread_cond_signal(&conn->context->busy_flag_cond);
	pthread_mutex_unlock(&conn->context->busy_flag_mutex);
	
	return return_code;
}

static int ib_receive_target_buffer(xni_connection_t conn_, xni_target_buffer_t *targetbuf_)
{
  struct ib_connection *conn = (struct ib_connection*)conn_;
  struct ib_target_buffer **targetbuf = (struct ib_target_buffer**)targetbuf_;

  if (!conn->destination)
    return XNI_ERR;

  // poll until a message arrives
  struct ib_target_buffer *tb = NULL;
  while (tb == NULL && !conn->eof) {
    struct ibv_wc wc;
    memset(&wc, 0, sizeof(wc)); 
    int completions = ibv_poll_cq(conn->receive_cq, 1, &wc);
    if (completions < 0)
      return XNI_ERR;
    if (completions > 0) {
      if (wc.status != IBV_WC_SUCCESS)
        return XNI_ERR;

      // decode the message
      tb = (struct ib_target_buffer*)wc.wr_id;
      if (memcmp(tb->header, DATA_MESSAGE_TAG, TAG_LENGTH) == 0) {
        memcpy(&tb->target_offset, ((char*)tb->header)+TAG_LENGTH, 8);
        tb->data_length = wc.byte_len - (int)((char*)tb->data - (char*)tb->header);
      } else if (memcmp(tb->header, EOF_MESSAGE_TAG, TAG_LENGTH) == 0)
        conn->eof = 1;
      else
        return XNI_ERR;
    }
  }

  if (conn->eof)
    return XNI_EOF;

  *targetbuf = tb;
  return XNI_OK;
}

static int ib_release_target_buffer(xni_target_buffer_t *targetbuf_)
{
  struct ib_target_buffer **targetbuf = (struct ib_target_buffer**)targetbuf_;
  struct ib_target_buffer *tb = *targetbuf;

  tb->target_offset = 0;
  tb->data_length = -1;

  if (tb->connection->destination) {
    //TODO: busy flag?
    //TODO: better error handling
    if (post_receive(tb->connection->queue_pair,
                     tb->memory_region,
                     tb->header,
                     ((int)((char*)tb->data - (char*)tb->header) + tb->buffer_size),
                     (uintptr_t)tb))
      return XNI_ERR;
    if (send_credits(tb->connection, 1))
      return XNI_ERR;
  } else {
    pthread_mutex_lock(&tb->connection->context->busy_flag_mutex);
    tb->busy = 0;
    pthread_cond_signal(&tb->connection->context->busy_flag_cond);
    pthread_mutex_unlock(&tb->connection->context->busy_flag_mutex);
  }

  *targetbuf = NULL;
  return XNI_OK;
}


static struct xni_protocol protocol_ib = {
  .name = PROTOCOL_NAME,
  .context_create = ib_context_create,
  .context_destroy = ib_context_destroy,
  .register_buffer = ib_register_buffer,
  .unregister_buffer = ib_unregister_buffer,
  .accept_connection = ib_accept_connection,
  .connect = ib_connect,
  .close_connection = ib_close_connection,
  .request_target_buffer = ib_request_target_buffer,
  .send_target_buffer = ib_send_target_buffer,
  .receive_target_buffer = ib_receive_target_buffer,
  .release_target_buffer = ib_release_target_buffer,
};
#endif

#ifdef HAVE_ENABLE_IB
struct xni_protocol *xni_protocol_ib = &protocol_ib;
#else
struct xni_protocol *xni_protocol_ib = NULL;
#endif

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
