#include <algorithm>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>
#include <asio.hpp>
#include <asio-zmq.hpp>

void s_send(asio::zmq::socket& socket, std::string const& str)
{
    std::vector<asio::zmq::frame> message;
    message.push_back(asio::zmq::frame(str.size()));
    std::copy(std::begin(str), std::end(str),
              static_cast<char*>(message[0].data()));
    socket.write_message(std::begin(message), std::end(message));
}

void s_dump(asio::zmq::socket& socket)
{
    std::vector<asio::zmq::frame> buffer;
    socket.read_message(std::back_inserter(buffer));
    std::for_each(std::begin(buffer), std::end(buffer),
    [](asio::zmq::frame const& frame) {
        std::cout << std::string(static_cast<char const*>(frame.data()),
                                 frame.size()) << std::endl;
    });
}

int main()
{
    asio::io_service ios;
    asio::zmq::context ctx;

    asio::zmq::socket sink(ios, ctx, ZMQ_ROUTER);
    sink.bind("inproc://example");

    asio::zmq::socket anonymous(ios, ctx, ZMQ_REQ);
    anonymous.connect("inproc://example");

    s_send(anonymous, "ROUTER uses a generated UUID");
    s_dump(sink);

    asio::zmq::socket identified(ios, ctx, ZMQ_REQ);
    std::string id = "PEER2";
    identified.set_option(
        asio::zmq::socket_option::identity(id.c_str(), id.size()));
    identified.connect("inproc://example");

    s_send(identified, "ROUTER socket uses REQ's socket identity");
    s_dump(sink);

    return 0;
}
