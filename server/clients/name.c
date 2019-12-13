#define RESET   "\033[0m"
#define DIR   "\033[32;22m"
#define HOST  "\033[34;22;1m"

#include <stdio.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <string.h>
#include <unistd.h>

#define QMAX 2
#define TIMEOUT 5
#define NAMEBUFSIZE 50

#include "common.h"

int NameHasNoEndSymbol (char *test_name, int *check_name) {
    if (!strchr(test_name, '\n')) {
        *check_name = -1;
        memset (test_name, '\0', NAMEBUFSIZE);
        return 1;
    }
    return 0;
}

int CheckLetters (char *name) {
    for (int i = 0; i < strlen(name) - 2; i++) {
        if (name[i] != '_' && !((name[i] >= 'a' && name[i] <= 'z') || (name[i] >= 'A' && name[i] <= 'Z'))) {
            return 0;
        }
    }
    return 1;
}

int CheckOverlap (char * name, client *users) {
    char temp_name[50];
    memset (temp_name, '\0', sizeof(temp_name));
    strncpy (temp_name, name, strlen(name) - 2);
    for (int i = 0; i < QMAX; i++) {
        if (!strcmp (temp_name, users[i].name)) {
            return 1;
        }
    }
    return 0;
}

int CheckNameAvailability (char *name, client *users, int *check_name) {
    if (*check_name == -1) {
        *check_name = 0;
        return 0;
    }
    if (strlen(name) < 5 || strlen(name) > 18) {
        return 0;
    }
    if (!CheckLetters (name)) {
        return 0;
    }
    if (CheckOverlap (name, users)) {
        return -1;
    }
    
    return 1;
}

int NotUsableName (char *test_name, client *users, int i) {
    int availability_status = 0;
    if ((availability_status = CheckNameAvailability (test_name, users, &users[i].check_name)) != 1) {
        char problem[150];
        memset (problem, '\0', sizeof(problem));
        switch (availability_status) {
            case 0: {
                strcpy (problem, "\033[0m# invalid name\nEnter your name, please: \033[32;22m");
                break;
            }
            case -1: {
                strcpy (problem, "\033[0m# such name already exists\nEnter your name, please: \033[32;22m");
                break;
            }
            default: break;
        }
        send(users[i].socket, problem, sizeof(problem), 0);
        return 1;
    }
    return 0;
}

void HandleUsableName (char *test_name, client *users, int i) {
    alarm(TIMEOUT);
    printf ("%s changed name to ", users[i].name);
    memset(users[i].name, '\0', sizeof(users[i].name));
    strncpy(users[i].name, test_name, strlen(test_name) - 2);
    printf ("'%s'\n", users[i].name);
    users[i].check_name = 1;
    char temp[100];
    memset(temp, '\0', sizeof(temp));
    sprintf (temp, "%s%s%s%s%s", RESET, "Welcome, ", users[i].name, "!\n", DIR);
    send (users[i].socket, temp, sizeof(temp), 0);
}

void CheckName (char *test_name, client *users, int i) {
    if (NotUsableName(test_name, users, i)) {
        return;
    }
    HandleUsableName (test_name, users, i);
}
  
int GetName (client *users, int i, fd_set *available_sockets, int *clients_amount) {
    if (users[i].check_name != 1) {
        char test_name[NAMEBUFSIZE];
        memset (test_name, '\0', sizeof(test_name));
        int mes_len = recv(users[i].socket, test_name, sizeof(test_name) - 1, 0);

        if (!HandleLeaving (mes_len, available_sockets, clients_amount, &users[i], test_name)) {
            return 0;
        }
        
        if (NameHasNoEndSymbol (test_name, &users[i].check_name)) {
            return 0;
        }
        CheckName (test_name, users, i);
        return 0;
    }
    return 1;
}