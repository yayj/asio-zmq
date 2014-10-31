//
//  Least-recently used (LRU) queue device
//  Clients and workers are shown here in-process
//
//  NOTICE: increase file open limitation
//          if increasing clients or workers.

#include <functional>
#include <iostream>
#include <memory>
#include <queue>
#include <string>
#include <vector>
#include <boost/asio.hpp>
#include <asio-zmq.hpp>

static int const client_count = 10;
static int const worker_count = 3;

static std::string const front_endpoint = "ipc://frontend.ipc";
static std::string const back_endpoint = "ipc://backend.ipc";

//  Basic request-reply client using REQ socket
//
class req_client {
private:
    static int count_;

    boost::asio::io_service& ios_;
    boost::asio::zmq::socket requester_;
    std::vector<boost::asio::zmq::frame> buffer_;

    void handle_write(boost::system::error_code const& ec)
    {
        buffer_.clear();
        requester_.async_read_message(
            std::back_inserter(buffer_),
            std::bind(&req_client::handle_read, this, std::placeholders::_1));
    }

    void handle_read(boost::system::error_code const& ec)
    {
        std::cout << "Client: " << std::to_string(buffer_[0]) << "\n";
        if (--count_ == 0) ios_.stop();
    }

public:
    req_client(boost::asio::io_service& ios, boost::asio::zmq::context& ctx, int id)
        : ios_(ios), requester_(ios, ctx, ZMQ_REQ), buffer_()
    {
        ++count_;
        std::string str_id = std::to_string(id);
        requester_.set_option(
            boost::asio::zmq::socket_option::identity(str_id.c_str(), str_id.size()));
        requester_.connect(front_endpoint);

        //  Send request, get reply
        buffer_.push_back(boost::asio::zmq::frame("HELLO"));
        requester_.async_write_message(
            std::begin(buffer_), std::end(buffer_),
            std::bind(&req_client::handle_write, this, std::placeholders::_1));
    }
};

int req_client::count_ = 0;

//  Worker using REQ socket to do LRU routing
//
class req_worker {
private:
    boost::asio::zmq::socket requester_;
    std::vector<boost::asio::zmq::frame> buffer_;

    void handle_write(boost::system::error_code const& ec)
    {
        //  Read and save all frames until we get an empty frame
        //  In this example there is only 1 but it could be more
        buffer_.clear();
        requester_.async_read_message(
            std::back_inserter(buffer_),
            std::bind(&req_worker::handle_read, this, std::placeholders::_1));
    }

    void handle_read(boost::system::error_code const& ec)
    {
        //  Get request, send reply
        boost::asio::zmq::frame instruction = std::move(buffer_.back());
        buffer_.pop_back();
        buffer_.push_back(boost::asio::zmq::frame("OK"));
        std::cout << "Worker: " << std::to_string(instruction) << "\n";

        requester_.async_write_message(
            std::begin(buffer_), std::end(buffer_),
            std::bind(&req_worker::handle_write, this, std::placeholders::_1));
    }

public:
    req_worker(boost::asio::io_service& ios, boost::asio::zmq::context& ctx, int id)
        : requester_(ios, ctx, ZMQ_REQ), buffer_()
    {
        std::string str_id = std::to_string(id);
        requester_.set_option(
            boost::asio::zmq::socket_option::identity(str_id.c_str(), str_id.size()));
        requester_.connect(back_endpoint);

        //  Tell backend we're ready for work
        buffer_.push_back(boost::asio::zmq::frame("READY"));
        requester_.async_write_message(
            std::begin(buffer_), std::end(buffer_),
            std::bind(&req_worker::handle_write, this, std::placeholders::_1));
    }
};

class lbbroker {
private:
    typedef std::vector<boost::asio::zmq::frame> message_t;
    typedef std::shared_ptr<message_t> message_ptr;

    boost::asio::zmq::socket frontend_;
    boost::asio::zmq::socket backend_;
    //  Logic of LRU loop
    //  - Poll backend always, frontend only if 1+ worker ready
    //  - If worker replies, queue worker as ready and forward reply
    //    to client if necessary
    //  - If client requests, pop next worker and send request to it
    //
    //  A very simple queue structure with known max size
    std::queue<std::string> worker_queue_;

    //  Handle worker activity on backend
    void handle_worker_ready(boost::system::error_code const& ec, message_ptr buff)
    {
        //  Queue worker address for LRU routing
        worker_queue_.push(std::to_string(buff->front()));

        //  If client reply, send rest back to frontend
        if (buff->size() == 5)
            frontend_.async_write_message(
                std::begin(*buff) + 2, std::end(*buff),
                std::bind(&lbbroker::null_handler, this, std::placeholders::_1, buff));

        message_ptr frontend_buff{new message_t};
        frontend_.async_read_message(std::back_inserter(*frontend_buff),
                                     std::bind(&lbbroker::handle_client_requested, this,
                                               std::placeholders::_1, frontend_buff));

        message_ptr backend_buff{new message_t};
        backend_.async_read_message(
            std::back_inserter(*backend_buff),
            std::bind(&lbbroker::handle_worker_ready, this, std::placeholders::_1, backend_buff));
    }

    //  Now get next client request, route to LRU worker
    //  Client request is [address][empty][request]
    void handle_client_requested(boost::system::error_code const& ec, message_ptr buff)
    {
        buff->emplace(std::begin(*buff), boost::asio::zmq::frame(""));
        buff->emplace(std::begin(*buff), boost::asio::zmq::frame(worker_queue_.front()));
        worker_queue_.pop();
        backend_.async_write_message(
            std::begin(*buff), std::end(*buff),
            std::bind(&lbbroker::null_handler, this, std::placeholders::_1, buff));

        message_ptr backend_buff{new message_t};
        backend_.async_read_message(
            std::back_inserter(*backend_buff),
            std::bind(&lbbroker::handle_worker_ready, this, std::placeholders::_1, backend_buff));
    }

    void null_handler(boost::system::error_code const& ec, message_ptr buff) {}

public:
    lbbroker(boost::asio::io_service& ios, boost::asio::zmq::context& ctx)
        : frontend_(ios, ctx, ZMQ_ROUTER), backend_(ios, ctx, ZMQ_ROUTER), worker_queue_()
    {
        frontend_.bind(front_endpoint);
        backend_.bind(back_endpoint);
        message_ptr buff{new message_t};
        backend_.async_read_message(
            std::back_inserter(*buff),
            std::bind(&lbbroker::handle_worker_ready, this, std::placeholders::_1, buff));
    }
};

int main(int argc, char* argv[])
{
    boost::asio::io_service ios;
    boost::asio::zmq::context ctx;

    std::vector<std::unique_ptr<req_client>> clients(client_count);
    std::vector<std::unique_ptr<req_worker>> workers(worker_count);

    lbbroker broker(ios, ctx);

    for (int i = 0; i < client_count; ++i) clients[i].reset(new req_client(ios, ctx, i));

    for (int i = 0; i < worker_count; ++i) workers[i].reset(new req_worker(ios, ctx, i));

    ios.run();

    return 0;
}
