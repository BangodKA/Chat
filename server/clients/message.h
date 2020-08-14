#pragma once

#include "common.h"

int IsLastPackage (int end_index_n, int end_index_r, client * user, char *package);

void GatherWholeMessage (int end_index_r, int end_index_n, client *users, int i, char *package);

void SendChosenUsers (client *users, int i, char *message);

int FindSymbol (char *package, int size, char symbol);

int GetSendMessage (char *package, client *users, int i);

int HandleMessage (client *users, int i, int *clients_amount, fd_set *available_sockets);