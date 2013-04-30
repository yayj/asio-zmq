#include <functional>
#include <iterator>
#include <memory>
#include <vector>
#include <asio.hpp>
#include <asio-zmq.hpp>

class rrbroker {
private:
    typedef std::vector<asio::zmq::frame> message_t;
    typedef std::shared_ptr<message_t> message_ptr;
    typedef std::shared_ptr<asio::zmq::socket> socket_ptr;

    asio::zmq::socket frontend_;
    asio::zmq::socket backend_;
    message_t buffer_;

public:
    rrbroker(asio::io_service& ios, asio::zmq::context& ctx)
        : frontend_(ios, ctx, ZMQ_ROUTER),
          backend_(ios, ctx, ZMQ_DEALER), buffer_() {
        frontend_.bind("tcp://*:5559");
        backend_.bind("tcp://*:5560");

        frontend_.async_read_message(
            std::back_inserter(buffer_),
            std::bind(&rrbroker::handle_recv, this, std::placeholders::_1,
                      std::ref(frontend_), std::ref(backend_)));

        backend_.async_read_message(
            std::back_inserter(buffer_),
            std::bind(&rrbroker::handle_recv, this, std::placeholders::_1,
                      std::ref(backend_), std::ref(frontend_)));
    }

    void handle_recv(asio::error_code const& ec,
                     asio::zmq::socket& receiver,
                     asio::zmq::socket& forwarder) {
        message_ptr tmp {new message_t};
        std::swap(*tmp, buffer_);
        forwarder.async_write_message(
            tmp->begin(), tmp->end(),
            std::bind(&rrbroker::null_handler, this,
                      std::placeholders::_1, tmp));
        receiver.async_read_message(
            std::back_inserter(buffer_),
            std::bind(&rrbroker::handle_recv, this, std::placeholders::_1,
                      std::ref(receiver), std::ref(forwarder)));
    }

    void null_handler(asio::error_code const& ec, message_ptr dummy) {}
};

int main(int argc, char* argv[])
{
    //  Prepare our context and sockets
    asio::io_service ios;
    asio::zmq::context ctx;
    rrbroker broker(ios, ctx);

    ios.run();

    return 0;
}
