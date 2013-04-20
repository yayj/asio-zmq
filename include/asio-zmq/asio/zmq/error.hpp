#ifndef ASIO_ZMQ_ERROR_HPP_
#define ASIO_ZMQ_ERROR_HPP_

#include <string>
#include <system_error>
#include <type_traits>
#include <asio/error.hpp>
#include <asio/error_code.hpp>
#include <zmq.h>

namespace asio {
namespace error {

enum class zmq_error {};

struct zmq_error_category_impl : public asio::error_category
{
    virtual const char* name() const noexcept
    {
        return "zeromq";
    }

    virtual std::string message(int ev) const noexcept
    {
        return ::zmq_strerror(ev);
    }
};

const asio::error_category& zmq_category()
{
    static zmq_error_category_impl instance;
    return instance;
}

asio::error_code make_error_code(zmq_error e)
{
    return asio::error_code(static_cast<int>(e), zmq_category());
}

} // namespace error
} // namespace asio

namespace std {

template <>
struct is_error_code_enum<asio::error::zmq_error>
    : std::true_type {};

} // namespace std

#endif // ASIO_ZMQ_EE_HPP_
