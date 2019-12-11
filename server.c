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

#define QMAX 2

typedef struct Client{
    char name[50];
    int socket;
}client;

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

    if (listen(main_socket, QMAX) < 0) {
        perror("Can't listen socket! ");
        return 0;
    }
    return 1;
}

int main(int argc, char **argv) {
    struct timeval timeout;
    int main_socket;
    client clients[QMAX];
    memset (clients, 0, QMAX * sizeof(client));
    char package[8];
    memset(package, '\0', 80);
    struct sockaddr_in main_addr, clients_addr[QMAX];
    main_socket = socket(AF_INET, SOCK_STREAM, 0);
    
    memset(&main_addr, 0, sizeof(main_addr));

    main_addr.sin_family = AF_INET;
    main_addr.sin_port = htons(atoi(argv[1]));
    main_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (!PrepareServer(main_socket, main_addr)) {
        return 0;
    }

    while (1) {
        fd_set available_sockets;
        FD_ZERO(&available_sockets);

        FD_SET(main_socket, &available_sockets);
        int max_sd = main_socket;

        int free_index = -1;

        for (int i = 0 ; i < QMAX ; i++) {
            int sd = clients[i].socket;

            if (sd > 0) {
                FD_SET(sd , &available_sockets);
            }

            else if (free_index == -1) {
                free_index = i;
            }
            

            if (sd > max_sd)
                max_sd = sd;
        }

        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
        int status = select(max_sd + 1, &available_sockets, NULL, NULL, &timeout);
        switch (status) {
            case -1: {
                perror("Error: ");
                break;
            };
            case 0: {
                printf("Chat server is running...\n");
                continue;
            };
            default: {
                //ToDo
            }
            
        }

        if (FD_ISSET(main_socket, &available_sockets)) {
            int cl_addr_len = sizeof(clients_addr[free_index]);
            memset (&clients_addr, 0, cl_addr_len);
            int new_socket = accept(main_socket, (struct sockaddr *) NULL, (socklen_t *) NULL);
            if (free_index == -1) {
                char buf[50] = "Unfortunately, server is full...\nTry again later\n";
                send(new_socket, buf, sizeof(buf), 0);
                shutdown(new_socket, 1);
                close(new_socket);
            }
            else {
                while (1) {
                    char buf[50] = "Enter your name, please: ";
                    send(new_socket, buf, sizeof(buf), 0);
                    recv(new_socket, clients[free_index].name, sizeof(buf), 0);
                    if (strlen(clients[free_index].name) >= 5 && strlen(clients[free_index].name) <= 18) {
                        break;
                    }
                    char name[50] = "# invalid name\n";
                    send(new_socket, name, sizeof(name), 0);
                }
                clients[free_index].socket = new_socket;
            }
        }
        for (int i = 0; i < QMAX; i++) {
            if (FD_ISSET(clients[i].socket, &available_sockets)) {
                int size = 8;
                int length = 0;
                char *message = (char *)malloc(size * sizeof(char));
                memset(message, '\0', 8);
                while (1) {
                    //printf("size = %d\n", size);
                    int mes_len = recv(clients[i].socket, package, sizeof(package), 0);
                    //printf("tr: %s\n", package);
                    if ((!strncmp(package, "bye!\r", 5) && length == 0) || mes_len == 0) {
                        FD_CLR (clients[i].socket, &available_sockets);
                        shutdown(clients[i].socket, 1);
                        close(clients[i].socket);
                        clients[i].socket = -1;
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
                        break;
                        //memset(package, '\0', sizeof(package));
                    }
                    //printf("last: %s\n", message);
                }
            }
        }
    }

    shutdown(main_socket, 1);
    close(main_socket);
 
    return 0;
}