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
    ClientData* client = (ClientData*)handle->data;

    if (client->read_buffer.len == 0) {
        client->read_buffer = (uv_buf_t){
            .base = malloc(DEFAULT_BUFFER_SIZE),
            .len = DEFAULT_BUFFER_SIZE
        };
    }

    *buf = client->read_buffer;
}

bool build_varint(uint8_t byte, int* curr_pos, int* val) {
    *val |= (byte & VARINT_SEGMENT_BITS) << *curr_pos;

    if ((byte & VARINT_CONTINUE_BIT) == 0) return true;

    *curr_pos += 7;

    return false;
}

void connection_read_cb(uv_stream_t* handle,
                        ssize_t nread,
                        const uv_buf_t* buf) {
    ClientData* client = handle->data;

    int current_idx = 0;

    if (nread > 0) {
        // LOG_DEBUG("Recieved %ld bytes", nread);

        while (true) {
            if (current_idx >= nread) {
                break;
            }

            if (client->ri.read_left == 0) {
                // The current buffer represents the start of a new sequence of packets
                while (!client->ri.done_reading_size && current_idx < nread) {
                    client->ri.done_reading_size |= build_varint(buf->base[current_idx], &client->ri.varint_pos, &client->ri.packet_size);
                    ++current_idx;
                }

                if (client->ri.done_reading_size) {
                    client->ri.done_reading_size = false;

                    client->ri.packet_data = realloc(client->ri.packet_data, client->ri.packet_size);

                    if (current_idx + client->ri.packet_size <= nread) {
                        // Entire packet is in this buffer
                        memcpy(client->ri.packet_data, buf->base + current_idx, client->ri.packet_size);

                        PacketReader pr = pr_from_uv((uv_buf_t){ .base = (char*)client->ri.packet_data, .len = client->ri.packet_size });
                        PacketBuilder* pb = &client->pb;
                        pb_reset(pb);
                        
                        int packet_id = pr_read_varint(&pr);

                        // LOG_DEBUG("Received packet { state = %d, id = 0x%02X, len = %d }", client->state, packet_id, client->ri.packet_size);

                        process_packet(packet_id, handle, client, &pr, pb);

                        current_idx += client->ri.packet_size;

                        client->ri.read_left = 0;
                        client->ri.packet_size = 0;
                        client->ri.varint_pos = 0;
                    } else {
                        memcpy(client->ri.packet_data + client->ri.packet_data_write_pos, buf->base + current_idx, nread - current_idx);
                        client->ri.packet_data_write_pos = nread - current_idx;
                        client->ri.read_left = client->ri.packet_size - (nread - current_idx);
                    }
                }
            } else {
                int can_read = min(client->ri.read_left, nread);
                client->ri.read_left -= can_read;

                memcpy(client->ri.packet_data + client->ri.packet_data_write_pos, buf->base + current_idx, can_read);

                if (client->ri.read_left == 0) {
                    client->ri.packet_data_write_pos = 0;
                    client->ri.packet_size = 0;
                } else {
                    client->ri.packet_data_write_pos += can_read;
                }
            }
        }
    } else {
        if (nread == UV_EOF) {
            uv_close((uv_handle_t*)handle, client_close_connection_cb);
        } else {
            CHECK_UV(nread);
        }
    }
}

static void new_connection_cb(uv_stream_t* server, int status) {
    assert(status >= 0);

    // TODO(eli): Client pools
    uv_tcp_t* client_stream = talloc(uv_tcp_t);
    uv_tcp_init(loop, client_stream);

    ClientData* client_data = tzalloc(ClientData);

    uv_timer_init(loop, &client_data->heartbeat);
    client_data->heartbeat.data = client_data;
    client_data->client_stream = (uv_stream_t*)client_stream;

    client_stream->data = client_data;
    CHECK_UV(uv_accept(server, (uv_stream_t*)client_stream));
    uv_read_start((uv_stream_t*)client_stream, default_alloc_cb, connection_read_cb);
}

int main() {
    loop = uv_default_loop();
    srand(time(0));
    
    CHECK_UV(uv_poll_init(loop, &stdin_watcher, STDIN_FILENO));
    CHECK_UV(uv_poll_start(&stdin_watcher, UV_READABLE, on_stdin_ready));

    uv_tcp_t server;
    CHECK_UV(uv_tcp_init(loop, &server));

    struct sockaddr_in addr;
    CHECK_UV(uv_ip4_addr("0.0.0.0", 25565, &addr));
    CHECK_UV(uv_tcp_bind(&server, (const struct sockaddr*)&addr, 0));
    CHECK_UV(uv_tcp_nodelay(&server, false));
    CHECK_UV(uv_listen((uv_stream_t*)&server, 128, new_connection_cb));

    uv_run(loop, UV_RUN_DEFAULT);

    uv_close((uv_handle_t*)&server, NULL);
    uv_close((uv_handle_t*)&stdin_watcher, NULL);

    uv_loop_close(loop);
    return 0;
}
