#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zmq.h>
#include <zmq_utils.h>

int main(int argc, char* argv [])
{
    size_t message_size;
    int roundtrip_count;
    void* ctx;
    void* req;
    void* rep;
    int rc;
    int i;
    zmq_msg_t msg;
    void* watch;
    unsigned long elapsed;
    double latency;

    if (argc != 3) {
        printf("usage: inproc_lat <message-size> <roundtrip-count>\n");
        return 1;
    }

    message_size = atoi(argv [1]);
    roundtrip_count = atoi(argv [2]);

    ctx = zmq_init(1);
    if (!ctx) {
        printf("error in zmq_init: %s\n", zmq_strerror(errno));
        return -1;
    }

    req = zmq_socket(ctx, ZMQ_REQ);
    if (!req) {
        printf("error in zmq_socket: %s\n", zmq_strerror(errno));
        return -1;
    }

    rc = zmq_bind(req, "inproc://lat_test");
    if (rc != 0) {
        printf("error in zmq_bind: %s\n", zmq_strerror(errno));
        return -1;
    }

    rep = zmq_socket(ctx, ZMQ_REP);
    if (!rep) {
        printf("error in zmq_socket: %s\n", zmq_strerror(errno));
        exit(1);
    }

    rc = zmq_connect(rep, "inproc://lat_test");
    if (rc != 0) {
        printf("error in zmq_connect: %s\n", zmq_strerror(errno));
        exit(1);
    }

    zmq_pollitem_t items[] = {
        {req, 0, ZMQ_POLLOUT, 0},
        {rep, 0, ZMQ_POLLIN, 0}
    };

    printf("message size: %d [B]\n", (int) message_size);
    printf("roundtrip count: %d\n", (int) roundtrip_count);

    if (zmq_msg_init_size(&msg, message_size) != 0) {
        printf("error in zmq_msg_init: %s\n", zmq_strerror(errno));
        exit(1);
    }

    watch = zmq_stopwatch_start();

    i = 0;
    while (true) {
        zmq_poll(items, 2, -1);
        if ((items[0].revents & ZMQ_POLLIN) == ZMQ_POLLIN) {
            if (zmq_msg_recv(&msg, req, 0) < 0) {
                printf("error in zmq_msg_recv: %s\n", zmq_strerror(errno));
                exit(1);
            }
            items[0].events = ZMQ_POLLOUT;
        }
        if ((items[0].revents & ZMQ_POLLOUT) == ZMQ_POLLOUT) {
            if (zmq_msg_send(&msg, req, 0) < 0) {
                printf("error in zmq_msg_send: %s\n", zmq_strerror(errno));
                exit(1);
            }
            items[0].events = ZMQ_POLLIN;
        }
        if ((items[1].revents & ZMQ_POLLIN) == ZMQ_POLLIN) {
            if (zmq_msg_recv(&msg, rep, 0) < 0) {
                printf("error in zmq_msg_recv: %s\n", zmq_strerror(errno));
                exit(1);
            }
            items[1].events = ZMQ_POLLOUT;
        }
        if ((items[1].revents & ZMQ_POLLOUT) == ZMQ_POLLOUT) {
            if (++i == roundtrip_count)
                break;
            if (zmq_msg_send(&msg, rep, 0) < ZMQ_DONTWAIT) {
                printf("error in zmq_msg_send: %s\n", zmq_strerror(errno));
                exit(1);
            }
            items[1].events = ZMQ_POLLIN;
        }
    }

    elapsed = zmq_stopwatch_stop(watch);

    zmq_msg_close(&msg);

    latency = (double) elapsed / (roundtrip_count * 2);

    printf("average latency: %.3f [us]\n", (double) latency);

    rc = zmq_close(req);
    if (rc != 0) {
        printf("error in zmq_close: %s\n", zmq_strerror(errno));
        return -1;
    }

    rc = zmq_close(rep);
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

