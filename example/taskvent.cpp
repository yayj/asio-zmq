#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <vector>
#include <asio.hpp>
#include <asio-zmq.hpp>

int main(int argc, char* argv[])
{
    asio::io_service ios;
    asio::zmq::context ctx;

    //  Socket to send messages on
    asio::zmq::socket sender(ios, ctx, ZMQ_PUSH);
    sender.set_option(asio::zmq::socket_option::linger(-1));
    sender.bind("tcp://*:5557");

    std::cout << "Press Enter when the workers are ready: \n";
    std::getchar();
    std::cout << "Sending tasks to workers…\n";

    //  The first message is "0" and signals start of batch
    asio::zmq::socket sink(ios, ctx, ZMQ_PUSH);
    sink.set_option(asio::zmq::socket_option::linger(-1));
    sink.connect("tcp://localhost:5558");

    std::vector<asio::zmq::frame> buffer(1);
    sink.write_message(std::begin(buffer), std::end(buffer));

    //  Initialize random number generator
    std::srand(std::chrono::duration_cast<std::chrono::seconds>(
                   std::chrono::system_clock::now()
                   .time_since_epoch()).count());

    //  Send 100 tasks
    int task_nbr;
    int total_msec = 0;     //  Total expected cost in msecs
    for (task_nbr = 0; task_nbr < 100; task_nbr++) {
        int workload = std::rand() % 100 + 1;
        //  Random workload from 1 to 100msecs
        total_msec += workload;

        buffer.clear();
        buffer.push_back(asio::zmq::frame(std::to_string(workload)));
        sender.write_message(std::begin(buffer), std::end(buffer));
    }
    std::cout << "Total expected cost: "
              << total_msec << " msec\n";

    return 0;
}
