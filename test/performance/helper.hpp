#ifndef ASIO_ZMQ_PERF_HELPER_HPP
#define ASIO_ZMQ_PERF_HELPER_HPP

#include <iterator>
#include <string>
#include <vector>
#include <boost/asio.hpp>
#include <asio-zmq.hpp>

namespace boost {
namespace asio {
namespace zmq {
namespace test {
namespace perf {

typedef std::vector<boost::asio::zmq::frame> message_t;

class requester {
private:
    boost::asio::zmq::socket req_;
    message_t msg_;
    int rc_;
    int const message_size_;

    void handle_write(boost::system::error_code const& ec) {
        msg_.clear();
        req_.async_read_message(
            std::back_inserter(msg_),
            std::bind(&requester::handle_read, this, std::placeholders::_1));
    }

    void handle_read(boost::system::error_code const& ec) {
        if (--rc_ == 0)
            return;

        msg_.clear();
        msg_.push_back(boost::asio::zmq::frame(message_size_));
        req_.async_write_message(
            std::begin(msg_), std::end(msg_),
            std::bind(&requester::handle_write, this, std::placeholders::_1));
    }

public:
    requester(boost::asio::io_service& ios, boost::asio::zmq::context& ctx,
              int rc, int message_size, std::string const& ep)
        : req_(ios, ctx, ZMQ_REQ), msg_(), rc_(rc),
          message_size_(message_size) {
        req_.connect(ep);

        msg_.push_back(boost::asio::zmq::frame(message_size_));
        req_.async_write_message(
            std::begin(msg_), std::end(msg_),
            std::bind(&requester::handle_write, this, std::placeholders::_1));
    }
};

class replier {
private:
    boost::asio::zmq::socket rep_;
    message_t msg_;
    int rc_;

    void handle_write(boost::system::error_code const& ec) {
        if (--rc_ == 0)
            return;

        msg_.clear();
        rep_.async_read_message(
            std::back_inserter(msg_),
            std::bind(&replier::handle_read, this, std::placeholders::_1));
    }

    void handle_read(boost::system::error_code const& ec) {
        rep_.async_write_message(
            std::begin(msg_), std::end(msg_),
            std::bind(&replier::handle_write, this, std::placeholders::_1));
    }

public:
    replier(boost::asio::io_service& ios, boost::asio::zmq::context& ctx,
            int rc, std::string const& ep)
        : rep_(ios, ctx, ZMQ_REP), msg_(), rc_(rc) {
        rep_.bind(ep);

        rep_.async_read_message(
            std::back_inserter(msg_),
            std::bind(&replier::handle_read, this, std::placeholders::_1));
    }
};

class pusher {
private:
    boost::asio::zmq::socket pusher_;
    int count_;
    int size_;
    message_t msg_;

    void handle_write(boost::system::error_code const& ec) {
        if (--count_ == 0)
            return;

        msg_.clear();
        msg_.push_back(boost::asio::zmq::frame(size_));
        pusher_.async_write_message(
            std::begin(msg_), std::end(msg_),
            std::bind(&pusher::handle_write, this, std::placeholders::_1));
    }

public:
    explicit pusher(boost::asio::io_service& ios, boost::asio::zmq::context& ctx,
                    int count, int size, std::string const& ep)
        : pusher_(ios, ctx, ZMQ_PUSH), count_(count), size_(size), msg_() {
        pusher_.connect(ep);

        msg_.push_back(boost::asio::zmq::frame(size_));
        pusher_.async_write_message(
            std::begin(msg_), std::end(msg_),
            std::bind(&pusher::handle_write, this, std::placeholders::_1));
    }
};

class puller {
private:
    boost::asio::zmq::socket puller_;
    int count_;
    message_t msg_;

    void handle_read(boost::system::error_code const& ec) {
        if (--count_ == 0)
            return;

        msg_.clear();
        puller_.async_read_message(
            std::back_inserter(msg_),
            std::bind(&puller::handle_read, this, std::placeholders::_1));
    }

public:
    puller(boost::asio::io_service& ios, boost::asio::zmq::context& ctx,
           int count, std::string const& ep)
        : puller_(ios, ctx, ZMQ_PULL), count_(count), msg_() {
        puller_.bind(ep);
        puller_.async_read_message(
            std::back_inserter(msg_),
            std::bind(&puller::handle_read, this, std::placeholders::_1));
    }
};

}
}
}
}
}

#endif // ASIO_ZMQ_PERF_HELPER_HPP