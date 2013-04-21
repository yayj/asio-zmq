#ifndef ASIO_ZMQ_HELPERS_HPP_
#define ASIO_ZMQ_HELPERS_HPP_

#include <asio/io_service.hpp>
#include <zmq.h>

namespace asio {
namespace zmq {

template <typename IoObjectService>
struct non_closing_io_object_service : public IoObjectService {
    explicit non_closing_io_object_service(asio::io_service& io)
        : IoObjectService(io)
    {}

    void destroy(typename IoObjectService::implementation_type& impl)
    {}
};

struct message_deleter {
    void operator()(zmq_msg_t *msg) noexcept
    {
        zmq_msg_close(msg);
        delete msg;
    }
};

struct socket_deleter {
    void operator()(void* sock) noexcept
    {
        zmq_close(sock);
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
