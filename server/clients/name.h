#pragma once

#include "common.h"

#define NAME_PROMPT "Enter your name, please: \033[32;22m\n"

int NameHasNoEndSymbol (char *test_name, int *check_name);

int CheckLetters (char *name);

int CheckOverlap (char * name, client *users);

int CheckNameAvailability (char *name, client *users, int *check_name);

int NotUsableName (char *test_name, client *users, int i);

void HandleUsableName (char *test_name, client *users, int i);

void CheckName (char *test_name, client *users, int i);
  
int GetName (client *users, int i, fd_set *available_sockets, int *clients_amount);