#ifndef AUTH_H
#define AUTH_H

#include <mongoc/mongoc.h>
#include <stdbool.h>
#include "user.h"

User* login(const char *username, const char *password);
bool register_user(const char *username, const char *password);

#endif
