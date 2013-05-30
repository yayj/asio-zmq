#include <stdio.h>
#include <stdlib.h>
#include <zmq.h>
#include <zmq_utils.h>

int main(int argc, char* argv [])
{
    const char* bind_to;
    int message_count;
    size_t message_size;
    void* ctx;
    void* s;
    int rc;
    int i;
    zmq_msg_t msg;
    void* watch;
    unsigned long elapsed;
    unsigned long throughput;
    double megabits;

    if (argc != 4) {
        printf("usage: local_thr <bind-to> <message-size> <message-count>\n");
        return 1;
    }
    bind_to = argv [1];
    message_size = atoi(argv [2]);
    message_count = atoi(argv [3]);

    ctx = zmq_init(1);
    if (!ctx) {
        printf("error in zmq_init: %s\n", zmq_strerror(errno));
        return -1;
    }

    s = zmq_socket(ctx, ZMQ_PULL);
    if (!s) {
        printf("error in zmq_socket: %s\n", zmq_strerror(errno));
        return -1;
    }

    rc = zmq_bind(s, bind_to);
    if (rc != 0) {
        printf("error in zmq_bind: %s\n", zmq_strerror(errno));
        return -1;
    }

    rc = zmq_msg_init(&msg);
    if (rc != 0) {
        printf("error in zmq_msg_init: %s\n", zmq_strerror(errno));
        return -1;
    }

    rc = zmq_recvmsg(s, &msg, 0);
    if (rc < 0) {
        printf("error in zmq_recvmsg: %s\n", zmq_strerror(errno));
        return -1;
    }
    if (zmq_msg_size(&msg) != message_size) {
        printf("message of incorrect size received\n");
        return -1;
    }

    zmq_pollitem_t items[] = { {s, 0, ZMQ_POLLIN, 0} };

    watch = zmq_stopwatch_start();

    for (i = 0; i != message_count - 1; ++i) {
        zmq_poll(items, 1, -1);
        if ((items[0].revents & ZMQ_POLLIN) == ZMQ_POLLIN) {
            if (zmq_msg_recv(&msg, s, 0) < 0) {
                printf("error in zmq_msg_recv: %s\n", zmq_strerror(errno));
                return -1;
            }
            if (zmq_msg_size(&msg) != message_size) {
                printf("message of incorrect size received\n");
                return -1;
            }
        }
    }

    elapsed = zmq_stopwatch_stop(watch);
    if (elapsed == 0)
        elapsed = 1;

    rc = zmq_msg_close(&msg);
    if (rc != 0) {
        printf("error in zmq_msg_close: %s\n", zmq_strerror(errno));
        return -1;
    }

    throughput = (unsigned long)
                 ((double) message_count / (double) elapsed * 1000000);
    megabits = (double)(throughput * message_size * 8) / 1000000;

    printf("message size: %d [B]\n", (int) message_size);
    printf("message count: %d\n", (int) message_count);
    printf("mean throughput: %d [msg/s]\n", (int) throughput);
    printf("mean throughput: %.3f [Mb/s]\n", (double) megabits);

    rc = zmq_close(s);
    if (rc != 0) {
        printf("error in zmq_close: %s\n", zmq_strerror(errno));
        return -1;
    }

    rc = zmq_term(ctx);
    if (rc != 0) {
        printf("error in zmq_term: %s\n", zmq_strerror(errno));
        return -1;
    }

    return 0;
}
