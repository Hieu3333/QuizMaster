#include "auth.h"
#include "user.h"
#include "mongodb_connect.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

// Login function
User* login(const char *username, const char *password) {
    // Fetch the user by username from the database
    User *user = get_one_by_username(username);
    
    if (user == NULL) {
        return user; // User not found
    }

    // Compare the provided password with the stored password
    if (strcmp(user->password, password) == 0) {  // Password matches
        return user;
    }

    free(user);  // Don't forget to free the allocated memory
    return NULL;  // Password mismatch
}

// Register function
bool register_user(const char *username, const char *password) {
    // Check if the username already exists
    if (persists_by_username(username)) {
        return false; // Username already exists
    }

    // Add the new user to the database
    add_user(username, password);
    return true;
}
