#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "list.h"
#include "uloop.h"
#include "usock.h"
#include "ustream.h"

static struct uloop_fd server;
static const char *host = "127.0.0.1";
static const char *port = "10000";
struct client *next_client = NULL;

struct client {
    struct list_head list;
    struct ustream_fd s;
    char addr[INET6_ADDRSTRLEN + 1];
    int ctr;
};

char *sockaddr_to_string(struct sockaddr_in *addr, char *out, size_t len) {
    int ret = 0;
    char str[INET6_ADDRSTRLEN + 1] = {0};

    if (!addr || !out || !len) {
        return NULL;
    }

    ret = snprintf(str, sizeof(str), "%s:%d", inet_ntop(addr->sin_family, &(addr->sin_addr), str, len),
                   ntohs(addr->sin_port));
    if (ret < 0) {
        return NULL;
    }

    strncpy(out, str, len);
    return out;
}

static void client_notify_read(struct ustream *s, int bytes) {
    struct client *cl = NULL;
    struct ustream_buf *buf = NULL;
    char *newline = NULL;
    char *str = NULL;

    if (!s) {
        return;
    }
    buf = s->r.head;
    cl = container_of(s, struct client, s.stream);

    do {
        str = ustream_get_read_buf(s, NULL);
        if (!str) {
            break;
        }

        newline = strchr(buf->data, '\n');
        if (!newline) {
            break;
        }

        *newline = 0;
        fprintf(stderr, "Block read: %s, bytes: %d from %s\n", str, newline + 1 - str, cl->addr);
        ustream_printf(s, "%s\n", str);
        ustream_consume(s, newline + 1 - str);
        cl->ctr += newline + 1 - str;
    } while (1);

    if (s->w.data_bytes > 256 && !ustream_read_blocked(s)) {
        fprintf(stderr, "Block read, bytes: %d\n", s->w.data_bytes);
        ustream_set_read_blocked(s, true);
    }
}

static void client_close(struct ustream *s) {
    struct client *cl = NULL;
    if (!s) {
        return;
    }
    cl = container_of(s, struct client, s.stream);
    fprintf(stderr, "Connection %s closed\n", cl->addr);

    list_del(&(cl->list));

    ustream_free(s);
    close(cl->s.fd.fd);
    free(cl);
}

static void client_notify_write(struct ustream *s, int bytes) {
    struct client *cl = NULL;
    if (!s) {
        return;
    }
    cl = container_of(s, struct client, s.stream);
    fprintf(stderr, "Wrote %d bytes to %s, pending: %d\n", bytes, cl->addr, s->w.data_bytes);

    if (s->w.data_bytes < 128 && ustream_read_blocked(s)) {
        fprintf(stderr, "Unblock read\n");
        ustream_set_read_blocked(s, false);
    }
}

static void client_notify_state(struct ustream *s) {
    struct client *cl = NULL;
    if (!s) {
        return;
    }
    if (!s->eof) {
        return;
    }
    cl = container_of(s, struct client, s.stream);
    fprintf(stderr, "Connection %s EOF!, pending: %d, total: %d\n", cl->addr, s->w.data_bytes, cl->ctr);
    if (!s->w.data_bytes) {
        return client_close(s);
    }
}

static void server_cb(struct uloop_fd *fd, unsigned int events) {
    struct client *cl = NULL;
    struct sockaddr_in sin = {0};
    unsigned int sl = sizeof(sin);
    int sfd = 0;

    if (!next_client) {
        next_client = calloc(1, sizeof(*next_client));
        INIT_LIST_HEAD(&(next_client->list));
    }

    cl = calloc(1, sizeof(struct client));
    sfd = accept(server.fd, (struct sockaddr *)&sin, &sl);
    if (sfd < 0) {
        fprintf(stderr, "Accept failed\n");
        return;
    }

    cl->s.stream.string_data = true;
    cl->s.stream.notify_read = client_notify_read;
    cl->s.stream.notify_state = client_notify_state;
    cl->s.stream.notify_write = client_notify_write;
    ustream_fd_init(&cl->s, sfd);
    list_add_tail(&(cl->list), &(next_client->list));
    sockaddr_to_string(&sin, cl->addr, sizeof(cl->addr));
    fprintf(stderr, "New connection from %s\n", cl->addr);
}

static int run_server(void) {
    server.cb = server_cb;
    server.fd = usock(USOCK_TCP | USOCK_SERVER | USOCK_IPV4ONLY | USOCK_NUMERIC, host, port);
    if (server.fd < 0) {
        perror("usock");
        return 1;
    }

    uloop_init();
    uloop_fd_add(&server, ULOOP_READ);
    uloop_run();

    close(server.fd);
    uloop_done();

    return 0;
}

static int usage(const char *name) {
    fprintf(stderr, "Usage: %s [-p <port>]\n", name);
    return 1;
}

int main(int argc, char **argv) {
    int ch;

    while ((ch = getopt(argc, argv, "p:")) != -1) {
        switch (ch) {
            case 'p':
                port = optarg;
                break;
            default:
                return usage(argv[0]);
        }
    }

    return run_server();
}
