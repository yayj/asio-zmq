#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zmq.h>
#include <zmq_utils.h>

int main(int argc, char* argv [])
{
    const char* connect_to;
    int message_count;
    int message_size;
    void* ctx;
    void* s;
    int rc;
    int i;
    zmq_msg_t msg;

    if (argc != 4) {
        printf("usage: remote_thr <connect-to> <message-size> "
               "<message-count>\n");
        return 1;
    }
    connect_to = argv [1];
    message_size = atoi(argv [2]);
    message_count = atoi(argv [3]);

    ctx = zmq_init(1);
    if (!ctx) {
        printf("error in zmq_init: %s\n", zmq_strerror(errno));
        return -1;
    }

    s = zmq_socket(ctx, ZMQ_PUSH);
    if (!s) {
        printf("error in zmq_socket: %s\n", zmq_strerror(errno));
        return -1;
    }

    //  Add your socket options here.
    //  For example ZMQ_RATE, ZMQ_RECOVERY_IVL and ZMQ_MCAST_LOOP for PGM.

    rc = zmq_connect(s, connect_to);
    if (rc != 0) {
        printf("error in zmq_connect: %s\n", zmq_strerror(errno));
        return -1;
    }

    zmq_pollitem_t items[] = { {s, 0, ZMQ_POLLOUT, 0} };

    for (i = 0; i != message_count; i++) {
        zmq_poll(items, 1, -1);
        if ((items[0].revents & ZMQ_POLLOUT) == ZMQ_POLLOUT) {
            rc = zmq_msg_init_size(&msg, message_size);
            if (rc != 0) {
                printf("error in zmq_msg_init_size: %s\n", zmq_strerror(errno));
                return -1;
            }
            rc = zmq_sendmsg(s, &msg, 0);
            if (rc < 0) {
                printf("error in zmq_sendmsg: %s\n", zmq_strerror(errno));
                return -1;
            }
            rc = zmq_msg_close(&msg);
            if (rc != 0) {
                printf("error in zmq_msg_close: %s\n", zmq_strerror(errno));
                return -1;
            }
        }
    }

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
