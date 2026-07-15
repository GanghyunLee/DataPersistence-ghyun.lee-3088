#include "SaveManager.h"

#include <fstream>
#include <iostream>
#include <sstream>

#include "JsonParser.h"

bool SaveManager::save(const PlayerSaveData& data, const std::string& path) {
    std::ofstream file(path, std::ios::trunc);
    if (!file.is_open()) {
        return false;
    }
    file << data.toJson().toString();
    return file.good();
}

PlayerSaveData SaveManager::load(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return PlayerSaveData{};
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    std::string text = buffer.str();

    try {
        JsonValue json = JsonParser::parse(text);
        return PlayerSaveData::fromJson(json);
    } catch (const std::exception& e) {
        std::cout << "저장 파일이 손상되어 기본값으로 대체합니다: " << e.what() << std::endl;
        return PlayerSaveData{};
    }
}
