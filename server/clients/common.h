#pragma once

#include <sys/select.h>

#define QMAX 3
#define TIMEOUT 5

#define MESBUFSIZE 8
#define NAMEBUFSIZE 50

typedef struct Client{
    char name[50];
    char *message;
    int socket;
    int mes_len;
    int size;
    int check_name;
}client;

void GiveMoreSpace (char **small, int *data_volume);

int HandleLeaving (int mes_len, fd_set *available_sockets, int *clients_amount, client *user, char *package);