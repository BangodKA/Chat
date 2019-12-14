#include <stdio.h>
#include "main/main.h"

int main(int argc, char **argv) {
    const char* port = argc == 2 ? argv[1] : "5599";
    RunServer(port);
 
    return 0;
}