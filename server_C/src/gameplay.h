#ifndef GAMEPLAY_H


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libwebsockets.h>
#include <jansson.h>
#include <bson.h>
#include "user.h"

#define GAMEPLAY_PORT 5000  // WebSocket server port

#define MAX_PLAYERS 4
#define MAX_ROOMS 10
#define MAX_QUESTIONS 7
typedef struct Question {
    char question[256];
    char choices[4][128];
    char correctAnswer[128];
} Question;

typedef struct Room {
    int id;
    User* players[MAX_PLAYERS];
    int player_count;
    char votes[MAX_PLAYERS][50];
    int scores[MAX_PLAYERS];
    int answered[MAX_PLAYERS];
    int isOngoing;
    Question questions[MAX_QUESTIONS];
    int currentQuestion;
} Room;



int start_server();
#endif