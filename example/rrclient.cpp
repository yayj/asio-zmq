#include <algorithm>
#include <iostream>
#include <iterator>
#include <vector>
#include <asio.hpp>
#include <asio-zmq.hpp>

int main(int argc, char* argv[])
{
    std::string request = "Hello";
    std::vector<asio::zmq::frame> buffer;
    asio::io_service ios;
    asio::zmq::context ctx;
    asio::zmq::socket requester(ios, ctx, ZMQ_REQ);

    requester.connect("tcp://localhost:5559");

    for (int count = 0; count < 10; ++count) {
    	buffer.clear();
        buffer.push_back(asio::zmq::frame(request.size()));
        std::copy(std::begin(request), std::end(request),
                  static_cast<char*>(buffer[0].data()));
        requester.write_message(std::begin(buffer), std::end(buffer));

        buffer.clear();
        requester.read_message(std::back_inserter(buffer));
        std::cout << "Received reply " << count << " ["
                  << std::string(static_cast<char*>(
                                     buffer[0].data()), buffer[0].size())
                  << "]\n";
    }

    return 0;
}