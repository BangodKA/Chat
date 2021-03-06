#include <stdio.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h> 
#include <signal.h>

#include "main.h"
#include "../clients/actions.h"
#include "../clients/message.h"
#include "../clients/common.h"

void TimeHandler (int s) {
    action = 1;
    alarm(TIMEOUT);
}

void StopHandler (int s) {
    finish = 1;
}

int PrepareServer(int *main_socket, struct sockaddr_in *main_addr, const char *s) {
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

int NotNumber(const char *s) {
    for (int i = 0; i < strlen(s); i++) {
        if (!(s[i] <= '9' && s[i] >= '0')) {
            return 1;
        } 
    }
    return 0;
}

void RunServer (const char *s) {
    if (NotNumber(s)) {
        return;
    }
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

    char * message = (char *)malloc((100) * sizeof(char));
    memset(message, '\0', (100));
    sprintf(message, "Connection closed by foreign host.\n");
    SendChosenUsers(users, -1, message);
    
    shutdown(main_socket, 2);
    close(main_socket);
}