#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <pthread.h>
#include "gameplay.h"

Room rooms[MAX_ROOMS];
int room_count = 0;
int current_question_index = 0;
Question questions[7];  // You can have a fixed number of questions or fetch dynamically

// Utility functions
void init_room(Room *room, int id) {
    room->id = id;
    room->num_players = 0;
    room->is_ongoing = 0;
    memset(room->scores, 0, sizeof(room->scores));
    memset(room->votes, 0, sizeof(room->votes));
}

void send_to_client(int client_socket, const char *message) {
    send(client_socket, message, strlen(message), 0);
}

void *handle_player(void *arg);

// Server functions
void start_game(int room_id) {
    Room *room = &rooms[room_id];
    if (room->num_players < 4) return;
    
    // Begin the voting process (assuming 1 category for simplicity)
    send_to_client(room->players[0].id, "Voting started...");

    // Wait for all players to vote
    if (room->num_players == 4) {
        // Simulate voting, pick a winner
        int winner = rand() % 4; // Randomly pick a winner for this example
        send_to_client(room->players[winner].id, "You won the vote!");

        // Start the game with the selected category
        room->is_ongoing = 1;
        send_to_client(room->players[0].id, "Game started!");
    }
}

void join_room(int client_socket, Player player) {
    int joined = 0;
    for (int i = 0; i < room_count; i++) {
        if (rooms[i].num_players < MAX_PLAYERS) {
            rooms[i].players[rooms[i].num_players] = player;
            rooms[i].num_players++;
            send_to_client(client_socket, "Joined room successfully!");
            joined = 1;
            if (rooms[i].num_players == MAX_PLAYERS) {
                start_game(i);
            }
            break;
        }
    }
    if (!joined) {
        if (room_count < MAX_ROOMS) {
            Room new_room;
            init_room(&new_room, room_count);
            new_room.players[0] = player;
            new_room.num_players = 1;
            rooms[room_count] = new_room;
            room_count++;
            send_to_client(client_socket, "New room created and you joined!");
        } else {
            send_to_client(client_socket, "No available rooms.");
        }
    }
}

// Socket communication
int setup_server_socket(int port) {
    int server_socket;
    struct sockaddr_in server_addr;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Socket creation failed");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed");
        exit(1);
    }

    if (listen(server_socket, 10) == -1) {
        perror("Listen failed");
        exit(1);
    }

    return server_socket;
}

// Main game loop
void *handle_player(void *arg) {
    int client_socket = *((int *)arg);
    Player player;
    char buffer[BUFFER_SIZE];

    recv(client_socket, buffer, sizeof(buffer), 0);  // Receive player name
    strcpy(player.name, buffer);
    player.id = client_socket;

    join_room(client_socket, player);

    while (1) {
        // Handle voting or answering
        memset(buffer, 0, sizeof(buffer));
        int recv_size = recv(client_socket, buffer, sizeof(buffer), 0);
        if (recv_size <= 0) {
            close(client_socket);
            break;
        }

        // Process the message (e.g., vote, answer)
        if (strcmp(buffer, "vote") == 0) {
            // Handle voting logic
            send_to_client(client_socket, "Your vote was recorded.");
        } else if (strcmp(buffer, "answer") == 0) {
            // Handle answering logic
            send_to_client(client_socket, "Answer recorded.");
        }
    }

    return NULL;
}