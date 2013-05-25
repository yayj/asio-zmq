#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <asio.hpp>
#include <asio-zmq.hpp>

static std::string const ep = "inproc://lat_test";
static asio::io_service ios;
static asio::zmq::context ctx;

void requester(int rc, int msize)
{
    asio::zmq::socket req(ios, ctx, ZMQ_REQ);
    req.connect(ep);

    while (--rc >= 0) {
        req.write_frame(asio::zmq::frame(msize));
        req.read_frame();
    }
}

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

    asio::zmq::socket rep(ios, ctx, ZMQ_REP);
    rep.bind(ep);

    std::thread worker(std::bind(requester, roundtrip_count, message_size));

    auto watch = std::chrono::system_clock::now();

    for (int i = 0; i < roundtrip_count; ++i)
        rep.write_frame(rep.read_frame());

    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
                       std::chrono::system_clock::now() - watch).count();
    double latency = static_cast<double>(elapsed) / (roundtrip_count * 2);

    std::cout << "average latency: " << latency << " [us]\n";

    worker.join();
}
