#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zmq.h>
#include <zmq_utils.h>

int main (int argc, char *argv [])
{
    const char *connect_to;
    int roundtrip_count;
    size_t message_size;
    void *ctx;
    void *s;
    int rc;
    int i;
    zmq_msg_t msg;
    void *watch;
    unsigned long elapsed;
    double latency;

    if (argc != 4) {
        printf ("usage: remote_lat <connect-to> <message-size> "
            "<roundtrip-count>\n");
        return 1;
    }
    connect_to = argv [1];
    message_size = atoi (argv [2]);
    roundtrip_count = atoi (argv [3]);

    ctx = zmq_init (1);
    if (!ctx) {
        printf ("error in zmq_init: %s\n", zmq_strerror (errno));
        return -1;
    }

    s = zmq_socket (ctx, ZMQ_REQ);
    if (!s) {
        printf ("error in zmq_socket: %s\n", zmq_strerror (errno));
        return -1;
    }

    rc = zmq_connect (s, connect_to);
    if (rc != 0) {
        printf ("error in zmq_connect: %s\n", zmq_strerror (errno));
        return -1;
    }

    rc = zmq_msg_init_size (&msg, message_size);
    if (rc != 0) {
        printf ("error in zmq_msg_init_size: %s\n", zmq_strerror (errno));
        return -1;
    }
    memset (zmq_msg_data (&msg), 0, message_size);

    zmq_pollitem_t items[] = { {s, 0, ZMQ_POLLOUT, 0} };

    watch = zmq_stopwatch_start ();

    i = 0;
    while (true) {
    	zmq_poll(items, 1, -1);
    	if ((items[0].revents & ZMQ_POLLIN) == ZMQ_POLLIN) {
    		if (zmq_msg_recv(&msg, s, 0) < 0) {
    			printf("error in zmq_msg_recv: %s\n", zmq_strerror(errno));
    			return -1;
    		}
    		items[0].events = ZMQ_POLLOUT;
    	}
    	if ((items[0].revents & ZMQ_POLLOUT) == ZMQ_POLLOUT) {
    		if (i++ == roundtrip_count)
    			break;
    		if (zmq_msg_send(&msg, s, 0) < 0) {
    			printf("error in zmq_msg_send: %s\n", zmq_strerror(errno));
    			return -1;
    		}
    		items[0].events = ZMQ_POLLIN;
    	}
    }

    elapsed = zmq_stopwatch_stop (watch);

    rc = zmq_msg_close (&msg);
    if (rc != 0) {
        printf ("error in zmq_msg_close: %s\n", zmq_strerror (errno));
        return -1;
    }

    latency = (double) elapsed / (roundtrip_count * 2);

    printf ("message size: %d [B]\n", (int) message_size);
    printf ("roundtrip count: %d\n", (int) roundtrip_count);
    printf ("average latency: %.3f [us]\n", (double) latency);

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
