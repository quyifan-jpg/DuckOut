// #ifndef _COMMUNICATION_H_
// #define _COMMUNICATION_H_

// #include <ucp/api/ucp.h>
// #include <stdbool.h>
// #include <stddef.h>

// // UCX context structure
// typedef struct
// {
//     ucp_context_h context;
//     ucp_worker_h worker;
//     ucp_ep_h ep;
//     ucp_listener_h listener; // For server
// } ucx_context_t;

// // Function declarations
// bool ucp_init_context(ucx_context_t *ctx, const char *transport_type);
// void ucp_cleanup_context(ucx_context_t *ctx);
// bool create_server_ucp(ucx_context_t *ctx, const char *ip, int port);
// bool ucp_wait_for_connection(ucx_context_t *ctx, int timeout_ms);
// bool create_client_ucp(ucx_context_t *ctx, const char *server_ip, int port);
// bool ucp_send_data(ucx_context_t *ctx, const void *buffer, size_t length, ucp_tag_t tag);
// bool ucp_recv_data(ucx_context_t *ctx, void *buffer, size_t length,
//                    ucp_tag_t tag, ucp_tag_t tag_mask, size_t *received_length);

// #endif // _COMMUNICATION_H_