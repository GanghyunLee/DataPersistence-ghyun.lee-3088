#pragma once

#include <string>

#include "JsonValue.h"

struct PlayerSaveData {
    std::string playerName = "Hero";
    int level = 1;
    int hp = 100;
    int maxHp = 100;

    struct Position {
        double x = 0.0;
        double y = 0.0;
        double z = 0.0;
    } position;

    JsonValue toJson() const;
    static PlayerSaveData fromJson(const JsonValue& json);
};
