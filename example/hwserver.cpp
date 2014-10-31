#include <functional>
#include <iostream>
#include <iterator>
#include <thread>
#include <vector>
#include <boost/asio.hpp>
#include <asio-zmq.hpp>

class hwserver {
private:
    boost::asio::zmq::socket socket_;
    std::vector<boost::asio::zmq::frame> buffer_;

public:
    hwserver(boost::asio::io_service& ios, boost::asio::zmq::context& ctx)
        : socket_(ios, ctx, ZMQ_REP), buffer_()
    {
    }

    void start()
    {
        socket_.bind("tcp://*:5555");
        socket_.async_read_message(std::back_inserter(buffer_),
                                   std::bind(&hwserver::handle_read, this, std::placeholders::_1));
    }

    void handle_read(boost::system::error_code const& ec)
    {
        std::cout << "Received Hello\n";

        std::this_thread::sleep_for(std::chrono::seconds(1));

        buffer_.clear();
        buffer_.push_back(boost::asio::zmq::frame("World"));

        socket_.async_write_message(
            std::begin(buffer_), std::end(buffer_),
            std::bind(&hwserver::handle_write, this, std::placeholders::_1));
    }

    void handle_write(boost::system::error_code const& ec)
    {
        buffer_.clear();
        socket_.async_read_message(std::back_inserter(buffer_),
                                   std::bind(&hwserver::handle_read, this, std::placeholders::_1));
    }
};

int main()
{
    boost::asio::io_service ios;
    boost::asio::zmq::context ctx;
    hwserver server(ios, ctx);

    server.start();

    ios.run();

    return 0;
}
