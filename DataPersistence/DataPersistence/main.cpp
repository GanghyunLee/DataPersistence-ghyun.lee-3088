#include <iostream>

#include "JsonParser.h"
#include "JsonValue.h"

int main() {
    JsonValue player;
    player.set("playerName", std::string("Hero"));
    player.set("level", 1.0);
    player.set("hp", 100.0);

    std::string serialized = player.toString();
    std::cout << "직렬화:" << std::endl << serialized << std::endl;

    JsonValue parsed = JsonParser::parse(serialized);
    std::cout << std::endl << "파싱 후 재직렬화:" << std::endl << parsed.toString() << std::endl;

    return 0;
}
