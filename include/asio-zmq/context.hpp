#ifndef ASIO_ZMQ_CONTEXT_HPP_
#define ASIO_ZMQ_CONTEXT_HPP_

#include <memory>
#include <zmq.h>
#include "helpers.hpp"
#include "exception.hpp"

namespace asio {
namespace zmq {

class context {
    friend class socket;

private:
    std::unique_ptr<void, context_deleter> zctx_;

    int get_option(int option)
    {
        int ret = zmq_ctx_get(zctx_.get(), option);

        if (ret < 0)
            throw exception();

        return ret;
    }

    void set_option(int option, int value)
    {
        if (0 != zmq_ctx_set(zctx_.get(), option, value))
            throw exception();
    }
    
public:
    explicit context() : zctx_(zmq_ctx_new())
    {
        if (zctx_ == nullptr)
            throw exception();
    }

    context(context const&) = delete;
    context(context&&) = delete;
    context& operator=(context const&) = delete;
    context& operator=(context&&) = delete;

    int get_io_threads()
    {
        return get_option(ZMQ_IO_THREADS);
    }

    int get_max_sockets()
    {
        return get_option(ZMQ_MAX_SOCKETS);
    }

    void set_io_threads(int num)
    {
        set_option(ZMQ_IO_THREADS, num);
    }

    void set_max_sockets(int num)
    {
        set_option(ZMQ_MAX_SOCKETS, num);
    }
};

} // namespace zmq
} // namespace asio

#endif // ASIO_ZMQ_CONTEXT_HPP_
