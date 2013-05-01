#include <chrono>
#include <functional>
#include <iostream>
#include <iterator>
#include <thread>
#include <vector>
#include <asio.hpp>
#include <asio-zmq.hpp>

class rrworker {
private:
    asio::zmq::socket responder_;
    std::vector<asio::zmq::frame> buffer_;

public:
    rrworker(asio::io_service& ios, asio::zmq::context& ctx)
        : responder_(ios, ctx, ZMQ_REP), buffer_() {
        responder_.connect("tcp://localhost:5560");
        responder_.async_read_message(
            std::back_inserter(buffer_),
            std::bind(&rrworker::handle_req, this, std::placeholders::_1));
    }

    void handle_req(asio::error_code const& ec) {
        std::cout << "Received request: "
                  << std::to_string(buffer_[0])
                  << "\n";

        std::this_thread::sleep_for(std::chrono::seconds(1));

        buffer_.clear();
        buffer_.push_back(asio::zmq::frame("World"));
        responder_.write_message(std::begin(buffer_), std::end(buffer_));

        buffer_.clear();
        responder_.async_read_message(
            std::back_inserter(buffer_),
            std::bind(&rrworker::handle_req, this, std::placeholders::_1));
    }
};

int main(int argc, char* argv[])
{
    asio::io_service ios;
    asio::zmq::context ctx;

    rrworker worker(ios, ctx);

    ios.run();

    return 0;
}