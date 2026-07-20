// =============================================================================
//  第 14 課：預設建構函數 4  —  = default：把編譯器生成的版本要回來
// =============================================================================
//
// 【主題資訊 Information】
//   語法      : ClassName() = default;
//   標準版本  : **C++11**（explicitly-defaulted function）
//   標頭檔    : <iostream>、<type_traits>
//   核心陷阱  : 它要回來的是「編譯器版」，而編譯器版**不會**初始化內建型別成員
//
// 【詳細解釋 Explanation】
//
// 【1. = default 解決什麼問題】
//   一旦你宣告了 Point(int, int)，隱式的 default constructor 就消失了。
//   你可以自己補一個空的 Point() { }，但那會有副作用（見第 3 點）。
//   = default 的意思是：「請編譯器產生它原本就會產生的那一個版本」。
//   語意清楚、意圖明確，而且保留了編譯器版本的所有特殊性質。
//
// 【2. 最大的誤解：= default ≠ 歸零】
//   本檔的 Point 用了 Point() = default;，但：
//       Point p1;                 // x、y 依然是**不確定值**
//       cout << p1.x;             // 讀取 indeterminate value → UB
//   因為編譯器生成的版本就是「對每個成員做 default-initialization」，
//   而 int 的 default-initialization 是「什麼都不做」。
//   = default 忠實地要回了這個行為，包括它不初始化內建型別這一點。
//   想要有確定的值，正解是 **NSDMI + = default**：
//       int x = 0, y = 0;
//       Point() = default;        // 現在它會用 NSDMI 的值
//
// 【3. = default 與手寫 { } 的實質差異（這才是重點）】
//   ┌──────────────────────┬─────────────────┬──────────────────┐
//   │                       │ T() = default    │ T() { }          │
//   ├──────────────────────┼─────────────────┼──────────────────┤
//   │ user-provided?        │ 否               │ 是               │
//   │ 可能是 trivial?       │ 是               │ 否（永遠不是）    │
//   │ T obj{}; 會歸零?      │ **會**           │ **不會**         │
//   └──────────────────────┴─────────────────┴──────────────────┘
//   「T obj{}; 會不會歸零」是兩者最重要的實際差別，5.cpp 會專門實測。
//
//   ▍aggregate 資格：條件比上表更嚴，務必分清楚
//   aggregate 要求類別**完全沒有** user-provided／explicit／inherited 的 constructor。
//   所以只有「= default 是唯一的 constructor」時才談得上：
//       struct OnlyDefaulted { int v; OnlyDefaulted() = default; };
//           → C++17：是 aggregate；C++20：**不是**（P1008R1 收緊為
//             「任何使用者宣告的 constructor 都取消資格」）
//   本檔的 Point 另外還有 user-provided 的 Point(int,int)，
//   因此它在 C++17 與 C++20 **都不是** aggregate——
//   別把「= default 保住 aggregate」誤記成無條件成立。
//   本檔以 static_assert 在兩個標準下實測驗證了以上每一句。
//
// 【4. = default 必須寫在「第一次宣告」才保有這些性質】
//   在類別內直接寫 Point() = default; → 不是 user-provided，享有上表的好處。
//   若在類別外補定義：
//       class Point { public: Point(); };
//       Point::Point() = default;      // ← 這樣就變成 user-provided！
//   後者失去 trivial 與 aggregate 的資格。這個細節很少人知道，
//   但它會實際改變 Point p{}; 的行為。
//
// 【概念補充 Concept Deep Dive】
//   ▍為什麼要保留 trivial 這個性質
//     trivial 的型別可以用 memcpy 整塊複製、可以安全地寫進檔案或送上網路、
//     可以放進需要 trivially copyable 的 API（例如 std::atomic<T>）。
//     手寫一個空的 { } 會無聲地摧毀這些能力，= default 則不會。
//
//   ▍= default 也可用於其他特殊成員函式
//     copy/move constructor、copy/move assignment、destructor 都能 = default。
//     常見用途是 Rule of Zero／Five：明確宣告「這五個我全都要編譯器版」。
//
//   ▍= default 與 = delete 是一對
//     = default 說「請生成」，= delete 說「請禁止」（9.cpp 主題）。
//     兩者都是 C++11 引入，目的都是讓「特殊成員函式的意圖」變成程式碼，
//     而不是靠註解或「剛好沒寫」來表達。
//
// 【注意事項 Pay Attention】
//   1. = default **不會**把內建型別成員歸零——這是本檔最重要的一句話。
//   2. 要有確定的值請搭配 NSDMI（int x = 0;）。
//   3. = default 必須寫在第一次宣告處才保有 trivial / aggregate 性質。
//   4. C++17 與 C++20 對 aggregate 的判定不同（P1008R1）；且只有「= default 是
//      唯一 constructor」時才看得到這個差異——本檔以 OnlyDefaulted 實測呈現。
//   5. 本檔輸出中 p1.x / p1.y 為不確定值，每次執行都可能不同。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】= default
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. Point() = default; 和 Point() { } 有什麼差別？
//     答：後者是 user-provided，前者不是。實際影響有三：
//         (1) Point p{}; 對 = default 會先 zero-initialize，對 { } 不會；
//         (2) = default 的版本可能是 trivial（可 memcpy、可放進要求
//             trivially copyable 的 API），手寫 { } 永遠不是；
//         (3) 若 = default 是類別唯一的 constructor，C++17 中它仍保有 aggregate
//             資格而手寫 { } 沒有（C++20 起兩者都沒有，見 P1008R1）。
//     追問：那 = default 會把 int 成員設成 0 嗎？→ **不會**。
//         它要回的就是編譯器版，而編譯器版不初始化內建型別。要歸零請用 NSDMI。
//
// 🔥 Q2. 在類別內寫 = default 和在類別外寫，有差別嗎？
//     答：有，而且是實質差別。寫在類別內的第一次宣告 → 不是 user-provided，
//         保有 trivial 與 aggregate 性質；在類別外補 Point::Point() = default;
//         → 變成 user-provided，那些性質全部失去，連 Point p{}; 的行為都會改變。
//     追問：那什麼時候需要在類別外寫？→ 想把定義藏進 .cpp 以減少標頭檔相依
//         （pimpl 常見），此時就必須接受失去 trivial 的代價。
//
// ⚠️ 陷阱. 「我加了 = default，所以 Point p; 之後 p.x 就是 0 了」
//     答：不是。= default 生成的版本對 int 做的是 default-initialization，
//         也就是「什麼都不做」。p.x 仍是不確定值，讀取它是 UB。
//         本檔的輸出實測了這一點（而且它常常剛好是 0，更容易騙過你）。
//     為什麼會錯：把「= default」讀成「= 預設值」。它的意思是
//         「使用預設的**實作**」，不是「使用預設的**數值**」——差之毫釐。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <cstring>   // memcpy
#include <type_traits>
using namespace std;

class Point {
public:
    int x, y;

    // 告訴編譯器：請幫我生成預設建構函數
    // 效果等同於「完全不寫任何 constructor」時編譯器會生成的那一個，
    // 差別只在於：即使你另外定義了帶參 constructor，這個也還在。
    // ⚠️ 注意：它**不會**把 x、y 初始化為 0！
    Point() = default;

    Point(int px, int py) {
        x = px;
        y = py;
        cout << "帶參建構: Point(" << x << ", " << y << ")" << endl;
    }
};

// ✅ 正解：NSDMI + = default → 既簡潔，值又確定
class PointSafe {
public:
    int x = 0;
    int y = 0;

    PointSafe() = default;                        // 現在它會採用 NSDMI 的值
    PointSafe(int px, int py) : x(px), y(py) {}
};

// 對照組：手寫空的 constructor（失去 trivial 與 aggregate 資格）
class PointHandWritten {
public:
    int x, y;
    PointHandWritten() { }
    PointHandWritten(int a, int b) : x(a), y(b) {}
};

// 編譯期事實查核（本機 GCC 15.2 實測）
static_assert(std::is_trivially_default_constructible<Point>::value,
              "= default 寫在類別內 → 可以是 trivial");
static_assert(!std::is_trivially_default_constructible<PointHandWritten>::value,
              "手寫 { } → 永遠不是 trivial");
static_assert(!std::is_trivially_default_constructible<PointSafe>::value,
              "有 NSDMI → 不是 trivial（因為要執行那些初始化）");

// aggregate 的判定要用「= default 是唯一 constructor」的型別才看得出標準差異
struct OnlyDefaulted {
    int v;
    OnlyDefaulted() = default;
};

#if __cplusplus >= 202002L
static_assert(!std::is_aggregate<OnlyDefaulted>::value,
              "C++20（P1008R1）：任何使用者宣告的 constructor 都取消 aggregate 資格");
#else
static_assert(std::is_aggregate<OnlyDefaulted>::value,
              "C++17：只有 user-provided 才取消資格，單獨的 = default 仍是 aggregate");
#endif
static_assert(!std::is_aggregate<Point>::value,
              "Point 另有 user-provided 的 Point(int,int) → 兩個標準下都不是 aggregate");
static_assert(!std::is_aggregate<PointHandWritten>::value,
              "手寫 { } 在 C++17／C++20 都不是 aggregate");

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】從缺
//   理由：= default 影響的是 trivial 性、aggregate 資格與值初始化語意，
//         這些在 LeetCode 的評測環境中完全觀察不到（題目只比對輸出）。
//         本檔是純語言規則的主題，硬掛一題設計題毫無關聯。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】封包標頭（PacketHeader）：為什麼要保住 trivial
//   情境：網路程式或二進位檔案格式中，常需要把一個 struct 直接
//         memcpy 進／出位元組緩衝區。這要求該型別是 trivially copyable。
//   關鍵：只要有人「順手」把 PacketHeader() = default; 改成 PacketHeader() { }，
//         型別就不再 trivial，那些 memcpy 與 static_assert 就會失效——
//         而且改動者通常完全不知道自己弄壞了什麼。
//   本範例用 static_assert 把這個要求釘在程式碼裡，讓破壞它的人立刻編譯失敗。
// -----------------------------------------------------------------------------
struct PacketHeader {
    unsigned int  magic;
    unsigned short version;
    unsigned short payloadLen;

    PacketHeader() = default;    // 保住 trivial，才能安全 memcpy
    PacketHeader(unsigned int m, unsigned short v, unsigned short len)
        : magic(m), version(v), payloadLen(len) {}
};

// 把「這個型別必須可以整塊複製」變成編譯期契約，而不是註解裡的口頭約定
static_assert(std::is_trivially_copyable<PacketHeader>::value,
              "PacketHeader 必須 trivially copyable 才能 memcpy 進網路緩衝區");

int main() {
    cout << "=== = default 要回了預設建構，但沒有幫你歸零 ===" << endl;
    Point p1;          // OK：使用編譯器生成的預設建構函數
    Point p2(3, 4);

    cout << "  p1.x = " << p1.x << ", p1.y = " << p1.y
         << "  ← 不確定值！= default 不做初始化" << endl;
    cout << "  p2.x = " << p2.x << ", p2.y = " << p2.y
         << "  ← 帶參建構明確設定過，有保證" << endl;

    cout << "\n=== 但值初始化 {} 對 = default 是有效的 ===" << endl;
    Point p3{};        // 值初始化：因為 Point() 不是 user-provided → 先零初始化
    cout << "  p3.x = " << p3.x << ", p3.y = " << p3.y
         << "  ← 標準保證是 0" << endl;

    cout << "\n=== 正解：NSDMI + = default ===" << endl;
    PointSafe s1;      // 不必靠呼叫端寫 {}，怎麼建構都有值
    cout << "  s1.x = " << s1.x << ", s1.y = " << s1.y
         << "  ← 類別作者保證，呼叫端不必配合" << endl;

    cout << "\n=== 三種寫法的型別性質（編譯期事實）===" << endl;
    cout << boolalpha;
    cout << "  Point（= default）      trivially_default_constructible = "
         << std::is_trivially_default_constructible<Point>::value << endl;
    cout << "  PointHandWritten（{}）  trivially_default_constructible = "
         << std::is_trivially_default_constructible<PointHandWritten>::value << endl;
    cout << "  PointSafe（NSDMI）      trivially_default_constructible = "
         << std::is_trivially_default_constructible<PointSafe>::value << endl;
    cout << "  OnlyDefaulted（唯一 constructor 是 = default）is_aggregate = "
         << std::is_aggregate<OnlyDefaulted>::value
         << "（C++17 為 true；C++20 因 P1008R1 為 false）" << endl;
    cout << "  Point（另有 user-provided 帶參 constructor）is_aggregate = "
         << std::is_aggregate<Point>::value << "（兩個標準都是 false）" << endl;
    cout << "  PointHandWritten is_aggregate = "
         << std::is_aggregate<PointHandWritten>::value << endl;
    cout << "  本次編譯的標準版本 __cplusplus = " << __cplusplus << endl;

    cout << "\n=== 日常實務：封包標頭必須保持 trivially copyable ===" << endl;
    PacketHeader h(0xCAFEBABE, 2, 128);
    unsigned char buffer[sizeof(PacketHeader)];
    std::memcpy(buffer, &h, sizeof(h));          // 只有 trivially copyable 才安全

    PacketHeader back{};
    std::memcpy(&back, buffer, sizeof(back));
    cout << "  memcpy 往返後: magic=0x" << std::hex << back.magic << std::dec
         << ", version=" << back.version
         << ", payloadLen=" << back.payloadLen << endl;
    cout << "  sizeof(PacketHeader) = " << sizeof(PacketHeader)
         << " bytes（實作定義，含對齊 padding）" << endl;
    cout << "  若有人把 = default 改成 { }，上面的 static_assert 會立刻編譯失敗，" << endl;
    cout << "  這正是我們要的：讓破壞契約的人當場知道。" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 14 課：預設建構函數（Default Constructor）4.cpp" -o def4
//   （本檔亦可用 -std=c++20 編譯：is_aggregate 的結果會改變，
//     static_assert 已用 __cplusplus 分支處理，兩種標準都能通過）

// === 預期輸出 ===
// ⚠️ p1.x / p1.y 是不確定值（= default 不做初始化，讀取即 UB），下方為某一次執行的結果，
//    每次執行都可能不同。p3{}（值初始化）、PointSafe（NSDMI）與型別性質查詢才是有保證的。
//    以 -std=c++20 編譯時，OnlyDefaulted 的 is_aggregate 會變成 false、__cplusplus 為 202002。
// === = default 要回了預設建構，但沒有幫你歸零 ===
// 帶參建構: Point(3, 4)
//   p1.x = 135121, p1.y = 0  ← 不確定值！= default 不做初始化
//   p2.x = 3, p2.y = 4  ← 帶參建構明確設定過，有保證
//
// === 但值初始化 {} 對 = default 是有效的 ===
//   p3.x = 0, p3.y = 0  ← 標準保證是 0
//
// === 正解：NSDMI + = default ===
//   s1.x = 0, s1.y = 0  ← 類別作者保證，呼叫端不必配合
//
// === 三種寫法的型別性質（編譯期事實）===
//   Point（= default）      trivially_default_constructible = true
//   PointHandWritten（{}）  trivially_default_constructible = false
//   PointSafe（NSDMI）      trivially_default_constructible = false
//   OnlyDefaulted（唯一 constructor 是 = default）is_aggregate = true（C++17 為 true；C++20 因 P1008R1 為 false）
//   Point（另有 user-provided 帶參 constructor）is_aggregate = false（兩個標準都是 false）
//   PointHandWritten is_aggregate = false
//   本次編譯的標準版本 __cplusplus = 201703
//
// === 日常實務：封包標頭必須保持 trivially copyable ===
//   memcpy 往返後: magic=0xcafebabe, version=2, payloadLen=128
//   sizeof(PacketHeader) = 8 bytes（實作定義，含對齊 padding）
//   若有人把 = default 改成 { }，上面的 static_assert 會立刻編譯失敗，
//   這正是我們要的：讓破壞契約的人當場知道。
