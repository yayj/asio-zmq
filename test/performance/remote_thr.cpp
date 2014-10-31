#include <cstdlib>
#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <asio-zmq.hpp>
#include "helper.hpp"

int main(int argc, char* argv[])
{
    if (argc != 4) {
        std::cerr << "usage: remote_thr <connect-to> <message-size> "
                  << "<message-count>\n";
        return 1;
    }

    std::string const ep = argv[1];
    int message_size = std::atoi(argv[2]);
    int message_count = std::atoi(argv[3]);

    boost::asio::io_service ios;
    boost::asio::zmq::context ctx;

    boost::asio::zmq::test::perf::pusher ps(ios, ctx, message_count, message_size, ep);

    ios.run();
}