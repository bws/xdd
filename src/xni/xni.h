#ifndef XDD_XNI_H
#define XDD_XNI_H

#include <stdint.h>
#include <stdlib.h>

/*! \defgroup XNI Generic Interface
 * @{
 */

/*! \brief Return code.
 *
 * Most public API functions return a non-negative integer that is one
 * of these values. The user may depend on the value of #XNI_OK being
 * zero to simplify tests. All error codes will be greater than or
 * equal to #XNI_ERR, such that any return code that produces a
 * positive value when bitwise-anded with #XNI_ERR is an error code.
 */
enum {
  // begin status codes
  XNI_OK = 0,  /*!< No error. */
  XNI_EOF,  /*!< End-of-file condition. */
  // begin error codes
  XNI_ERR = (1 << 15),  /*!< Unspecified error. */
};


struct xni_protocol;
/*! \brief Opaque type representing an XNI implementation. */
typedef struct xni_protocol *xni_protocol_t;

/*! \brief Opaque type representing a control block.
 *
 * Control blocks store settings specific to the underlying XNI
 * implementation (e.g. TCP).
 */
typedef void *xni_control_block_t;

struct xni_context;
/*! \brief Opaque type representing a network context.
 *
 * A context encapsulates an #xni_control_block_t and stores variables
 * shared by all instances of #xni_connection_t created with the
 * context.
 *
 * \sa xni_context_create()
 * \sa xni_context_destroy()
 */
typedef struct xni_context *xni_context_t;

struct xni_connection;
/*! \brief Opaque type representing a connection.
  *
  * The precise meaning of \em connection is determined by the
  * specific XNI implementation. All connections represent an
  * association between two network endpoints. A connection is a
  * <em>source-side connection</em> iff. it was created by
  * xni_connect(). A connection is a <em>destination-side
  * connection</em> iff. it was created by xni_accept_connection().
  *
  * \sa xni_accept_connection() xni_connect()
  * \sa xni_close_connection()
  */
typedef struct xni_connection *xni_connection_t;

struct xni_endpoint {
  const char *host;  /*!< An IPv4 dotted-quad string. */
  int port;  /*!< An IPv4 port. */
};
/*! \brief Address of a remote process.
 */
typedef struct xni_endpoint xni_endpoint_t;

struct xni_target_buffer;
/*! \brief Opaque type representing a target file buffer.
 *
 * The primary goal of XNI is to facilitate the high-performance
 * transfer of these buffers. A target buffer contains a block of data
 * beginning at a given offset in a target (file).
 *
 * \b Accessors:
 *     \li xni_target_buffer_data()
 *     \li xni_target_buffer_target_offset() xni_target_buffer_set_target_offset()
 *     \li xni_target_buffer_data_length() xni_target_buffer_set_data_length()
 *
 * \sa xni_request_target_buffer() xni_send_target_buffer()
 * \sa xni_receive_target_buffer() xni_release_target_buffer()
 */
typedef struct xni_target_buffer *xni_target_buffer_t;


/*! \brief Perform library-specific initialization.
 *
 * This function must be called and return #XNI_OK before any other
 * XNI function may be called. It is forbidden to call this function
 * more than once.
 *
 * \return #XNI_OK if the initialization was successful.
 * \return #XNI_ERR if initialization failed.
 *
 * \sa xni_finalize()
 */
int xni_initialize(void);
/*! \brief Cleanup the XNI library.
 *
 * This function should be called to unload the XNI library. It is
 * forbidden to call this function more than once.
 *
 * \return #XNI_OK if the cleanup was successful.
 * \return #XNI_ERR if the cleanup failed.
 *
 * \sa xni_initialize()
 */
int xni_finalize(void);

/*! \brief Create a network context.
 *
 * A network context is bound to an implementation specified by \e
 * protocol and settings specified by \e control_block. Multiple
 * connections can be created from a single context.
 *
 * \param protocol The XNI protocol implementation to use.
 * \param control_block Protocol-specific settings for this context.
 * \param[out] context The context that was created.
 *
 * \return #XNI_OK if the context was succesfully created.
 * \return #XNI_ERR if the context could not be created.
 *
 * \sa xni_context_destroy()
 */
int xni_context_create(xni_protocol_t protocol, xni_control_block_t control_block, xni_context_t *context);
/*! \brief Cleanup a context.
 * 
 * This function frees any resources that were allocated when \e
 * context was created. It is forbidden to call this function on a
 * context that is still associated with open connections. It is
 * forbidden to call this function more than once on the same context.
 *
 * \param[in,out] context The context to cleanup.
 *
 * \return #XNI_OK if the context was successfully freed.
 * \return #XNI_ERR if the context could not be freed.
 *
 * \sa xni_context_create()
 */
int xni_context_destroy(xni_context_t *context);

/*! \brief Register memory with XNI.
 *
 * This function provides XNI drivers to perform optimizations based on
 * the adress of the memory buffers in use.
 *
 * \param[in,out] context The context to register the buffer with.
 * \param[in] buf The memory buffer to register.
 * \param[in] nbytes The total size of the buffer in bytes.
 * \param[in] reserved The offset into the buffer at which the caller will
 *  insert application data.  Although this seems backwards, it ensures both
 *  the caller and XNI can align data per their own requirements.
 * \param[out] tb The xni target buffer to use for send/recvs.
 *
 * \return #XNI_OK if registration was successful.
 * \return #XNI_ERR if registration failed.
 *
 * \sa xni_unregister()
 */
int xni_register_buffer(xni_context_t context, void* buf, size_t nbytes, size_t reserved, xni_target_buffer_t* tb);
/*! \brief Free resources associated with registering memory with XNI.
 *
 * This function frees any resources used to register memory for use with
 * XNI.
 *
 * \return #XNI_OK if the cleanup was successful.
 * \return #XNI_ERR if the cleanup failed.
 *
 * \sa xni_register()
 */
int xni_unregister_buffer(xni_context_t context, void* buf);

/*! \brief Wait for a connection from a remote process.
 *
 * This function creates a <em>destination-side connection</em> by
 * listening on the address specified by \e local for a connection
 * from a remote process. 
 *
 * It is forbidden for the \e num_buffers and \e buffer_size arguments
 * to differ from those specified at the remote end to xni_connect().
 *
 * \param context The network context under which to create the connection.
 * \param[in] local The local address to listen on.
 * \param[out] connection The newly created <em>destination-side connection</em>.
 *
 * \return #XNI_OK if the connection was successfully created.
 * \return #XNI_ERR if the connection could not be created.
 *
 * \sa xni_close_connection()
 * \sa xni_receive_target_buffer()
 */
int xni_accept_connection(xni_context_t context, struct xni_endpoint *local, xni_connection_t *connection);
/*! \brief Initiate a connection to a remote process.
 *
 * This function creates a <em>source-side connection</em> by
 * connecting to the remote process at the address specified by \e
 * remote. This function allocates \e num_buffers target buffers of
 * size \e buffer_size using the #xni_allocate_fn_t provided when \e
 * context was created. These buffers will be aligned on 512-byte
 * boundaries.
 *
 * It is forbidden for the \e num_buffers and \e buffer_size arguments
 * to differ from those specified at the remote end to
 * xni_accept_connection().
 *
 * \param context The network context under which to create the connection.
 * \param[in] remote The remote address to connect to.
 * \param[out] connection The newly created <em>source-side connection</em>.
 *
 * \return #XNI_OK if the connection was successfully created.
 * \return #XNI_ERR if the connection could not be created.
 *
 * \sa xni_close_connection()
 * \sa xni_request_target_buffer()
 */
//TODO: local_endpoint
int xni_connect(xni_context_t context, struct xni_endpoint *remote, xni_connection_t *connection);
/*! \brief Close a connection and free its resources.
 *
 * This function closes a connection and frees all allocated target
 * buffers using the #xni_free_fn_t specified when the context of \e
 * connection was created. Closing a <em>source-side connection</em>
 * will cause a call to xni_receive_target_buffer() at the destination
 * to return #XNI_EOF.
 *
 * <b>This function is not thread-safe</b> and should only be called
 * once all threads have finished with the connection.
 *
 * It is forbidden to call any function (including this one) on \e
 * connection after it has been closed.
 *
 * \param[in,out] connection The connect to close.
 *
 * \return #XNI_OK if the connection was successfully closed.
 * \return #XNI_ERR if the connection could not be closed.
 *
 * \sa xni_accept_connection() xni_connect()
 */
int xni_close_connection(xni_connection_t *connection);

/*! \brief Request a target buffer and reserve it.
 *
 * This function waits for a target buffer to become available and
 * then returns it in \e target_buffer. The returned buffer is
 * temporarily owned by the caller until the buffer is passed to
 * xni_send_target_buffer() or xni_release_target_buffer().
 *
 * \param contest The <em>source-side</em> context from which to request
 *   the buffer.
 * \param[out] buffer The requested target buffer.
 *
 * \return #XNI_OK if a target buffer was reserved.
 * \return #XNI_ERR if no target buffer could be reserved.
 *
 * \sa xni_send_target_buffer();
 */
int xni_request_target_buffer(xni_context_t ctx, xni_target_buffer_t *buffer);
/*! \brief Send a target buffer to the remote process.
 *
 * This function transfers the target buffer \e buffer to the remote
 * end of the buffer's connection. Before sending the buffer the
 * caller should set the \e target_offset and \e data_length
 * properties. This function blocks until the send completes (but not
 * necessarily until the remote side has received the buffer).
 *
 * XNI reclaims ownership of \em buffer <b>even if this function
 * fails.</b> It is an error to attempt to send a target buffer that
 * is not associated with a <em>source-side connection.</em> It is an
 * error to send a target buffer whose data length is less than one.
 *
 * \param[in] connection The connection to send target.
 * \param[in,out] buffer The buffer to send.
 *
 * \return #XNI_OK if the target buffer was sent.
 * \return #XNI_ERR if the target buffer could not be sent.
 *
 * \sa xni_request_target_buffer()
 */
int xni_send_target_buffer(xni_connection_t connection, xni_target_buffer_t *buffer);
/*! \brief Wait for a target buffer to arrive from the remote process.
  *
  * This function blocks on \e connection until a target buffer
  * arrives and then returns that buffer in \e buffer. This function
  * will set the \e target_offset and \e data_length properties of the
  * target buffer before returning it.  The returned buffer is owned
  * temporarily by the caller until it is released by
  * xni_release_target_buffer().
  *
  * If the source side has closed the connection then this function
  * will return #XNI_EOF and leave \e buffer untouched.
  *
  * \param connection The connection to receive on.
  * \param[out] buffer The buffer that was received.
  *
  * \return #XNI_OK if a buffer was received.
  * \return #XNI_EOF if the source side closed the connection.
  * \return #XNI_ERR if a buffer could not be received.
  *
  * \sa xni_release_target_buffer()
  */
int xni_receive_target_buffer(xni_connection_t connection, xni_target_buffer_t *buffer);
/*! \brief Relinquish ownership of a target buffer to XNI.
 *
 * After calling this function the caller no longer owns \e
 * buffer. The user should call this function after finishing with a
 * buffer returned by xni_receive_target_buffer().
 *
 * \param [in,out] buffer The buffer to relase to XNI.
 *
 * \return #XNI_OK if the buffer was released.
 * \return #XNI_ERR if the buffer could not be released.
 *
 * \sa xni_receive_target_buffer().
 */
int xni_release_target_buffer(xni_target_buffer_t *buffer);

/*! \brief Get a target buffer's data pointer.
 *
 * The data pointer will point to a block of memory aligned on a
 * 512-byte boundary.
 *
 * \param buffer The buffer to inspect.
 *
 * \return A pointer to the target data.
 */
void *xni_target_buffer_data(xni_target_buffer_t buffer);
/*! \brief Get a target buffer's target offset.
 *
 * The target offset is the offset in bytes of the buffer into the
 * target file.
 *
 * \param buffer The target buffer.
 *
 * \return the target buffer offset.
 *
 * \sa xni_target_buffer_set_target_offset()
 */
size_t xni_target_buffer_target_offset(xni_target_buffer_t buffer);
/*! \brief Set a target buffer's target offset
 *
 * The target offset is the offset in bytes of the buffer into the
 * target file.
 *
 * \param offset The target buffer offset.
 * \param buffer The buffer to set.
 *
 * \sa xni_target_buffer_target_offset()
 */
void xni_target_buffer_set_target_offset(size_t offset, xni_target_buffer_t buffer);
/*! \brief Get the length of data in the target buffer.
 *
 * The length specifies the actual number of data bytes in the buffer
 * returned by xni_target_buffer_data().
 *
 * \param buffer The buffer to inspect.
 *
 * \return the length of data in the buffer.
 *
 * \sa xni_target_buffer_set_data_length()
 */
int xni_target_buffer_data_length(xni_target_buffer_t buffer);
/*! \brief Set the length of data in the target buffer.
 *
 * The length specifies the actual number of data bytes in the buffer
 * returned by xni_target_buffer_data().
 *
 * \param length the length of data in bytes.
 * \param buffer The buffer to set.
 *
 * \sa xni_target_buffer_data_length()
 */
void xni_target_buffer_set_data_length(int length, xni_target_buffer_t buffer);

/*! @} */


/*! \defgroup XNITCP TCP Implementation
 * @{
 */

enum {
  XNI_TCP_DEFAULT_NUM_SOCKETS = 0,  /*!< \brief Use the default number of sockets. */
};
extern const char *XNI_TCP_DEFAULT_CONGESTION;  /*!< \brief Use the default TCP congestion avoidance algorithm. */
/*! \brief Create a control block for the TCP implementation.
 *
 * If \e num_sockets is #XNI_TCP_DEFAULT_NUM_SOCKETS then the number
 * of sockets will be equal to the number of target buffers specified
 * when a connection is created.
 *
 * If \e congestion is #XNI_TCP_DEFAULT_CONGESTION then the system
 * default congestion avoidance algorithm will be used.
 *
 * \param num_sockets The number of TCP sockets to create per connection.
 * \param congestion the congestion control algorithm to use
 * \param[out] control_block The newly allocated control block.
 *
 * \return #XNI_OK if the control block was successfully created.
 * \return #XNI_ERR if the control block could not be created.
 *
 * \sa xni_free_tcp_control_block()
 */
int xni_allocate_tcp_control_block(int num_sockets, const char *congestion, xni_control_block_t *control_block);
/*! \brief Free a TCP control block.
 *
 * It is forbidden to call this function more than once with the same
 * \e control_block.
 *
 * \param[in,out] control_block The control block to free.
 *
 * \return #XNI_OK if the control block was successfully freed.
 * \return #XNI_ERR if the control block could not be freed.
 *
 * \sa xni_allocate_tcp_control_block()
 */
int xni_free_tcp_control_block(xni_control_block_t *control_block);
/*! \brief The TCP implementation of XNI.
 *
 * The TCP implementation uses TCP stream sockets to transfer
 * buffers. The number of sockets is configurable.
 *
 * \sa xni_context_create()
 */
extern xni_protocol_t xni_protocol_tcp;

/*! @} */


/*! \defgroup XNIIB InfiniBand implementation
 * @{
 */

/*! \brief Create a control block for the InfiniBand implementation.
 *
 * If \e device_name is \c NULL then XNI will use the first IB device
 * found.
 *
 * \param[in] device_name The name of the IB device to bind to or \c NULL.
 * \param[in] num_buffers The number of memory buffers to register.
 * \param[out] control_block The newly allocated control block.
 *
 * \return #XNI_OK if the control block was successfully created.
 * \return #XNI_ERR if the control block could not be created.
 *
 * \sa xni_free_ib_control_block()
 */
int xni_allocate_ib_control_block(const char *device_name, size_t num_buffers, xni_control_block_t *control_block);
/*! \brief Free an InfiniBand control block.
 *
 * It is forbidden to call this function more than once with the same
 * \e control_block.
 *
 * \param[in,out] control_block The control block to free.
 *
 * \return #XNI_OK if the control block was successfully freed.
 * \return #XNI_ERR if the control block could not be freed.
 *
 * \sa xni_allocate_tcp_control_block()
 */
int xni_free_ib_control_block(xni_control_block_t *control_block);
/*! \brief The InfiniBand implementation of XNI.
 *
 * The InfiniBand implementation uses InfiniBand channel I/O over a
 * reliable connection to transfer buffers.
 *
 * \sa xni_context_create()
 */
extern xni_protocol_t xni_protocol_ib;

/*! @} */


#endif  // XDD_XNI_H

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
