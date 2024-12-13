// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <libwebsockets.h>
// #include <json-c/json.h>



// // Structures
// typedef struct User {
//     int id;
//     char name[50];
// } User;



// // WebSocket server callbacks
// static int callback_websocket(struct lws *wsi, enum lws_callback_reasons reason,
//                               void *user, void *in, size_t len) {
//     switch (reason) {
//         case LWS_CALLBACK_ESTABLISHED:
//             printf("New client connected\n");
//             break;

//         case LWS_CALLBACK_RECEIVE: {
//             // Parse the received JSON
//             struct json_object *parsed_json = json_tokener_parse((char *)in);
//             struct json_object *action;
//             json_object_object_get_ex(parsed_json, "action", &action);
//             const char *action_str = json_object_get_string(action);

//             if (strcmp(action_str, "findMatch") == 0) {
//                 // Handle "findMatch" logic
//                 printf("findMatch action received\n");
//                 // Add user to a room or create a new one
//             } else if (strcmp(action_str, "vote") == 0) {
//                 // Handle "vote" logic
//                 printf("vote action received\n");
//                 // Collect votes and determine the category
//             } else if (strcmp(action_str, "answer") == 0) {
//                 // Handle "answer" logic
//                 printf("answer action received\n");
//                 // Evaluate answer and manage scoring
//             }

//             json_object_put(parsed_json);  // Free JSON object
//             break;
//         }

//         case LWS_CALLBACK_CLOSED:
//             printf("Client disconnected\n");
//             break;

//         default:
//             break;
//     }

//     return 0;
// }

// // WebSocket protocols
// static struct lws_protocols protocols[] = {
//     {"ws-only", callback_websocket, 0, 1024},
//     {NULL, NULL, 0, 0}};

// // Main server function
// int main() {
//     struct lws_context_creation_info info;
//     memset(&info, 0, sizeof(info));
//     info.port = PORT;
//     info.protocols = protocols;
//     info.gid = -1;
//     info.uid = -1;

//     struct lws_context *context = lws_create_context(&info);
//     if (!context) {
//         printf("Error creating WebSocket context\n");
//         return -1;
//     }

//     printf("WebSocket server listening on ws://localhost:%d\n", PORT);
//     while (1) {
//         lws_service(context, 1000);
//     }

//     lws_context_destroy(context);
//     return 0;
// }
