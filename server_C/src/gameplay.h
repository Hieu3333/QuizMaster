#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "user.h"

#define MAX_PLAYERS 4
#define MAX_ROOMS 10
#define BUFFER_SIZE 1024
typedef struct {
    int id;
    char name[50];
    int score;
} Player;

typedef struct {
    int id;
    Player players[MAX_PLAYERS];
    int num_players;
    int votes[MAX_PLAYERS];
    int scores[MAX_PLAYERS];
    int is_ongoing;
} Room;

typedef struct {
    char question[255];
    char choices[4][255];
    char correct_answer[255];
} Question;