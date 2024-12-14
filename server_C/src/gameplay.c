#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libwebsockets.h>
#include <jansson.h>
#include <bson.h>
#include "user.h"
#include "gameplay.h"


const char *categories[] = {
    [9] = "General Knowledge",
    [11] = "Films",
    [12] = "Music",
    [15] = "Videogames",
    [22] = "Geography",
    [23] = "History",
};

// Global variables
Room* rooms[MAX_ROOMS];
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
    const char *username_str = json_string_value(username_json);
    const char *password_str = json_string_value(password_json);
    if (username_str && password_str) {
        user->username = strdup(username_str);
        user->password = strdup(password_str);
    } else {
        fprintf(stderr, "Error: Invalid or missing username/password.\n");
        free(user);
        return NULL;
    }

    // Extract integer values
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
        for (int j = 0; j < rooms[i]->player_count; j++) {
            if (bson_oid_compare(&rooms[i]->players[j].id,&user->id)==0) {
                char id_str1[25];  // Buffer to hold the string representation of the BSON Object ID
                char id_str2[25];
                bson_oid_to_string(&user->id, id_str1);
                bson_oid_to_string(&rooms[i]->players[j].id, id_str2);
                // User is already playing
                // printf("User %s is already in room %d.\n", user->username, rooms[i]->id);
                // printf("ID %s is finding room and ID %s is already in room.\n", id_str1, id_str2);
                return -1;  // User already in a room
            }
        }
    }

    // Try to add the user to an existing room
    for (int i = 0; i < room_count; i++) {
        if (rooms[i]->player_count < MAX_PLAYERS) {
        
            rooms[i]->players[rooms[i]->player_count] = *user;
            rooms[i]->player_count++;
            char id_str[25];  // Buffer to hold the string representation of the BSON Object ID
                bson_oid_to_string(&user->id, id_str);
            printf("User %s joined room %d.\n", user->username, rooms[i]->id);
            return rooms[i]->id;  // Room ID user joined
        }
    }

    // If no available room, create a new room
    return create_new_room(user);
}

// Function to create a new room for the user
int create_new_room(User *user) {
    if (room_count >= MAX_ROOMS) {
        printf("Error: Cannot create new room, maximum number of rooms reached.\n");
        return -1;  // Error code indicating failure
    }

    // Allocate memory for the new room
    Room *new_room = (Room *)malloc(sizeof(Room));
    if (new_room == NULL) {
        printf("Error: Memory allocation failed for the new room.\n");
        return -1;  // Memory allocation failed
    }

    // Initialize the new room
    new_room->id = room_count + 1;   // Assign room ID
    new_room->players[0] = *user;    // Add the user to the room
    new_room->player_count = 1;      // One player in the room
    new_room->isOngoing = 0;         // Game is not yet started

    // Convert BSON Object ID to string
    char id_str[25];  // Buffer to hold the string representation of the BSON Object ID
    bson_oid_to_string(&user->id, id_str);
    printf("User %s joined room %d.\n", user->username, new_room->id);

    // Store the new room in the rooms array
    rooms[room_count] = new_room;
    room_count++;

    return new_room->id;  // Return the room ID
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

                // Prepare the response JSON object
                json_t *response_json = json_object();
                json_t *room_data = json_object();

                if (room_id <= room_count) {
                    // Room was found or created, create the response accordingly
                    json_object_set_new(room_data, "roomId", json_integer(room_id));
                    int room_access_id = room_id-1;

                    json_t *players_array = json_array(); // Create an array of players

                    // Check if there are any players
                    if (rooms[room_access_id]->player_count == 0) {
                        printf("No players in the room.\n");
                    }

                    // Iterate over the players in the room and add their data to the array
                    for (int i = 0; i < rooms[room_access_id]->player_count; i++) {
                        printf("Room %d: Player %d\n",room_access_id+1,i);
                        User *player = &rooms[room_access_id]->players[i];  // Get the player
                         
                        if (player == NULL){
                            printf("Player is NULL");
                            return -1;
                        }

                        json_t *player_data = json_object();
                        char* id[25];
                        if (&player->id == NULL){
                            printf("Player Id is NULL");
                        }
                        bson_oid_to_string(&player->id, id);
                        printf("player[%d] - id: %s, username: %s, wins: %d, totalScore: %d, playedGames: %d\n",
           i, id, player->username, player->wins, player->totalScore, player->playedGames);

                        json_object_set_new(player_data, "id", json_string(id));
                        json_object_set_new(player_data, "username", json_string(player->username));
                        json_object_set_new(player_data, "password", json_string(player->password));
                        json_object_set_new(player_data, "wins", json_integer(player->wins));
                        json_object_set_new(player_data, "totalScore", json_integer(player->totalScore));
                        json_object_set_new(player_data, "playedGames", json_integer(player->playedGames));
                        json_object_set_new(player_data, "createdAt", json_integer(player->createdAt));
                        json_object_set_new(player_data, "updatedAt", json_integer(player->updatedAt));

                        json_array_append_new(players_array, player_data);
                        // free(player);

                    }

                    json_object_set_new(room_data, "roomPlayers", players_array);
                    json_object_set_new(response_json, "action", json_string("joinRoom"));
                    json_object_set_new(response_json, "data", room_data);
                }

                // Serialize the response and send it
                const char *response_str = json_dumps(response_json, 0);
                if (response_str == NULL) {
                    fprintf(stderr, "Error: Failed to serialize response to JSON.\n");
                    json_decref(parsed_json);
                    free(user);
                 
                    return -1;
                }

                // Send the message via WebSocket
                unsigned char *buf = (unsigned char *)response_str;
                int len_sent = lws_write(wsi, buf, strlen(response_str), LWS_WRITE_TEXT);
                if (len_sent < 0) {
                    fprintf(stderr, "Error: Failed to send message to client\n");
                    json_decref(parsed_json);
                    free(user);
                    return -1;
                }

                // Clean up
                json_decref(parsed_json);
                json_decref(response_json);
                free(user);


                //Send startVoting
                int room_access_id = room_id - 1;
                if (rooms[room_access_id]->player_count >= MAX_PLAYERS) {
                    // Prepare the response for "startVoting"
                    json_t *voting_response_json = json_object();
                    json_t *data_object = json_object(); // New object to hold "data"
                    json_t *categories_array = json_array(); // Array to hold categories

                    // Add categories to the array
                    for (int category_id = 0; category_id < sizeof(categories) / sizeof(categories[0]); category_id++) {
                        if (categories[category_id]) {
                            json_t *category_data = json_object();
                            json_object_set_new(category_data, "id", json_integer(category_id));
                            json_object_set_new(category_data, "value", json_string(categories[category_id]));
                            json_array_append_new(categories_array, category_data);
                        }
                    }

                    // Set the categories inside the "data" object
                    json_object_set_new(data_object, "categories", categories_array);

                    // Set the "action" and "data" fields
                    json_object_set_new(voting_response_json, "action", json_string("startVoting"));
                    json_object_set_new(voting_response_json, "data", data_object);

                    // Serialize the voting response
                    const char *voting_response_str = json_dumps(voting_response_json, 0);
                    if (voting_response_str == NULL) {
                        fprintf(stderr, "Error: Failed to serialize 'startVoting' response to JSON.\n");
                        json_decref(voting_response_json);
                        json_decref(parsed_json);
                        free(user);
                        return -1;
                    }

                    // Send the voting response via WebSocket
                    unsigned char *voting_buf = (unsigned char *)voting_response_str;
                    int len_voting_sent = lws_write(wsi, voting_buf, strlen(voting_response_str), LWS_WRITE_TEXT);
                    if (len_voting_sent < 0) {
                        fprintf(stderr, "Error: Failed to send 'startVoting' message to client\n");
                        json_decref(voting_response_json);
                        json_decref(parsed_json);
                        free(user);
                        return -1;
                    }
                    printf("Started voting");

                    // Clean up voting response JSON
                    json_decref(voting_response_json);
                }

            }




             else if (strcmp(action_str, "vote") == 0) {
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

// Function to initialize and run WebSocket server
int start_server() {
   
    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));
    info.port = GAMEPLAY_PORT;
    info.protocols = (struct lws_protocols[]){{"http", callback_websocket, 0, 0}, {NULL, NULL, 0, 0}};
    struct lws_context *context = lws_create_context(&info);
    if (!context) {
        fprintf(stderr, "Error: Unable to create WebSocket context\n");
        return -1;
    }

    while (1) {
        lws_service(context, 100);
    }
    
    lws_context_destroy(context);
    return 0;
}
