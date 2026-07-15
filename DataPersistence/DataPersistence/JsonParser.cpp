#include "JsonParser.h"

#include <cctype>
#include <stdexcept>

JsonValue JsonParser::parse(const std::string& text) {
    JsonParser parser(text);
    parser.skipWhitespace();
    JsonValue result = parser.parseValue();
    return result;
}

JsonParser::JsonParser(const std::string& text) : m_text(text), m_pos(0) {}

void JsonParser::skipWhitespace() {
    while (m_pos < m_text.size() && std::isspace(static_cast<unsigned char>(m_text[m_pos]))) {
        ++m_pos;
    }
}

char JsonParser::peek() const {
    if (m_pos >= m_text.size()) {
        throw std::runtime_error("Unexpected end of JSON input");
    }
    return m_text[m_pos];
}

char JsonParser::next() {
    char c = peek();
    ++m_pos;
    return c;
}

void JsonParser::expect(char c) {
    if (next() != c) {
        throw std::runtime_error(std::string("Expected '") + c + "' in JSON input");
    }
}

JsonValue JsonParser::parseValue() {
    skipWhitespace();
    char c = peek();
    if (c == '{') {
        return parseObject();
    }
    if (c == '"') {
        return parseString();
    }
    if (c == 't' || c == 'f') {
        return parseBool();
    }
    if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) {
        return parseNumber();
    }
    if (c == '[') {
        throw std::runtime_error("JSON arrays are not supported");
    }
    throw std::runtime_error("Unexpected character in JSON input");
}

JsonValue JsonParser::parseObject() {
    expect('{');
    std::map<std::string, JsonValue> result;
    skipWhitespace();
    if (peek() == '}') {
        next();
        return JsonValue(result);
    }
    while (true) {
        skipWhitespace();
        JsonValue keyValue = parseString();
        std::string key = keyValue.asString();
        skipWhitespace();
        expect(':');
        skipWhitespace();
        JsonValue value = parseValue();
        result[key] = value;
        skipWhitespace();
        char c = next();
        if (c == ',') {
            continue;
        }
        if (c == '}') {
            break;
        }
        throw std::runtime_error("Expected ',' or '}' in JSON object");
    }
    return JsonValue(result);
}

JsonValue JsonParser::parseString() {
    expect('"');
    std::string result;
    while (true) {
        char c = next();
        if (c == '"') {
            break;
        }
        if (c == '\\') {
            char escaped = next();
            result += escaped;
        } else {
            result += c;
        }
    }
    return JsonValue(result);
}

JsonValue JsonParser::parseNumber() {
    size_t start = m_pos;
    if (peek() == '-') {
        next();
    }
    while (m_pos < m_text.size() &&
           (std::isdigit(static_cast<unsigned char>(m_text[m_pos])) || m_text[m_pos] == '.')) {
        ++m_pos;
    }
    std::string numberText = m_text.substr(start, m_pos - start);
    return JsonValue(std::stod(numberText));
}

JsonValue JsonParser::parseBool() {
    if (m_text.compare(m_pos, 4, "true") == 0) {
        m_pos += 4;
        return JsonValue(true);
    }
    if (m_text.compare(m_pos, 5, "false") == 0) {
        m_pos += 5;
        return JsonValue(false);
    }
    throw std::runtime_error("Invalid boolean literal in JSON input");
}
