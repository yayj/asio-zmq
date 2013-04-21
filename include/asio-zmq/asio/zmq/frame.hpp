#ifndef ASIO_ZMQ_FRAME_HPP_
#define ASIO_ZMQ_FRAME_HPP_

#include <cstddef>
#include <memory>
#include <utility>
#include <zmq.h>
#include "helpers.hpp"
#include "exception.hpp"

namespace asio {
namespace zmq {

class frame {
    friend class socket;

private:
    typedef std::unique_ptr<zmq_msg_t, message_deleter> zmsg_type;

    zmsg_type body_;

    zmsg_type create(std::size_t size)
    {
        zmsg_type ret{ new zmq_msg_t };
        int rc;

        if (size == 0)
            rc = zmq_msg_init(ret.get());
        else
            rc = zmq_msg_init_size(ret.get(), size);

        if (rc != 0)
            throw exception();

        return ret;
    }
    
public:
    explicit frame(std::size_t size = 0) : body_(create(size))
    {
    }

    frame(frame&& other) noexcept : body_(std::move(other.body_))
    {
    }

    frame& operator=(frame&& other) noexcept
    {
        body_.reset();
        body_.swap(other.body_);
        return *this;
    }
    
    frame(frame const& other) : body_(create(other.size()))
    {
        if (0 != zmq_msg_copy(body_.get(), other.body_.get()))
            throw exception();
    }

    frame& operator=(frame const& other)
    {
        zmsg_type tmp{create(other.size())};

        if (0 != zmq_msg_copy(tmp.get(), other.body_.get()))
            throw exception();

        body_.swap(tmp);
        return *this;
    }

    std::size_t size() const noexcept
    {
        if (body_)
            return zmq_msg_size(const_cast<zmq_msg_t*>(body_.get()));
        return 0;
    }

    void *data() noexcept
    {
        if (body_)
            return zmq_msg_data(body_.get());
        return nullptr;
    }

    const void *data() const noexcept
    {
        if (body_)
            return zmq_msg_data(body_.get());
        return nullptr;
    }
};

} // namespace zmq
} // namespace asio

#endif // ASIO_ZMQ_FRAME_HPP_
