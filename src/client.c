#include "client.h"

#include <stdlib.h>

#include "packet_types.h"
#include "util.h"

void client_close_connection_cb(uv_handle_t* handle) {
    LOG_INFO("Closing client");
    ClientData* client = (ClientData*)(handle->data);

    uv_timer_stop(&client->heartbeat);

    pb_delete(&client->pb);
    free(client->read_buffer.base);
    free(client->ri.packet_data);

    free(client);
    free(handle);
}

void client_send_heartbeat_cb(uv_timer_t* handle) {
    ClientData* client = handle->data;
    PacketBuilder* pb = &client->pb;

    LOG_TRACE("S -> C: keep_alive");
    pb_reset(pb);
    pb_write_id(pb, P_KEEP_ALIVE);
    pb_write_i64(pb, (int64_t)(rand()));

    uv_buf_t packet = pb_finalize(pb);
    send_finalized_packet(client->client_stream, packet, false);
    pb_reset(pb);
}
