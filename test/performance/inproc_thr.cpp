#include <chrono>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>
#include <asio.hpp>
#include <asio-zmq.hpp>
#include "helper.hpp"

static std::string const ep = "inproc://thr_test";

int main(int argc, char* argv[])
{
    if (argc != 3) {
        std::cerr << "usage: inproc_thr <message-size> <message-count>\n";
        return 1;
    }

    int message_size = std::atoi(argv[1]);
    int message_count = std::atoi(argv[2]);

    std::cout << "message size: " << message_size << " [B]\n";
    std::cout << "message count: " << message_count << "\n";

    asio::io_service ios;
    asio::zmq::context ctx;

    asio::zmq::test::perf::puller pl(ios, ctx, message_count, ep);
    asio::zmq::test::perf::pusher ps(ios, ctx, message_count, message_size, ep);

    auto watch = std::chrono::system_clock::now();

    ios.run();

    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
                       std::chrono::system_clock::now() - watch).count();
    unsigned long throughput = static_cast<double>(message_count) /
                               static_cast<double>(elapsed) * 1000000;
    double megabits = static_cast<double>(throughput * message_size * 8)
                      / 1000000;

    std::cout << "mean throughput: " << throughput << " [msg/s]\n";
    std::cout << "mean throughput: " << megabits << " [Mb/s]\n";
}