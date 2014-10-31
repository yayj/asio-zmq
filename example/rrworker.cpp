#include <chrono>
#include <functional>
#include <iostream>
#include <iterator>
#include <thread>
#include <vector>
#include <boost/asio.hpp>
#include <asio-zmq.hpp>

class rrworker {
private:
    boost::asio::zmq::socket responder_;
    std::vector<boost::asio::zmq::frame> buffer_;

public:
    rrworker(boost::asio::io_service& ios, boost::asio::zmq::context& ctx)
        : responder_(ios, ctx, ZMQ_REP), buffer_() {
        responder_.connect("tcp://localhost:5560");
        responder_.async_read_message(
            std::back_inserter(buffer_),
            std::bind(&rrworker::handle_req, this, std::placeholders::_1));
    }

    void handle_req(boost::system::error_code const& ec) {
        std::cout << "Received request: "
                  << std::to_string(buffer_[0])
                  << "\n";

        std::this_thread::sleep_for(std::chrono::seconds(1));

        buffer_.clear();
        buffer_.push_back(boost::asio::zmq::frame("World"));
        responder_.write_message(std::begin(buffer_), std::end(buffer_));

        buffer_.clear();
        responder_.async_read_message(
            std::back_inserter(buffer_),
            std::bind(&rrworker::handle_req, this, std::placeholders::_1));
    }
};

int main(int argc, char* argv[])
{
    boost::asio::io_service ios;
    boost::asio::zmq::context ctx;

    rrworker worker(ios, ctx);

    ios.run();

    return 0;
}