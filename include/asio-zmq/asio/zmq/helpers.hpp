#ifndef ASIO_ZMQ_HELPERS_HPP_
#define ASIO_ZMQ_HELPERS_HPP_

#include <zmq.h>

namespace asio {
namespace zmq {
struct message_deleter {
    void operator()(zmq_msg_t *msg) noexcept
    {
        zmq_msg_close(msg);
        delete msg;
    }
};

struct context_deleter {
    void operator()(void* ctx) noexcept
    {
        zmq_ctx_destroy(ctx);
    }
};

} // namespace zmq
} // namespace asio

#endif // ASIO_ZMQ_HELPERS_HPP_
