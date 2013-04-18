#ifndef XDD_XNI_INTERNAL_H
#define XDD_XNI_INTERNAL_H


struct xni_protocol {
  const char *name;
  int (*context_create)(xni_protocol_t, xni_control_block_t, xni_context_t*);
  int (*context_destroy)(xni_context_t*);

  int (*accept_connection)(xni_context_t, struct xni_endpoint*, int, size_t, xni_connection_t*);
  int (*connect)(xni_context_t, struct xni_endpoint*, int, size_t, xni_connection_t*);
  int (*close_connection)(xni_connection_t*);

  int (*request_target_buffer)(xni_connection_t, xni_target_buffer_t*);
  int (*send_target_buffer)(xni_target_buffer_t*);
  int (*receive_target_buffer)(xni_connection_t, xni_target_buffer_t*);
  int (*release_target_buffer)(xni_target_buffer_t*);
};

struct xni_context {
  struct xni_protocol *protocol;
};

struct xni_connection {
  struct xni_context *context;
};

struct xni_target_buffer {
  struct xni_connection *connection;
  void *data;
  size_t target_offset;
  int data_length;
};


#endif // XDD_XNI_INTERNAL_H
