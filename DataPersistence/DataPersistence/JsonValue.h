#pragma once

#include <map>
#include <string>
#include <variant>

// std::monostate는 "값이 아직 설정되지 않음"을 나타내는 내부 상태이며,
// JSON의 null 리터럴을 파싱/생성하는 기능은 지원하지 않는다.
class JsonValue {
public:
    JsonValue();
    JsonValue(std::string value);
    JsonValue(const char* value);
    JsonValue(double value);
    JsonValue(bool value);
    JsonValue(std::map<std::string, JsonValue> value);

    bool isEmpty() const;
    bool isString() const;
    bool isNumber() const;
    bool isBool() const;
    bool isObject() const;

    const std::string& asString() const;
    double asNumber() const;
    bool asBool() const;

    const JsonValue& operator[](const std::string& key) const;
    void set(const std::string& key, JsonValue value);

    std::string toString(int indent = 0) const;

private:
    std::variant<std::monostate, std::string, double, bool, std::map<std::string, JsonValue>> m_value;
};
