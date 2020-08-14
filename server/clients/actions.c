#include <stdio.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h> 

#include "actions.h"
#include "common.h"
#include "name.h"
#include "message.h"

int action = 0; // To detect, whether SIGALRM came or not
int finish = 0; // To detect, whether SIGINT came or not

int FindAvailableSockets (fd_set *available_sockets, int *free_index, int main_socket, client *users) {
    FD_ZERO(available_sockets);
    FD_SET(main_socket, available_sockets);
    int max_sd = main_socket;

    for (int i = 0 ; i < QMAX ; i++) {
        if (users[i].socket > 0) {
            FD_SET(users[i].socket, available_sockets);
        }
        else if (*free_index == -1) {
            *free_index = i;
        }

        if (users[i].socket > max_sd)
            max_sd = users[i].socket;
    }

    return max_sd;
}

void HandleAlarm (int clients_amount) {
    if (action == 1) {
        action = 0;
        printf("# chat server is running... (%d users connected)\n", clients_amount);
    }
}

int FindActiveSockets (fd_set *available_sockets, int max_sd, int clients_amount) {
    struct timeval timeout;
    timeout.tv_sec = 1000;
    timeout.tv_usec = 0;
    int status = select(max_sd + 1, available_sockets, NULL, NULL, &timeout);
    switch (status) {
        case -1: {
            HandleAlarm(clients_amount);
            return 0;
        };
        case 0: {
            return 0;
        };
        default:    break;    
    }
    return 1;
}

void TooManyUsers (int new_socket, int clients_amount) {
    HandleAlarm(clients_amount);
    char buf[] = "# unfortunately, server is full...\n# try again later\n";
    send(new_socket, buf, sizeof(buf), 0);
    shutdown(new_socket, 1);
    close(new_socket);
}

void ConnectNewUser (client *users, int free_index, int new_socket, int *clients_amount) {
    alarm(TIMEOUT);
    users[free_index].socket = new_socket;
    char buf[100];
    memset (buf, '\0', sizeof(buf));
    strcpy (buf, NAME_PROMPT);
    sprintf(users[free_index].name, "user_%d", free_index + 1);
    printf ("%s joned the chat\n", users[free_index].name);
    users[free_index].check_name = 0;
    send(users[free_index].socket, buf, sizeof(buf), 0);
    (*clients_amount)++;
}

void HandleAccept (int main_socket, int free_index, int *clients_amount, client *users) {
    int new_socket = accept(main_socket, (struct sockaddr *) NULL, (socklen_t *) NULL);
    if (free_index == -1) {
        TooManyUsers(new_socket, *clients_amount);
    }
    else {
        ConnectNewUser (users, free_index, new_socket, clients_amount);   
    }
}

void PrepareNewUser (client *user) {
    if ((*user).mes_len == 0) {
        (*user).size = 8;
        (*user).message = (char *)malloc((*user).size * sizeof(char));
        memset ((*user).message, '\0', (*user).size);
    }
}

void HandleUsersActions (client *users, fd_set *available_sockets, int *clients_amount) {
    for (int i = 0; i < QMAX; i++) {
        if (FD_ISSET(users[i].socket, available_sockets)) {
            PrepareNewUser(&users[i]);

            if (!GetName(users, i, available_sockets, clients_amount)) {
                return;
            }

            if (!HandleMessage (users, i, clients_amount, available_sockets)) {
                return;
            }
        }
    }
}