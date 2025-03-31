#include <sys/socket.h>
#include <netinet/in.h>

#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ustream.h"
#include "uloop.h"
#include "usock.h"

static const char *host = "127.0.0.1";
static const char *port = "10000";

static struct uloop_fd client;
static struct ustream_fd stream, s_input;

static void client_teardown(void)
{
    if (s_input.fd.registered)
    {
        ustream_free(&s_input.stream);
    }

    ustream_free(&stream.stream);
    close(stream.fd.fd);
    uloop_end();
}

static void input_notify_read(struct ustream *s, int bytes)
{
    char *buf = NULL;
    int len = 0;

    buf = ustream_get_read_buf(s, &len);
    fprintf(stderr, "Wrote %d bytes to server.\n", bytes);
    ustream_write(&stream.stream, buf, len, false);
    ustream_consume(s, len);
}

static void client_notify_read(struct ustream *s, int bytes)
{
    char *buf = NULL;
    int len = 0;

    buf = ustream_get_read_buf(s, &len);
    fprintf(stderr, "Read %d bytes from server:\n", bytes);
    fwrite(buf, len, 1, stdout);
    fflush(stdout);
    ustream_consume(s, len);
}

static void client_notify_write(struct ustream *s, int bytes)
{
    fprintf(stderr, "Wrote %d bytes, pending %d\n", bytes, s->w.data_bytes);
}

static void client_notify_state(struct ustream *us)
{
    if (!us->write_error && !us->eof)
    {
        return;
    }

    fprintf(stderr, "Connection closed\n");
    client_teardown();
}

static void client_connect_cb(struct uloop_fd *f, unsigned int events)
{
    if (client.eof || client.error)
    {
        fprintf(stderr, "Connection failed\n");
        uloop_end();
        return;
    }

    fprintf(stderr, "Connection established\n");
    uloop_fd_delete(&client);

    stream.stream.notify_read = client_notify_read;
    stream.stream.notify_write = client_notify_write;
    stream.stream.notify_state = client_notify_state;

    ustream_fd_init(&stream, client.fd);

    s_input.stream.notify_read = input_notify_read;
    ustream_fd_init(&s_input, 0);

    fprintf(stderr, "Input message to send (or 'quit' to exit), end with 'ENTER'.\n");
}

static int run_client(void)
{
    
    client.cb = client_connect_cb;
	client.fd = usock(USOCK_TCP | USOCK_NONBLOCK, host, port);
    if (client.fd < 0)
	{
		perror("usock");
		return 1;
	}

	uloop_init();
	uloop_fd_add(&client, ULOOP_WRITE | ULOOP_EDGE_TRIGGER);
    uloop_run();

    close(client.fd);
    uloop_done();

    return 0;
}


static int usage(const char *name)
{
    fprintf(stderr, "Usage: %s [-s <server>] [-p <port>]\n", name);
    return 1;
}

int main(int argc, char **argv)
{
    int ch;

    while ((ch = getopt(argc, argv, "s:p:")) != -1)
    {
        switch (ch)
        {
        case 's':
            host = optarg;
            break;
        case 'p':
            port = optarg;
            break;
        default:
            return usage(argv[0]);
        }
    }

    return run_client();
}
