#include "user.h"
#include "mongodb_connect.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <mongoc/mongoc.h>
#include <json-c/json.h>

// Helper function to create a User object from a JSON object
User* json_to_user(const json_object *obj) {
    User *user = malloc(sizeof(User));
    if (!user) {
        return NULL;
    }

    // Convert JSON fields to User struct fields
    json_object *id_obj, *username_obj, *password_obj, *wins_obj, *total_score_obj, *played_games_obj, *created_at_obj, *updated_at_obj;

    if (json_object_object_get_ex(obj, "_id", &id_obj)) {
        strncpy(user->id, json_object_get_string(id_obj), sizeof(user->id) - 1);
    }

    if (json_object_object_get_ex(obj, "username", &username_obj)) {
        strncpy(user->username, json_object_get_string(username_obj), sizeof(user->username) - 1);
    }

    if (json_object_object_get_ex(obj, "password", &password_obj)) {
        strncpy(user->password, json_object_get_string(password_obj), sizeof(user->password) - 1);
    }

    if (json_object_object_get_ex(obj, "wins", &wins_obj)) {
        user->wins = json_object_get_int(wins_obj);
    }

    if (json_object_object_get_ex(obj, "totalScore", &total_score_obj)) {
        user->totalScore = json_object_get_int(total_score_obj);
    }

    if (json_object_object_get_ex(obj, "playedGames", &played_games_obj)) {
        user->playedGames = json_object_get_int(played_games_obj);
    }

    if (json_object_object_get_ex(obj, "createdAt", &created_at_obj)) {
        user->createdAt = json_object_get_int64(created_at_obj) / 1000;
    }

    if (json_object_object_get_ex(obj, "updatedAt", &updated_at_obj)) {
        user->updatedAt = json_object_get_int64(updated_at_obj) / 1000;
    }

    return user;
}

// Function to get a user by their ID (returns a User object)
User* get_one_by_id(const char *id) {
    mongoc_client_t *client = get_mongo_client();
    mongoc_collection_t *collection = mongoc_client_get_collection(client, "quizmaster", "User");

    bson_t *query = bson_new();
    BSON_APPEND_UTF8(query, "_id", id);

    mongoc_cursor_t *cursor = mongoc_collection_find_with_opts(collection, query, NULL, NULL);
    const bson_t *doc = NULL;
    if (mongoc_cursor_next(cursor, &doc)) {
        // Convert BSON to JSON
        char *json_str = bson_as_json(doc, NULL);
        json_object *json_obj = json_tokener_parse(json_str);
        bson_destroy(query);
        mongoc_cursor_destroy(cursor);
        mongoc_collection_destroy(collection);
        free(json_str);

        // Convert JSON object to User struct and return
        User *user = json_to_user(json_obj);
        json_object_put(json_obj); // Free JSON object
        return user;
    }

    bson_destroy(query);
    mongoc_cursor_destroy(cursor);
    mongoc_collection_destroy(collection);
    return NULL;
}

// Function to get a user by their username (returns a User object)
User* get_one_by_username(const char *username) {
    mongoc_client_t *client = get_mongo_client();
    mongoc_collection_t *collection = mongoc_client_get_collection(client, "quizmaster", "User");

    bson_t *query = bson_new();
    BSON_APPEND_UTF8(query, "username", username);

    mongoc_cursor_t *cursor = mongoc_collection_find_with_opts(collection, query, NULL, NULL);
    const bson_t *doc = NULL;
    if (mongoc_cursor_next(cursor, &doc)) {
        // Convert BSON to JSON
        char *json_str = bson_as_json(doc, NULL);
        json_object *json_obj = json_tokener_parse(json_str);
        bson_destroy(query);
        mongoc_cursor_destroy(cursor);
        mongoc_collection_destroy(collection);
        free(json_str);

        // Convert JSON object to User struct and return
        User *user = json_to_user(json_obj);
        json_object_put(json_obj); // Free JSON object
        return user;
    }

    bson_destroy(query);
    mongoc_cursor_destroy(cursor);
    mongoc_collection_destroy(collection);
    return NULL;
}

// Function to check if a user with the given ID exists
bool persists_by_id(const char *id) {
    User *user = get_one_by_id(id);
    bool exists = user != NULL;
    if (user) free(user); // Free User object
    return exists;
}

// Function to check if a user with the given username exists
bool persists_by_username(const char *username) {
    User *user = get_one_by_username(username);
    bool exists = user != NULL;
    if (user) free(user); // Free User object
    return exists;
}

// Function to add a new user
void add_user(const char *username, const char *password) {
    mongoc_client_t *client = get_mongo_client();
    mongoc_collection_t *collection = mongoc_client_get_collection(client, "quizmaster", "User");

    // Create a new JSON object for the new user
    json_object *new_user = json_object_new_object();
    json_object_object_add(new_user, "username", json_object_new_string(username));
    json_object_object_add(new_user, "password", json_object_new_string(password));
    json_object_object_add(new_user, "wins", json_object_new_int(0));
    json_object_object_add(new_user, "totalScore", json_object_new_int(0));
    json_object_object_add(new_user, "playedGames", json_object_new_int(0));
    json_object_object_add(new_user, "createdAt", json_object_new_int64((int64_t)time(NULL) * 1000));
    json_object_object_add(new_user, "updatedAt", json_object_new_int64((int64_t)time(NULL) * 1000));

    // Convert JSON to BSON for MongoDB insertion
    const char *json_str = json_object_to_json_string(new_user);
    bson_t *bson_data = bson_new_from_json((const uint8_t *)json_str, strlen(json_str), NULL);

    // Insert the user into the MongoDB collection
    mongoc_collection_insert_one(collection, bson_data, NULL, NULL, NULL);

    // Cleanup
    bson_destroy(bson_data);
    json_object_put(new_user);
    mongoc_collection_destroy(collection);
}

// Function to update a user's data (wins, totalScore, playedGames)
void update_user(const char *id, int wins, int totalScore, int playedGames) {
    mongoc_client_t *client = get_mongo_client();
    mongoc_collection_t *collection = mongoc_client_get_collection(client, "quizmaster", "User");

    // Create the query document to find the user by ID
    bson_t *query = bson_new();
    BSON_APPEND_UTF8(query, "_id", id);

    // Create the update document with $set
    bson_t *update = bson_new();
    bson_t child;  // Create a new bson_t object
    BSON_APPEND_DOCUMENT_BEGIN(update, "$set", &child);  // Pass the new bson_t object


    // Add fields to the update document manually
    BSON_APPEND_INT32(update, "wins", wins);
    BSON_APPEND_INT32(update, "totalScore", totalScore);
    BSON_APPEND_INT32(update, "playedGames", playedGames);
    BSON_APPEND_DATE_TIME(update, "updatedAt", (int64_t)time(NULL) * 1000);

    // Perform the update operation
    mongoc_collection_update_one(collection, query, update, NULL, NULL, NULL);

    // Cleanup resources
    bson_destroy(query);
    bson_destroy(update);
    mongoc_collection_destroy(collection);
}

// Function to delete a user by their id
void delete_user(const char *id) {
    mongoc_client_t *client = get_mongo_client();
    mongoc_collection_t *collection = mongoc_client_get_collection(client, "quizmaster", "User");

    // Create query to find the user by ID
    bson_t *query = bson_new();
    BSON_APPEND_UTF8(query, "_id", id);

    // Delete the user from the collection
    mongoc_collection_delete_one(collection, query, NULL, NULL, NULL);

    // Cleanup
    bson_destroy(query);
    mongoc_collection_destroy(collection);
}
