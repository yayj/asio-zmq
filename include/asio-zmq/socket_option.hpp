#ifndef ASIO_ZMQ_SOCKET_OPTION_HPP_
#define ASIO_ZMQ_SOCKET_OPTION_HPP_

#include <cstdint>
#include <type_traits>
#include <vector>
#include <zmq.h>
#include "helpers.hpp"

namespace asio {
namespace zmq {
namespace socket_option {

constexpr std::size_t max_buff_size = 255;

template <int option, typename T>
struct default_option_value {};

template <>
struct default_option_value<ZMQ_EVENTS, int> {
    constexpr static int value = -1;
};

template <>
struct default_option_value<ZMQ_SNDHWM, int> {
    constexpr static int value = 1000;
};

template <>
struct default_option_value<ZMQ_RCVMORE, bool> {
    constexpr static bool value = false;
};

template <>
struct default_option_value<ZMQ_LINGER, int> {
    constexpr static int value = -1;
};

template <>
struct default_option_value<ZMQ_FD, native_handle_type> {
    constexpr static native_handle_type value = -1;
};

template <int option, typename T>
class raw_type_option {
public:
    constexpr static int id = option;

    typedef T option_value_type;

    raw_type_option()
        : value_(default_option_value<option, option_value_type>::value) {}

    explicit raw_type_option(option_value_type value) : value_(value) {}

    option_value_type& value() {
        return value_;
    }

    option_value_type value() const {
        return value_;
    }

private:
    option_value_type value_;
};

template <int option>
class binary_type_option {
public:
    constexpr static int id = option;

    typedef void* option_value_type;

    binary_type_option() {}

    binary_type_option(void const* value, std::size_t size)
        : value_(static_cast<std::uint8_t const*>(value),
                 static_cast<std::uint8_t const*>(value) + size) {}

    void const* value() const {
        return static_cast<void const*>(value_.data());
    }

    void* value() {
        return static_cast<void*>(value_.data());
    }

    std::size_t size() const {
        return value_.size();
    }

    void resize(std::size_t size) {
        value_.resize(size);
    }

private:
    std::vector<std::uint8_t> value_;
};

typedef raw_type_option<ZMQ_EVENTS, int> events;
typedef raw_type_option<ZMQ_SNDHWM, int> send_buff_hwm;
typedef raw_type_option<ZMQ_RCVMORE, bool> recv_more;
typedef binary_type_option<ZMQ_IDENTITY> identity;
typedef raw_type_option<ZMQ_FD, native_handle_type> fd;
typedef raw_type_option<ZMQ_LINGER, int> linger;

template <typename OptionType>
struct is_binary_option
        : public std::is_same<OptionType,
          binary_type_option<OptionType::id>> {
};

template <typename OptionType>
struct is_bool_option
        : public std::is_same<OptionType,
          raw_type_option<OptionType::id, bool>> {
};

template <typename OptionType>
struct is_raw_option {
    constexpr static bool value =
        std::is_same<OptionType,
        raw_type_option<OptionType::id,
        typename OptionType::option_value_type>>::value
        && !is_bool_option<OptionType>::value;
};

} // namespace socket_option
} // namespace zmq
} // namespace asio

#endif // ASIO_ZMQ_SOCKET_OPTION_HPP_