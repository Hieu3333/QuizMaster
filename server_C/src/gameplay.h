#ifndef GAMEPLAY_H


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libwebsockets.h>
#include <jansson.h>
#include <bson.h>
#include "user.h"

#define GAMEPLAY_PORT 5000  // WebSocket server port
#define MAX_PLAYERS 2
#define MAX_ROOMS 10
#define MAX_QUESTIONS 7
#define MAX_USERS 10
#define API_URL "https://opentdb.com/api.php"
#define MAX_QUESTION_LENGTH 256
#define MAX_CHOICE_LENGTH 128
#define MAX_ANSWER_LENGTH 128

typedef struct Question {
    char question[MAX_QUESTION_LENGTH];  // Store question text
    char choices[4][MAX_CHOICE_LENGTH];  // Store choices (4 choices)
    char correctAnswer[MAX_ANSWER_LENGTH]; // Store the correct answer
} Question;

typedef struct Room {
    int id;
    User players[MAX_PLAYERS];
    int player_count;
    int votes[MAX_PLAYERS];
    int vote_count;
    int answer_count;
    int scores[MAX_PLAYERS];
    int answered[MAX_PLAYERS];
    int isOngoing;
    Question questions[MAX_QUESTIONS];
    int currentQuestion;
} Room;





int start_server();
#endif