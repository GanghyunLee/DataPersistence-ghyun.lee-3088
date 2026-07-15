#pragma once

#include <string>

#include "JsonValue.h"

class JsonParser {
public:
    // 텍스트를 JsonValue 트리로 변환한다. 배열을 포함하거나 문법이 잘못된 입력에는
    // std::runtime_error를 던진다.
    static JsonValue parse(const std::string& text);

private:
    explicit JsonParser(const std::string& text);

    JsonValue parseValue();
    JsonValue parseObject();
    JsonValue parseString();
    JsonValue parseNumber();
    JsonValue parseBool();

    void skipWhitespace();
    char peek() const;
    char next();
    void expect(char c);

    const std::string& m_text;
    size_t m_pos;
};
