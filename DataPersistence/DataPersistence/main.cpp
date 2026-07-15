#include <filesystem>
#include <iostream>

#include "SaveManager.h"

namespace {
void printData(const PlayerSaveData& data) {
    std::cout << "playerName: " << data.playerName << std::endl;
    std::cout << "level: " << data.level << std::endl;
    std::cout << "hp: " << data.hp << "/" << data.maxHp << std::endl;
    std::cout << "position: (" << data.position.x << ", " << data.position.y << ", "
               << data.position.z << ")" << std::endl;
}
}

int main() {
    const std::string savePath = "save.json";
    std::filesystem::remove(savePath);

    std::cout << "1) 최초 로드 (파일 없음, 기본값 기대):" << std::endl;
    PlayerSaveData data = SaveManager::load(savePath);
    printData(data);

    data.level = 7;
    data.hp = 55;
    data.position = {10.0, 20.0, 30.0};

    std::cout << std::endl << "2) 수정된 데이터 저장" << std::endl;
    bool saved = SaveManager::save(data, savePath);
    std::cout << "저장 성공 여부: " << (saved ? "true" : "false") << std::endl;

    std::cout << std::endl << "3) 다시 로드 (저장된 값 복원 기대):" << std::endl;
    PlayerSaveData reloaded = SaveManager::load(savePath);
    printData(reloaded);

    return 0;
}
