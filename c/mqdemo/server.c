//
// Created by rune on 2018/4/9.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <stdint.h>
#include <amqp_tcp_socket.h>
#include <amqp.h>
#include <amqp_framing.h>

#include <assert.h>

#include "utils.h"

char* do_something(void const *buffer, size_t len){
    amqp_dump(buffer, len);
    return "ack";
}

int main(int argc, char const *const *argv)
{
    char const *hostname = "127.0.0.1";
    int port = 5672, status;
    char const *queue_name = "rpc_queue";
    amqp_socket_t *socket = NULL;
    amqp_connection_state_t conn;
    char *messagebody;

    conn = amqp_new_connection();

    socket = amqp_tcp_socket_new(conn);
    if (!socket) {
        die("creating TCP socket");
    }

    status = amqp_socket_open(socket, hostname, port);
    if (status) {
        die("opening TCP socket");
    }

    die_on_amqp_error(amqp_login(conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, "guest", "guest"),
                      "Logging in");
    amqp_channel_open(conn, 1);
    die_on_amqp_error(amqp_get_rpc_reply(conn), "Opening channel");

    {
        amqp_queue_declare_ok_t *r = amqp_queue_declare(
                conn, 1, amqp_cstring_bytes(queue_name), 0, 0, 0, 1, amqp_empty_table);
        die_on_amqp_error(amqp_get_rpc_reply(conn), "Declaring queue");
    }

    amqp_basic_consume(conn, 1, amqp_cstring_bytes(queue_name), amqp_empty_bytes, 0, 0, 0, amqp_empty_table);
    die_on_amqp_error(amqp_get_rpc_reply(conn), "Consuming");

    {
        for (;;) {
            amqp_rpc_reply_t res;
            amqp_envelope_t envelope;

            amqp_maybe_release_buffers(conn);

            res = amqp_consume_message(conn, &envelope, NULL, 0);

            if (AMQP_RESPONSE_NORMAL != res.reply_type) {
                break;
            }
/*
Once you've consumed a message and processed it,
your code should publish to the default exchange (amqp_empty_bytes),
 with a routing key that is specified in the reply_to header in the request message.
Additionally your code should set the correlation_id header to be the same as what is in the request message.
*/
            printf("||Delivery %u,%d exchange %s  %d routingkey %s consumer_tag %s|| \n",
                   (unsigned) envelope.delivery_tag,
                   (int) envelope.exchange.len,
                   (char *) envelope.exchange.bytes,
                   (int) envelope.routing_key.len,
                   (char *) envelope.routing_key.bytes,
                   (char *) envelope.message.properties.reply_to.bytes);

            printf("----\n");

            messagebody = do_something(envelope.message.body.bytes, envelope.message.body.len);


//ALE

            amqp_basic_properties_t props;
            props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG |
                           AMQP_BASIC_DELIVERY_MODE_FLAG |
                           AMQP_BASIC_CORRELATION_ID_FLAG;
            props.content_type = amqp_cstring_bytes("text/plain");
            props.delivery_mode = 2; // persistent delivery mode
//Additionally your code should set the correlation_id header to be the same as what is in the request message.
            props.correlation_id = envelope.message.properties.correlation_id;

            die_on_error(amqp_basic_publish(conn,
                                            1,
//your code should publish to the default exchange (amqp_empty_bytes)
                                            amqp_empty_bytes,
// with a routing key that is specified in the reply_to header in the request message.
                                            amqp_cstring_bytes((char *)envelope.message.properties.reply_to.bytes),
                                            0,
                                            0,
                                            &props,
                                            amqp_cstring_bytes(messagebody)),
                         "Publishing");

            printf("MESSAGEBODY:[%s]\n",messagebody);
            amqp_bytes_free(props.reply_to);


//END ALE



            amqp_destroy_envelope(&envelope);
        }
    }

    die_on_amqp_error(amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS), "Closing channel");
    die_on_amqp_error(amqp_connection_close(conn, AMQP_REPLY_SUCCESS), "Closing connection");
    die_on_error(amqp_destroy_connection(conn), "Ending connection");

    return 0;
}