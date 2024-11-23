#ifndef MONGODB_CLIENT_H
#define MONGODB_CLIENT_H

#include <mongoc/mongoc.h>

void init_mongo();
void cleanup_mongo();
mongoc_client_t* get_mongo_client();

#endif
