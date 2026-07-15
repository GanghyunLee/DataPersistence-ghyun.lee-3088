#pragma once

#include <string>

#include "PlayerSaveData.h"

class SaveManager {
public:
    static bool save(const PlayerSaveData& data, const std::string& path);
    static PlayerSaveData load(const std::string& path);
};
