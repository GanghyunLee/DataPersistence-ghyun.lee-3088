# JSON 기반 게임 세이브 데이터 영속성 POC 설계

날짜: 2026-07-15

## 목적

`DataPersistence` C++20 콘솔 프로젝트에서, 외부 JSON 라이브러리 없이 직접 구현한 최소 JSON 파서/라이터를 통해 게임 세이브 데이터(플레이어 정보)를 파일에 저장하고 다시 불러오는 POC를 만든다.

## 범위

- 외부 JSON 라이브러리(nlohmann/json 등)는 사용하지 않고, 필요한 만큼만 최소화한 JSON 파서/라이터를 직접 구현한다.
- 지원하는 JSON 값 타입: 문자열, 숫자, bool, 중첩 객체. **배열은 지원하지 않는다** (POC 범위 밖).
- 저장 대상 데이터는 기본적인 플레이어 정보(이름, 레벨, HP, 위치)로 한정한다.
- 정식 유닛 테스트 프레임워크나 assert 기반 테스트는 두지 않는다. `main()`의 콘솔 출력으로 round-trip 성공 여부를 확인하는 수준으로 충분하다.

## 아키텍처

3개의 책임 단위로 나눈다.

1. **`JsonValue`** — JSON 값을 표현하는 최소 데이터 모델과 직렬화 기능
2. **`JsonParser`** — 텍스트를 `JsonValue` 트리로 변환하는 최소 재귀 하강 파서
3. **`PlayerSaveData`** — 게임 세이브 데이터 구조체와 `JsonValue` 간 변환
4. **`SaveManager`** — 파일 읽기/쓰기 및 손상/누락 데이터에 대한 기본값 대체 처리

데이터 흐름:

```
PlayerSaveData --toJson()--> JsonValue --toString()--> 파일 쓰기
파일 읽기 --JsonParser::parse()--> JsonValue --fromJson()--> PlayerSaveData
```

## 컴포넌트 상세

### JsonValue

- 내부 표현: `std::variant<std::monostate, std::string, double, bool, std::map<std::string, JsonValue>>`
  - `std::monostate`는 값이 아직 설정되지 않은 상태(빈 값)를 표현하는 데 사용하며, JSON의 `null` 리터럴은 별도로 지원하지 않는다.
- 편의 접근자: `asString()`, `asNumber()`, `asBool()`, `operator[](const std::string& key)` (객체 접근, 존재하지 않으면 빈 `JsonValue` 참조/기본값 반환)
- `std::string toString(int indent = 0) const`: 들여쓰기가 적용된 보기 좋은 JSON 문자열을 생성

### JsonParser

- 공개 API: `JsonValue parse(const std::string& text)`
- 내부적으로 문자열 커서 위치를 유지하는 재귀 하강 파서로 구현
  - 공백 스킵 → 다음 토큰이 `{`, `"`(문자열), 숫자 시작 문자, `t`/`f`(bool) 중 무엇인지에 따라 분기
- 객체 안에 배열(`[`)이 나타나면 파싱 실패로 처리한다 (예외를 던지거나 오류 상태를 반환).

### PlayerSaveData

```cpp
struct PlayerSaveData {
    std::string playerName;
    int level;
    int hp;
    int maxHp;
    struct { double x, y, z; } position;
};
```

- `JsonValue toJson() const`
- `static PlayerSaveData fromJson(const JsonValue&)`
  - 키가 없거나 타입이 맞지 않는 필드는 예외를 던지지 않고 해당 필드만 기본값으로 채운다 (부분 손상 시에도 나머지 필드는 정상 복구).

### SaveManager

- `bool save(const PlayerSaveData& data, const std::string& path)`
  - `data.toJson().toString()`으로 JSON 문자열을 만들어 파일에 쓴다. 쓰기 실패 시 `false` 반환.
- `PlayerSaveData load(const std::string& path)`
  - 파일이 없으면 (최초 실행) 조용히 기본값 `PlayerSaveData{}`를 반환한다.
  - 파일 열기 실패(권한 등)면 콘솔에 경고를 출력하고 기본값을 반환한다.
  - 파일은 열리지만 JSON 파싱에 실패하면 "저장 파일이 손상되어 기본값으로 대체합니다" 경고를 콘솔에 출력하고 기본값을 반환한다.

## 오류 처리 정책

| 상황 | 동작 |
|---|---|
| 파일 없음 (최초 실행) | 경고 없이 기본값으로 진행 |
| 파일 열기 실패 | 콘솔 경고 후 기본값으로 진행 |
| JSON 파싱 실패 (문법 오류) | 콘솔 경고 후 기본값으로 진행 |
| 일부 필드 누락/타입 불일치 | 예외 없이 해당 필드만 기본값 처리, 나머지는 정상 로드 |
| 저장(쓰기) 실패 | `save()`가 `false` 반환, 호출부에서 콘솔 경고만 출력하고 계속 진행 |

프로그램이 예외로 죽는 경로는 만들지 않는다. 항상 사용자 친화적으로 기본값 대체를 통해 복구한다.

## 데모 시나리오 (`main.cpp`)

1. `SaveManager::load("save.json")` 호출 → 최초 실행 시 파일 없음 → 기본값 반환, 콘솔에 로드된 값 출력
2. 기본값을 임의로 수정 (레벨업, HP 감소, 위치 이동 등)
3. `SaveManager::save(data, "save.json")` 호출 → JSON 파일 생성 확인
4. 다시 `load()` 호출 → 방금 저장한 값이 그대로 복원되는지 콘솔 출력으로 확인 (round-trip 검증)
5. `save.json`을 프로그램이 직접 손상된 텍스트로 덮어쓴 뒤 다시 `load()` 호출 → 콘솔에 손상 경고가 출력되고 기본값으로 복원되는지 확인

별도의 assert 기반 테스트 코드는 작성하지 않는다. 콘솔 출력으로 round-trip 및 손상 파일 복구 성공 여부를 확인하는 것으로 충분하다. (5번 단계는 최초 구현 시 수동으로 `save.json`을 외부에서 덮어쓰고 재실행하도록 설계했으나, `main()`이 매 실행마다 시작 시 `save.json`을 삭제하기 때문에 그 절차로는 손상 경로가 재현되지 않는 결함이 있었다. 이를 고쳐 `main()` 내부에서 직접 파일을 손상시키고 즉시 `load()`를 호출하도록 변경해 매 실행마다 결정적으로 검증되도록 했다.)

## 빌드/파일 구성

기존 `DataPersistence.vcxproj`(C++20, 콘솔 앱)에 다음 소스 파일들을 추가한다.

- `JsonValue.h` / `JsonValue.cpp`
- `JsonParser.h` / `JsonParser.cpp`
- `PlayerSaveData.h` / `PlayerSaveData.cpp`
- `SaveManager.h` / `SaveManager.cpp`
- `main.cpp` (데모 시나리오 실행)
