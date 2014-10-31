#include <chrono>
#include <functional>
#include <iostream>
#include <iterator>
#include <thread>
#include <vector>
#include <boost/asio.hpp>
#include <asio-zmq.hpp>

class taskwork {
private:
    boost::asio::zmq::socket receiver_;
    boost::asio::zmq::socket sender_;
    std::vector<boost::asio::zmq::frame> buffer_;

public:
    taskwork(boost::asio::io_service& ios, boost::asio::zmq::context& ctx)
        : receiver_(ios, ctx, ZMQ_PULL), sender_(ios, ctx, ZMQ_PUSH),
          buffer_() {
        //  Socket to receive messages on
        receiver_.connect("tcp://localhost:5557");

        //  Socket to send messages to
        sender_.connect("tcp://localhost:5558");
    }

    void start() {
        receiver_.async_read_message(
            std::back_inserter(buffer_),
            std::bind(&taskwork::handle_read, this, std::placeholders::_1));
    }

    void handle_read(boost::system::error_code const& ec) {
        //  Workload in msecs
        int workload = std::stoi(std::to_string(buffer_[0]));

        //  Do the work
        std::this_thread::sleep_for(std::chrono::milliseconds(workload));

        buffer_.clear();
        buffer_.push_back(boost::asio::zmq::frame());
        //  Send results to sink
        sender_.async_write_message(
            std::begin(buffer_), std::end(buffer_),
            std::bind(&taskwork::handle_write, this, std::placeholders::_1));
    }

    void handle_write(boost::system::error_code const& ec) {
        //  Simple progress indicator for the viewer
        std::cout << "." << std::flush;
        buffer_.clear();
        receiver_.async_read_message(
            std::back_inserter(buffer_),
            std::bind(&taskwork::handle_read, this, std::placeholders::_1));
    }
};

int main(int argc, char* argv[])
{
    boost::asio::io_service ios;
    boost::asio::zmq::context ctx;

    taskwork work(ios, ctx);

    work.start();

    //  Process tasks forever
    ios.run();

    return 0;
}