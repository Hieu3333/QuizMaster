#include "mongoose.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <json-c/json.h>
#include "auth.h"
#include "mongodb_connect.h"

#define PORT "3000"

// Utility to send JSON response
void send_json_response(struct mg_connection *c, int status_code, const char *json_response) {
    mg_http_reply(c, status_code, "Content-Type: application/json\r\n", "%s", json_response);
}

// Helper function to convert mg_str to a null-terminated C string
char* mg_str_to_cstr(struct mg_str s) {
    char *cstr = (char*) malloc(s.len + 1);  // Allocate memory for the C string
    if (cstr == NULL) return NULL;
    memcpy(cstr, s.buf, s.len);
    cstr[s.len] = '\0';  // Null-terminate the string
    return cstr;
}

// Handle the /api/auth/register endpoint
void handle_register(struct mg_connection *c, struct mg_http_message *hm) {
    // Convert the mg_str body to a C string
    char *body = mg_str_to_cstr(hm->body);
    if (!body) {
        send_json_response(c, 500, "{\"error\": \"Server error\"}");
        return;
    }

    // Parse JSON body
    struct json_object *parsed_json = json_tokener_parse(body);
    free(body);  // Free the body after parsing
    if (!parsed_json) {
        send_json_response(c, 400, "{\"error\": \"Invalid JSON\"}");
        return;
    }

    struct json_object *username_obj, *password_obj;
    json_object_object_get_ex(parsed_json, "username", &username_obj);
    json_object_object_get_ex(parsed_json, "password", &password_obj);

    const char *username = json_object_get_string(username_obj);
    const char *password = json_object_get_string(password_obj);

    // Check registration logic
    if (register_user(username, password)) {
        send_json_response(c, 201, "{\"message\": \"User created successfully\"}");
    } else {
        send_json_response(c, 400, "{\"error\": \"User with this username already exists\"}");
    }

    json_object_put(parsed_json);  // Free parsed JSON object
}

// Handle the /api/auth/login endpoint
void handle_login(struct mg_connection *c, struct mg_http_message *hm) {
    // Convert the mg_str body to a C string
    char *body = mg_str_to_cstr(hm->body);
    if (!body) {
        send_json_response(c, 500, "{\"error\": \"Server error\"}");
        return;
    }

    // Parse JSON body
    struct json_object *parsed_json = json_tokener_parse(body);
    free(body);  // Free the body after parsing
    if (!parsed_json) {
        send_json_response(c, 400, "{\"error\": \"Invalid JSON\"}");
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
        // User successfully logged in
        char user_json[512];
        snprintf(user_json, sizeof(user_json), "{\"id\": \"%s\", \"username\": \"%s\"}", user->id, user->username);

        send_json_response(c, 200, user_json);
        free(user);  // Free the user after sending response
    } else {
        send_json_response(c, 400, "{\"error\": \"Invalid username or password\"}");
    }

    json_object_put(parsed_json);  // Free parsed JSON object
}

// HTTP request handler
#include <string.h>  // For strcmp function

// HTTP request handler
void handle_request(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
    struct mg_http_message *hm = (struct mg_http_message *) ev_data;

    if (ev == MG_EV_HTTP_MSG) {
        // Compare the URI of the HTTP request with the specific endpoints
        if (strncmp(hm->uri.buf, "/api/auth/register", hm->uri.len) == 0) {
            handle_register(c, hm);
        } else if (strncmp(hm->uri.buf, "/api/auth/login", hm->uri.len) == 0) {
            handle_login(c, hm);
        } else {
            send_json_response(c, 404, "{\"error\": \"Not Found\"}");
        }
    }
}


int main() {
    struct mg_mgr mgr;
    mg_mgr_init(&mgr);
    printf("Starting Mongoose server on port %s...\n", PORT);

    // Initialize MongoDB connection
    init_mongo();

    // Start Mongoose event loop
    mg_http_listen(&mgr, "http://localhost:" PORT, handle_request, &mgr);

    for (;;) {
        mg_mgr_poll(&mgr, 1000);  // Infinite loop to keep server running
    }

    mg_mgr_free(&mgr);
    cleanup_mongo();  // Clean up MongoDB connection
    return 0;
}
