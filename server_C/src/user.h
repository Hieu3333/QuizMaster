#ifndef USER_H
#define USER_H

#include <stdbool.h>
#include <time.h>
#include <bson/bson.h> // Include bson library for bson_oid_t type

typedef struct {
    bson_oid_t id;
    char *username;   // Ensure these are char* for modification
    char *password;
    int wins;
    int totalScore;
    int playedGames;
    time_t createdAt;
    time_t updatedAt;
} User;


// Function to get a user by their ID
User* get_one_by_id( bson_oid_t *id);

// Function to get a user by their username
User* get_one_by_username(const char *username);


User* get_leaderboard(int quantity, int* result_size);

// Function to check if a user with the given ID exists
bool persists_by_id(const bson_oid_t *id);

// Function to check if a user with the given username exists
bool persists_by_username(const char *username);

// Function to add a new user
void add_user(const char *username, const char *password);

// Function to update user information (wins, totalScore, playedGames)
void update_user(const bson_oid_t *id, int wins, int totalScore, int playedGames);

// Function to delete a user by their ID
void delete_user(const bson_oid_t *id);

#endif // USER_H
