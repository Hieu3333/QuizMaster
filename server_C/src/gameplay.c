#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libwebsockets.h>
#include "user.h"

#define PORT 5000  // WebSocket server port

#define MAX_PLAYERS 4
#define MAX_ROOMS 10
#define MAX_QUESTIONS 7

typedef struct Question {
    char question[256];
    char choices[4][128];
    char correctAnswer[128];
} Question;

typedef struct Room {
    int id;
    User players[MAX_PLAYERS];
    int player_count;
    char votes[MAX_PLAYERS][50];
    int scores[MAX_PLAYERS];
    int answered[MAX_PLAYERS];
    int isOngoing;
    Question questions[MAX_QUESTIONS];
    int currentQuestion;
} Room;

// Global variables
Room rooms[MAX_ROOMS];
int room_count = 0;

// // Function to find an existing room or create a new one
// int find_room_for_user(User *user) {
//     // Check if the user is already in a room
//     for (int i = 0; i < room_count; i++) {
//         for (int j = 0; j < rooms[i].player_count; j++) {
//             if (rooms[i].players[j].id == user->id) {
//                 // User is already playing
//                 printf("User %d is already in room %d.\n", user->id, rooms[i].id);
//                 return -1;  // User already in a room
//             }
//         }
//     }

//     // Try to add the user to an existing room
//     for (int i = 0; i < room_count; i++) {
//         if (rooms[i].player_count < MAX_PLAYERS) {
//             rooms[i].players[rooms[i].player_count] = *user;
//             rooms[i].player_count++;
//             printf("User %d joined room %d.\n", user->id, rooms[i].id);
//             return rooms[i].id;  // Room ID user joined
//         }
//     }

//     // If no available room, create a new room
//     return create_new_room(user);
// }

// // Function to create a new room for the user
// int create_new_room(User *user) {
//     // Create a new room and add the user
//     Room new_room;
//     new_room.id = room_count + 1;
//     new_room.players[0] = *user;
//     new_room.player_count = 1;
//     new_room.is_ongoing = 0;  // Game is not yet started

//     rooms[room_count] = new_room;
//     room_count++;

//     printf("User %d created and joined room %d.\n", user->id, new_room.id);

//     return new_room.id;
// }

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
            // Check for invalid length or null pointer
            if (in == NULL || len <= 0) {
                fprintf(stderr, "Error: Invalid input data (NULL or length <= 0)\n");
                return -1;
            }

            // Print the message length
            printf("Length: %d\n", (int)len);

            // Ensure that the length is valid for accessing memory
            if (len > 0) {
                // Null-terminate the string in case it isn't
                char *msg = (char *)in;
                msg[len] = '\0'; // Ensure the string is null-terminated

                // Print the received message safely
                printf("Received message: %s\n", msg);
            } else {
                // Handle edge case if len is not valid
                fprintf(stderr, "Error: Received invalid length\n");
                return -1;
            }

            // Parse the incoming message as JSON
            struct json_object *parsed_json = json_tokener_parse((char *)in);

            if (parsed_json == NULL) {
                fprintf(stderr, "Error: Failed to parse JSON. Raw message: %.*s\n", (int)len, (char *)in);
                return -1;
            }

            // Extract the 'action' field from the parsed JSON
            struct json_object *action;
            if (!json_object_object_get_ex(parsed_json, "action", &action)) {
                fprintf(stderr, "Error: Key 'action' not found in JSON.\n");
                json_object_put(parsed_json);  // Free JSON object
                return -1;
            }

            const char *action_str = json_object_get_string(action);
            if (action_str == NULL) {
                fprintf(stderr, "Error: 'action' value is not a string or is null.\n");
                json_object_put(parsed_json);  // Free JSON object
                return -1;
            }

            // Handle different actions based on the 'action' field value
            if (strcmp(action_str, "greet") == 0) {
                // Handle the "greet" action
                printf("Greet action received!\n");

                // Respond with a greeting message (JSON)
                const char *greeting_response = "{\"response\": \"Hello from the server!\"}";
                size_t resp_len = strlen(greeting_response);

                // Allocate buffer with LWS_PRE padding
                unsigned char *buf = (unsigned char *)malloc(LWS_PRE + resp_len);
                if (!buf) {
                    printf("Memory allocation failed!\n");
                    json_object_put(parsed_json);  // Free JSON object
                    return -1;
                }

                // Copy the response into the buffer
                memcpy(buf + LWS_PRE, greeting_response, resp_len);

                // Send the response
                int n = lws_write(wsi, buf + LWS_PRE, resp_len, LWS_WRITE_TEXT);
                free(buf);

                if (n < 0) {
                    printf("Error writing to WebSocket!\n");
                    json_object_put(parsed_json);  // Free JSON object
                    return -1;
                }

            } else if (strcmp(action_str, "findMatch") == 0) {
                // Handle "findMatch" logic
                printf("findMatch action received\n");
                // Add user to a room or create a new one
            } else if (strcmp(action_str, "vote") == 0) {
                // Handle "vote" logic
                printf("vote action received\n");
                // Collect votes and determine the category
            } else if (strcmp(action_str, "answer") == 0) {
                // Handle "answer" logic
                printf("answer action received\n");
                // Evaluate answer and manage scoring
            } else {
                // If action is unrecognized, respond with an error
                printf("Unknown action received: %s\n", action_str);
            }

            // Free the parsed JSON object after usage
            json_object_put(parsed_json);

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
