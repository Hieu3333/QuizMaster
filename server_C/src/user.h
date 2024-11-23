#ifndef USER_H
#define USER_H

#include <stdbool.h>
#include <time.h>
typedef struct {
    char id[25];             // User ID (MongoDB ObjectId)
    char username[256];      // Username
    char password[256];      // Password (Store as plain text, consider hashing in practice)
    int wins;                // Number of wins
    int totalScore;          // Total score
    int playedGames;         // Total games played
    time_t createdAt;        // Account creation timestamp
    time_t updatedAt;        // Last updated timestamp
} User;

// Function to get a user by their ID
User* get_one_by_id(const char *id);

// Function to get a user by their username
User* get_one_by_username(const char *username);

// Function to check if a user with the given ID exists
bool persists_by_id(const char *id);

// Function to check if a user with the given username exists
bool persists_by_username(const char *username);

// Function to add a new user
void add_user(const char *username, const char *password);

// Function to update user information (wins, totalScore, playedGames)
void update_user(const char *id, int wins, int totalScore, int playedGames);

void delete_user(const char *id);

#endif // USER_H
