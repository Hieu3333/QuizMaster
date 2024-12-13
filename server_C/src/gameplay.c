#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libwebsockets.h>
#include <jansson.h>
#include <bson.h>
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

// Helper function to parse ISO8601 datetime string to time_t
time_t iso8601_to_time(const char *datetime) {
    struct tm tm = {0};
    if (strptime(datetime, "%Y-%m-%dT%H:%M:%S", &tm) == NULL) {
        fprintf(stderr, "Error: Failed to parse datetime string: %s\n", datetime);
        return (time_t)-1;
    }
    return mktime(&tm);
}

// Function to extract user info from JSON 'data' object
User *extract_user_from_data(json_t *data) {
    if (!data) {
        fprintf(stderr, "Error: 'data' object is NULL.\n");
        return NULL;
    }

    // Allocate memory for the new user
    User *user = (User *)malloc(sizeof(User));
    if (!user) {
        fprintf(stderr, "Error: Memory allocation for User failed.\n");
        return NULL;
    }
    memset(user, 0, sizeof(User)); // Initialize all fields to zero

    // Extract required fields
    json_t *id_json, *username_json, *password_json, *wins_json, *totalScore_json, *playedGames_json, *createdAt_json, *updatedAt_json;

    // Get each field using jansson's json_object_get (in jansson, it's more direct than json-c)
    id_json = json_object_get(data, "id");
    username_json = json_object_get(data, "username");
    password_json = json_object_get(data, "password");
    wins_json = json_object_get(data, "wins");
    totalScore_json = json_object_get(data, "totalScore");
    playedGames_json = json_object_get(data, "playedGames");
    createdAt_json = json_object_get(data, "createdAt");
    updatedAt_json = json_object_get(data, "updatedAt");

    if (!id_json || !username_json || !password_json || !wins_json || !totalScore_json || !playedGames_json || !createdAt_json || !updatedAt_json) {
        fprintf(stderr, "Error: Missing required fields in 'data'.\n");
        free(user); // Free allocated memory
        return NULL;
    }

   

    const char *id_str = json_string_value(id_json);
    if (id_str) {
        // First, check if the ObjectId string is valid
        if (!bson_oid_is_valid(id_str,strlen(id_str))) {
            fprintf(stderr, "Error: Invalid BSON ObjectId format in 'id'.\n");
            free(user); // Free allocated memory
            return NULL;
        }
        
        // Initialize BSON ObjectId from the valid string
        bson_oid_init_from_string(&user->id, id_str);
    }

    // Copy strings for username and password
    user->username = strdup(json_string_value(username_json));
    user->password = strdup(json_string_value(password_json));

    // Get integer values
    user->wins = json_integer_value(wins_json);
    user->totalScore = json_integer_value(totalScore_json);
    user->playedGames = json_integer_value(playedGames_json);

    // Convert ISO8601 date strings to time_t
    user->createdAt = iso8601_to_time(json_string_value(createdAt_json));
    user->updatedAt = iso8601_to_time(json_string_value(updatedAt_json));

    // Check for conversion errors
    if (user->createdAt == (time_t)-1 || user->updatedAt == (time_t)-1) {
        fprintf(stderr, "Error: Invalid datetime format in 'data'.\n");
        free(user->username);
        free(user->password);
        free(user);
        return NULL;
    }

    return user;
}

// Function to find an existing room or create a new one
int find_room_for_user(User *user) {
    // Check if the user is already in a room
    for (int i = 0; i < room_count; i++) {
        for (int j = 0; j < rooms[i].player_count; j++) {
            if (bson_oid_compare(&rooms[i].players[j].id,&user->id)) {
                // User is already playing
                printf("User %d is already in room %d.\n", user->id, rooms[i].id);
                return -1;  // User already in a room
            }
        }
    }

    // Try to add the user to an existing room
    for (int i = 0; i < room_count; i++) {
        if (rooms[i].player_count < MAX_PLAYERS) {
            rooms[i].players[rooms[i].player_count] = *user;
            rooms[i].player_count++;
            printf("User %d joined room %d.\n", user->id, rooms[i].id);
            return rooms[i].id;  // Room ID user joined
        }
    }

    // If no available room, create a new room
    return create_new_room(user);
}

// Function to create a new room for the user
int create_new_room(User *user) {
    // Create a new room and add the user
    Room new_room;
    
    new_room.id = room_count + 1;
    new_room.players[0] = *user;
    new_room.player_count = 1;
    new_room.isOngoing = 0;  // Game is not yet started

    rooms[room_count] = new_room;
    room_count++;


    printf("Joined room %d.\n", new_room.id);
    printf("Num room: %d\n",room_count);

    return new_room.id;
}

// This structure stores per-session data for each client connection
struct per_session_data__echo {
    int dummy;  // Placeholder for any session-specific data
};

// Callback function that handles WebSocket events
static int callback_websocket(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len) {
    switch (reason) {
        // case LWS_CALLBACK_SERVER_WRITEABLE: {
        //     // Prepare a message to send to the client
        //     const char *msg = "Hello from the WebSocket server!";
        //     size_t msg_len = strlen(msg);

        //     // Allocate buffer with LWS_PRE padding
        //     unsigned char *buf = (unsigned char *) malloc(LWS_PRE + msg_len);
        //     if (!buf) {
        //         printf("Memory allocation failed!\n");
        //         return -1;
        //     }

        //     // Copy the message into the buffer
        //     memcpy(buf + LWS_PRE, msg, msg_len);

        //     // Send the message
        //     int n = lws_write(wsi, buf + LWS_PRE, msg_len, LWS_WRITE_TEXT);
        //     free(buf);

        //     if (n < 0) {
        //         printf("Error writing to WebSocket!\n");
        //         return -1;
        //     }
        //     break;
        // }

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
            // Null-terminate the string in case it isn't
            char *msg = (char *)in;
            msg[len] = '\0'; // Ensure the string is null-terminated

            // Print the received message safely
            printf("Received message: %s\n", msg);

            // Parse the incoming message as JSON using Jansson
            json_error_t error;
            json_t *parsed_json = json_loads(msg, 0, &error);

            if (!parsed_json) {
                fprintf(stderr, "Error: Failed to parse JSON. Raw message: %s\n", msg);
                return -1;
            }

            // Extract the 'action' field from the parsed JSON
            json_t *action = json_object_get(parsed_json, "action");
            if (!action || !json_is_string(action)) {
                fprintf(stderr, "Error: Key 'action' not found or is not a string in JSON.\n");
                json_decref(parsed_json);  // Free JSON object
                return -1;
            }

            const char *action_str = json_string_value(action);
            if (action_str == NULL) {
                fprintf(stderr, "Error: 'action' value is not a string or is null.\n");
                json_decref(parsed_json);  // Free JSON object
                return -1;
            }

            if (strcmp(action_str, "findMatch") == 0) {
                // Handle "findMatch" logic
                printf("findMatch action received\n");

                // Extract 'data' field from JSON
                json_t *user_json = json_object_get(parsed_json, "data");
                if (!user_json || !json_is_object(user_json)) {
                    fprintf(stderr, "Error: Key 'data' not found or is not an object in JSON.\n");
                    json_decref(parsed_json);  // Free JSON object
                    return -1;
                }

                // Assuming extract_user_from_data is a function that handles the user extraction
                User* user = extract_user_from_data(user_json);

                if (user == NULL) {
                    fprintf(stderr, "Error: Failed to extract user from 'data'.\n");
                    json_decref(parsed_json);  // Free JSON object
                    return -1;
                }

                // Find or create a room for the user
                int room_id = find_room_for_user(user);
                if (room_id < 0) {
                    fprintf(stderr, "Error: Failed to find or create room for user.\n");
                    json_decref(parsed_json);  // Free JSON object
                    free(user);  // Free the user object
                    return -1;
                }

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
            json_decref(parsed_json);

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
