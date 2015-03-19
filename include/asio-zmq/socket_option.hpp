#pragma once

#include <cstdint>
#include <string>
#include <type_traits>
#include <vector>
#include <zmq.h>
#include "helpers.hpp"

namespace boost {
namespace asio {
namespace zmq {
namespace socket_option {

std::size_t const max_buff_size = 255;

template <int option, typename T> struct socket_option_impl {
    static int const id = option;
    typedef T option_value_type;

    socket_option_impl(option_value_type value) : value_(value) {}

    option_value_type value() const { return value_; }

    option_value_type& value() { return value_; }

private:
    option_value_type value_;
};

template <int option> struct socket_option_impl<option, void*> {
    static int const id = option;
    typedef void* option_value_type;

    socket_option_impl() {}

    socket_option_impl(void const* value, std::size_t size)
        : value_(static_cast<std::uint8_t const*>(value),
                 static_cast<std::uint8_t const*>(value) + size)
    {
    }

    void const* value() const { return static_cast<void const*>(value_.data()); }

    void* value() { return static_cast<void*>(value_.data()); }

    std::size_t size() const { return value_.size(); }

private:
    std::vector<std::uint8_t> value_;
};

struct events : public socket_option_impl<ZMQ_EVENTS, int> {
    static int const default_value = -1;
    explicit events(int v = default_value) : socket_option_impl<ZMQ_EVENTS, int>(v) {}
};

struct send_buff_hwm : public socket_option_impl<ZMQ_SNDHWM, int> {
    static int const default_value = 1000;
    explicit send_buff_hwm(int v = default_value) : socket_option_impl<ZMQ_SNDHWM, int>(v) {}
};

struct recv_more : public socket_option_impl<ZMQ_RCVMORE, bool> {
    static bool const default_value = false;
    explicit recv_more(int v = default_value) : socket_option_impl<ZMQ_RCVMORE, bool>(v) {}
};

struct identity : public socket_option_impl<ZMQ_IDENTITY, void*> {
    identity() {}
    identity(void const* value, std::size_t size)
        : socket_option_impl<ZMQ_IDENTITY, void*>(value, size)
    {
    }
    identity(const std::string& value)
        : socket_option_impl<ZMQ_IDENTITY, void*>(value.c_str(),value.size())
    {
    }
};

struct subscribe : public socket_option_impl<ZMQ_SUBSCRIBE, void*> {
    subscribe() {}
    subscribe(void const* value, std::size_t size)
        : socket_option_impl<ZMQ_SUBSCRIBE, void*>(value, size)
    {
    }
    subscribe(const std::string& value)
        : socket_option_impl<ZMQ_SUBSCRIBE, void*>(value.c_str(), value.size())
    {
    }
};

struct fd : public socket_option_impl<ZMQ_FD, native_handle_type> {
    static native_handle_type const default_value = -1;
    explicit fd(native_handle_type v = default_value)
        : socket_option_impl<ZMQ_FD, native_handle_type>(v)
    {
    }
};

struct linger : public socket_option_impl<ZMQ_LINGER, int> {
    static int const default_value = -1;
    explicit linger(int v = default_value) : socket_option_impl<ZMQ_LINGER, int>(v) {}
};

template <typename OptionType>
struct is_binary_option
    : public std::is_base_of<socket_option_impl<OptionType::id, void*>, OptionType> {
};

template <typename OptionType>
struct enable_if_binary : public std::enable_if<is_binary_option<OptionType>::value> {
};

template <typename OptionType>
struct is_bool_option
    : public std::is_base_of<socket_option_impl<OptionType::id, bool>, OptionType> {
};

template <typename OptionType>
struct enable_if_bool : public std::enable_if<is_bool_option<OptionType>::value> {
};

template <typename OptionType>
struct is_raw_option
    : public std::integral_constant<
          bool, std::is_base_of<
                    socket_option_impl<OptionType::id, typename OptionType::option_value_type>,
                    OptionType>::value &&
                    !is_bool_option<OptionType>::value && !is_binary_option<OptionType>::value> {
};

template <typename OptionType>
struct enable_if_raw : public std::enable_if<is_raw_option<OptionType>::value> {
};

}  // namespace socket_option
}  // namespace zmq
}  // namespace asio
}  // namespace boost
