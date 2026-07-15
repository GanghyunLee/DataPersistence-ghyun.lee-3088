#include "JsonValue.h"

#include <sstream>

namespace {
std::string escapeString(const std::string& s) {
    std::string result;
    for (char c : s) {
        if (c == '"' || c == '\\') {
            result += '\\';
        }
        result += c;
    }
    return result;
}
}

JsonValue::JsonValue() : m_value(std::monostate{}) {}
JsonValue::JsonValue(std::string value) : m_value(std::move(value)) {}
JsonValue::JsonValue(const char* value) : m_value(std::string(value)) {}
JsonValue::JsonValue(double value) : m_value(value) {}
JsonValue::JsonValue(bool value) : m_value(value) {}
JsonValue::JsonValue(std::map<std::string, JsonValue> value) : m_value(std::move(value)) {}

bool JsonValue::isEmpty() const { return std::holds_alternative<std::monostate>(m_value); }
bool JsonValue::isString() const { return std::holds_alternative<std::string>(m_value); }
bool JsonValue::isNumber() const { return std::holds_alternative<double>(m_value); }
bool JsonValue::isBool() const { return std::holds_alternative<bool>(m_value); }
bool JsonValue::isObject() const { return std::holds_alternative<std::map<std::string, JsonValue>>(m_value); }

const std::string& JsonValue::asString() const {
    static const std::string empty;
    if (!isString()) {
        return empty;
    }
    return std::get<std::string>(m_value);
}

double JsonValue::asNumber() const {
    if (!isNumber()) {
        return 0.0;
    }
    return std::get<double>(m_value);
}

bool JsonValue::asBool() const {
    if (!isBool()) {
        return false;
    }
    return std::get<bool>(m_value);
}

const JsonValue& JsonValue::operator[](const std::string& key) const {
    static const JsonValue empty;
    if (!isObject()) {
        return empty;
    }
    const auto& obj = std::get<std::map<std::string, JsonValue>>(m_value);
    auto it = obj.find(key);
    if (it == obj.end()) {
        return empty;
    }
    return it->second;
}

void JsonValue::set(const std::string& key, JsonValue value) {
    if (!isObject()) {
        m_value = std::map<std::string, JsonValue>{};
    }
    std::get<std::map<std::string, JsonValue>>(m_value)[key] = std::move(value);
}

std::string JsonValue::toString(int indent) const {
    std::string pad(static_cast<size_t>(indent) * 2, ' ');
    std::string childPad(static_cast<size_t>(indent + 1) * 2, ' ');

    if (isString()) {
        return "\"" + escapeString(asString()) + "\"";
    }
    if (isNumber()) {
        std::ostringstream oss;
        oss << asNumber();
        return oss.str();
    }
    if (isBool()) {
        return asBool() ? "true" : "false";
    }
    if (isObject()) {
        const auto& obj = std::get<std::map<std::string, JsonValue>>(m_value);
        if (obj.empty()) {
            return "{}";
        }
        std::ostringstream oss;
        oss << "{\n";
        size_t i = 0;
        for (const auto& [key, value] : obj) {
            oss << childPad << "\"" << escapeString(key) << "\": " << value.toString(indent + 1);
            if (++i != obj.size()) {
                oss << ",";
            }
            oss << "\n";
        }
        oss << pad << "}";
        return oss.str();
    }
    return "null";
}
