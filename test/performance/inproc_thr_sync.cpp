#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <boost/asio.hpp>
#include <asio-zmq.hpp>

static std::string const ep = "inproc://thr_test";
static boost::asio::io_service ios;
static boost::asio::zmq::context ctx;

void pusher(int count, int msize)
{
    boost::asio::zmq::socket s(ios, ctx, ZMQ_PUSH);
    s.connect(ep);

    while (--count >= 0) s.write_frame(boost::asio::zmq::frame(msize));
}

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

    boost::asio::zmq::socket puller(ios, ctx, ZMQ_PULL);
    puller.bind(ep);

    std::thread worker(std::bind(pusher, message_count, message_size));

    puller.read_frame();

    auto watch = std::chrono::system_clock::now();

    for (int i = 1; i < message_count; ++i) puller.read_frame();

    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
                       std::chrono::system_clock::now() - watch).count();
    unsigned long throughput =
        static_cast<double>(message_count) / static_cast<double>(elapsed) * 1000000;
    double megabits = static_cast<double>(throughput * message_size * 8) / 1000000;

    std::cout << "mean throughput: " << throughput << " [msg/s]\n";
    std::cout << "mean throughput: " << megabits << " [Mb/s]\n";

    worker.join();
}