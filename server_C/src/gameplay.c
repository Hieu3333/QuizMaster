#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libwebsockets.h>

#define PORT 5000  // WebSocket server port

// This structure stores per-session data for each client connection
struct per_session_data__echo {
    int dummy;  // Placeholder for any session-specific data
};

// Callback function that handles WebSocket events
static int callback_websocket(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len) {
    switch (reason) {
        case LWS_CALLBACK_SERVER_WRITEABLE: {
            // Prepare a message to send to the client
            const char *msg = "Hello from the WebSocket server!";
            size_t msg_len = strlen(msg);

            // Allocate buffer with LWS_PRE padding
            unsigned char *buf = (unsigned char *) malloc(LWS_PRE + msg_len);
            if (!buf) {
                printf("Memory allocation failed!\n");
                return -1;
            }

            // Copy the message into the buffer
            memcpy(buf + LWS_PRE, msg, msg_len);

            // Send the message
            int n = lws_write(wsi, buf + LWS_PRE, msg_len, LWS_WRITE_TEXT);
            free(buf);

            if (n < 0) {
                printf("Error writing to WebSocket!\n");
                return -1;
            }
            break;
        }

        case LWS_CALLBACK_ESTABLISHED: {
            printf("New client connected!\n");
            break;
        }

        case LWS_CALLBACK_RECEIVE: {
            // Print the message received from the client
            printf("Received message: %.*s\n", (int)len, (char *)in);

            // Respond with a message (for example, echoing the received message)
            const char *response = "Message received!";
            size_t resp_len = strlen(response);

            // Allocate buffer with LWS_PRE padding
            unsigned char *buf = (unsigned char *) malloc(LWS_PRE + resp_len);
            if (!buf) {
                printf("Memory allocation failed!\n");
                return -1;
            }

            // Copy the response into the buffer
            memcpy(buf + LWS_PRE, response, resp_len);

            // Send the response
            int n = lws_write(wsi, buf + LWS_PRE, resp_len, LWS_WRITE_TEXT);
            free(buf);

            if (n < 0) {
                printf("Error writing to WebSocket!\n");
                return -1;
            }

            // Mark the connection as writeable again if needed
            lws_callback_on_writable(wsi);
            break;
        }

        case LWS_CALLBACK_CLOSED: {
            printf("Client disconnected!\n");
            break;
        }

        default:
            break;
    }
    return 0;
}

// Protocols array, used to configure the WebSocket server
static struct lws_protocols protocols[] = {
    {
        "ws-only",  // WebSocket protocol name
        callback_websocket,  // Callback function for handling events
        sizeof(struct per_session_data__echo),  // Size of per-session data
        1024,  // Maximum message size
    },
    { NULL, NULL, 0, 0 }  // Terminating entry
};

int main() {
    // WebSocket server info configuration
    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));
    info.port = PORT;  // The port the WebSocket server will listen on
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;

    // Create the WebSocket server context
    struct lws_context *context = lws_create_context(&info);
    if (!context) {
        printf("Error creating WebSocket server context!\n");
        return -1;
    }

    // Main server loop
    printf("WebSocket server listening on ws://localhost:%d\n", PORT);
    while (1) {
        // Handle WebSocket events
        lws_service(context, 1000);
    }

    // Cleanup and shutdown
    lws_context_destroy(context);
    return 0;
}
