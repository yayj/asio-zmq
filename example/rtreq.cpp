#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <asio.hpp>
#include <asio-zmq.hpp>

static std::mutex g_mutex;
static int const NBR_WORKERS = 10;
static std::string const endpoint = "ipc://routing.ipc";
static std::string const ready_inst = "READY";
static std::string const end_inst = "END";
static std::string const work_inst = "This is the workload";

class req_worker {
private:
    asio::io_service& ios_;
    asio::zmq::socket sock_;
    std::unique_ptr<std::thread> work_thread_;
    std::vector<asio::zmq::frame> message_;
    int total_;
    std::string id_;

    void ping_pong() {
        message_.clear();
        message_.push_back(asio::zmq::frame(ready_inst.size()));
        std::copy(std::begin(ready_inst), std::end(ready_inst),
                  static_cast<char*>(message_[0].data()));
        //  Tell the router we're ready for work
        sock_.write_message(std::begin(message_), std::end(message_));

        message_.clear();
        sock_.async_read_message(
            std::back_inserter(message_),
            std::bind(&req_worker::handle_work, this, std::placeholders::_1));
    }

    void go() {
        std::ostringstream oss;
        oss << std::this_thread::get_id();
        id_ = oss.str();
        sock_.set_option(
            asio::zmq::socket_option::identity(id_.c_str(), id_.size()));
        sock_.connect(endpoint);

        ping_pong();
        ios_.run();
    }

    void handle_work(asio::error_code const& ec) {
        //  Get workload from router, until finished
        if (end_inst ==
                std::string(static_cast<char*>(
                                message_[0].data()), message_[0].size())) {
            std::lock_guard<std::mutex> lock(g_mutex);
            std::cout << id_ << " Processed: " << total_ << " tasks\n";
        } else {
            ++total_;
            //  Do some random work
            std::this_thread::sleep_for(
                std::chrono::milliseconds(std::rand() % 100 + 1));

            ping_pong();
        }
    }

public:
    req_worker(asio::io_service& ios, asio::zmq::context& ctx)
        : ios_(ios), sock_(ios, ctx, ZMQ_REQ), work_thread_(),
          message_(), total_(0), id_()
    {}

    ~req_worker() {
        work_thread_->join();
    }

    void start() {
        work_thread_.reset(new std::thread(
                               std::bind(&req_worker::go, this)));
    }
};

class client {
private:
    typedef std::vector<asio::zmq::frame> message_t;
    typedef std::shared_ptr<message_t> message_ptr;

    asio::zmq::socket sock_;
    message_t buffer_;
    int task_nbr_;

    void handle_req(asio::error_code const& ec) {
        std::string const inst =
            ++task_nbr_ <= NBR_WORKERS * 10 ? work_inst : end_inst;

        message_ptr tmp {new message_t};
        std::swap(*tmp, buffer_);
        tmp->pop_back();
        tmp->push_back(asio::zmq::frame(inst.size()));
        std::copy(std::begin(inst), std::end(inst),
                  static_cast<char*>(tmp->back().data()));
        sock_.async_write_message(
            std::begin(*tmp), std::end(*tmp),
            std::bind(&client::null_handler, this,
                      std::placeholders::_1, tmp));

        if (task_nbr_ < NBR_WORKERS * 11)
            sock_.async_read_message(
                std::back_inserter(buffer_),
                std::bind(&client::handle_req, this,
                          std::placeholders::_1));
    }

    void null_handler(asio::error_code const& ec, message_ptr dummy) {}

public:
    client(asio::io_service& ios, asio::zmq::context& ctx)
        : sock_(ios, ctx, ZMQ_ROUTER), buffer_(), task_nbr_(0)
    {}

    void start() {
        sock_.bind(endpoint);
        sock_.async_read_message(
            std::back_inserter(buffer_),
            std::bind(&client::handle_req, this, std::placeholders::_1));
    }
};

int main()
{
    std::srand(std::time(0));
    asio::io_service ios;
    asio::zmq::context ctx;

    std::vector<std::unique_ptr<req_worker>> workers(NBR_WORKERS);
    for (int worker_nbr = 0; worker_nbr < NBR_WORKERS; worker_nbr++) {
        workers[worker_nbr].reset(new req_worker(ios, ctx));
        workers[worker_nbr]->start();
    }

    client c(ios, ctx);
    c.start();

    ios.run();

    return 0;
}
