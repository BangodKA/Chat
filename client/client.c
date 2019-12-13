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

int finish = 0;

void StopHandler (int s) {
    finish = 1;
}

void GiveMoreSpace (char **small, int *data_volume) {
    *data_volume *= 2;
    char* big = (char *)malloc(*data_volume * sizeof(char));
    memset (big, '\0', *data_volume);
    strcpy (big, *small);
    free(*small);
    *small = big;
}

int main (int argc, char **argv) {
    struct sockaddr_in addr; 
    int sock = socket(AF_INET, SOCK_STREAM, 0); 

    if (sock < 0) {
        perror("Can't create socket! ");
        return 0;
    }
    addr.sin_family = AF_INET; 
    addr.sin_port = htons(5599); 
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if(connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Can't connect socket! ");
        return  0;
    }

    signal (SIGINT, StopHandler);

    while (1) {
        if (finish) {
            break;
        }
        char buf[100];
        memset (buf, '\0', sizeof(buf));
        recv(sock, buf, sizeof(buf), 0);
        printf("%s", buf);
        if (buf[0] == 'W') {
            break;
        }
        char c;
        int size = 8;
        int length = 0;
        char *name = (char *)malloc(size * sizeof(char));
        memset (name, '\0', size);
        while ((c = getchar()) != '\n') {
            name[length] = c;
            length++;
            if (length == size) {
                GiveMoreSpace (&name, &size);
            }
        }
        if (length + 2 == size) {
            GiveMoreSpace (&name, &size);
        }
        name[length] = '\r';
        name[length + 1] = '\n';
        send (sock, name, length + 2, 0);
    }

    while (1) {
        if (finish) {
            break;
        }
        char c;
        int size = 8;
        int length = 0;
        char *message = (char *)malloc(size * sizeof(char));
        memset (message, '\0', size);
        while ((c = getchar()) != '\n') {
            message[length] = c;
            length++;
            if (length == size) {
                GiveMoreSpace (&message, &size);
            }
        }
        if (length + 2 == size) {
            GiveMoreSpace (&message, &size);
        }
        message[length] = '\r';
        message[length + 1] = '\n';
        send (sock, message, length + 2, 0);
        if (!strcmp(message, "bye!\r\n")) {
            break;
        }

        fd_set available_socket;
        FD_ZERO(&available_socket);
        FD_SET(sock, &available_socket);
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        int status = select(sock + 1, &available_socket, NULL, NULL, &timeout);
        switch (status) {
            case -1: {
                continue;
            };
            case 0: {
                continue;
            };
            default:    break;    
       
        }

        /*size = 8;
        length = 0;
        char *companion_message = (char *)malloc(size * sizeof(char));
        while (1) {
            char *fre;
            char package[8];
            fcntl(sock, F_SETFL, O_NONBLOCK);
            int status = recv (sock, package, sizeof(package), 0);
            fcntl(sock, F_SETFL, 0);
            if (status == -1) {
                break;
            }
            if (!(fre = strchr(package, '\n'))) {
                if ((size - length) < sizeof(package)) {
                    GiveMoreSpace(&companion_message, &size);
                }
                strcat(companion_message, package);
                length += sizeof(package);
            }
            else {
                if ((size - length) < (strchr(package, '\n') - package)) {
                    GiveMoreSpace(&companion_message, &size);
                }
                strncat(companion_message, package, (strchr(package, '\n') - package + 1));
                printf("%s", companion_message);
                break;
            }
        }*/
    }

    shutdown(sock, 1);
    close(sock);

    return 0;
}