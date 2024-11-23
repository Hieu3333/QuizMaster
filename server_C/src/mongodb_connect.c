#include "mongodb_connect.h"
#include <stdio.h>
#include <stdlib.h>

static mongoc_client_t *client = NULL;

void init_mongo() {
    mongoc_init();
    const char *uri_string = "mongodb+srv://hieu:(al)chemist@cluster0.p5gccdx.mongodb.net/?retryWrites=true&w=majority&appName=Cluster0";
    client = mongoc_client_new(uri_string);
    if (!client) {
        fprintf(stderr, "Failed to connect to MongoDB.\n");
        exit(EXIT_FAILURE);
    }
    printf("Connected to MongoDB successfully!\n");
}

void cleanup_mongo() {
    if (client) {
        mongoc_client_destroy(client);
    }
    mongoc_cleanup();
}

mongoc_client_t* get_mongo_client() {
    return client;
}
