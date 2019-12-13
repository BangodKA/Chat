#pragma once

#include <netinet/in.h>
#include "../clients/actions.h"

void TimeHandler (int s);

void StopHandler (int s);

int PrepareServer(int *main_socket, struct sockaddr_in *main_addr, char *s);

void SetUpSigHndl ();

void RunServer(char *s);