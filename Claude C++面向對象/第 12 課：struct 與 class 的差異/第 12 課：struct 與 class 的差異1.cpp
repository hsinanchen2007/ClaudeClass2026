// =============================================================================
//  第 12 課 -1  —  struct 也能寫完整 OOP：兩者真的只差預設存取權
// =============================================================================
//
// 【主題資訊 Information】
//   語法：  struct X { ... };   // 成員預設 public，繼承預設 public
//           class  X { ... };   // 成員預設 private，繼承預設 private
//   標準：  C++98 起兩者即為同義（只差預設值）。
//   標頭檔：<iostream>、<string>
//   關鍵事實：除了「預設存取權」與「預設繼承方式」，**沒有任何其他差異**。
//
// 【詳細解釋 Explanation】
//
// 【1. 本檔在證明什麼】
//   DogStruct 與 DogClass 的內容**逐字相同**，只有關鍵字不同。
//   兩者都有 private 資料成員、public 成員函式，都能正常封裝、正常運作。
//   這直接推翻了最普遍的誤解：「struct 只能裝資料、不能有函式」。
//   那是 **C** 的 struct，不是 C++ 的。
//   在 C++ 裡，struct 與 class 是同一套機制，
//   可以有建構函式、解構函式、virtual 函式、繼承、運算子多載、template 特化 ——
//   凡是 class 做得到的，struct 全都做得到。
//
// 【2. 唯一的兩個差異】
//     (a) 成員的預設存取權：struct 是 public，class 是 private。
//     (b) 繼承的預設方式：  struct X : Y  等同 struct X : public Y
//                          class  X : Y  等同 class  X : private Y
//   本檔兩個類別都**顯式寫出** private: 與 public:，
//   所以連這唯一的差異都被抵銷了 —— 兩者變得完全等價。
//
// 【3. 既然一樣，為什麼還要有兩個關鍵字】
//   歷史因素 ＋ 溝通因素：
//     - 歷史：C++ 要與 C 的 struct 相容，不能改變它的預設 public 語意；
//             class 則是為 OOP 新增的關鍵字，預設 private 才符合封裝直覺。
//     - 溝通：關鍵字本身成了一種**設計意圖的宣告**。
//             讀者看到 struct 會預期「這是一包公開的資料，沒有不變量要維護」；
//             看到 class 會預期「這是一個有不變量、需要保護內部狀態的型別」。
//   所以選哪一個不是語法問題，是**表達意圖**的問題。
//
// 【4. 業界通行的選用準則】
//   Google C++ Style Guide 與 C++ Core Guidelines 的共識大致是：
//     用 struct：所有成員都 public、彼此獨立、沒有不變量要維護的**純資料聚合**
//                （例如 Point、Config、RGB、函式的回傳組合）。
//     用 class ：有 private 狀態、有不變量要靠成員函式維護的型別。
//   一句話判準：**「這個型別有沒有不變量要保護？」**
//   有 → class；沒有 → struct。
//   ★ 本檔的 DogStruct 有 private 成員與受控介面，
//     依這條準則它其實該寫成 class —— 這正是本檔要示範的「能做，但不該做」。
//
// 【概念補充 Concept Deep Dive】
//   有一個實務上會咬人的細節：**前向宣告時 struct 與 class 可以混用**。
//       class Dog;            // 前向宣告
//       struct Dog { ... };   // 定義 —— 標準上合法
//   標準允許這樣寫（兩者是同一個 class-key 家族）。
//   但 MSVC 會發出 C4099 警告，因為它的除錯資訊會記錄 class-key，
//   混用可能導致連結期的除錯資訊不一致。
//   實務準則：**前向宣告與定義用同一個關鍵字**。
//
//   另一個常見誤解是「struct 是 POD、class 不是」。
//   POD／trivial／standard-layout 這些性質**與關鍵字完全無關**，
//   只取決於成員的性質：有沒有 virtual 函式、有沒有使用者提供的建構函式、
//   資料成員是否都在同一個 access 區塊等等。
//   用 class 寫的純資料型別一樣可以是 standard-layout。
//   下方 demoKeywordIrrelevant() 會實測給你看。
//
// 【注意事項 Pay Attention】
//   1. C++ 的 struct **不是** C 的 struct，它可以有函式、繼承、virtual。
//   2. 繼承的預設方式也不同：struct 預設 public 繼承，class 預設 private 繼承 ——
//      這比成員預設值更容易踩到，因為 class X : Y 幾乎不是你要的意思。
//   3. 前向宣告與定義建議用同一個關鍵字（MSVC C4099）。
//   4. POD／standard-layout 與用哪個關鍵字無關。
//   5. 本檔的 bark() 只讀不寫，理應宣告為 const 成員函式。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】struct 與 class 的差異
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. C++ 中 struct 和 class 有什麼差別？
//     答：只有兩個差別：(1) 成員的預設存取權 —— struct 是 public、class 是 private；
//         (2) 繼承的預設方式 —— struct 預設 public 繼承、class 預設 private 繼承。
//         其餘完全相同：struct 一樣可以有建構函式、解構函式、virtual 函式、
//         繼承、運算子多載、private 成員。
//     追問：那實務上怎麼選？
//         → 看「有沒有不變量要保護」。純資料聚合（成員彼此獨立、
//           任意組合都合法）用 struct；有 private 狀態需要成員函式
//           維護一致性的用 class。
//
// 🔥 Q2. class X : Y 和 struct X : Y 有什麼不同？哪個比較容易出錯？
//     答：class X : Y 是 **private 繼承**，struct X : Y 是 **public 繼承**。
//         class 那個比較容易出錯 —— private 繼承表達的是「用 Y 來實作 X」，
//         而非「X is-a Y」，因此 X* 無法隱式轉成 Y*，多型完全失效。
//         多數人寫 class X : Y 時想要的其實是 public 繼承。
//     追問：所以該怎麼寫？
//         → 永遠**顯式**寫出繼承方式：class X : public Y。
//           不要依賴預設值。
//
// ⚠️ 陷阱. 「struct 是 POD、class 不是」——這個說法對嗎？
//     答：**完全錯誤**。POD／trivial／standard-layout 這些性質
//         與用哪個關鍵字毫無關係，只取決於型別的實際內容：
//         有沒有 virtual 函式、有沒有使用者提供的建構函式、
//         資料成員是否都在同一個 access 區塊等等。
//         用 class 寫的純資料型別一樣是 standard-layout；
//         用 struct 寫的、帶 virtual 函式的型別一樣不是。
//         （見下方 demoKeywordIrrelevant() 的實測。）
//     為什麼會錯：把 C 的世界觀（struct = 純資料）直接套進 C++，
//         再把「純資料」與「POD」畫上等號。
//         實際上關鍵字只影響預設存取權，這些性質是由內容決定的。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <type_traits>
#include <optional>
using namespace std;

// 用 struct 寫的「完整 OOP」
struct DogStruct {
private:
    string name;
    int age = 0;

public:
    void setName(const string& n) { name = n; }
    void setAge(int a) { age = a; }

    void bark() {
        cout << name << "（struct）汪汪！" << endl;
    }
};

// 用 class 寫的完全等價版本
class DogClass {
private:
    string name;
    int age = 0;

public:
    void setName(const string& n) { name = n; }
    void setAge(int a) { age = a; }

    void bark() {
        cout << name << "（class）汪汪！" << endl;
    }
};

// -----------------------------------------------------------------------------
// 【可執行示範】關鍵字與 POD／standard-layout 完全無關
//   下面四個型別兩兩配對，只差 struct/class 關鍵字，性質完全相同；
//   真正造成差異的是「有沒有 virtual 函式」。
// -----------------------------------------------------------------------------
struct PlainStruct { int x; int y; };            // 純資料，用 struct
class  PlainClass  { public: int x; int y; };    // 純資料，用 class

struct VirtualStruct { virtual ~VirtualStruct() = default; int x; };
class  VirtualClass  { public: virtual ~VirtualClass() = default; int x; };

void demoKeywordIrrelevant() {
    cout << "  PlainStruct  : standard_layout=" << boolalpha
         << is_standard_layout<PlainStruct>::value
         << ", trivial=" << is_trivial<PlainStruct>::value
         << ", sizeof=" << sizeof(PlainStruct) << endl;
    cout << "  PlainClass   : standard_layout="
         << is_standard_layout<PlainClass>::value
         << ", trivial=" << is_trivial<PlainClass>::value
         << ", sizeof=" << sizeof(PlainClass) << endl;
    cout << "  -> 純資料時，struct 與 class 的性質完全相同" << endl;

    cout << "  VirtualStruct: standard_layout="
         << is_standard_layout<VirtualStruct>::value
         << ", trivial=" << is_trivial<VirtualStruct>::value
         << ", sizeof=" << sizeof(VirtualStruct) << endl;
    cout << "  VirtualClass : standard_layout="
         << is_standard_layout<VirtualClass>::value
         << ", trivial=" << is_trivial<VirtualClass>::value
         << ", sizeof=" << sizeof(VirtualClass) << endl;
    cout << "  -> 有 virtual 時兩者也一樣（都不是 POD）；" << endl;
    cout << "     決定性質的是「內容」，不是關鍵字" << endl;
    cout << "     （sizeof 值為本機 x86-64 實測，含 vptr 與對齊，屬實作定義）" << endl;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】同一份資料，兩種寫法各自的正確場合
//   情境：解析設定檔後要回傳結果。這裡有兩個型別：
//     (1) ParsedLine —— 純資料聚合：三個欄位彼此獨立，
//         任意組合都是合法狀態，沒有不變量要維護 → **用 struct**
//     (2) LogLevel   —— 有不變量：等級必須落在已知集合內，
//         而且要能安全地與門檻比較 → **用 class**
//   為什麼用到本主題：這正是「有沒有不變量」這條判準的實際應用。
//         注意 ParsedLine 完全不需要 getter/setter ——
//         為純資料聚合寫一堆 get/set 是常見的過度設計。
// -----------------------------------------------------------------------------
struct ParsedLine {          // 純資料聚合：用 struct，成員直接公開
    string key;
    string value;
    int    lineNo = 0;
};

class LogLevel {             // 有不變量：用 class，內部狀態受保護
public:
    enum Value { Debug = 0, Info = 1, Warn = 2, Error = 3 };

    static optional<LogLevel> fromString(const string& s) {
        if (s == "DEBUG") return LogLevel(Debug);
        if (s == "INFO")  return LogLevel(Info);
        if (s == "WARN")  return LogLevel(Warn);
        if (s == "ERROR") return LogLevel(Error);
        return nullopt;                       // 未知字串 → 無法建立，不變量得保
    }

    bool atLeast(Value threshold) const { return m_value >= threshold; }

    string name() const {
        switch (m_value) {
            case Debug: return "DEBUG";
            case Info:  return "INFO";
            case Warn:  return "WARN";
            default:    return "ERROR";
        }
    }

private:
    explicit LogLevel(Value v) : m_value(v) {}
    Value m_value;
};

// 純資料聚合可以直接用聚合初始化，不需要建構函式
ParsedLine parseConfigLine(const string& line, int lineNo) {
    size_t eq = line.find('=');
    if (eq == string::npos) return ParsedLine{"", "", lineNo};
    return ParsedLine{line.substr(0, eq), line.substr(eq + 1), lineNo};
}

int main() {
    cout << "=== 基本：struct 與 class 寫出完全等價的 OOP ===" << endl;
    DogStruct ds;
    ds.setName("旺財");
    ds.bark();

    DogClass dc;
    dc.setName("小黑");
    dc.bark();

    cout << "\n=== 關鍵字與 POD/standard-layout 無關 ===" << endl;
    demoKeywordIrrelevant();

    cout << "\n=== 日常實務：純資料用 struct、有不變量用 class ===" << endl;
    ParsedLine p1 = parseConfigLine("log_level=WARN", 12);
    cout << "  第 " << p1.lineNo << " 行: key=[" << p1.key
         << "] value=[" << p1.value << "]   <- struct，欄位直接存取" << endl;

    auto lv = LogLevel::fromString(p1.value);
    if (lv) {
        cout << "  解析出等級: " << lv->name() << "   <- class，內部狀態受保護" << endl;
        cout << "  達到 WARN 門檻嗎? " << boolalpha << lv->atLeast(LogLevel::Warn) << endl;
        cout << "  達到 ERROR 門檻嗎? " << lv->atLeast(LogLevel::Error) << endl;
    }

    auto bad = LogLevel::fromString("VERBOSE");
    cout << "  未知等級 VERBOSE -> "
         << (bad ? "建立成功(不該發生)" : "建立失敗，回傳 nullopt") << endl;

    ParsedLine p2 = parseConfigLine("這行沒有等號", 13);
    cout << "  第 " << p2.lineNo << " 行沒有等號 -> key 為空: "
         << (p2.key.empty() ? "是" : "否") << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 12 課：struct 與 class 的差異1.cpp" -o structclass1

// === 預期輸出 ===
// === 基本：struct 與 class 寫出完全等價的 OOP ===
// 旺財（struct）汪汪！
// 小黑（class）汪汪！
// 
// === 關鍵字與 POD/standard-layout 無關 ===
//   PlainStruct  : standard_layout=true, trivial=true, sizeof=8
//   PlainClass   : standard_layout=true, trivial=true, sizeof=8
//   -> 純資料時，struct 與 class 的性質完全相同
//   VirtualStruct: standard_layout=false, trivial=false, sizeof=16
//   VirtualClass : standard_layout=false, trivial=false, sizeof=16
//   -> 有 virtual 時兩者也一樣（都不是 POD）；
//      決定性質的是「內容」，不是關鍵字
//      （sizeof 值為本機 x86-64 實測，含 vptr 與對齊，屬實作定義）
// 
// === 日常實務：純資料用 struct、有不變量用 class ===
//   第 12 行: key=[log_level] value=[WARN]   <- struct，欄位直接存取
//   解析出等級: WARN   <- class，內部狀態受保護
//   達到 WARN 門檻嗎? true
//   達到 ERROR 門檻嗎? false
//   未知等級 VERBOSE -> 建立失敗，回傳 nullopt
//   第 13 行沒有等號 -> key 為空: 是
