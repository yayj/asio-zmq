#pragma once

#include <boost/asio/io_service.hpp>
#include <boost/asio/posix/basic_stream_descriptor.hpp>
#include <boost/asio/posix/stream_descriptor_service.hpp>
#include <zmq.h>

namespace boost {
namespace asio {
namespace zmq {

template <typename IoObjectService> struct non_closing_io_object_service : public IoObjectService {
    explicit non_closing_io_object_service(asio::io_service& io) : IoObjectService(io) {}

    void destroy(typename IoObjectService::implementation_type& impl) {}
};

typedef non_closing_io_object_service<asio::posix::stream_descriptor_service> descriptor_service;

typedef asio::posix::basic_stream_descriptor<descriptor_service> descriptor_type;

typedef descriptor_type::native_handle_type native_handle_type;

struct socket_deleter {
    void operator()(void* sock) noexcept { zmq_close(sock); }
};

struct context_deleter {
    void operator()(void* ctx) noexcept { zmq_ctx_destroy(ctx); }
};

}  // namespace zmq
}  // namespace asio
}  // namespace boost
