#include <iostream>
#include <iterator>
#include <vector>
#include <asio.hpp>
#include <asio-zmq.hpp>

int main(int argc, char* argv[])
{
    std::vector<asio::zmq::frame> buffer;
    asio::io_service ios;
    asio::zmq::context ctx;
    asio::zmq::socket requester(ios, ctx, ZMQ_REQ);

    requester.connect("tcp://localhost:5559");

    for (int count = 0; count < 10; ++count) {
    	buffer.clear();
        buffer.push_back(asio::zmq::frame("Hello"));
        requester.write_message(std::begin(buffer), std::end(buffer));

        buffer.clear();
        requester.read_message(std::back_inserter(buffer));
        std::cout << "Received reply " << count << " ["
                  << std::to_string(buffer[0])
                  << "]\n";
    }

    return 0;
}