#ifndef XDD_XNI_INTERNAL_H
#define XDD_XNI_INTERNAL_H


#include <arpa/inet.h>  // for ntohl()


struct xni_protocol {
    const char *name;
    int (*context_create)(xni_protocol_t, xni_control_block_t, xni_context_t*);
    int (*context_destroy)(xni_context_t*);

    int (*register_buffer)(xni_context_t, void*, size_t, size_t);

    int (*accept_connection)(xni_context_t, struct xni_endpoint*, xni_connection_t*);
    int (*connect)(xni_context_t, struct xni_endpoint*, xni_connection_t*);
    int (*close_connection)(xni_connection_t*);

    int (*request_target_buffer)(xni_context_t, xni_target_buffer_t*);
    int (*send_target_buffer)(xni_connection_t, xni_target_buffer_t*);
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
    struct xni_context *context;
    void *data;
	int64_t sequence_number;
    size_t target_offset;
    int data_length;
};


/**
 * Convert from network byte-order (big endian) to host order
 */
static inline uint64_t ntohll(uint64_t value)
{
    int endian_test = 1;

    // Determine if host order is little endian
    if (endian_test == *((char*)(&endian_test))) {
        // Swap the bytes
        uint32_t low = ntohl((uint32_t)(value & 0xFFFFFFFFLL));
        uint32_t high = ntohl((uint32_t)(value >> 32));
        value = ((uint64_t)(low) << 32) | (uint64_t)(high);
    }
    return value;
}

/**
 * Convert from host byte-order to network byte-order (big endian)
 */
static inline uint64_t htonll(uint64_t value)
{
    // Re-use the htonll implementation to swap the bytes
    return htonll(value);
}

#endif // XDD_XNI_INTERNAL_H

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
