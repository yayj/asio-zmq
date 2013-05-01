#include <functional>
#include <iostream>
#include <iterator>
#include <thread>
#include <vector>
#include <asio.hpp>
#include <asio-zmq.hpp>

class hwserver {
private:
    asio::zmq::socket socket_;
    std::vector<asio::zmq::frame> buffer_;

public:
    hwserver(asio::io_service& ios, asio::zmq::context& ctx)
        : socket_(ios, ctx, ZMQ_REP), buffer_()
    {}

    void start() {
        socket_.bind("tcp://*:5555");
        socket_.async_read_message(
            std::back_inserter(buffer_),
            std::bind(&hwserver::handle_read, this, std::placeholders::_1));
    }

    void handle_read(asio::error_code const& ec) {
        std::cout << "Received Hello\n";

        std::this_thread::sleep_for(std::chrono::seconds(1));

        buffer_.clear();
        buffer_.push_back(asio::zmq::frame("World"));

        socket_.async_write_message(
            std::begin(buffer_), std::end(buffer_),
            std::bind(&hwserver::handle_write, this, std::placeholders::_1));
    }

    void handle_write(asio::error_code const& ec) {
        buffer_.clear();
        socket_.async_read_message(
            std::back_inserter(buffer_),
            std::bind(&hwserver::handle_read, this, std::placeholders::_1));
    }
};

int main()
{
    asio::io_service ios;
    asio::zmq::context ctx;
    hwserver server(ios, ctx);

    server.start();

    ios.run();

    return 0;
}
