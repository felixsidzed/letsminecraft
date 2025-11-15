#include <iostream>

#include "server/server.h"

int main() {
    lmc::Server server(4);
    server.description = "letsminecraft - A Minecraft server API written in C++, from scratch";
    server.listen(25565);
}
