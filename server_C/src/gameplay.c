#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libwebsockets.h>
#include <jansson.h>
#include <bson.h>
#include "user.h"
#include "gameplay.h"
#include <curl/curl.h>
#include <jansson.h>
#include <b64/cdecode.h>


typedef struct {
    const char *id;
    const char *name;
} Category;

// Create an array of categories with string IDs
Category categories[] = {
    {"9", "General Knowledge"},
    {"11", "Films"},
    {"12", "Music"},
    {"15", "Videogames"},
    {"22", "Geography"},
    {"23", "History"}
};

// Global variables
Room* rooms[MAX_ROOMS];
int room_count = 0;
Question questions[MAX_QUESTIONS];

typedef struct ConnectedUser {
    struct lws *wsi; // WebSocket instance
    int id;
} ConnectedUser;

ConnectedUser *connected_users[MAX_USERS];
int connected_user_count = 0;
int next_user_id = 1; 

typedef struct {
    int category;             // Question category
    Question *questions;      // Array to store fetched questions
    int *result;              // Pointer to store the number of fetched questions
} FetchQuestionsArgs;

#define BUFFER_SIZE 4096

// Function to send JSON message to all users
void send_message_to_all_users(json_t *response_json, char *action) {
    // Serialize the JSON object to a string
    const char *response_str = json_dumps(response_json, 0);
    printf("%s\n",response_str);
    if (response_str == NULL) {
        fprintf(stderr, "Error: Failed to serialize response to JSON.\n");
        json_decref(response_json);
        return;
    }

    // Get the length of the serialized JSON string
    size_t response_len = strlen(response_str);

    // Allocate buffer with LWS_PRE padding
    unsigned char *buf = malloc(LWS_PRE + response_len);
    if (!buf) {
        fprintf(stderr, "Error: Memory allocation failed.\n");
        free((void *)response_str);
        json_decref(response_json);
        return;
    }

    // Fill the buffer with the serialized JSON message
    memcpy(&buf[LWS_PRE], response_str, response_len);

    // Send the message to each connected user
    for (int i = 0; i < connected_user_count; i++) {
        struct lws *current_wsi = connected_users[i]->wsi; // Get the WebSocket instance
        if (current_wsi == NULL) {
            printf("User %d is invalid, cant send message",connected_users[i]->id);
            continue; // Skip invalid WebSocket instances
        }

        // Send the message via WebSocket
        int len_voting_sent = lws_write(current_wsi, &buf[LWS_PRE], response_len, LWS_WRITE_TEXT);
        if (len_voting_sent < 0) {
            fprintf(stderr, "Error: Failed to send 'startMatch' message to client with ID %d\n", connected_users[i]->id);
        } else {
            printf("Sent %s message to client %d\n", action, connected_users[i]->id);
        }
    }

    // Cleanup
    free(buf);
    free((void *)response_str);
    json_decref(response_json);
}

// Write callback function to store response data
size_t write_callback(void *ptr, size_t size, size_t nmemb, char *data) {
    size_t total_size = size * nmemb;
    if (total_size + strlen(data) < BUFFER_SIZE) {
        strncat(data, (char *)ptr, total_size);
    } else {
        fprintf(stderr, "Buffer overflow detected in write_callback\n");
        return 0;
    }
    return total_size;
}


// Function to decode a base64 string
void decode_base64(const char *input, char *output) {
    base64_decodestate state;
    base64_init_decodestate(&state);
    int output_length = base64_decode_block(input, strlen(input), output, &state);
    output[output_length] = '\0';
}


void fetch_questions_thread(void *args) {
    FetchQuestionsArgs *fetch_args = (FetchQuestionsArgs *)args;
    int category = fetch_args->category;
    Question *questions = fetch_args->questions;
    int *result = fetch_args->result;

    CURL *curl;
    CURLcode res;
    char api_url[] = "https://opentdb.com/api.php";
    char data[BUFFER_SIZE] = "";  // To store the response data
    char url[512];

    // Initialize the URL for the API request
    snprintf(url, sizeof(url), "%s?amount=7&category=%d&difficulty=easy&type=multiple&encode=base64", api_url, category);

    // Initialize CURL
    curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Error: Failed to initialize CURL.\n");
        *result = -1;
        pthread_exit(NULL);
    }

    // Set options for the request
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, data);

    // Perform the request
    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "CURL error: %s\n", curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        *result = -1;
        pthread_exit(NULL);
    }

    // Parse the JSON response
    json_error_t error;
    json_t *root = json_loads(data, 0, &error);
    if (!root) {
        fprintf(stderr, "Error parsing JSON response: %s\n", error.text);
        curl_easy_cleanup(curl);
        *result = -1;
        pthread_exit(NULL);
    }

    json_t *results = json_object_get(root, "results");
    if (!json_is_array(results)) {
        fprintf(stderr, "Error: 'results' key is not an array.\n");
        json_decref(root);
        curl_easy_cleanup(curl);
        *result = -1;
        pthread_exit(NULL);
    }

    // Parse each question
    size_t i;
    size_t question_count = json_array_size(results);
    for (i = 0; i < question_count; i++) {
        json_t *question_data = json_array_get(results, i);

        // Allocate memory for each question, choices, and correctAnswer
        questions[i].question = (char *)malloc(500 * sizeof(char));
        questions[i].correctAnswer = (char *)malloc(256 * sizeof(char));
        for (int j = 0; j < 4; j++) {
            questions[i].choices[j] = (char *)malloc(256 * sizeof(char));
            if (questions[i].choices[j] == NULL) {
                fprintf(stderr, "Memory allocation failed for choice %d\n", j);
                *result = -1;
                pthread_exit(NULL);
            }
        }

        if (questions[i].question == NULL || questions[i].correctAnswer == NULL) {
            fprintf(stderr, "Memory allocation failed for question or correctAnswer\n");
            *result = -1;
            pthread_exit(NULL);
        }

        // Decode question and correct answer from base64
        const char *encoded_question = json_string_value(json_object_get(question_data, "question"));
        const char *encoded_correct_answer = json_string_value(json_object_get(question_data, "correct_answer"));
        json_t *incorrect_answers = json_object_get(question_data, "incorrect_answers");

        decode_base64(encoded_question, questions[i].question);
        decode_base64(encoded_correct_answer, questions[i].correctAnswer);

        // Decode choices (include correct answer in the choices)
        size_t j;
        int choice_index = 0;
        json_t *answer;
        json_array_foreach(incorrect_answers, j, answer) {
            decode_base64(json_string_value(answer), questions[i].choices[choice_index]);
            choice_index++;
        }

        // Add correct answer as the last choice
        decode_base64(encoded_correct_answer, questions[i].choices[choice_index]);

        // Print out the question, choices, and correct answer after decoding
        printf("Question %zu: %s\n", i + 1, questions[i].question);
        printf("Choices:\n");
        for (int j = 0; j < 4; j++) {
            printf("  Choice %d: %s\n", j + 1, questions[i].choices[j]);
        }
        printf("Correct Answer: %s\n\n", questions[i].correctAnswer);
    }

    // Clean up
    json_decref(root);
    curl_easy_cleanup(curl);

    *result = question_count;  // Set the result to the number of fetched questions
    pthread_exit(NULL);
}



// Add a new user to the connected users array
void add_websocket_user(struct lws *wsi) {
    if (connected_user_count >= MAX_USERS) {
        printf("Error: Maximum number of users reached.\n");
        return;
    }

    // Create a new connected user entry
    ConnectedUser *new_user = (ConnectedUser *)malloc(sizeof(ConnectedUser));
    if (new_user == NULL) {
        printf("Error: Memory allocation failed for new user.\n");
        return;
    }

    // Assign the WebSocket instance and a unique ID
    new_user->wsi = wsi;
    new_user->id = next_user_id++;

    // Store the user in the connected_users array
    connected_users[connected_user_count++] = new_user;
    printf("User with ID %d connected. Total connected users: %d\n", new_user->id, connected_user_count);
}

// Remove a user from the connected users array
void remove_websocket_user(struct lws *wsi) {
    for (int i = 0; i < connected_user_count; i++) {
        if (connected_users[i]->wsi == wsi) {
            printf("User with ID %d disconnected.\n", connected_users[i]->id);

            // Free the memory for the user and shift the array
            free(connected_users[i]);
            for (int j = i; j < connected_user_count - 1; j++) {
                connected_users[j] = connected_users[j + 1];
            }
            connected_user_count--;
            return;
        }
    }
    printf("Error: User not found for disconnection.\n");
}


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
    new_room->vote_count = 0;
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

        case LWS_CALLBACK_ESTABLISHED: {
            printf("New client connected!\n");
            add_websocket_user(wsi);
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
            json_t *message_json = json_loads(msg, 0, &error);

            if (!message_json) {
                fprintf(stderr, "Error: Failed to parse JSON. Raw message: %s\n", msg);
                return -1;
            }

            // Extract the 'action' field from the parsed JSON
            json_t *action = json_object_get(message_json, "action");
            if (!action || !json_is_string(action)) {
                fprintf(stderr, "Error: Key 'action' not found or is not a string in JSON.\n");
                json_decref(message_json);  // Free JSON object
                return -1;
            }

            const char *action_str = json_string_value(action);
            if (action_str == NULL) {
                fprintf(stderr, "Error: 'action' value is not a string or is null.\n");
                json_decref(message_json);  // Free JSON object
                return -1;
            }

            if (strcmp(action_str, "findMatch") == 0) {
                // Handle "findMatch" logic
                printf("findMatch action received\n");

                // Extract 'data' field from JSON
                json_t *user_json = json_object_get(message_json, "data");
                if (!user_json || !json_is_object(user_json)) {
                    fprintf(stderr, "Error: Key 'data' not found or is not an object in JSON.\n");
                    json_decref(message_json);  // Free JSON object
                    return -1;
                }

                // Assuming extract_user_from_data is a function that handles the user extraction
                User* user = extract_user_from_data(user_json);
                if (user == NULL) {
                    fprintf(stderr, "Error: Failed to extract user from 'data'.\n");
                    json_decref(message_json);  // Free JSON object
                    free(user);
                    return -1;
                }

                // Find or create a room for the user
                int room_id = find_room_for_user(user);
                if (room_id < 0) {
                    fprintf(stderr, "Error: Failed to find or create room for user.\n");
                    json_decref(message_json);  // Free JSON object
                    free(user);  // Free the user object
                    return -1;
                }

                // Prepare the response JSON object
                json_t *response_json = json_object();
                json_t *room_data = json_object();

                if (room_id <= room_count) {
                    // Room was found or created, create the response accordingly
                    json_object_set_new(room_data, "roomId", json_integer(room_id));
                    json_object_set_new(room_data, "playerName", json_string(user->username));
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

                send_message_to_all_users(response_json,"joinRoom");
                free(user);

                


                //Send startVoting
                int room_access_id = room_id - 1;
                if (rooms[room_access_id]->player_count >= MAX_PLAYERS) {
                    // Prepare the response for "startVoting"
                    json_t *voting_response_json = json_object();
                    json_t *data_object = json_object(); // New object to hold "data"
                    json_t *categories_array = json_array(); // Array to hold categories
     

                    // Add categories to the array
                    for (int i = 0; i < sizeof(categories) / sizeof(categories[0]); i ++) {
                        // Create a category object
                        json_t *category_data = json_object();
                        
                        // Use the string IDs for "id" (e.g., "9", "11")
                        json_object_set_new(category_data, "id", json_string(categories[i].id)); // Set "id"
                        json_object_set_new(category_data, "name", json_string(categories[i].name)); // Set "name"

                        

                        // Add the category object to the categories array
                        json_array_append_new(categories_array, category_data);
                    }

                    // Set the categories inside the "data" object
                    json_object_set_new(data_object, "categories", categories_array);

                    // Set the "action" and "data" fields
                    json_object_set_new(voting_response_json, "action", json_string("startVoting"));
                    json_object_set_new(voting_response_json, "data", data_object);

                    send_message_to_all_users(voting_response_json,"startVoting");
                }

            }

            if (strcmp(action_str, "vote") == 0) {
            // Handle "vote" logic
            printf("Vote action received\n");

            // Extract 'data' field from JSON
            json_t *vote_data_json = json_object_get(message_json, "data");
            if (!vote_data_json || !json_is_object(vote_data_json)) {
                fprintf(stderr, "Error: Key 'data' not found or is not an object in JSON.\n");
                json_decref(message_json);  // Free JSON object
                return -1;
            }

            // Extract category and playerId from data
            const char *category_name = json_string_value(json_object_get(vote_data_json, "category"));
            const char *player_id_str = json_string_value(json_object_get(vote_data_json, "playerId"));

            if (!category_name || !player_id_str) {
                fprintf(stderr, "Error: Missing 'category' or 'playerId' in vote data.\n");
                json_decref(message_json);  // Free JSON object
                return -1;
            }

            // Find the player based on playerId
            User *voting_player = NULL;
            bson_oid_t player_oid;
            bson_oid_init_from_string(&player_oid, player_id_str);  
            int current_room_id;

            for (int i = 0; i < room_count; i++) {
                for (int j = 0; j < rooms[i]->player_count; j++) {
                    User *player = &rooms[i]->players[j];
                    if (bson_oid_compare(&player->id, &player_oid) == 0) {
                        voting_player = player;
                        current_room_id = i;
                        rooms[i]->votes[rooms[i]->vote_count++] = atoi(category_name);
                        break;
                    }
                }
                if (voting_player) {
                    break;
                }
            }

            if (!voting_player) {
                fprintf(stderr, "Error: Player with ID %s not found.\n", player_id_str);
                json_decref(message_json);  // Free JSON object
                return -1;
            }

            // Process the vote
            printf("Player %s voted for category '%s'\n", voting_player->username, category_name);
            
            // Check if the votes have reached MAX_PLAYERS
            if (rooms[current_room_id]->vote_count == MAX_PLAYERS) {
                // Find the category with the maximum votes
                rooms[current_room_id]->isOngoing=1;
                int category_votes[100] = {0};  // Assuming there are 100 possible categories
                for (int i = 0; i < rooms[current_room_id]->vote_count; i++) {
                    category_votes[rooms[current_room_id]->votes[i]]++;
                }

                int max_votes = 0;
                int selected_category = -1;
                for (int i = 0; i < 100; i++) {
                    if (category_votes[i] > max_votes) {
                        max_votes = category_votes[i];
                        selected_category = i;
                    }
                }

                printf("Category %d has the most votes (%d votes)\n", selected_category, max_votes);

                int result = 0;         // Number of fetched questions

                // Create thread arguments
                FetchQuestionsArgs fetch_args;
                fetch_args.category = selected_category;
                fetch_args.questions = questions;
                fetch_args.result = &result;

                // Create a thread to fetch questions
                pthread_t thread_id;
                if (pthread_create(&thread_id, NULL, fetch_questions_thread, &fetch_args) != 0) {
                    fprintf(stderr, "Error: Failed to create thread.\n");
                    return -1;
                }

                // Wait for the thread to finish
                if (pthread_join(thread_id, NULL) != 0) {
                    fprintf(stderr, "Error: Failed to join thread.\n");
                    return -1;
                }

                // Check the result
                if (result > 0) {
                    printf("Fetched %d questions successfully:\n", result);
                    // Prepare the message to send back to the client
                    json_t *response_json = json_object();
                    json_object_set_new(response_json, "action", json_string("startMatch"));
                    json_object_set_new(response_json, "category", json_string(category_name));

                    // Send the first question as part of the data
                    json_t *first_question_json = json_object();
                    json_object_set_new(first_question_json, "question", json_string(questions[0].question));
                    json_t *choices_json = json_array();
                    for (int j = 0; j < 4; j++) {
                        json_array_append_new(choices_json, json_string(questions[0].choices[j]));
                    }
                    json_object_set_new(first_question_json, "choices", choices_json);
                    json_object_set_new(response_json, "firstQuestion", first_question_json);

                    send_message_to_all_users(response_json,"startMatch");
                } else {
                    fprintf(stderr, "Failed to fetch questions.\n");
                }
            }
        }



              else if (strcmp(action_str, "answer") == 0) {
                // Handle "answer" logic
                printf("answer action received\n");
                // Evaluate answer and manage scoring

            } else {
                // If action is unrecognized, respond with an error
                printf("Unknown action received: %s\n", action_str);
            }

            // Free the parsed JSON object after usage
            json_decref(message_json);

            break;
        }



                case LWS_CALLBACK_CLOSED: {
                    printf("Client disconnected!\n");
                    // When a client disconnects
                    remove_websocket_user(wsi);
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
