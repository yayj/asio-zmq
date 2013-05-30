#include <stdio.h>
#include <stdlib.h>
#include <zmq.h>
#include <zmq_utils.h>

int main (int argc, char *argv [])
{
    const char *bind_to;
    int roundtrip_count;
    size_t message_size;
    void *ctx;
    void *s;
    int rc;
    zmq_msg_t msg;

    if (argc != 4) {
        printf ("usage: local_lat <bind-to> <message-size> "
            "<roundtrip-count>\n");
        return 1;
    }
    bind_to = argv [1];
    message_size = atoi (argv [2]);
    roundtrip_count = atoi (argv [3]);

    ctx = zmq_init (1);
    if (!ctx) {
        printf ("error in zmq_init: %s\n", zmq_strerror (errno));
        return -1;
    }

    s = zmq_socket (ctx, ZMQ_REP);
    if (!s) {
        printf ("error in zmq_socket: %s\n", zmq_strerror (errno));
        return -1;
    }

    rc = zmq_bind (s, bind_to);
    if (rc != 0) {
        printf ("error in zmq_bind: %s\n", zmq_strerror (errno));
        return -1;
    }

    rc = zmq_msg_init (&msg);
    if (rc != 0) {
        printf ("error in zmq_msg_init: %s\n", zmq_strerror (errno));
        return -1;
    }

    zmq_pollitem_t items[] = { {s, 0, ZMQ_POLLIN, 0} };

    while (true) {
        zmq_poll(items, 1, -1);
        if ((items[0].revents & ZMQ_POLLIN) == ZMQ_POLLIN) {
            --roundtrip_count;
            if (zmq_msg_recv(&msg, s, 0) < 0) {
                printf("error in zmq_msg_recv: %s\n", zmq_strerror(errno));
                return -1;
            }
            items[0].events = ZMQ_POLLOUT;
        }
        if ((items[0].revents & ZMQ_POLLOUT) == ZMQ_POLLOUT) {
            if (zmq_msg_send(&msg, s, 0) < 0) {
                printf("error in zmq_msg_send: %s\n", zmq_strerror(errno));
                return -1;
            }
            items[0].events = ZMQ_POLLIN;
            if (roundtrip_count == 0)
                break;
        }
    }

    rc = zmq_msg_close (&msg);
    if (rc != 0) {
        printf ("error in zmq_msg_close: %s\n", zmq_strerror (errno));
        return -1;
    }

    rc = zmq_close (s);
    if (rc != 0) {
        printf ("error in zmq_close: %s\n", zmq_strerror (errno));
        return -1;
    }

    rc = zmq_term (ctx);
    if (rc != 0) {
        printf ("error in zmq_term: %s\n", zmq_strerror (errno));
        return -1;
    }

    return 0;
}
