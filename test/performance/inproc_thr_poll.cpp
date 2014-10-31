#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zmq.h>
#include <zmq_utils.h>

int main(int argc, char* argv[])
{
    int message_count;
    size_t message_size;
    void* ctx;
    void* pusher;
    void* puller;
    int rc;
    int i;
    zmq_msg_t msg;
    void* watch;
    unsigned long elapsed;
    unsigned long throughput;
    double megabits;

    if (argc != 3) {
        printf("usage: thread_thr <message-size> <message-count>\n");
        return 1;
    }

    message_size = atoi(argv[1]);
    message_count = atoi(argv[2]);

    ctx = zmq_init(1);
    if (!ctx) {
        printf("error in zmq_init: %s\n", zmq_strerror(errno));
        return -1;
    }

    puller = zmq_socket(ctx, ZMQ_PULL);
    if (!puller) {
        printf("error in zmq_socket: %s\n", zmq_strerror(errno));
        return -1;
    }

    rc = zmq_bind(puller, "inproc://thr_test");
    if (rc != 0) {
        printf("error in zmq_bind: %s\n", zmq_strerror(errno));
        return -1;
    }

    pusher = zmq_socket(ctx, ZMQ_PUSH);
    if (!pusher) {
        printf("error in zmq_socket: %s\n", zmq_strerror(errno));
        exit(1);
    }

    rc = zmq_connect(pusher, "inproc://thr_test");
    if (rc != 0) {
        printf("error in zmq_connect: %s\n", zmq_strerror(errno));
        exit(1);
    }

    rc = zmq_msg_init(&msg);
    if (rc != 0) {
        printf("error in zmq_msg_init: %s\n", zmq_strerror(errno));
        return -1;
    }

    printf("message size: %d [B]\n", (int)message_size);
    printf("message count: %d\n", (int)message_count);

    zmq_pollitem_t items[] = {{pusher, 0, ZMQ_POLLOUT, 0}, {puller, 0, ZMQ_POLLIN, 0}};

    zmq_msg_t buff;
    watch = zmq_stopwatch_start();

    i = 0;
    while (true) {
        zmq_poll(items, 2, -1);
        if ((items[0].revents & ZMQ_POLLOUT) == ZMQ_POLLOUT) {
            if (++i == message_count) break;
            if (zmq_msg_init_size(&buff, message_size) < 0) {
                printf("error in zmq_msg_init_size: %s\n", zmq_strerror(errno));
                exit(1);
            }
            if (zmq_msg_send(&buff, pusher, 0) < 0) {
                printf("error in zmq_msg_send: %s\n", zmq_strerror(errno));
                exit(1);
            }
            zmq_msg_close(&buff);
        }
        if ((items[1].revents & ZMQ_POLLIN) == ZMQ_POLLIN) {
            if (zmq_msg_recv(&msg, puller, 0) < 0) {
                printf("error in zmq_msg_recv: %s\n", zmq_strerror(errno));
                exit(1);
            }
        }
    }

    elapsed = zmq_stopwatch_stop(watch);
    if (elapsed == 0) elapsed = 1;

    rc = zmq_msg_close(&msg);
    if (rc != 0) {
        printf("error in zmq_msg_close: %s\n", zmq_strerror(errno));
        return -1;
    }

    rc = zmq_close(pusher);
    if (rc != 0) {
        printf("error in zmq_close: %s\n", zmq_strerror(errno));
        return -1;
    }

    rc = zmq_close(puller);
    if (rc != 0) {
        printf("error in zmq_close: %s\n", zmq_strerror(errno));
        return -1;
    }

    rc = zmq_term(ctx);
    if (rc != 0) {
        printf("error in zmq_term: %s\n", zmq_strerror(errno));
        return -1;
    }

    throughput = (unsigned long)((double)message_count / (double)elapsed * 1000000);
    megabits = (double)(throughput * message_size * 8) / 1000000;

    printf("mean throughput: %d [msg/s]\n", (int)throughput);
    printf("mean throughput: %.3f [Mb/s]\n", (double)megabits);

    return 0;
}
