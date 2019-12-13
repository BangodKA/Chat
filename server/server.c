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
#define NAMEBUFSIZE 50
#define MESBUFSIZE 8

int action = 0; // To detect, whether SIGALRM came or not
int finish = 0; // To detect, whether SIGINT came or not

typedef struct Client{
    char name[50];
    char *message;
    int socket;
    int mes_len;
    int size;
    int check_name;
}client;

void TimeHandler (int s) {
    action = 1;
    alarm(TIMEOUT);
}

void StopHandler (int s) {
    finish = 1;
}

int FindSymbol (char *package, int size, char symbol) {
    for (int i = 0; i < size; i++) {
        if (package[i] == symbol) {
            return i;
        }
        if (package[i] == '\0') {
            break;
        }
    }
    return -1;
}

void GiveMoreSpace (char **small, int *data_volume) {
    *data_volume *= 2;
    char* big = (char *)malloc(*data_volume * sizeof(char));
    memset (big, '\0', *data_volume);
    strcpy (big, *small);
    free(*small);
    *small = big;
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

int CheckLetters (char *name) {
    for (int i = 0; i < strlen(name) - 2; i++) {
        if (name[i] != '_' && !((name[i] >= 'a' && name[i] <= 'z') || (name[i] >= 'A' && name[i] <= 'Z'))) {
            return 0;
        }
    }
    return 1;
}

int CheckOverlap (char * name, client users[QMAX]) {
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

int CheckNameAvailability (char *name, client users[QMAX], int *check_name) {
    if (*check_name == -1) {
        *check_name = 0;
        return 0;
    }
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
        char buf[60] = "# unfortunately, server is full...\n# try again later\n";
        send(new_socket, buf, sizeof(buf), 0);
        shutdown(new_socket, 1);
        close(new_socket);
}

void ConnectNewUser (client *users, int free_index, int new_socket, int *clients_amount) {
    alarm(TIMEOUT);
    users[free_index].socket = new_socket;
    char buf[100];
    memset (buf, '\0', sizeof(buf));
    strcpy (buf, "Enter your name, please: \033[32;22m");
    char temp[4]; 
    memset (temp, '\0', sizeof(temp));
    sprintf(temp, "%d", free_index + 1);
    strcpy(users[free_index].name, "user_");
    strcat(users[free_index].name, temp);
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

int HandleLeaving (int mes_len, fd_set *available_sockets, int *clients_amount, client *user, char *package) {
    if (mes_len == 0 || (!strncmp(package, "bye!\r", 5) && (*user).mes_len == 0)) {
        alarm(TIMEOUT);
        FD_CLR ((*user).socket, available_sockets);
        shutdown((*user).socket, 1);
        close((*user).socket);
        printf("%s left the chat\n", (*user).name);
        (*user).socket = 0;
        (*clients_amount)--;
        return 0;
    }
    return 1;
}

int NameHasNoEndSymbol (char *test_name, int *check_name) {
    if (!strchr(test_name, '\n')) {
        *check_name = -1;
        memset (test_name, '\0', NAMEBUFSIZE);
        return 1;
    }
    return 0;
}

int NotUsableName (char *test_name, client *users, int i) {
    int availability_status = 0;
    if ((availability_status = CheckNameAvailability (test_name, users, &users[i].check_name)) != 1) {
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
            default: break;
        }
        send(users[i].socket, problem, sizeof(problem), 0);
        return 1;
    }
    return 0;
}

void HandleUsableName (char *test_name, client *users, int i) {
    alarm(TIMEOUT);
    printf ("%s changed name to ", users[i].name);
    memset(users[i].name, '\0', sizeof(users[i].name));
    strncpy(users[i].name, test_name, strlen(test_name) - 2);
    printf ("'%s'\n", users[i].name);
    users[i].check_name = 1;
    char temp[100];
    memset(temp, '\0', sizeof(temp));
    sprintf (temp, "%s%s%s%s%s", RESET, "Welcome, ", users[i].name, "!\n", DIR);
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

int IsLastPackage (int end_index_n, int end_index_r, client * user, char *package) {
    if (end_index_n == -1 && end_index_r == -1) {
        if (((*user).size - (*user).mes_len) < MESBUFSIZE - 1) {
            GiveMoreSpace(&(*user).message, &(*user).size);
        }
        strncat((*user).message, package, MESBUFSIZE - 1);
        (*user).mes_len += MESBUFSIZE - 1;
        return 0;
    }
    return 1;
}

void GatherWholeMessage (int end_index_r, int end_index_n, client *users, int i, char *package) {
    alarm(TIMEOUT);
    if ((users[i].size - users[i].mes_len) < end_index_r + 1) {
        GiveMoreSpace(&users[i].message, &users[i].size);
    }
    package[end_index_r] = '\n';
    users[i].mes_len += end_index_r + 1;
    strncat(users[i].message, package, end_index_r + 1);
    printf("[%s]: %s", users[i].name, users[i].message);
}

void SendOtherUsers (client *users, int i) {
    for (int k = 0; k < QMAX; k++) {
        if (users[k].socket > 0 && k != i && users[k].check_name) {
            char *package_others = (char *)malloc((users[i].mes_len + 100) * sizeof(char));
            memset (package_others, '\0', (users[i].mes_len + 100));
            sprintf (package_others, "%s%30s\n%s %30s\n", HOST, users[i].name, DIR, users[i].message);
            send (users[k].socket, package_others, (users[i].mes_len + 100), 0);
        }
    }
}

int GetSendMessage (char *package, client *users, int i) {
    int end_index_r = FindSymbol(package, MESBUFSIZE, '\r');
    int end_index_n = FindSymbol(package, MESBUFSIZE, '\n');

    if (IsLastPackage (end_index_n, end_index_r, &users[i], package)) {
        if ((end_index_r == 0 && !strlen(users[i].message)) || end_index_n == 0) {
            return 0;
        }
        GatherWholeMessage (end_index_r, end_index_n, users, i, package);

        SendOtherUsers (users, i);
        
        users[i].mes_len = 0;
        free (users[i].message);
    }
    return 1;
}

int HandleMessage (client *users, int i, int *clients_amount, fd_set *available_sockets) {
    char package[MESBUFSIZE];
    memset(package, '\0', MESBUFSIZE);
    int mes_len = recv(users[i].socket, package, MESBUFSIZE - 1, 0);

    if (!HandleLeaving (mes_len, available_sockets, clients_amount, &users[i], package)) {
        return 0;
    }

    if (!GetSendMessage (package, users, i)) {
        return 0;
    }
    return 1;
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

void SetUpSigHndl () {
    alarm(TIMEOUT); 
    signal (SIGALRM, TimeHandler);
    signal (SIGINT, StopHandler);  
}

int main(int argc, char **argv) {
    int main_socket;
    client users[QMAX];
    memset (users, 0, QMAX * sizeof(client));
    struct sockaddr_in main_addr;
    int clients_amount = 0;
    fd_set available_sockets;

    if (!PrepareServer(&main_socket, &main_addr, "5599")) {
        return 0;
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
 
    return 0;
}