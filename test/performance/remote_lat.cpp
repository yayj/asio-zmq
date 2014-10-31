#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <asio-zmq.hpp>
#include "helper.hpp"

int main(int argc, char* argv[])
{
    if (argc != 4) {
        std::cerr << "usage: local_lat <bind-to> <message-size> "
                  << "<roundtrip-count>\n";
        return 1;
    }

    std::string const ep = argv[1];
    int message_size = std::atoi(argv[2]);
    int roundtrip_count = std::atoi(argv[3]);

    boost::asio::io_service ios;
    boost::asio::zmq::context ctx;

    boost::asio::zmq::test::perf::requester requester(ios, ctx, roundtrip_count, message_size, ep);

    auto watch = std::chrono::system_clock::now();

    ios.run();

    auto elapsed = std::chrono::system_clock::now() - watch;
    double latency = static_cast<double>(
                         std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count()) /
                     (roundtrip_count * 2);
    std::cout << "message size: " << message_size << " [B]\n";
    std::cout << "roundtrip count: " << roundtrip_count << "\n";
    std::cout << "average latency: " << latency << " [us]\n";
}