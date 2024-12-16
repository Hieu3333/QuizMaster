#include "user.h"
#include "mongodb_connect.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <mongoc/mongoc.h>
#include <json-c/json.h>
#include <bson/bson.h>/

// Helper function to create a User object from a JSON object
User* json_to_user(const json_object *obj) {
    User *user = malloc(sizeof(User));
    if (!user) {
        return NULL;
    }

    // Convert JSON fields to User struct fields
    json_object *id_obj, *username_obj, *password_obj, *wins_obj, *total_score_obj, *played_games_obj, *created_at_obj, *updated_at_obj;

    if (json_object_object_get_ex(obj, "_id", &id_obj)) {
        const char *id_str = json_object_get_string(id_obj);
        bson_oid_init_from_string(&user->id, id_str); // Convert the string to bson_oid_t
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

User* get_one_by_id(bson_oid_t *id) {
    mongoc_client_t *client = get_mongo_client();
    mongoc_collection_t *collection = mongoc_client_get_collection(client, "quizmaster", "User");

    if (!client || !collection) {
        fprintf(stderr, "Failed to connect to MongoDB.\n");
        return NULL;
    }

    bson_t *query = bson_new();
    BSON_APPEND_OID(query, "_id", id);  // Query by ObjectId

    mongoc_cursor_t *cursor = mongoc_collection_find_with_opts(collection, query, NULL, NULL);
    const bson_t *doc = NULL;

    if (mongoc_cursor_next(cursor, &doc)) {
        User *user = (User *)malloc(sizeof(User));
        if (!user) {
            fprintf(stderr, "Memory allocation failed.\n");
            bson_destroy(query);
            mongoc_cursor_destroy(cursor);
            mongoc_collection_destroy(collection);
            return NULL;
        }

        bson_iter_t iter;
        if (bson_iter_init_find(&iter, doc, "_id")) {
            bson_oid_copy(bson_iter_oid(&iter), &user->id);
        }
        if (bson_iter_init_find(&iter, doc, "username")) {
            user->username = strdup(bson_iter_utf8(&iter, NULL));
        }
        if (bson_iter_init_find(&iter, doc, "password")) {
            user->password = strdup(bson_iter_utf8(&iter, NULL));
        }
        if (bson_iter_init_find(&iter, doc, "wins")) {
            user->wins = bson_iter_int32(&iter);
        }
        if (bson_iter_init_find(&iter, doc, "totalScore")) {
            user->totalScore = bson_iter_int32(&iter);
        }
        if (bson_iter_init_find(&iter, doc, "playedGames")) {
            user->playedGames = bson_iter_int32(&iter);
        }
        if (bson_iter_init_find(&iter, doc, "createdAt")) {
            user->createdAt = bson_iter_int64(&iter);
        }
        if (bson_iter_init_find(&iter, doc, "updatedAt")) {
            user->updatedAt = bson_iter_int64(&iter);
        }

        bson_destroy(query);
        mongoc_cursor_destroy(cursor);
        mongoc_collection_destroy(collection);

        return user;
    } else {
        // Debug: Cursor error or no document found
        bson_error_t error;
        if (mongoc_cursor_error(cursor, &error)) {
            fprintf(stderr, "Cursor Error: %s\n", error.message);
        } 
    }

    bson_destroy(query);
    mongoc_cursor_destroy(cursor);
    mongoc_collection_destroy(collection);

    return NULL;
}




User* get_one_by_username(const char *username) {
    mongoc_client_t *client = get_mongo_client();
    mongoc_collection_t *collection = mongoc_client_get_collection(client, "quizmaster", "User");

    bson_t *query = bson_new();
    BSON_APPEND_UTF8(query, "username", username);

    mongoc_cursor_t *cursor = mongoc_collection_find_with_opts(collection, query, NULL, NULL);
    const bson_t *doc = NULL;
    if (mongoc_cursor_next(cursor, &doc)) {
        // Create a new User object
        User *user = (User*) malloc(sizeof(User));
        if (!user) {
            bson_destroy(query);
            mongoc_cursor_destroy(cursor);
            mongoc_collection_destroy(collection);
            return NULL; // Memory allocation failure
        }

        // Extract fields directly from BSON into the User struct
        bson_iter_t iter;

        // Extract "username"
        if (bson_iter_init_find(&iter, doc, "username")) {
            user->username = strdup(bson_iter_utf8(&iter, NULL)); // Ensure username is properly copied
        }

        // Extract "password"
        if (bson_iter_init_find(&iter, doc, "password")) {
            user->password = strdup(bson_iter_utf8(&iter, NULL)); // Ensure password is properly copied
        }

        // Extract "wins"
        if (bson_iter_init_find(&iter, doc, "wins")) {
            user->wins = bson_iter_int32(&iter);
        }

        // Extract "totalScore"
        if (bson_iter_init_find(&iter, doc, "totalScore")) {
            user->totalScore = bson_iter_int32(&iter);
        }

        // Extract "playedGames"
        if (bson_iter_init_find(&iter, doc, "playedGames")) {
            user->playedGames = bson_iter_int32(&iter);
        }

        // Extract "createdAt" and "updatedAt" timestamps
        if (bson_iter_init_find(&iter, doc, "createdAt")) {
            user->createdAt = bson_iter_time_t(&iter);
        }
        if (bson_iter_init_find(&iter, doc, "updatedAt")) {
            user->updatedAt = bson_iter_time_t(&iter);
        }

        // Extract the ObjectId
        if (bson_iter_init_find(&iter, doc, "_id")) {
            bson_oid_copy(bson_iter_oid(&iter), &user->id);  // Copy the ObjectId directly into the user struct
        }

        // Clean up
        bson_destroy(query);
        mongoc_cursor_destroy(cursor);
        mongoc_collection_destroy(collection);

        return user;
    }

    // Clean up if no user is found
    bson_destroy(query);
    mongoc_cursor_destroy(cursor);
    mongoc_collection_destroy(collection);

    return NULL;
}

#include <mongoc/mongoc.h>
#include <stdio.h>
#include <stdlib.h>

// Function to get the leaderboard with a given quantity (number of users)
// Function to get the leaderboard with a given quantity (number of users)
User* get_leaderboard(int quantity, int* result_size) {
    mongoc_client_t *client = get_mongo_client();
    mongoc_collection_t *collection = mongoc_client_get_collection(client, "quizmaster", "User");

    // Create a query (no specific query, just get all users)
    bson_t *query = bson_new();
    if (!query) {
        fprintf(stderr, "Failed to create query BSON document\n");
        return NULL;
    }

    // Create options BSON for sorting by totalScore in descending order and limiting the result to `quantity` users
    bson_t *opts = bson_new();
    if (!opts) {
        fprintf(stderr, "Failed to create options BSON document\n");
        bson_destroy(query);
        return NULL;
    }

    BSON_APPEND_INT32(opts, "limit", quantity);  // Limit the result to `quantity` users
    BSON_APPEND_DOCUMENT(opts, "sort", BCON_NEW("totalScore", BCON_INT32(-1)));  // Sort by totalScore in descending order

    // Get the users sorted by totalScore with the given limit
    mongoc_cursor_t *cursor = mongoc_collection_find_with_opts(collection, query, opts, NULL);
    if (!cursor) {
        fprintf(stderr, "Failed to create cursor\n");
        bson_destroy(query);
        bson_destroy(opts);
        return NULL;
    }

    const bson_t *doc = NULL;

    // Allocate memory for storing users (check if memory allocation is successful)
    User* leaderboard = malloc(sizeof(User) * quantity);
    if (!leaderboard) {
        fprintf(stderr, "Memory allocation failed for leaderboard\n");
        bson_destroy(query);
        bson_destroy(opts);
        mongoc_cursor_destroy(cursor);
        return NULL;
    }

    *result_size = 0;  // To track how many users are found

    // Iterate over the cursor and extract users
    while (mongoc_cursor_next(cursor, &doc)) {
        bson_iter_t iter;
        User* user = &leaderboard[*result_size];

        // Extract fields directly from BSON document
        if (bson_iter_init_find(&iter, doc, "_id")) {
            bson_oid_copy(bson_iter_oid(&iter), &user->id);
        }

        if (bson_iter_init_find(&iter, doc, "username")) {
            user->username = strdup(bson_iter_utf8(&iter, NULL));
        }

        if (bson_iter_init_find(&iter, doc, "password")) {
            user->password = strdup(bson_iter_utf8(&iter, NULL));
        }

        if (bson_iter_init_find(&iter, doc, "wins")) {
            user->wins = bson_iter_int32(&iter);
        }

        if (bson_iter_init_find(&iter, doc, "totalScore")) {
            user->totalScore = bson_iter_int32(&iter);
        }

        if (bson_iter_init_find(&iter, doc, "playedGames")) {
            user->playedGames = bson_iter_int32(&iter);
        }

        if (bson_iter_init_find(&iter, doc, "createdAt")) {
            user->createdAt = bson_iter_int64(&iter);
        }

        if (bson_iter_init_find(&iter, doc, "updatedAt")) {
            user->updatedAt = bson_iter_int64(&iter);
        }

        (*result_size)++;  // Increment the number of users found

        // Stop if we've reached the desired quantity
        if (*result_size >= quantity) {
            break;
        }
    }

    // If no users found, return NULL
    if (*result_size == 0) {
        free(leaderboard);
        bson_destroy(query);
        bson_destroy(opts);
        mongoc_cursor_destroy(cursor);
        return NULL;
    }

    // Cleanup resources
    bson_destroy(query);
    bson_destroy(opts);
    mongoc_cursor_destroy(cursor);
    mongoc_collection_destroy(collection);

    return leaderboard;
}





// Function to check if a user with the given ID exists
bool persists_by_id(const bson_oid_t *id) {
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


    // Create a BSON document for the new user
    bson_t *new_user = bson_new();
  
    BSON_APPEND_UTF8(new_user, "username", username);
    BSON_APPEND_UTF8(new_user, "password", password);
    BSON_APPEND_INT32(new_user, "wins", 0);
    BSON_APPEND_INT32(new_user, "totalScore", 0);
    BSON_APPEND_INT32(new_user, "playedGames", 0);
    BSON_APPEND_DATE_TIME(new_user, "createdAt", (int64_t)time(NULL) * 1000);
    BSON_APPEND_DATE_TIME(new_user, "updatedAt", (int64_t)time(NULL) * 1000);

    // Insert the user into the MongoDB collection
    mongoc_collection_insert_one(collection, new_user, NULL, NULL, NULL);

    // Cleanup
    bson_destroy(new_user);
    mongoc_collection_destroy(collection);
}

// Function to update a user's data (wins, totalScore, playedGames)
void update_user(const bson_oid_t *id, int wins, int totalScore, int playedGames) {
    mongoc_client_t *client = get_mongo_client();
    mongoc_collection_t *collection = mongoc_client_get_collection(client, "quizmaster", "User");

    // Create the query document to find the user by ID
    bson_t *query = bson_new();
    BSON_APPEND_OID(query, "_id", id);

    // Create the update document with $set
    bson_t *update = bson_new();
    bson_t child;
    BSON_APPEND_DOCUMENT_BEGIN(update, "$set", &child);

    if (wins >= 0){
        BSON_APPEND_INT32(&child, "wins", wins);
    }
    if (totalScore >=0){
        BSON_APPEND_INT32(&child, "totalScore", totalScore);
    }
    if (playedGames>=0){
        BSON_APPEND_INT32(&child, "playedGames", playedGames);
    }
    BSON_APPEND_DATE_TIME(&child, "updatedAt", (int64_t)time(NULL) * 1000);
    bson_append_document_end(update, &child);

    // Perform the update operation
    mongoc_collection_update_one(collection, query, update, NULL, NULL, NULL);

    // Cleanup resources
    bson_destroy(query);
    bson_destroy(update);
    mongoc_collection_destroy(collection);
}

// Function to delete a user by their id
void delete_user(const bson_oid_t *id) {
    mongoc_client_t *client = get_mongo_client();
    mongoc_collection_t *collection = mongoc_client_get_collection(client, "quizmaster", "User");

    // Create query to find the user by ID
    bson_t *query = bson_new();
    BSON_APPEND_OID(query, "_id", id);

    // Delete the user from the collection
    mongoc_collection_delete_one(collection, query, NULL, NULL, NULL);

    // Cleanup
    bson_destroy(query);
    mongoc_collection_destroy(collection);
}
