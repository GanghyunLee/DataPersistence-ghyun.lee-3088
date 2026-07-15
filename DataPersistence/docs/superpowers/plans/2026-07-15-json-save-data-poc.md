# JSON 기반 게임 세이브 데이터 POC Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** `DataPersistence` C++20 콘솔 프로젝트에, 외부 라이브러리 없이 직접 구현한 최소 JSON 파서/라이터로 게임 세이브 데이터(플레이어 정보)를 파일에 저장하고 다시 불러오는 POC를 완성한다.

**Architecture:** `JsonValue`(값 모델+직렬화) → `JsonParser`(텍스트→`JsonValue`) → `PlayerSaveData`(도메인 구조체 ↔ `JsonValue` 변환) → `SaveManager`(파일 I/O + 기본값 대체). `main.cpp`가 각 계층을 단계적으로 시연한다.

**Tech Stack:** C++20 (MSVC v145 toolset), Visual Studio `.vcxproj`/`.slnx`, `std::variant`, `std::map`, `std::filesystem`. 외부 JSON 라이브러리 없음. 자동화된 유닛 테스트 프레임워크 없음(콘솔 출력 육안 확인).

## Global Constraints

- 언어 표준: C++20 (`stdcpp20`), 기존 4개 구성(Debug/Release × Win32/x64) 모두 컴파일 가능해야 함
- 외부 JSON 라이브러리 사용 금지 — `JsonValue`/`JsonParser`는 직접 구현
- 지원 JSON 값 타입: 문자열, 숫자, bool, 중첩 객체. **배열은 지원하지 않음** — 배열을 만나면 파싱 실패 처리
- assert 기반 테스트 코드는 작성하지 않음 — `main()` 빌드 후 실행하여 콘솔 출력을 육안으로 확인하는 것으로 검증
- 오류 처리: 파일 없음/열기 실패 시 조용히 기본값 사용, JSON 파싱 실패 시 콘솔 경고 후 기본값 사용, 일부 필드 누락/타입 불일치 시 해당 필드만 기본값 처리, 저장 실패는 `save()`가 `false` 반환 — 예외로 프로그램이 죽는 경로는 없음
- 소스 파일에 한글 문자열 리터럴을 사용하므로 MSVC에 `/utf-8` 옵션이 필요함

---

### Task 1: 프로젝트 배선 + `JsonValue`

**Files:**
- Create: `DataPersistence/JsonValue.h`
- Create: `DataPersistence/JsonValue.cpp`
- Modify: `DataPersistence/DataPersistence.vcxproj` (빈 `<ItemGroup></ItemGroup>`에 소스 등록, 빌드 출력 경로 고정, `/utf-8` 옵션 추가)
- Modify: `DataPersistence/DataPersistence.vcxproj.filters` (필터 등록)
- Create: `DataPersistence/main.cpp`

**Interfaces:**
- Produces: `class JsonValue` — 생성자 `JsonValue()`, `JsonValue(std::string)`, `JsonValue(const char*)`, `JsonValue(double)`, `JsonValue(bool)`, `JsonValue(std::map<std::string, JsonValue>)`; `bool isEmpty()/isString()/isNumber()/isBool()/isObject() const`; `const std::string& asString() const`; `double asNumber() const`; `bool asBool() const`; `const JsonValue& operator[](const std::string&) const`; `void set(const std::string&, JsonValue)`; `std::string toString(int indent = 0) const`

- [ ] **Step 1: `JsonValue.h` 작성**

```cpp
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
```

- [ ] **Step 2: `JsonValue.cpp` 작성**

```cpp
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
```

- [ ] **Step 3: `DataPersistence.vcxproj`에 소스 등록 및 빌드 설정 추가**

`<PropertyGroup Label="UserMacros" />` 바로 다음에 아래 `PropertyGroup`을 추가해 빌드 출력 경로를 고정한다 (4개 구성 전체에 적용):

```xml
  <PropertyGroup Label="UserMacros" />

  <PropertyGroup>
    <OutDir>$(SolutionDir)build\$(Platform)\$(Configuration)\</OutDir>
    <IntDir>$(SolutionDir)build\obj\$(Platform)\$(Configuration)\</IntDir>
  </PropertyGroup>
```

4개의 `<ItemDefinitionGroup>` 각각의 `<ClCompile>` 블록에 `/utf-8` 옵션을 추가한다 (예: Debug|x64):

```xml
    <ClCompile>
      <WarningLevel>Level3</WarningLevel>
      <SDLCheck>true</SDLCheck>
      <PreprocessorDefinitions>_DEBUG;_CONSOLE;%(PreprocessorDefinitions)</PreprocessorDefinitions>
      <ConformanceMode>true</ConformanceMode>
      <LanguageStandard>stdcpp20</LanguageStandard>
      <AdditionalOptions>/utf-8 %(AdditionalOptions)</AdditionalOptions>
    </ClCompile>
```

동일하게 나머지 3개 `<ItemDefinitionGroup>`(Release|Win32, Debug|Win32, Release|x64)의 `<ClCompile>`에도 `<AdditionalOptions>/utf-8 %(AdditionalOptions)</AdditionalOptions>`를 추가한다.

파일 맨 아래쪽의 빈 `<ItemGroup></ItemGroup>`을 다음으로 교체한다:

```xml
  <ItemGroup>
    <ClInclude Include="JsonValue.h" />
    <ClCompile Include="JsonValue.cpp" />
    <ClCompile Include="main.cpp" />
  </ItemGroup>
```

- [ ] **Step 4: `DataPersistence.vcxproj.filters`에 필터 등록**

`<ItemGroup>` 태그 뒤(파일 끝, `</Project>` 앞)에 새 `<ItemGroup>`을 추가한다:

```xml
  <ItemGroup>
    <ClInclude Include="JsonValue.h">
      <Filter>헤더 파일</Filter>
    </ClInclude>
    <ClCompile Include="JsonValue.cpp">
      <Filter>소스 파일</Filter>
    </ClCompile>
    <ClCompile Include="main.cpp">
      <Filter>소스 파일</Filter>
    </ClCompile>
  </ItemGroup>
```

- [ ] **Step 5: `main.cpp` 작성 (1단계 시연: `JsonValue` 직렬화)**

```cpp
#include <iostream>

#include "JsonValue.h"

int main() {
    JsonValue player;
    player.set("playerName", std::string("Hero"));
    player.set("level", 1.0);
    player.set("hp", 100.0);

    std::cout << player.toString() << std::endl;

    return 0;
}
```

- [ ] **Step 6: 빌드**

Run: `msbuild DataPersistence.slnx /p:Configuration=Debug /p:Platform=x64`
Expected: `Build succeeded.` (오류/경고 없이 종료 코드 0)

- [ ] **Step 7: 실행 및 콘솔 출력 확인**

Run: `.\build\x64\Debug\DataPersistence.exe`
Expected (정확히 일치):

```
{
  "hp": 100,
  "level": 1,
  "playerName": "Hero"
}
```

(`std::map`은 키를 알파벳순으로 정렬하므로 `hp`, `level`, `playerName` 순서로 출력된다.)

- [ ] **Step 8: 커밋**

```bash
git add DataPersistence/JsonValue.h DataPersistence/JsonValue.cpp DataPersistence/main.cpp DataPersistence/DataPersistence.vcxproj DataPersistence/DataPersistence.vcxproj.filters
git commit -m "feat: add JsonValue model and wire project build"
```

---

### Task 2: `JsonParser`

**Files:**
- Create: `DataPersistence/JsonParser.h`
- Create: `DataPersistence/JsonParser.cpp`
- Modify: `DataPersistence/DataPersistence.vcxproj` (ItemGroup에 소스 추가)
- Modify: `DataPersistence/DataPersistence.vcxproj.filters` (필터 추가)
- Modify: `DataPersistence/main.cpp` (라운드트립 시연 추가)

**Interfaces:**
- Consumes: `JsonValue` (Task 1) — 생성자, `set()`, `toString()`, `isString()/asString()`
- Produces: `class JsonParser` — `static JsonValue JsonParser::parse(const std::string& text)`. 배열(`[`)을 만나거나 문법 오류가 있으면 `std::runtime_error`를 던진다.

- [ ] **Step 1: `JsonParser.h` 작성**

```cpp
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
```

- [ ] **Step 2: `JsonParser.cpp` 작성**

```cpp
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
```

- [ ] **Step 3: 프로젝트 파일에 등록**

`DataPersistence.vcxproj`의 `<ItemGroup>`을 다음으로 갱신한다:

```xml
  <ItemGroup>
    <ClInclude Include="JsonValue.h" />
    <ClInclude Include="JsonParser.h" />
    <ClCompile Include="JsonValue.cpp" />
    <ClCompile Include="JsonParser.cpp" />
    <ClCompile Include="main.cpp" />
  </ItemGroup>
```

`DataPersistence.vcxproj.filters`의 마지막 `<ItemGroup>`을 다음으로 갱신한다:

```xml
  <ItemGroup>
    <ClInclude Include="JsonValue.h">
      <Filter>헤더 파일</Filter>
    </ClInclude>
    <ClInclude Include="JsonParser.h">
      <Filter>헤더 파일</Filter>
    </ClInclude>
    <ClCompile Include="JsonValue.cpp">
      <Filter>소스 파일</Filter>
    </ClCompile>
    <ClCompile Include="JsonParser.cpp">
      <Filter>소스 파일</Filter>
    </ClCompile>
    <ClCompile Include="main.cpp">
      <Filter>소스 파일</Filter>
    </ClCompile>
  </ItemGroup>
```

- [ ] **Step 4: `main.cpp` 갱신 (라운드트립 시연)**

```cpp
#include <iostream>

#include "JsonParser.h"
#include "JsonValue.h"

int main() {
    JsonValue player;
    player.set("playerName", std::string("Hero"));
    player.set("level", 1.0);
    player.set("hp", 100.0);

    std::string serialized = player.toString();
    std::cout << "직렬화:" << std::endl << serialized << std::endl;

    JsonValue parsed = JsonParser::parse(serialized);
    std::cout << std::endl << "파싱 후 재직렬화:" << std::endl << parsed.toString() << std::endl;

    return 0;
}
```

- [ ] **Step 5: 빌드**

Run: `msbuild DataPersistence.slnx /p:Configuration=Debug /p:Platform=x64`
Expected: `Build succeeded.`

- [ ] **Step 6: 실행 및 콘솔 출력 확인**

Run: `.\build\x64\Debug\DataPersistence.exe`
Expected (정확히 일치, 두 블록이 동일해야 라운드트립 성공):

```
직렬화:
{
  "hp": 100,
  "level": 1,
  "playerName": "Hero"
}

파싱 후 재직렬화:
{
  "hp": 100,
  "level": 1,
  "playerName": "Hero"
}
```

- [ ] **Step 7: 커밋**

```bash
git add DataPersistence/JsonParser.h DataPersistence/JsonParser.cpp DataPersistence/main.cpp DataPersistence/DataPersistence.vcxproj DataPersistence/DataPersistence.vcxproj.filters
git commit -m "feat: add JsonParser and verify round-trip serialization"
```

---

### Task 3: `PlayerSaveData`

**Files:**
- Create: `DataPersistence/PlayerSaveData.h`
- Create: `DataPersistence/PlayerSaveData.cpp`
- Modify: `DataPersistence/DataPersistence.vcxproj` (ItemGroup에 소스 추가)
- Modify: `DataPersistence/DataPersistence.vcxproj.filters` (필터 추가)
- Modify: `DataPersistence/main.cpp` (`PlayerSaveData` 변환 시연으로 교체)

**Interfaces:**
- Consumes: `JsonValue` (Task 1) — 생성자, `set()`, `operator[]`, `isNumber()/asNumber()`, `isString()/asString()`, `isObject()`
- Produces: `struct PlayerSaveData` — 필드 `std::string playerName = "Hero"`, `int level = 1`, `int hp = 100`, `int maxHp = 100`, `struct Position { double x = 0.0, y = 0.0, z = 0.0; } position`; 메서드 `JsonValue toJson() const`, `static PlayerSaveData fromJson(const JsonValue&)`

- [ ] **Step 1: `PlayerSaveData.h` 작성**

```cpp
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
```

- [ ] **Step 2: `PlayerSaveData.cpp` 작성**

```cpp
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
```

- [ ] **Step 3: 프로젝트 파일에 등록**

`DataPersistence.vcxproj`의 `<ItemGroup>`을 다음으로 갱신한다:

```xml
  <ItemGroup>
    <ClInclude Include="JsonValue.h" />
    <ClInclude Include="JsonParser.h" />
    <ClInclude Include="PlayerSaveData.h" />
    <ClCompile Include="JsonValue.cpp" />
    <ClCompile Include="JsonParser.cpp" />
    <ClCompile Include="PlayerSaveData.cpp" />
    <ClCompile Include="main.cpp" />
  </ItemGroup>
```

`DataPersistence.vcxproj.filters`의 마지막 `<ItemGroup>`을 다음으로 갱신한다:

```xml
  <ItemGroup>
    <ClInclude Include="JsonValue.h">
      <Filter>헤더 파일</Filter>
    </ClInclude>
    <ClInclude Include="JsonParser.h">
      <Filter>헤더 파일</Filter>
    </ClInclude>
    <ClInclude Include="PlayerSaveData.h">
      <Filter>헤더 파일</Filter>
    </ClInclude>
    <ClCompile Include="JsonValue.cpp">
      <Filter>소스 파일</Filter>
    </ClCompile>
    <ClCompile Include="JsonParser.cpp">
      <Filter>소스 파일</Filter>
    </ClCompile>
    <ClCompile Include="PlayerSaveData.cpp">
      <Filter>소스 파일</Filter>
    </ClCompile>
    <ClCompile Include="main.cpp">
      <Filter>소스 파일</Filter>
    </ClCompile>
  </ItemGroup>
```

- [ ] **Step 4: `main.cpp` 갱신 (`PlayerSaveData` 변환 시연)**

```cpp
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
```

- [ ] **Step 5: 빌드**

Run: `msbuild DataPersistence.slnx /p:Configuration=Debug /p:Platform=x64`
Expected: `Build succeeded.`

- [ ] **Step 6: 실행 및 콘솔 출력 확인**

Run: `.\build\x64\Debug\DataPersistence.exe`
Expected (정확히 일치):

```
저장될 JSON:
{
  "hp": 42,
  "level": 5,
  "maxHp": 80,
  "playerName": "Aria",
  "position": {
    "x": 1.5,
    "y": 2.5,
    "z": 3.5
  }
}

복원된 데이터:
playerName: Aria
level: 5
hp: 42
maxHp: 80
position: (1.5, 2.5, 3.5)
```

- [ ] **Step 7: 커밋**

```bash
git add DataPersistence/PlayerSaveData.h DataPersistence/PlayerSaveData.cpp DataPersistence/main.cpp DataPersistence/DataPersistence.vcxproj DataPersistence/DataPersistence.vcxproj.filters
git commit -m "feat: add PlayerSaveData JSON conversion"
```

---

### Task 4: `SaveManager` + 전체 데모 시나리오

**Files:**
- Create: `DataPersistence/SaveManager.h`
- Create: `DataPersistence/SaveManager.cpp`
- Modify: `DataPersistence/DataPersistence.vcxproj` (ItemGroup에 소스 추가)
- Modify: `DataPersistence/DataPersistence.vcxproj.filters` (필터 추가)
- Modify: `DataPersistence/main.cpp` (전체 로드→수정→저장→재로드 데모로 교체)

**Interfaces:**
- Consumes: `PlayerSaveData` (Task 3) — `toJson()`, `fromJson()`; `JsonParser::parse()` (Task 2)
- Produces: `class SaveManager` — `static bool save(const PlayerSaveData& data, const std::string& path)`, `static PlayerSaveData load(const std::string& path)`

**설계상 단순화 노트:** `std::ifstream::is_open() == false`인 경우(파일 없음, 권한 실패 등)를 모두 "조용히 기본값 반환"으로 통합 처리한다. 실제 파일 시스템에서 두 원인을 구분하려면 플랫폼별 오류 코드 조회가 필요한데, 이는 POC 범위를 벗어나는 복잡도이므로 두 경우 모두 첫 실행과 동일하게 취급한다. 콘솔 경고가 필요한 유일한 경우는 파일은 열렸지만 JSON 파싱에 실패한 "손상된 파일" 상황이다.

- [ ] **Step 1: `SaveManager.h` 작성**

```cpp
#pragma once

#include <string>

#include "PlayerSaveData.h"

class SaveManager {
public:
    static bool save(const PlayerSaveData& data, const std::string& path);
    static PlayerSaveData load(const std::string& path);
};
```

- [ ] **Step 2: `SaveManager.cpp` 작성**

```cpp
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
```

- [ ] **Step 3: 프로젝트 파일에 등록**

`DataPersistence.vcxproj`의 `<ItemGroup>`을 다음으로 갱신한다:

```xml
  <ItemGroup>
    <ClInclude Include="JsonValue.h" />
    <ClInclude Include="JsonParser.h" />
    <ClInclude Include="PlayerSaveData.h" />
    <ClInclude Include="SaveManager.h" />
    <ClCompile Include="JsonValue.cpp" />
    <ClCompile Include="JsonParser.cpp" />
    <ClCompile Include="PlayerSaveData.cpp" />
    <ClCompile Include="SaveManager.cpp" />
    <ClCompile Include="main.cpp" />
  </ItemGroup>
```

`DataPersistence.vcxproj.filters`의 마지막 `<ItemGroup>`을 다음으로 갱신한다:

```xml
  <ItemGroup>
    <ClInclude Include="JsonValue.h">
      <Filter>헤더 파일</Filter>
    </ClInclude>
    <ClInclude Include="JsonParser.h">
      <Filter>헤더 파일</Filter>
    </ClInclude>
    <ClInclude Include="PlayerSaveData.h">
      <Filter>헤더 파일</Filter>
    </ClInclude>
    <ClInclude Include="SaveManager.h">
      <Filter>헤더 파일</Filter>
    </ClInclude>
    <ClCompile Include="JsonValue.cpp">
      <Filter>소스 파일</Filter>
    </ClCompile>
    <ClCompile Include="JsonParser.cpp">
      <Filter>소스 파일</Filter>
    </ClCompile>
    <ClCompile Include="PlayerSaveData.cpp">
      <Filter>소스 파일</Filter>
    </ClCompile>
    <ClCompile Include="SaveManager.cpp">
      <Filter>소스 파일</Filter>
    </ClCompile>
    <ClCompile Include="main.cpp">
      <Filter>소스 파일</Filter>
    </ClCompile>
  </ItemGroup>
```

- [ ] **Step 4: `main.cpp` 갱신 (전체 데모 시나리오, 손상된 파일 복구까지 자동 검증)**

이전 버전은 손상된 파일 대체 경로를 수동으로(PowerShell로 `save.json`을 직접 덮어쓴 뒤 재실행) 확인하도록 했었다. 하지만 `main()`이 매 실행 시작 시 `std::filesystem::remove(savePath)`로 `save.json`을 무조건 삭제하기 때문에, 그 수동 절차대로 실행 파일을 재실행하면 미리 손상시켜 둔 파일이 `load()`가 열어보기도 전에 지워져 버려 손상 경로를 확인할 수 없었다 (계획 문서 자체의 결함이었음). 이를 고쳐 4)번 단계에서 실행 중에 직접 `save.json`을 손상된 텍스트로 덮어쓴 뒤 바로 `load()`를 호출하도록 시나리오에 포함시켜, 매 실행마다 결정적으로 손상 복구 경로까지 검증되도록 한다.

```cpp
#include <filesystem>
#include <fstream>
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

    std::cout << std::endl << "4) 손상된 파일 로드 (경고 후 기본값 기대):" << std::endl;
    {
        std::ofstream corrupted(savePath, std::ios::trunc);
        corrupted << "{not valid json";
    }
    PlayerSaveData afterCorruption = SaveManager::load(savePath);
    printData(afterCorruption);

    return 0;
}
```

- [ ] **Step 5: 빌드**

Run: `msbuild DataPersistence.slnx /p:Configuration=Debug /p:Platform=x64`
Expected: `Build succeeded.`

- [ ] **Step 6: 실행 및 콘솔 출력 확인 (정상 시나리오 + 손상 파일 복구 시나리오 모두 자동 검증)**

Run: `.\build\x64\Debug\DataPersistence.exe` (실행 디렉터리 기준으로 `save.json`이 생성/삭제/재작성됨)
Expected (정확히 일치):

```
1) 최초 로드 (파일 없음, 기본값 기대):
playerName: Hero
level: 1
hp: 100/100
position: (0, 0, 0)

2) 수정된 데이터 저장
저장 성공 여부: true

3) 다시 로드 (저장된 값 복원 기대):
playerName: Hero
level: 7
hp: 55/100
position: (10, 20, 30)

4) 손상된 파일 로드 (경고 후 기본값 기대):
저장 파일이 손상되어 기본값으로 대체합니다: Expected '"' in JSON input
playerName: Hero
level: 1
hp: 100/100
position: (0, 0, 0)
```

(4)번의 경고 메시지 뒤에 붙는 파서 오류 문구는 손상시킨 텍스트의 정확한 모양에 따라 달라질 수 있다 — 중요한 것은 경고가 출력되고 이어서 기본값이 복원된다는 사실이다.)

- [ ] **Step 7: 커밋**

```bash
git add DataPersistence/SaveManager.h DataPersistence/SaveManager.cpp DataPersistence/main.cpp DataPersistence/DataPersistence.vcxproj DataPersistence/DataPersistence.vcxproj.filters
git commit -m "feat: add SaveManager and full save/load demo scenario"
```
