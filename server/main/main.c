#include <stdio.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h> 
#include <signal.h>

#include "../clients/actions.h"

#define QMAX 2
#define TIMEOUT 5

void TimeHandler (int s) {
    action = 1;
    alarm(TIMEOUT);
}

void StopHandler (int s) {
    finish = 1;
}

int PrepareServer(int *main_socket, struct sockaddr_in *main_addr, char *s) {
    *main_socket = socket(AF_INET, SOCK_STREAM, 0);
    memset(main_addr, 0, sizeof(*main_addr));
    (*main_addr).sin_family = AF_INET;
    (*main_addr).sin_port = htons(atoi(s));
    (*main_addr).sin_addr.s_addr = htonl(INADDR_ANY);
    if (*main_socket < 0) {
        perror("Can't create socket! ");
        return 0;
    }

    if (setsockopt(*main_socket, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
    }

    if (bind(*main_socket, (struct sockaddr *) main_addr, sizeof(*main_addr)) < 0) {
        perror("Can't bind socket! ");
        return 0;
    }

    if (listen(*main_socket, QMAX) < 0) {
        perror("Can't listen socket! ");
        return 0;
    }

    printf ("# chat server is running at port %s\n", s);
    return 1;
}

void SetUpSigHndl () {
    alarm(TIMEOUT); 
    signal (SIGALRM, TimeHandler);
    signal (SIGINT, StopHandler);  
}

void RunServer (char *s) {
    int main_socket;
    client users[QMAX];
    memset (users, 0, QMAX * sizeof(client));
    struct sockaddr_in main_addr;
    int clients_amount = 0;
    fd_set available_sockets;

    if (!PrepareServer(&main_socket, &main_addr, s)) {
        return;
    }
    SetUpSigHndl();

    while (1) {
        if (finish) {
            break;
        }
        int free_index = -1;

        int max_sd = FindAvailableSockets(&available_sockets, &free_index, main_socket, users);
        
        if (!FindActiveSockets(&available_sockets, max_sd, clients_amount)) {
            continue;
        }

        if (FD_ISSET(main_socket, &available_sockets)) {
            HandleAccept(main_socket, free_index, &clients_amount, users);
        }
        
        HandleUsersActions(users, &available_sockets, &clients_amount);
    }

    shutdown(main_socket, 1);
    close(main_socket);
}