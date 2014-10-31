#ifndef ASIO_ZMQ_ERROR_HPP_
#define ASIO_ZMQ_ERROR_HPP_

#include <string>
#include <system_error>
#include <type_traits>
#include <boost/asio/error.hpp>
#include <boost/system/error_code.hpp>
#include <zmq.h>

namespace boost {
namespace asio {
namespace error {

enum class zmq_error {};

struct zmq_error_category_impl : public boost::system::error_category
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

const boost::system::error_category& zmq_category()
{
    static zmq_error_category_impl instance;
    return instance;
}

boost::system::error_code make_error_code(zmq_error e)
{
    return boost::system::error_code(static_cast<int>(e), zmq_category());
}

} // namespace error
} // namespace asio
} // namespace boost

namespace std {

template <>
struct is_error_code_enum<boost::asio::error::zmq_error>
    : std::true_type {};

} // namespace std

#endif // ASIO_ZMQ_EE_HPP_
