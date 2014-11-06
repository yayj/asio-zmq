#pragma once

#include <array>
#include <algorithm>
#include <cstddef>
#include <memory>
#include <string>
#include <boost/asio/io_service.hpp>
#include <boost/system/error_code.hpp>
#include <zmq.h>
#include "helpers.hpp"
#include "socket_option.hpp"
#include "context.hpp"
#include "frame.hpp"

namespace boost {
namespace asio {
namespace zmq {

class socket {
private:
    typedef std::unique_ptr<void, socket_deleter> zsocket_type;

    asio::io_service& io_;
    descriptor_type descriptor_;
    zsocket_type zsock_;

    template <typename OutputIt, typename HandlerPtr>
    void read_one_message(OutputIt buff_it, HandlerPtr handler, boost::system::error_code const& ec)
    {
        if (ec) {
            io_.post([=] { (*handler)(ec); });
            return;
        }

        try {
            if (is_readable()) {
                read_message(buff_it);
                io_.post([=] { (*handler)(boost::system::error_code()); });
            } else {
                descriptor_.async_read_some(asio::null_buffers(),
                                            [=](boost::system::error_code const& ec, std::size_t) {
                    read_one_message(buff_it, handler, ec);
                });
            }
        }
        catch (exception const& e) {
            auto code = e.get_code();
            io_.post([=] { (*handler)(code); });
        }
    }

    template <typename InputIt, typename HandlerPtr>
    void write_one_message(InputIt first_it, InputIt last_it, HandlerPtr handler,
                           boost::system::error_code const& ec)
    {
        if (ec) {
            io_.post([=] { (*handler)(ec); });
            return;
        }

        try {
            if (is_writable()) {
                write_message(first_it, last_it);
                io_.post([=] { (*handler)(boost::system::error_code()); });
            } else {
                descriptor_.async_write_some(asio::null_buffers(),
                                             [=](boost::system::error_code const& ec, std::size_t) {
                    write_one_message(first_it, last_it, handler, ec);
                });
            }
        }
        catch (exception const& e) {
            auto code = e.get_code();
            io_.post([=] { (*handler)(code); });
        }
    }

public:
    explicit socket(asio::io_service& io, context& ctx, int type)
        : io_(io), descriptor_(io), zsock_(::zmq_socket(ctx.zctx_.get(), type))
    {
        if (!zsock_) {
            throw exception();
        }

        socket_option::fd fd;
        get_option(fd);
        descriptor_.assign(fd.value());
    }

    void cancel() { descriptor_.cancel(); }

    void bind(std::string const& endpoint)
    {
        if (0 != zmq_bind(zsock_.get(), endpoint.c_str())) throw exception();
    }

    void connect(std::string const& endpoint)
    {
        if (0 != zmq_connect(zsock_.get(), endpoint.c_str())) throw exception();
    }

    bool is_readable() const
    {
        socket_option::events events;
        get_option(events);
        return (events.value() & ZMQ_POLLIN) == ZMQ_POLLIN;
    }

    bool is_writable() const
    {
        socket_option::events events;
        get_option(events);
        return (events.value() & ZMQ_POLLOUT) == ZMQ_POLLOUT;
    }

    bool has_more() const
    {
        socket_option::recv_more more;
        get_option(more);
        return more.value();
    }

    frame read_frame(int flag = 0)
    {
        frame tmp;
        if (-1 == zmq_msg_recv(&tmp.raw_msg_, zsock_.get(), flag)) throw exception();
        return tmp;
    }

    void write_frame(frame const& frm, int flag = 0)
    {
        if (-1 == zmq_msg_send(const_cast<zmq_msg_t*>(&frm.raw_msg_), zsock_.get(), flag))
            throw exception();
    }

    template <typename OutputIt> void read_message(OutputIt buff_it)
    {
        do {
            *buff_it++ = read_frame();
        } while (has_more());
    }

    template <typename InputIt> void write_message(InputIt first_it, InputIt last_it)
    {
        InputIt prev = first_it;
        InputIt curr = first_it;

        while (++curr != last_it) {
            write_frame(*prev, ZMQ_SNDMORE);
            ++prev;
        }
        if (prev != last_it) write_frame(*prev);
    }

    template <typename OutputIt, typename ReadHandler>
    void async_read_message(OutputIt buff_it, ReadHandler handler)
    {
        read_one_message(buff_it, std::make_shared<ReadHandler>(handler),
                         boost::system::error_code());
    }

    template <typename InputIt, typename WriteHandler>
    void async_write_message(InputIt first_it, InputIt last_it, WriteHandler handler)
    {
        write_one_message(first_it, last_it, std::make_shared<WriteHandler>(handler),
                          boost::system::error_code());
    }

    template <typename Option>
    void get_option(
        Option& option,
        typename std::enable_if<socket_option::is_raw_option<Option>::value>::type* = nullptr) const
    {
        std::size_t size = sizeof(option.value());
        if (-1 ==
            zmq_getsockopt(zsock_.get(), Option::id, static_cast<void*>(&option.value()), &size))
            throw exception();
    }

    template <typename Option>
    void get_option(Option& option,
                    typename std::enable_if<socket_option::is_bool_option<Option>::value>::type* =
                        nullptr) const
    {
        int v;
        std::size_t size = sizeof(v);
        if (-1 == zmq_getsockopt(zsock_.get(), Option::id, &v, &size)) throw exception();
        option.value() = static_cast<bool>(v);
    }

    template <typename Option>
    void set_option(
        Option const& option,
        typename std::enable_if<socket_option::is_raw_option<Option>::value>::type* = nullptr)
    {
        typename Option::option_value_type v = option.value();
        std::size_t size = sizeof(v);
        if (-1 == zmq_setsockopt(zsock_.get(), Option::id, &v, size)) throw exception();
    }

    template <typename Option>
    void set_option(
        Option const& option,
        typename std::enable_if<socket_option::is_bool_option<Option>::value>::type* = nullptr)
    {
        int v = static_cast<int>(option.value());
        std::size_t size = sizeof(v);
        if (-1 == zmq_getsockopt(zsock_.get(), Option::id, &v, size)) throw exception();
    }

    template <typename Option>
    void get_option(Option& option,
                    typename std::enable_if<socket_option::is_binary_option<Option>::value>::type* =
                        nullptr) const
    {
        std::array<std::uint8_t, socket_option::max_buff_size> buffer;
        std::size_t size = socket_option::max_buff_size;
        if (-1 == zmq_getsockopt(zsock_.get(), Option::id, buffer.data(), &size)) throw exception();
        option.resize(size);
        std::copy(buffer.data(), buffer.data() + size, static_cast<std::uint8_t*>(option.value()));
    }

    template <typename Option>
    void set_option(
        Option const& option,
        typename std::enable_if<socket_option::is_binary_option<Option>::value>::type* = nullptr)
    {
        if (-1 == zmq_setsockopt(zsock_.get(), Option::id, option.value(), option.size()))
            throw exception();
    }
};

}  // namespace zmq
}  // namespace asio
}  // namespace boost
