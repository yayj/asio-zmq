#include <chrono>
#include <iostream>
#include <iterator>
#include <vector>
#include <time.h>
#include <sys/time.h>
#include <asio.hpp>
#include <asio-zmq.hpp>

int main(int argc, char* argv[])
{
    //  Prepare our context and socket
    asio::io_service ios;
    asio::zmq::context ctx;
    asio::zmq::socket receiver(ios, ctx, ZMQ_PULL);
    receiver.bind("tcp://*:5558");

    //  Wait for start of batch
    std::vector<asio::zmq::frame> buffer;
    receiver.read_message(std::back_inserter(buffer));

    //  Start our clock now
    auto tstart = std::chrono::system_clock::now();

    //  Process 100 confirmations
    for (int task_nbr = 0; task_nbr < 100; task_nbr++) {
        buffer.clear();
        receiver.read_message(std::back_inserter(buffer));

        if ((task_nbr / 10) * 10 == task_nbr)
            std::cout << ":" << std::flush;
        else
            std::cout << "." << std::flush;
    }
    //  Calculate and report duration of batch
    auto duration = std::chrono::system_clock::now() - tstart;
    std::cout << "\nTotal elapsed time: "
              << std::chrono::duration_cast<
              std::chrono::milliseconds>(duration).count()
              << " msec\n" << std::endl;
    return 0;
}
