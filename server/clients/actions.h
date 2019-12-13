#pragma once

#include "common.h"
#include "name.h"
#include "message.h"

int action; // To detect, whether SIGALRM came or not
int finish; // To detect, whether SIGINT came or not

int FindAvailableSockets (fd_set *available_sockets, int *free_index, int main_socket, client *users);

void HandleAlarm (int clients_amount);

int FindActiveSockets (fd_set *available_sockets, int max_sd, int clients_amount);

void TooManyUsers (int new_socket, int clients_amount);

void ConnectNewUser (client *users, int free_index, int new_socket, int *clients_amount);

void HandleAccept (int main_socket, int free_index, int *clients_amount, client *users);

void PrepareNewUser (client *user);

void HandleUsersActions (client *users, fd_set *available_sockets, int *clients_amount) ;