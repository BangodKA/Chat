#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h> 
#include <sys/wait.h>
#include <signal.h>

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
    int port = argc == 2 ? atoi(argv[1]) : 5599;
    addr.sin_port = htons(port); 
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Can't connect socket! ");
        return  0;
    }

    signal (SIGINT, StopHandler);

    pid_t sig_kill = fork();
    if (sig_kill == 0) {
        while (!finish) {
            if (getppid() == 1) {
                exit(1);
            }
        }
        printf("\n");
        kill(getppid(), SIGKILL);
    }
    else {
        int fd[2];
        pipe(fd);

        int name_status = 0;

        pid_t child = fork();
        if (child == 0) {
            close(fd[0]);
            while (1) {
                char buf[100];
                memset (buf, '\0', sizeof(buf));
                while (!strcmp(buf, "")) {
                    if (getppid() == 1) {
                        exit(1);
                    }
                    recv(sock, buf, sizeof(buf), 0);
                }
                printf("%s", buf);
                if (!strcmp(buf, "# unfortunately, server is full...\n# try again later\n") 
                    || !strcmp(buf, "Connection closed by foreign host.\n")) {
                    break;
                }
                if (!name_status) {
                    if (strstr(buf, "Welcome")) {
                        name_status = 1;
                    }
                    write(fd[1], &name_status, sizeof(name_status));
                }
                else {
                    close(fd[1]);
                    printf("\033[32;22m");
                }
            }
            kill(getppid(), SIGKILL);
        }
        else {
            close(fd[1]);

            while (1) {

                if (!name_status) {
                    read(fd[0], &name_status, sizeof(name_status));
                }
                else {
                    close(fd[0]);
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

                if (!strcmp(name, "bye!\r\n") && name_status) {
                    break;
                }
            }

            kill(child, SIGKILL);
        }
        kill(sig_kill, SIGKILL);
    }
    shutdown(sock, 1);
    close(sock);

    return 0;
}