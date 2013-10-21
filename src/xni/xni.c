#include <sys/types.h>

#include "xni.h"
#include "xni_internal.h"


int xni_initialize(void)
{
  return XNI_OK;
}

int xni_finalize(void)
{
  return XNI_OK;
}

int xni_context_create(xni_protocol_t proto, xni_control_block_t cb, xni_context_t* ctx)
{
  return proto->context_create(proto, cb, ctx);
}


int xni_context_destroy(xni_context_t* ctx)
{
  return (*ctx)->protocol->context_destroy(ctx);
}

int xni_accept_connection(xni_context_t ctx, struct xni_endpoint* local, xni_connection_t* conn)
{
  return ctx->protocol->accept_connection(ctx, local, conn);
}

int xni_register_buffer(xni_context_t ctx, void* buf, size_t nbytes, size_t reserved,
                        xni_target_buffer_t* tbp)
{
    return ctx->protocol->register_buffer(ctx, buf, nbytes, reserved, tbp);
}

int xni_unregister_buffer(xni_context_t ctx, void* buf)
{
    return ctx->protocol->unregister_buffer(ctx, buf);
}

//TODO: local_endpoint
int xni_connect(xni_context_t ctx, struct xni_endpoint* remote, xni_connection_t* conn)
{
  return ctx->protocol->connect(ctx, remote, conn);
}

int xni_close_connection(xni_connection_t* conn)
{
  return (*conn)->context->protocol->close_connection(conn);
}

int xni_request_target_buffer(xni_context_t context,
                              xni_target_buffer_t* buffer)
{
  return context->protocol->request_target_buffer(context, buffer);
}

int xni_send_target_buffer(xni_connection_t conn, xni_target_buffer_t* buffer)
{
    return conn->context->protocol->send_target_buffer(conn, buffer);
}

int xni_receive_target_buffer(xni_connection_t conn, xni_target_buffer_t* buffer)
{
  return conn->context->protocol->receive_target_buffer(conn, buffer);
}

int xni_release_target_buffer(xni_target_buffer_t* buffer)
{
  return (*buffer)->context->protocol->release_target_buffer(buffer);
}

void *xni_target_buffer_data(xni_target_buffer_t tb)
{
  return tb->data;
}

size_t xni_target_buffer_target_offset(xni_target_buffer_t tb)
{
  return tb->target_offset;
}

void xni_target_buffer_set_target_offset(size_t offset, xni_target_buffer_t tb)
{
  tb->target_offset = offset;
}

int xni_target_buffer_data_length(xni_target_buffer_t tb)
{
  return tb->data_length;
}

void xni_target_buffer_set_data_length(int length, xni_target_buffer_t tb)
{
  tb->data_length = length;
}
