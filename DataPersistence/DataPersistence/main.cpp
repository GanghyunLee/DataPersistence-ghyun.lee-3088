#include <iostream>

#include "JsonValue.h"

int main() {
    JsonValue player;
    player.set("playerName", std::string("Hero"));
    player.set("level", 1.0);
    player.set("hp", 100.0);

    std::cout << player.toString() << std::endl;

    return 0;
}
