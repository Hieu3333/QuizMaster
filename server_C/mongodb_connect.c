#include <mongoc/mongoc.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
    // Initialize the MongoDB C driver
    mongoc_init();

    // Define the MongoDB URI string
    const char *uri_string = "mongodb+srv://hieu:(al)chemist@cluster0.p5gccdx.mongodb.net/?retryWrites=true&w=majority&appName=Cluster0";
    
    // Create a MongoDB client using the URI
    mongoc_client_t *client = mongoc_client_new(uri_string);

    // Check if the connection was successful
    if (!client) {
        fprintf(stderr, "Failed to parse URI.\n");
        return EXIT_FAILURE;
    }

    // Verify that the client can connect to the MongoDB server
    if (!mongoc_client_get_database(client, "database")) {
        fprintf(stderr, "Failed to connect to MongoDB.\n");
        mongoc_client_destroy(client);
        mongoc_cleanup();
        return EXIT_FAILURE;
    }

    printf("Connected to MongoDB successfully!\n");

    // Perform your database operations here...

    // Cleanup and release resources
    mongoc_client_destroy(client);
    mongoc_cleanup();

    return EXIT_SUCCESS;
}
