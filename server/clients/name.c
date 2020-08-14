#include <stdio.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "name.h"

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
        return 1;
    }
    if (strlen(name) < 5 || strlen(name) > 18) {
        return 1;
    }
    if (!CheckLetters (name)) {
        return 1;
    }
    if (CheckOverlap (name, users)) {
        return -1;
    }
    
    return 0;
}

int NotUsableName (char *test_name, client *users, int i) {
    int availability_status = 0;
    if ((availability_status = CheckNameAvailability (test_name, users, &users[i].check_name)) != 0) {
        char problem[150];
        memset (problem, '\0', sizeof(problem));
        switch (availability_status) {
            case 1: {
                strcpy (problem, "\033[0m# invalid name\n");
                break;
            }
            case -1: {
                strcpy (problem, "\033[0m# such name already exists\n");
                break;
            }
        }
        strcat(problem, NAME_PROMPT);
        send(users[i].socket, problem, sizeof(problem), 0);
        return 1;
    }
    return 0;
}

void HandleUsableName (char *test_name, client *users, int i) {
    alarm(TIMEOUT);
    printf ("%s changed name to %s", users[i].name, test_name);
    memset(users[i].name, '\0', sizeof(users[i].name));
    strncpy(users[i].name, test_name, strlen(test_name) - 2);
    users[i].check_name = 1;
    char temp[100];
    memset(temp, '\0', sizeof(temp));
    sprintf (temp, "%sWelcome, %s!%s\n", RESET, users[i].name, DIR);
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