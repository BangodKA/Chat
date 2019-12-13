#define RESET   "\033[0m"
#define DIR   "\033[32;22m"
#define HOST  "\033[34;22;1m"

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
#include <limits.h>

#define QMAX 2
#define TIMEOUT 5

int action = 0;
int finish = 0;

typedef void (*sighandler_t)(int);

typedef struct Client{
    char name[50];
    char *message;
    int socket;
    int mes_len;
    int size;
    int name_len;
    int check_name;
}client;

void GiveMoreSpace (char **small, int *data_volume) {
    *data_volume *= 2;
    char* big = (char *)calloc(*data_volume, sizeof(char));
    memset (big, '\0', *data_volume);
    strcpy (big, *small);
    free(*small);
    *small = big;
}

int PrepareServer(int main_socket, struct sockaddr_in addr) {
    if (main_socket < 0) {
        perror("Can't create socket! ");
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

int CheckLetters (char *name) {
    for (int i = 0; i < strlen(name) - 2; i++) {
        if (name[i] != '_' && !((name[i] >= 'a' && name[i] <= 'z') || (name[i] >= 'A' && name[i] <= 'Z'))) {
            return 0;
        }
    }
    return 1;
}

int CheckOverlap (char * name, client *users) {
    for (int i = 0; i < sizeof(users); i++) {
        if (!strcmp (name, users[i].name)) {
            return 1;
        }
    }
    return 0;
}

int CheckNameAvailability (char *name, client *users) {
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

void TimeHandler (int s) {
    action = 1;
    alarm(TIMEOUT);
}

void StopHandler (int s) {
    finish = 1;
}

int main(int argc, char **argv) {
    struct timeval timeout;
    int main_socket;
    client users[QMAX];
    memset (users, 0, QMAX * sizeof(client));
    char package[8];
    memset(package, '\0', sizeof(package));
    struct sockaddr_in main_addr, clients_addr[QMAX];
    main_socket = socket(AF_INET, SOCK_STREAM, 0);
    
    memset(&main_addr, 0, sizeof(main_addr));

    main_addr.sin_family = AF_INET;
    main_addr.sin_port = htons(5599);
    main_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (!PrepareServer(main_socket, main_addr)) {
        return 0;
    }

    printf ("# chat server is running at port %s\n", argv[1]);

    int amount = 0;
    alarm(TIMEOUT);
    signal (SIGALRM, TimeHandler);
    signal (SIGINT, StopHandler);   


    while (1) {
        if (finish) {
            break;
        }
        fd_set available_sockets;
        FD_ZERO(&available_sockets);

        FD_SET(main_socket, &available_sockets);
        int max_sd = main_socket;

        int free_index = -1;

        for (int i = 0 ; i < QMAX ; i++) {
            int sd = users[i].socket;

            if (sd > 0) {
                FD_SET(sd , &available_sockets);
            }

            else if (free_index == -1) {
                free_index = i;
            }
            

            if (sd > max_sd)
                max_sd = sd;
        }

        timeout.tv_sec = 1000;
        timeout.tv_usec = 0;
        int status = select(max_sd + 1, &available_sockets, NULL, NULL, &timeout);
        switch (status) {
            case -1: {
                if (action == 1) {
                    action = 0;
                    printf("# chat server is running... (%d users connected)\n", amount);
                }
                continue;
            };
            case 0: {
                continue;
            };
            default:    break;    
       
        }

        if (FD_ISSET(main_socket, &available_sockets)) {
            int cl_addr_len = sizeof(clients_addr[free_index]);
            memset (&clients_addr, 0, cl_addr_len);
            int new_socket = accept(main_socket, (struct sockaddr *) NULL, (socklen_t *) NULL);
            if (free_index == -1) {
                if (action == 1) {
                    action = 0;
                    printf("# chat server is running... (%d users connected)\n", amount);
                }
                char buf[60] = "# unfortunately, server is full...\n# try again later\n";
                send(new_socket, buf, sizeof(buf), 0);
                shutdown(new_socket, 1);
                close(new_socket);
            }
            else {
                alarm(TIMEOUT);
                users[free_index].socket = new_socket;
                char buf[100] = "Enter your name, please: \033[32;22m";
                char temp[4]; 
                sprintf(temp, "%d", free_index + 1);
                strcpy(users[free_index].name, "user_");
                strcat(users[free_index].name, temp);
                printf ("%s joned the chat\n", users[free_index].name);
                users[free_index].name_len = 0;
                send(users[free_index].socket, buf, sizeof(buf), 0);
                amount++;
            }
        }
        for (int i = 0; i < QMAX; i++) {
            if (FD_ISSET(users[i].socket, &available_sockets)) {
                if (users[i].mes_len == 0) {
                    users[i].size = 8;
                    users[i].message = (char *)calloc(users[i].size, sizeof(char));
                }
                if (!users[i].name_len) {
                    char test_name[50];
                    memset (test_name, '\0', sizeof(test_name));
                    int mes_len = recv(users[i].socket, test_name, sizeof(test_name), 0);
                    if (mes_len == 0) {
                        alarm(TIMEOUT);
                        FD_CLR (users[i].socket, &available_sockets);
                        shutdown(users[i].socket, 1);
                        close(users[i].socket);
                        printf("%s left the chat\n", users[i].name);
                        users[i].socket = 0;
                        amount--;
                        continue;
                    }
                    if (!strchr(test_name, '\n')) {
                        users[i].check_name = 1;
                        memset (test_name, '\0', sizeof(test_name));
                        continue;
                    }
                    if (users[i].check_name == 1) {
                        users[i].check_name = 0;
                        memset (test_name, '\0', sizeof(test_name));
                        continue;
                    }
                    int availability_status = 0;
                    if ((availability_status = CheckNameAvailability (test_name, users)) != 1) {
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
                            default: {
                                strcpy (problem, "\033[0m# invalid name\nEnter your name, please: \033[32;22m");
                                break;
                            }
                        }
                        send(users[i].socket, problem, sizeof(problem), 0);
                    }
                    else {
                        alarm(TIMEOUT);
                        printf ("%s changed name to ", users[i].name);
                        memset(users[i].name, '\0', sizeof(users[i].name));
                        strncpy(users[i].name, test_name, strlen(test_name) - 2);
                        printf ("'%s'\n", users[i].name);
                        users[i].name_len = strlen(test_name);
                        char temp[100];
                        memset(temp, '\0', sizeof(temp));
                        sprintf (temp, "%s%s%s%s%s", RESET, "Welcome, ", users[i].name, "!\n", DIR);
                        send (users[i].socket, temp, sizeof(temp), 0);
                    }
                    memset (test_name, '\0', sizeof(test_name));
                    continue;
                }
                alarm(TIMEOUT);
                memset(package, '\0', sizeof(package));
                int mes_len = recv(users[i].socket, package, sizeof(package), 0);
                if ((!strncmp(package, "bye!\r", 5) && users[i].mes_len == 0) || mes_len == 0) {
                    FD_CLR (users[i].socket, &available_sockets);
                    shutdown(users[i].socket, 1);
                    close(users[i].socket);
                    printf("%s left the chat\n", users[i].name);
                    users[i].socket = 0;
                    amount--;
                    continue;
                }
                char *fre;
                if (!(fre = strchr(package, '\n'))) {
                    if ((users[i].size - users[i].mes_len) < sizeof(package)) {
                        GiveMoreSpace(&users[i].message, &users[i].size);
                    }
                    strcat(users[i].message, package);
                    users[i].mes_len += sizeof(package);
                }
                else {
                    if ((users[i].size - users[i].mes_len) < (strchr(package, '\n') - package)) {
                        GiveMoreSpace(&users[i].message, &users[i].size);
                    }
                    users[i].mes_len += strchr(package, '\n') - package + 1;
                    strncat(users[i].message, package, (strchr(package, '\n') - package + 1));
                    printf("[%s]: %s", users[i].name, users[i].message);
                    for (int k = 0; k < QMAX; k++) {
                        if (users[k].socket > 0 && k != i && users[k].name_len) {
                            char *package_others = (char *)malloc((users[i].mes_len + 100) * sizeof(char));
                            memset (package_others, '\0', (users[i].mes_len + 100));
                            //sprintf (package_others, "%s%30s\n%s  %30s", HOST, users[i].name, DIR, users[i].message);
                            sprintf (package_others, "%s%30s\n%s  %30s\n", HOST, users[i].name, DIR, users[i].message);
                            send (users[k].socket, package_others, (users[i].mes_len + 100), 0);
                        }
                    }
                    users[i].mes_len = 0;
                    free (users[i].message);
                }
            }
        }
    }

    shutdown(main_socket, 1);
    close(main_socket);
 
    return 0;
}