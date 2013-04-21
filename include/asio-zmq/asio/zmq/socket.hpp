#ifndef ASIO_ZMQ_SOCKET_HPP_
#define ASIO_ZMQ_SOCKET_HPP_

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <asio/error_code.hpp>
#include <asio/io_service.hpp>
#include <asio/posix/basic_stream_descriptor.hpp>
#include <asio/posix/stream_descriptor_service.hpp>
#include <zmq.h>
#include "helpers.hpp"
#include "context.hpp"
#include "frame.hpp"

namespace asio {
namespace zmq {

class socket {
private:
    typedef
    non_closing_io_object_service<asio::posix::stream_descriptor_service>
    descriptor_service;

    typedef
    asio::posix::basic_stream_descriptor<descriptor_service>
    descriptor_type;

    typedef descriptor_type::native_handle_type native_handle_type;
    typedef std::unique_ptr<void, socket_deleter> zsocket_type;

    asio::io_service& io_;
    descriptor_type descriptor_;
    zsocket_type zsock_;

    template <typename ValueType>
    void get_option(ValueType& value, int option) const
    {
        std::size_t size = sizeof(ValueType);
        if (0 != zmq_getsockopt(zsock_.get(), option, &value, &size)) {
            throw exception();
        }
    }

    template <typename OutputIt, typename ReadHandler>
    void read_one_message(OutputIt buff_it, ReadHandler handler,
                          asio::error_code const& ec)
    {
        if (ec) {
            io_.post(std::bind(*handler, ec));
            return;
        }

        try {
            if (is_readable()) {
                read_message(buff_it);
                io_.post(
                    std::bind(*handler, asio::error_code()));
            } else {
                descriptor_.async_read_some(
                    asio::null_buffers(),
                    std::bind(
                        &socket::read_one_message<OutputIt, ReadHandler>,
                        this, buff_it,
                        handler, std::placeholders::_1));
            }
        } catch (exception e) {
            io_.post(std::bind(*handler, e.get_code()));
        }
    }

    template <typename InputIt, typename WriteHandler>
    void write_one_message(InputIt first_it, InputIt last_it,
                           WriteHandler handler,
                           asio::error_code const& ec)
    {
        if (ec) {
            io_.post(std::bind(*handler, ec));
            return;
        }

        try {
            if (is_writable()) {
                write_message(first_it, last_it);
                io_.post(std::bind(*handler, asio::error_code()));
            } else {
                descriptor_.async_write_some(
                    asio::null_buffers(),
                    std::bind(
                        &socket::write_one_message<InputIt, WriteHandler>,
                        this, first_it, last_it, handler,
                        std::placeholders::_1));
            }
        } catch (exception e) {
            io_.post(std::bind(*handler, e.get_code()));
        }
    }

public:
    explicit socket(asio::io_service& io, context& ctx, int type)
        : io_(io), descriptor_(io),
          zsock_(::zmq_socket(ctx.zctx_.get(), type))
    {
        if (!zsock_) {
            throw exception();
        }

        native_handle_type handle = -1;
        get_option(handle, ZMQ_FD);
        descriptor_.assign(handle);
    }

    void cancel()
    {
        descriptor_.cancel();
    }
    
    void bind(std::string const& endpoint)
    {
        if (0 != zmq_bind(zsock_.get(), endpoint.c_str()))
            throw exception();
    }

    void connect(std::string const& endpoint)
    {
        if (0 != zmq_connect(zsock_.get(), endpoint.c_str()))
            throw exception();
    }

    bool has_more() const
    {
        int more = 0;
        get_option(more, ZMQ_RCVMORE);
        return static_cast<bool>(more);
    }

    bool is_readable() const
    {
        uint32_t events = 0;
        get_option(events, ZMQ_EVENTS);
        return (events & ZMQ_POLLIN) == ZMQ_POLLIN;
    }

    bool is_writable() const
    {
        uint32_t events = 0;
        get_option(events, ZMQ_EVENTS);
        return (events & ZMQ_POLLOUT) == ZMQ_POLLOUT;
    }

    template <typename OutputIt>
    void read_message(OutputIt buff_it)
    {
        do {
            frame frm;
            if (-1 == zmq_msg_recv(frm.body_.get(), zsock_.get(), 0))
                throw exception();
            *buff_it++ = std::move(frm);
        } while (has_more());
    }

    template <typename InputIt>
    void write_message(InputIt first_it, InputIt last_it)
    {
        InputIt prev = first_it;
        InputIt curr = first_it;

        while (++curr != last_it) {
            if (-1 ==
                zmq_msg_send(
                    prev->body_.get(), zsock_.get(), ZMQ_SNDMORE))
                throw exception();
            ++prev;
        }
        if (prev != last_it &&
            -1 == zmq_msg_send(prev->body_.get(), zsock_.get(), 0))
            throw exception();
    }
    
    template <typename OutputIt, typename ReadHandler>
    void async_read_message(OutputIt buff_it, ReadHandler handler)
    {
        read_one_message(buff_it,
                         std::shared_ptr<ReadHandler>(
                             new ReadHandler(handler)),
                         asio::error_code());
    }

    template <typename InputIt, typename WriteHandler>
    void async_write_message(InputIt first_it, InputIt last_it,
                             WriteHandler handler)
    {
        write_one_message(first_it, last_it,
                          std::shared_ptr<WriteHandler>(
                              new WriteHandler(handler)),
                          asio::error_code());
    }
};

} // namespace zmq
} // namespace asio

#endif // ASIO_ZMQ_SOCKET_HPP_
