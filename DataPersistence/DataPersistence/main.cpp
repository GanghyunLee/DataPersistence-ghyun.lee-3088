#include <iostream>

#include "PlayerSaveData.h"

int main() {
    PlayerSaveData data;
    data.playerName = "Aria";
    data.level = 5;
    data.hp = 42;
    data.maxHp = 80;
    data.position = {1.5, 2.5, 3.5};

    JsonValue json = data.toJson();
    std::cout << "저장될 JSON:" << std::endl << json.toString() << std::endl;

    PlayerSaveData restored = PlayerSaveData::fromJson(json);
    std::cout << std::endl << "복원된 데이터:" << std::endl;
    std::cout << "playerName: " << restored.playerName << std::endl;
    std::cout << "level: " << restored.level << std::endl;
    std::cout << "hp: " << restored.hp << std::endl;
    std::cout << "maxHp: " << restored.maxHp << std::endl;
    std::cout << "position: (" << restored.position.x << ", " << restored.position.y << ", "
               << restored.position.z << ")" << std::endl;

    return 0;
}
