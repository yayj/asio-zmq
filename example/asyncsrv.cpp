#include <cstdlib>
#include <chrono>
#include <functional>
#include <iostream>
#include <iterator>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <boost/asio.hpp>
#include <boost/asio/system_timer.hpp>
#include <asio-zmq.hpp>

typedef std::vector<boost::asio::zmq::frame> message_t;
typedef std::shared_ptr<message_t> message_ptr;

static std::string const back_endpoint = "inproc://backend_";
static std::string const front_endpoint = "tcp://127.0.0.1:5570";

constexpr static int worker_amount = 5;
constexpr static int client_amount = 3;
constexpr static int thread_amount = 2;

constexpr static std::chrono::seconds interval{2};

static std::mutex g_mutex;

static void null_handler(boost::system::error_code const& ec, message_ptr p) {}

static void print(std::string const& msg)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    std::cout << msg << "\n";
}

class client_task {
private:
    boost::asio::zmq::socket dealer_;
    boost::asio::system_timer timer_;
    boost::asio::io_service::strand strand_;
    std::string id_;
    int req_no_;
    std::chrono::time_point<std::chrono::system_clock> ts_;

    void on_timeout()
    {
        if (std::chrono::system_clock::now() - ts_ >= interval) {
            print("Client " + id_ + " Req#" + std::to_string(++req_no_) + " sent");
            message_ptr p{new message_t};
            p->push_back(boost::asio::zmq::frame("request " + std::to_string(req_no_)));
            dealer_.async_write_message(p->begin(), p->end(),
                                        std::bind(null_handler, std::placeholders::_1, p));

            ts_ = std::chrono::system_clock::now();
            timer_.expires_from_now(interval);
        }
        timer_.async_wait(strand_.wrap(std::bind(&client_task::on_timeout, this)));
    }

    void on_response(boost::system::error_code const& ec, message_ptr buff)
    {
        print("Client " + id_ + " received " + std::to_string(buff->front()));
        ts_ = std::chrono::system_clock::now();
        timer_.expires_from_now(interval);

        message_ptr p{new message_t};
        dealer_.async_read_message(
            std::back_inserter(*p),
            strand_.wrap(std::bind(&client_task::on_response, this, std::placeholders::_1, p)));
    }

public:
    client_task(boost::asio::io_service& ios, boost::asio::zmq::context& ctx, int id)
        : dealer_(ios, ctx, ZMQ_DEALER),
          timer_(ios),
          strand_(ios),
          id_(std::to_string(id)),
          req_no_(0),
          ts_()
    {
        dealer_.set_option(boost::asio::zmq::socket_option::identity(id_.c_str(), id_.size()));
        dealer_.connect(front_endpoint);
    }

    void start()
    {
        print("Client " + id_ + " started.");
        ts_ = std::chrono::system_clock::now();
        timer_.expires_from_now(interval);
        timer_.async_wait(strand_.wrap(std::bind(&client_task::on_timeout, this)));

        message_ptr buff{new message_t};
        dealer_.async_read_message(
            std::back_inserter(*buff),
            strand_.wrap(std::bind(&client_task::on_response, this, std::placeholders::_1, buff)));
    }
};

class async_broker {
private:
    boost::asio::zmq::socket frontend_;
    boost::asio::zmq::socket backend_;

    static void forward(boost::system::error_code const& ec, message_ptr buff,
                        boost::asio::zmq::socket& recver, boost::asio::zmq::socket& sender,
                        std::string const& header)
    {
        print(header + std::to_string(buff->back()) + " id " + std::to_string(buff->front()));
        sender.async_write_message(buff->begin(), buff->end(),
                                   std::bind(null_handler, std::placeholders::_1, buff));

        message_ptr tmp{new message_t};
        recver.async_read_message(std::back_inserter(*tmp),
                                  std::bind(&async_broker::forward, std::placeholders::_1, tmp,
                                            std::ref(recver), std::ref(sender), header));
    }

public:
    async_broker(boost::asio::io_service& ios, boost::asio::zmq::context& ctx)
        : frontend_(ios, ctx, ZMQ_ROUTER), backend_(ios, ctx, ZMQ_ROUTER)
    {
        frontend_.bind(front_endpoint);
        backend_.bind(back_endpoint);
    }

    void start()
    {
        message_ptr backend_msg{new message_t};
        backend_.async_read_message(std::back_inserter(*backend_msg),
                                    std::bind(&async_broker::forward, std::placeholders::_1,
                                              backend_msg, std::ref(backend_), std::ref(frontend_),
                                              "Sending to frontend "));

        message_ptr frontend_msg{new message_t};
        frontend_.async_read_message(std::back_inserter(*frontend_msg),
                                     std::bind(&async_broker::forward, std::placeholders::_1,
                                               frontend_msg, std::ref(frontend_),
                                               std::ref(backend_), "Server received "));
    }
};

class async_worker {
private:
    boost::asio::zmq::socket dealer_;
    std::string id_;

    void handle_read(boost::system::error_code const& ec, message_ptr buff)
    {
        int reply_count = std::rand() % 5;
        for (int i = 0; i < reply_count; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000 / (1 + std::rand() % 8)));
            message_ptr msg{new message_t};
            std::copy(buff->begin(), buff->end(), std::back_inserter(*msg));
            dealer_.async_write_message(msg->begin(), msg->end(),
                                        std::bind(null_handler, std::placeholders::_1, msg));
        }

        message_ptr another_msg{new message_t};
        dealer_.async_read_message(
            std::back_inserter(*another_msg),
            std::bind(&async_worker::handle_read, this, std::placeholders::_1, another_msg));
    }

public:
    async_worker(boost::asio::io_service& ios, boost::asio::zmq::context& ctx, int id)
        : dealer_(ios, ctx, ZMQ_DEALER), id_(std::to_string(id))
    {
        dealer_.set_option(boost::asio::zmq::socket_option::identity(id_.c_str(), id_.size()));
        dealer_.connect(back_endpoint);
    }

    void start()
    {
        print("Server " + id_ + " started.");
        message_ptr buff{new message_t};
        dealer_.async_read_message(
            std::back_inserter(*buff),
            std::bind(&async_worker::handle_read, this, std::placeholders::_1, buff));
    }
};

class thread_pool {
private:
    std::vector<std::unique_ptr<std::thread>> pool_;
    boost::asio::io_service& ios_;

public:
    thread_pool(boost::asio::io_service& ios, int amount) : pool_(amount - 1), ios_(ios) {}

    ~thread_pool()
    {
        for (auto& t : pool_) t->join();
    }

    void run()
    {
        for (auto& t : pool_)
            t.reset(
                new std::thread(std::bind(static_cast<std::size_t (boost::asio::io_service::*)()>(
                                              &boost::asio::io_service::run),
                                          &ios_)));
        ios_.run();
    }
};

int main()
{
    std::srand(std::time(0));

    boost::asio::io_service ios;
    boost::asio::zmq::context ctx;

    async_broker broker(ios, ctx);
    broker.start();

    std::vector<std::unique_ptr<client_task>> clients(client_amount);
    for (int i = 0; i < clients.size(); ++i) {
        clients[i].reset(new client_task(ios, ctx, i));
        clients[i]->start();
    }

    std::vector<std::unique_ptr<async_worker>> workers(worker_amount);
    for (int i = 0; i < workers.size(); ++i) {
        workers[i].reset(new async_worker(ios, ctx, i));
        workers[i]->start();
    }

    // ios.run();
    thread_pool pool(ios, thread_amount);
    pool.run();
}
