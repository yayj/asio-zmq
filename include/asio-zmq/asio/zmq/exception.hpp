#ifndef ASIO_ZMQ_EXCEPTION_HPP_
#define ASIO_ZMQ_EXCEPTION_HPP_

#include <exception>
#include <zmq.h>
#include "error.hpp"

namespace asio {
namespace zmq {

class exception : public std::exception {
private:
    int errno_;

public:
    exception() : errno_(zmq_errno())
    {}

    const char *what() const noexcept
    {
        return zmq_strerror(errno_);
    }

    asio::error_code get_code() const
    {
        return asio::error::make_error_code(
            static_cast<asio::error::zmq_error>(errno_));
    }
};

} // namespace zmq
} // namespace asio

#endif // ASIO_ZMQ_EXCEPTION_HPP_
