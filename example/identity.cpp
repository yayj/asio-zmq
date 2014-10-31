#include <iostream>
#include <iterator>
#include <string>
#include <vector>
#include <boost/asio.hpp>
#include <asio-zmq.hpp>

void s_send(boost::asio::zmq::socket& socket, std::string const& str)
{
    std::vector<boost::asio::zmq::frame> message;
    message.push_back(boost::asio::zmq::frame(str));
    socket.write_message(std::begin(message), std::end(message));
}

void s_dump(boost::asio::zmq::socket& socket)
{
    std::cout << "----------------------------------------\n";
    std::vector<boost::asio::zmq::frame> buffer;
    socket.read_message(std::back_inserter(buffer));
    std::for_each(std::begin(buffer), std::end(buffer),
    [](boost::asio::zmq::frame const& frame) {
        std::cout << std::to_string(frame) << "\n";
    });
}

int main()
{
    boost::asio::io_service ios;
    boost::asio::zmq::context ctx;

    boost::asio::zmq::socket sink(ios, ctx, ZMQ_ROUTER);
    sink.bind("inproc://example");

    boost::asio::zmq::socket anonymous(ios, ctx, ZMQ_REQ);
    anonymous.connect("inproc://example");

    s_send(anonymous, "ROUTER uses a generated UUID");
    s_dump(sink);

    boost::asio::zmq::socket identified(ios, ctx, ZMQ_REQ);
    std::string id = "PEER2";
    identified.set_option(
        boost::asio::zmq::socket_option::identity(id.c_str(), id.size()));
    identified.connect("inproc://example");

    s_send(identified, "ROUTER socket uses REQ's socket identity");
    s_dump(sink);

    return 0;
}
