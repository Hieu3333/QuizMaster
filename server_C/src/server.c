#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <json-c/json.h>
#include "auth.h"
#include "mongodb_connect.h"

#define PORT 3000
#define BUFFER_SIZE 4096

// Helper function to format time_t to ISO 8601 format with millisecond precision
void format_time_iso8601(time_t time_val, char *buffer, size_t buffer_size) {
    struct tm *tm_info = gmtime(&time_val); // Use gmtime for UTC time
    strftime(buffer, buffer_size, "%Y-%m-%dT%H:%M:%S", tm_info);
    int milliseconds = (int)(time_val % 1000); // Assuming time_val has millisecond precision

    // Add milliseconds and UTC timezone indicator
    snprintf(buffer + strlen(buffer), buffer_size - strlen(buffer), ".%03dZ", milliseconds);
}

// Utility to send a raw HTTP response
void send_http_response(int client_sock, int status_code, const char *status_text, const char *content_type, const char *body) {
    char response[BUFFER_SIZE];
    const char *response_body = body ? body : "";
    size_t content_length = body ? strlen(body) : 0;

    // Handle OPTIONS request (No Content)
    if (status_code == 204) {
        snprintf(response, sizeof(response),
                 "HTTP/1.1 204 No Content\r\n"
                 "Access-Control-Allow-Origin: *\r\n"
                 "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
                 "Access-Control-Allow-Headers: Content-Type\r\n"
                 "Content-Length: 0\r\n"
                 "\r\n");
        send(client_sock, response, strlen(response), 0);
        return;
    }

    // General HTTP response
    snprintf(response, sizeof(response),
             "HTTP/1.1 %d %s\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %zu\r\n"
             "Access-Control-Allow-Origin: *\r\n"
             "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
             "Access-Control-Allow-Headers: Content-Type\r\n"
             "\r\n"
             "%s",
             status_code, status_text, content_type, content_length, response_body);

    send(client_sock, response, strlen(response), 0);
}


// Helper function to extract the request body from raw HTTP request
char* extract_body(const char *request) {
    const char *body_start = strstr(request, "\r\n\r\n");
    if (body_start) {
        body_start += 4;  // Skip the delimiter
        return strdup(body_start);
    }
    return NULL;
}

// Helper function to handle /api/auth/register
void handle_register(int client_sock, const char *body) {
    // Parse JSON body
    struct json_object *parsed_json = json_tokener_parse(body);
    if (!parsed_json) {
        send_http_response(client_sock, 400, "Bad Request", "application/json", "{\"error\": \"Invalid JSON\"}");
        return;
    }

    struct json_object *username_obj, *password_obj;
    json_object_object_get_ex(parsed_json, "username", &username_obj);
    json_object_object_get_ex(parsed_json, "password", &password_obj);

    const char *username = json_object_get_string(username_obj);
    const char *password = json_object_get_string(password_obj);

    // Check registration logic
    if (register_user(username, password)) {
        send_http_response(client_sock, 201, "Created", "application/json", "{\"message\": \"User created successfully\"}");
        printf("200 GET /api/auth/register");
    } else {
        send_http_response(client_sock, 400, "Bad Request", "application/json", "{\"error\": \"User with this username already exists\"}");
        printf("400 GET /api/auth/register error");
    }

    json_object_put(parsed_json);  // Free parsed JSON object
}

// Helper function to handle /api/auth/login
void handle_login(int client_sock, const char *body) {
    // Parse JSON body
    struct json_object *parsed_json = json_tokener_parse(body);
    if (!parsed_json) {
        send_http_response(client_sock, 400, "Bad Request", "application/json", "{\"error\": \"Invalid JSON\"}");
        return;
    }

    struct json_object *username_obj, *password_obj;
    json_object_object_get_ex(parsed_json, "username", &username_obj);
    json_object_object_get_ex(parsed_json, "password", &password_obj);

    const char *username = json_object_get_string(username_obj);
    const char *password = json_object_get_string(password_obj);

    // Handle login logic
    User *user = login(username, password);

    if (user != NULL) {
        // Convert BSON ObjectId to string
        char id_str[25];
        bson_oid_to_string(&user->id, id_str);

        char created_at_iso[30], updated_at_iso[30];
        format_time_iso8601(user->createdAt, created_at_iso, sizeof(created_at_iso));
        format_time_iso8601(user->updatedAt, updated_at_iso, sizeof(updated_at_iso));

        char user_json[1024];

        // Prepare JSON response
        char response[1024];
        snprintf(user_json, sizeof(user_json),
                 "{"
                 "\"id\": \"%s\", "
                 "\"username\": \"%s\", "
                 "\"password\": \"%s\", "  // Consider removing password in production
                 "\"wins\": %d, "
                 "\"totalScore\": %d, "
                 "\"playedGames\": %d, "
                 "\"createdAt\": \"%s\", "
                 "\"updatedAt\": \"%s\""
                 "}",
                 id_str, user->username, user->password,
                 user->wins, user->totalScore, user->playedGames,
                 created_at_iso, updated_at_iso);

        send_http_response(client_sock, 200, "OK", "application/json", user_json);
        printf("200 GET /api/auth/login");
        free(user);  // Free the user object
    } else {
        send_http_response(client_sock, 400, "Bad Request", "application/json", "{\"error\": \"Invalid username or password\"}");
        printf("400 GET /api/auth/login");
    }

    json_object_put(parsed_json);  // Free parsed JSON object
}




// Function to handle /api/users/leaderboard/:quantity request
void handle_leaderboard(int client_sock, const char *quantity_str) {
   
    // Convert quantity parameter to integer
    int quantity = atoi(quantity_str);
    if (quantity <= 0) {
        send_http_response(client_sock, 400, "Bad Request", "application/json", "{\"error\": \"Invalid quantity \"}");
        printf("400 GET /api/users/leaderboard error");
        return;
    }

    // Fetch leaderboard data from MongoDB
    int result_size = 0;
    User* leaderboard = get_leaderboard(quantity, &result_size);

    if (leaderboard == NULL || result_size == 0) {
        send_http_response(client_sock, 400, "Bad Request", "application/json", "{\"error\": \"Invalid quantity\"}");
        printf("400 GET /api/users/leaderboard error");
        return;
    }

    // Build the JSON response
    char response[BUFFER_SIZE];
    char user_json[BUFFER_SIZE];
    user_json[0] = '\0';  // Initialize as empty string

    // Iterate over the leaderboard and build JSON for each user
    for (int i = 0; i < result_size; i++) {
        char user_details[512];
        
        // Convert BSON OID to string for user ID
        char user_id_str[25];  // BSON OID is 24 bytes, plus null terminator
        bson_oid_to_string(&leaderboard[i].id, user_id_str);

        // Build user details JSON
        snprintf(user_details, sizeof(user_details),
                 "{\"id\": \"%s\", \"username\": \"%s\", \"totalScore\": %d, \"wins\": %d, \"playedGames\": %d}",
                 user_id_str, leaderboard[i].username, leaderboard[i].totalScore,
                 leaderboard[i].wins, leaderboard[i].playedGames);

        // Ensure that user_json is not overflowing
        if (strlen(user_json) + strlen(user_details) + 1 < sizeof(user_json)) {
            strcat(user_json, user_details);
        } else {
            fprintf(stderr, "Buffer overflow detected while building user JSON\n");
            break;
        }

        if (i < result_size - 1) {
            strcat(user_json, ",");
        }
    }

    // Format the entire leaderboard JSON
    snprintf(response, sizeof(response),
             "[%s]",
             user_json);

    // Send the HTTP response with the leaderboard
    send_http_response(client_sock, 200, "OK", "application/json", response);
    printf("200 GET /api/users/leaderboard ");

    // Free the leaderboard memory
    free(leaderboard);
}

void handle_get_user_by_id(int client_sock, const char *id_str) {
    // Get the user by ID
    User *user = get_one_by_id(id_str);

    if (user != NULL) {
        // Convert BSON ObjectId to string
        char user_id_str[25];
        bson_oid_to_string(&user->id, user_id_str);

        char created_at_iso[30], updated_at_iso[30];
        format_time_iso8601(user->createdAt, created_at_iso, sizeof(created_at_iso));
        format_time_iso8601(user->updatedAt, updated_at_iso, sizeof(updated_at_iso));

        char user_json[1024];

        // Prepare JSON response
        char response[1024];
        snprintf(user_json, sizeof(user_json),
                 "{"
                 "\"id\": \"%s\", "
                 "\"username\": \"%s\", "
                 "\"password\": \"%s\", "  // Consider removing password in production
                 "\"wins\": %d, "
                 "\"totalScore\": %d, "
                 "\"playedGames\": %d, "
                 "\"createdAt\": \"%s\", "
                 "\"updatedAt\": \"%s\""
                 "}",
                 id_str, user->username, user->password,
                 user->wins, user->totalScore, user->playedGames,
                 created_at_iso, updated_at_iso);

        send_http_response(client_sock, 200, "OK", "application/json", user_json);

        // Free the user object
        free(user->username);
        free(user->password);
        free(user);
    } else {
        send_http_response(client_sock, 404, "Not Found", "application/json", "{\"error\": \"User not found\"}");
    }
}

// Helper function to handle /api/users/:id (PATCH) request
void handle_update_user(int client_sock, const char *id_str, const char *body) {
    // Parse the request body
    struct json_object *parsed_json = json_tokener_parse(body);
    if (!parsed_json) {
        send_http_response(client_sock, 400, "Bad Request", "application/json", "{\"error\": \"Invalid JSON\"}");
        return;
    }

    struct json_object *wins_obj, *totalScore_obj, *playedGames_obj;
    int wins = -1, totalScore = -1, playedGames = -1;

    // Extract fields from JSON, if present
    if (json_object_object_get_ex(parsed_json, "wins", &wins_obj)) {
        wins = json_object_get_int(wins_obj);
    }
    if (json_object_object_get_ex(parsed_json, "totalScore", &totalScore_obj)) {
        totalScore = json_object_get_int(totalScore_obj);
    }
    if (json_object_object_get_ex(parsed_json, "playedGames", &playedGames_obj)) {
        playedGames = json_object_get_int(playedGames_obj);
    }

    // Validate the extracted values (optional, adjust based on your requirements)
    if (wins < 0 || totalScore < 0 || playedGames < 0) {
        send_http_response(client_sock, 400, "Bad Request", "application/json", "{\"error\": \"Invalid or missing fields\"}");
        json_object_put(parsed_json);  // Free parsed JSON object
        return;
    }

    // Convert the user ID from string to bson_oid_t
    bson_oid_t user_id;

    // Update user data
    update_user(&user_id, wins, totalScore, playedGames);

    // Prepare success response
    send_http_response(client_sock, 200, "OK", "application/json", "{\"message\": \"User updated successfully\"}");

    json_object_put(parsed_json);  // Free parsed JSON object
}

// Function to handle client requests
void handle_client(int client_sock) {
    char buffer[BUFFER_SIZE];
    ssize_t received = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
    if (received <= 0) {
        close(client_sock);
        return;
    }
    buffer[received] = '\0';  // Null-terminate the buffer

    // Extract the request method and path
    char method[16], path[128];
    sscanf(buffer, "%15s %127s", method, path);

    // Extract the body from the request
    char *body = extract_body(buffer);

    // Route the request based on the path
    if (strcmp(path, "/api/auth/register") == 0 && strcmp(method, "POST") == 0) {
        handle_register(client_sock, body);
    } else if (strcmp(path, "/api/auth/login") == 0 && strcmp(method, "POST") == 0) {
        handle_login(client_sock, body);
    } else if (strncmp(path, "/api/users/leaderboard/", 23) == 0 && strcmp(method, "GET") == 0) {
        // Extract the quantity parameter from the URL
        char *quantity_str = path + 23; // Skip "/api/users/leaderboard/"
        handle_leaderboard(client_sock, quantity_str);
    } else if (strncmp(path, "/api/users/", 11) == 0 && strcmp(method, "GET") == 0) {
        // Handle GET /api/users/:id request
        char *user_id_str = path + 11; // Skip "/api/users/"
        handle_get_user_by_id(client_sock,user_id_str);
           
    } else if (strncmp(path, "/api/users/", 11) == 0 && strcmp(method, "PATCH") == 0) {
        // Handle PATCH /api/users/:id request
        char *user_id_str = path + 11; // Skip "/api/users/"
        handle_update_user(client_sock, user_id_str, body);
    } 
    else {
        send_http_response(client_sock, 404, "Not Found", "application/json", "{\"error\": \"Not Found\"}");
    }

    free(body);  // Free the body if allocated
    close(client_sock);  // Close the client connection
}


int main() {
    // Initialize MongoDB connection
    init_mongo();

    // Create a socket
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Bind the socket to the port
    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_sock, 5) < 0) {
        perror("Listen failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    // Accept and handle incoming connections
    while (1) {
        int client_sock = accept(server_sock, NULL, NULL);
        if (client_sock < 0) {
            perror("Accept failed");
            continue;
        }
        handle_client(client_sock);
    }

    // Clean up
    close(server_sock);
    cleanup_mongo();
    return 0;
}
