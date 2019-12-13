#define RESET   "\033[0m"
#define DIR   "\033[32;22m"
#define HOST  "\033[34;22;1m"

#include <stdio.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h> 

#define QMAX 2
#define TIMEOUT 5
#define MESBUFSIZE 8

#include "common.h"

int IsLastPackage (int end_index_n, int end_index_r, client * user, char *package) {
    if (end_index_n == -1 && end_index_r == -1) {
        if (((*user).size - (*user).mes_len) < MESBUFSIZE - 1) {
            GiveMoreSpace(&(*user).message, &(*user).size);
        }
        strncat((*user).message, package, MESBUFSIZE - 1);
        (*user).mes_len += MESBUFSIZE - 1;
        return 0;
    }
    return 1;
}

void GatherWholeMessage (int end_index_r, int end_index_n, client *users, int i, char *package) {
    alarm(TIMEOUT);
    if ((users[i].size - users[i].mes_len) < end_index_r + 1) {
        GiveMoreSpace(&users[i].message, &users[i].size);
    }
    package[end_index_r] = '\n';
    users[i].mes_len += end_index_r + 1;
    strncat(users[i].message, package, end_index_r + 1);
    printf("[%s]: %s", users[i].name, users[i].message);
}

void SendOtherUsers (client *users, int i) {
    for (int k = 0; k < QMAX; k++) {
        if (users[k].socket > 0 && k != i && users[k].check_name) {
            char *package_others = (char *)malloc((users[i].mes_len + 100) * sizeof(char));
            memset (package_others, '\0', (users[i].mes_len + 100));
            sprintf (package_others, "%s%30s\n%s %30s\n", HOST, users[i].name, DIR, users[i].message);
            send (users[k].socket, package_others, (users[i].mes_len + 100), 0);
        }
    }
}

int FindSymbol (char *package, int size, char symbol) {
    for (int i = 0; i < size; i++) {
        if (package[i] == symbol) {
            return i;
        }
        if (package[i] == '\0') {
            break;
        }
    }
    return -1;
}

int GetSendMessage (char *package, client *users, int i) {
    int end_index_r = FindSymbol(package, MESBUFSIZE, '\r');
    int end_index_n = FindSymbol(package, MESBUFSIZE, '\n');

    if (IsLastPackage (end_index_n, end_index_r, &users[i], package)) {
        if ((end_index_r == 0 && !strlen(users[i].message)) || end_index_n == 0) {
            return 0;
        }
        GatherWholeMessage (end_index_r, end_index_n, users, i, package);

        SendOtherUsers (users, i);
        
        users[i].mes_len = 0;
        free (users[i].message);
    }
    return 1;
}

int HandleMessage (client *users, int i, int *clients_amount, fd_set *available_sockets) {
    char package[MESBUFSIZE];
    memset(package, '\0', MESBUFSIZE);
    int mes_len = recv(users[i].socket, package, MESBUFSIZE - 1, 0);

    if (!HandleLeaving (mes_len, available_sockets, clients_amount, &users[i], package)) {
        return 0;
    }

    if (!GetSendMessage (package, users, i)) {
        return 0;
    }
    return 1;
}