#include <stdio.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h> 

#define TIMEOUT 5

typedef struct Client{
    char name[50];
    char *message;
    int socket;
    int mes_len;
    int size;
    int check_name;
}client;

void GiveMoreSpace (char **small, int *data_volume) {
    *data_volume *= 2;
    char* big = (char *)malloc(*data_volume * sizeof(char));
    memset (big, '\0', *data_volume);
    strcpy (big, *small);
    free(*small);
    *small = big;
}

int HandleLeaving (int mes_len, fd_set *available_sockets, int *clients_amount, client *user, char *package) {
    if (mes_len == 0 || (!strncmp(package, "bye!\r", 5) && (*user).mes_len == 0)) {
        alarm(TIMEOUT);
        FD_CLR ((*user).socket, available_sockets);
        shutdown((*user).socket, 2);
        close((*user).socket);
        printf("%s left the chat\n", (*user).name);
        (*user).socket = 0;
        (*clients_amount)--;
        return 0;
    }
    return 1;
}