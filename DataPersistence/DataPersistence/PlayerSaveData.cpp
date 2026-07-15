#include "PlayerSaveData.h"

JsonValue PlayerSaveData::toJson() const {
    JsonValue json;
    json.set("playerName", playerName);
    json.set("level", static_cast<double>(level));
    json.set("hp", static_cast<double>(hp));
    json.set("maxHp", static_cast<double>(maxHp));

    JsonValue positionJson;
    positionJson.set("x", position.x);
    positionJson.set("y", position.y);
    positionJson.set("z", position.z);
    json.set("position", positionJson);

    return json;
}

PlayerSaveData PlayerSaveData::fromJson(const JsonValue& json) {
    PlayerSaveData data;

    const JsonValue& name = json["playerName"];
    if (name.isString()) {
        data.playerName = name.asString();
    }

    const JsonValue& level = json["level"];
    if (level.isNumber()) {
        data.level = static_cast<int>(level.asNumber());
    }

    const JsonValue& hp = json["hp"];
    if (hp.isNumber()) {
        data.hp = static_cast<int>(hp.asNumber());
    }

    const JsonValue& maxHp = json["maxHp"];
    if (maxHp.isNumber()) {
        data.maxHp = static_cast<int>(maxHp.asNumber());
    }

    const JsonValue& position = json["position"];
    if (position.isObject()) {
        const JsonValue& x = position["x"];
        if (x.isNumber()) {
            data.position.x = x.asNumber();
        }
        const JsonValue& y = position["y"];
        if (y.isNumber()) {
            data.position.y = y.asNumber();
        }
        const JsonValue& z = position["z"];
        if (z.isNumber()) {
            data.position.z = z.asNumber();
        }
    }

    return data;
}
