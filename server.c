#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h> 
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <string.h>


void Copy(const char *small, char *big, int data_volume) {
    for(int i = 0; i < data_volume; i++) {
        big[i] = small[i];
    }
}

void GiveMoreSpace (char **small, int *data_volume) {
    int temp = *data_volume;
    *data_volume *= 2;
    char* big = (char *)malloc(*data_volume * sizeof(char));
    Copy(*small, big, temp);
    free(*small);
    *small = big;
}

int PrepareServer(int main_socket, struct sockaddr_in addr) {
    if (main_socket < 0) {
        perror("Can't create socket!");
        return 0;
    }

    if (setsockopt(main_socket, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
    }

    if (bind(main_socket, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        perror("Can't bind socket! ");
        return 0;
    }

    if (listen(main_socket, 15) < 0) {
        perror("Can't listen socket! ");
        return 0;
    }
    return 1;
}


int main(int argc, char **argv) {
    int main_socket;
    int client_sockets[15];
    char package[8];
    memset(package, '\0', 80);
    struct sockaddr_in main_addr, clients_addr[15];
    main_socket = socket(AF_INET, SOCK_STREAM, 0);
    
    memset(&main_addr, 0, sizeof(main_addr));

    main_addr.sin_family = AF_INET;
    main_addr.sin_port = htons(atoi(argv[1]));
    main_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (!PrepareServer(main_socket, main_addr)) {
        return 0;
    }
    int i = -1;
    while (1) {
        i++;
        int cl_addr_len = sizeof(clients_addr[i]);
        memset (&clients_addr, 0, cl_addr_len);
        client_sockets[i] = accept(main_socket, (struct sockaddr *) NULL, (socklen_t *) NULL);

        if(fork() == 0) {
            close(main_socket);
            int size = 8;
            int length = 0;
            char *message = (char *)malloc(size * sizeof(char));
            memset(message, '\0', 8);
            while (1) {
                //printf("size = %d\n", size);
                int mes_len = recv(client_sockets[i], package, sizeof(package), 0);
                //printf("tr: %s\n", package);
                if (!strncmp(package, "bye!\r", 5) || mes_len == 0) {
                    break;
                }
                if (!strchr(package, '\n')) {
                    if ((size - length) < sizeof(package)) {
                        GiveMoreSpace(&message, &size);
                    }
                    //printf("11::");
                    strcat(message, package);
                    //printf("tr: %s\n", message);
                    length += sizeof(package);
                }
                else {
                    //printf("prelast: %s\n", message);
                    if ((size - length) < (strchr(package, '\n') - package)) {
                        GiveMoreSpace(&message, &size);
                    }
                    //printf("22::");
                    //printf("1tr: %s\n", message);
                    strncat(message, package, (strchr(package, '\n') - package));
                    printf("2tr: %s\n", message);
                    memset(message, '\0', size);
                    length = 0;
                    //memset(package, '\0', sizeof(package));
                }
                //printf("last: %s\n", message);
            }

            shutdown(client_sockets[i], 1);
            close(client_sockets[i]);
            exit(EXIT_SUCCESS);
        }
    }

    shutdown(main_socket, 1);
    close(main_socket);
 
    return 0;
}