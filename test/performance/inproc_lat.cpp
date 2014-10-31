#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <asio-zmq.hpp>
#include "helper.hpp"

static std::string const ep = "inproc://lat_test";

int main(int argc, char* argv[])
{
    if (argc != 3) {
        std::cerr << "usage: inproc_lat <message-size> <roundtrip-count>\n";
        return 1;
    }

    int message_size = std::atoi(argv[1]);
    int roundtrip_count = std::atoi(argv[2]);

    std::cout << "message size: " << message_size << " [B]\n";
    std::cout << "roundtrip count: " << roundtrip_count << "\n";

    boost::asio::io_service ios;
    boost::asio::zmq::context ctx;

    auto watch = std::chrono::system_clock::now();

    boost::asio::zmq::test::perf::replier rep(ios, ctx, roundtrip_count, ep);
    boost::asio::zmq::test::perf::requester req(ios, ctx, roundtrip_count,
                                         message_size, ep);

    ios.run();

    auto elapsed = std::chrono::system_clock::now() - watch;
    double latency = static_cast<double>(
                         std::chrono::duration_cast<std::chrono::microseconds>(
                             elapsed).count()) / (roundtrip_count * 2);

    std::cout << "average latency: " << latency << " [us]\n";
}