#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <uv.h>

#include "client.h"
#include "packet.h"
#include "packet_handlers.h"
#include "util.h"

uv_loop_t* loop;
uv_poll_t stdin_watcher;

void on_stdin_ready(uv_poll_t* handle, int status, int events) {
    if (events & UV_READABLE) {
        char buffer[1024];
        ssize_t nread = read(STDIN_FILENO, buffer, sizeof(buffer));
        if (nread > 0) {
            if (buffer[0] == '\n') {
                printf("Enter pressed, stopping loop...\n");
                uv_stop(loop);
            }
        }
    }
}

#define DEFAULT_BUFFER_SIZE 32768

static void default_alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
    Client* client = (Client*)handle->data;

    if (client->read_buffer.len == 0) {
        client->read_buffer = (uv_buf_t){
            .base = malloc(DEFAULT_BUFFER_SIZE),
            .len = DEFAULT_BUFFER_SIZE
        };
    }

    *buf = client->read_buffer;
}

void connection_read_cb(uv_stream_t* handle,
                        ssize_t nread,
                        const uv_buf_t* buf) {
    // XXX(eli): BAD BAD BAD BAD... assuming the buffer contains the entire
    // packets and that there is exactly 1 packet in it

    Client* client = handle->data;

    if (nread > 0) {
        PacketReader pr = pr_from_uv(*buf);
        PacketBuilder *pb = &client->pb;
        pb_reset(pb);

        int packet_len = pr_read_varint(&pr);
        int packet_id = pr_read_varint(&pr);

        LOG_DEBUG("Received packet { state = %d, id = 0x%02X, len = %d }", client->state, packet_id, packet_len);

        switch (client->state) {
        case HANDSHAKE:
            HANDSHAKE_HANDLERS[packet_id](handle, client, &pr, pb);
            break;
        case STATUS:
            STATUS_HANDLERS[packet_id](handle, client, &pr, pb);
            break;
        case LOGIN:
            LOGIN_HANDLERS[packet_id](handle, client, &pr, pb);
            break;
        case TRANSFER:
            abort();
            break;
        case CONFIG:
            CONFIG_HANDLERS[packet_id](handle, client, &pr, pb);
            break;
        case PLAY:
          break;
        }
    } else {
        if (nread == UV_EOF) {
            uv_shutdown(talloc(uv_shutdown_t), handle, client_close_connection_cb);
        } else {
            CHECK_UV(nread);
        }
    }
}

static void new_connection_cb(uv_stream_t* server, int status) {
    assert(status >= 0);

    // TODO(eli): Client pools
    uv_tcp_t* client = talloc(uv_tcp_t);
    uv_tcp_init(loop, client);

    client->data = tzalloc(Client);

    CHECK_UV(uv_accept(server, (uv_stream_t*)client));
    uv_read_start((uv_stream_t*)client, default_alloc_cb, connection_read_cb);
}

int main() {
    loop = uv_default_loop();
    
    CHECK_UV(uv_poll_init(loop, &stdin_watcher, STDIN_FILENO));
    CHECK_UV(uv_poll_start(&stdin_watcher, UV_READABLE, on_stdin_ready));

    uv_tcp_t server;
    CHECK_UV(uv_tcp_init(loop, &server));

    struct sockaddr_in addr;
    CHECK_UV(uv_ip4_addr("0.0.0.0", 25565, &addr));
    CHECK_UV(uv_tcp_bind(&server, (const struct sockaddr*)&addr, 0));
    CHECK_UV(uv_listen((uv_stream_t*)&server, 128, new_connection_cb));

    uv_run(loop, UV_RUN_DEFAULT);

    uv_loop_close(loop);
    return 0;
}
