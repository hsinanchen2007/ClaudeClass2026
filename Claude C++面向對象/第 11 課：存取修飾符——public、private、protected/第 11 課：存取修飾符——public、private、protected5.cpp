// =============================================================================
//  第 11 課 -5  —  預設存取權：class 是 private，struct 是 public
// =============================================================================
//
// 【主題資訊 Information】
//   語法：  class  X { int a; };   // a 預設 private
//           struct Y { int a; };   // a 預設 public
//   標準：  C++98 起（此規則從未改變）
//   標頭檔：本例僅需 <iostream>
//   關鍵詞：default member access、default base access、aggregate、POD
//
//   class 與 struct 在 C++ 中只有「兩個」語言層面的差異：
//     ① 預設的成員存取權：class = private，struct = public
//     ② 預設的繼承存取權：class : Base 等於 private 繼承
//                          struct : Base 等於 public 繼承
//   除此之外「完全等價」—— struct 可以有建構函式、虛擬函式、繼承、樣板、
//   運算子多載、private 成員，什麼都能做。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼會有這個差異：C 相容性的歷史包袱】
// C 語言的 struct 是「一堆欄位的集合」，所有欄位本來就都能直接存取。
// Stroustrup 設計 C++ 時的核心約束之一是「既有的 C 程式碼要能繼續編譯」，
// 所以 struct 的預設存取權必須維持 public。
// 而 class 是 C++ 新引入的關鍵字，沒有相容包袱，於是選了更安全的預設值 private
//（「預設封閉，需要才開放」是比較好的設計預設值）。
// 換句話說，這個差異是歷史決定的，不是因為「struct 用來裝資料、class 用來裝行為」
// —— 那是慣例，不是語言規則。
//
// 【2. 第二個差異：預設繼承存取權（更容易踩雷）】
// 這一點比成員存取權更少人知道，但殺傷力更大：
//     class  D1 : Base { };   // 等同 class  D1 : private   Base
//     struct D2 : Base { };   // 等同 struct D2 : public    Base
// private 繼承會讓「D1* → Base*」的隱式轉換失效，於是：
//     Base* p = new D1();     // ❌ 編譯錯誤（在 class 版本）
// 多型完全失靈，而錯誤訊息常常讓初學者摸不著頭緒。
// 結論：繼承時「永遠明確寫出 public/protected/private」，不要依賴預設值。
// 本檔實際示範了這個差異。
//
// 【3. 慣例：什麼時候用 struct、什麼時候用 class】
// 既然語言上幾乎等價，選擇就是「向讀者傳達意圖」的溝通問題。
// 業界通行的慣例（Google C++ Style Guide、C++ Core Guidelines C.2 / C.8）是：
//   * 用 struct：純資料聚合，成員全 public，沒有需要維護的不變量。
//     例：座標點、函式的多回傳值、設定檔結構、網路封包欄位。
//   * 用 class：有不變量要維護、有 private 狀態、有行為封裝。
//     例：帳戶、連線、快取、狀態機。
// 判準一句話：**有沒有不變量**。有 → class；沒有 → struct。
//
// 【4. aggregate initialization：struct 慣例的實際好處】
// 若一個型別是 aggregate（無 user-declared 建構函式、無 private/protected
// 非靜態資料成員、無虛擬函式、無虛擬或 private/protected 基底），
// 就能用大括號逐欄位初始化：
//     Config c{8080, "localhost", true};
// 一旦你加了 private 成員或自訂建構函式，它就不再是 aggregate，這個語法即失效。
// 這也是「純資料就用 struct」慣例能帶來的具體語法便利。
// 注意 aggregate 的定義隨標準演進有調整（C++11/14/17/20 各有微調），
// 例如 C++17 起允許有基底類別的 aggregate；本檔的範例在 C++17 下成立。
//
// 【概念補充 Concept Deep Dive】
// (A) class 與 struct 是可互換的關鍵字，連前向宣告都可混用
//     `class Foo;` 與 `struct Foo;` 宣告的是同一個型別。
//     實務上混用不會有語意問題，但 MSVC 等編譯器可能對此發出警告
//     （因為某些 ABI 情境下不一致的標籤會造成困擾），建議前後保持一致。
//
// (B) 存取權不影響物件佈局大小，但可能影響「順序保證」
//     C++11 起，同一 access 區段內的非靜態資料成員位址遞增順序與宣告順序一致；
//     跨不同 access 區段的相對順序則由實作決定（實作定義）。
//     所以 struct（單一 public 區段）的佈局順序保證比多區段的 class 更強。
//     這正是與 C 互通、序列化、memcpy 場景偏好使用單一 public 區段 struct 的原因。
//
// (C) standard-layout 與 POD 與 access specifier 有關
//     standard-layout 型別的條件之一，是「所有非靜態資料成員具有相同的存取權」。
//     也就是說把成員拆成 public + private 兩段，就會失去 standard-layout 資格，
//     連帶失去 offsetof 的良好定義與部分 C 互通保證 —— 這跟你用 class 或 struct
//     這個關鍵字無關，取決於實際的存取權配置。
//
// (D) 「struct 沒有成員函式」是 C 的印象，不是 C++ 的規則
//     C++ 的 struct 可以有建構函式、解構函式、虛擬函式、繼承、運算子多載。
//     本檔的 Timestamp 就是一個「有成員函式的 struct」，完全合法。
//
// 【注意事項 Pay Attention】
// 1. class 與 struct 只差「預設成員存取權」與「預設繼承存取權」兩點。
// 2. 繼承時務必明確寫 public/private —— 依賴預設值是常見的多型失效原因。
// 3. 「struct 只能裝資料」是慣例不是規則；C++ 的 struct 什麼都能做。
// 4. 加了 private 成員或自訂建構函式，型別就不再是 aggregate，大括號初始化會失效。
// 5. 成員拆成多個 access 區段會失去 standard-layout 資格，影響 offsetof 與 C 互通。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】class vs struct
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. C++ 裡 class 和 struct 有什麼差別？
//     答：語言層面只有兩點：① 預設成員存取權（class=private、struct=public）；
//         ② 預設繼承存取權（class=private 繼承、struct=public 繼承）。
//         其餘完全等價 —— struct 一樣能有建構函式、虛擬函式、繼承、private 成員。
//         其他差異都是團隊慣例，不是語言規則。
//     追問：那實務上你怎麼選？
//         → 判準是「有沒有不變量」。純資料聚合、成員全 public 用 struct；
//           有 private 狀態要維護用 class。
//
// 🔥 Q2. `class D : Base { };` 和 `struct D : Base { };` 行為一樣嗎？
//     答：不一樣，而且差很多。前者是 private 繼承，後者是 public 繼承。
//         private 繼承下 `Base* p = new D();` 會編譯失敗，多型直接失靈。
//         這是實務上最容易踩到的預設值陷阱，所以繼承一律明寫存取權。
//     追問：private 繼承代表什麼語意？
//         → implemented-in-terms-of（用它來實作），而非 is-a。
//           多數情況下用組合（composition）取代會更清楚。
//
// ⚠️ 陷阱. 「struct 是 C 的東西，只能放資料，不能有成員函式或建構函式」——錯在哪？
//     答：這是 C 的 struct。C++ 的 struct 是完整的類別型別，
//         可以有建構函式、解構函式、virtual 函式、繼承、運算子多載、
//         甚至 private 成員 —— 它與 class 只差預設存取權。
//     為什麼會錯：多數人從 C 轉 C++，把兩個語言的同名關鍵字當成同一個東西。
//         實際上 C++ 只是「沿用了 struct 這個關鍵字」以維持相容，
//         但賦予它完整的類別語意。真正決定能力的是型別本身的定義內容，
//         而不是你寫 class 還是 struct。
//
// 【LeetCode 實戰範例】—— 從缺（刻意不加）
//     本主題是「語言關鍵字的預設存取權」，屬於型別宣告層面的規則，
//     不對應任何演算法或資料結構設計題。LeetCode 上沒有題目在考
//     「class 與 struct 的預設存取權差異」。
//     真要練 OOP 設計題，該去的是 146 LRU Cache、155 Min Stack、
//     705 Design HashSet 這類「設計資料結構」題（本課程後續會遇到），
//     而不是在這裡硬湊一題不相關的。寧缺勿濫。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <cstdio>     // snprintf（Timestamp::format 使用）
using namespace std;

// -----------------------------------------------------------------------------
// 差異 ①：預設「成員」存取權
// -----------------------------------------------------------------------------
class MyClass {
    int x = 10;   // 預設 private
public:
    int getX() const { return x; }
};

struct MyStruct {
    int x = 10;   // 預設 public
};

// -----------------------------------------------------------------------------
// 差異 ②：預設「繼承」存取權（更容易踩雷的一點）
// -----------------------------------------------------------------------------
struct Base {
    int id = 7;
    void hello() const { cout << "  Base::hello(), id=" << id << endl; }
};

class  DerivedByClass  : Base { };   // 等同 : private Base
struct DerivedByStruct : Base { };   // 等同 : public  Base

// 證明差異：只有 public 繼承才能隱式轉成 Base*
// Base* p1 = new DerivedByClass();   // ❌ 編譯錯誤：'Base' is an inaccessible base
// Base* p2 = new DerivedByStruct();  // ✅ 合法

// -----------------------------------------------------------------------------
// 反例證明：struct 一樣可以有建構函式、成員函式、private 成員
//   「struct 只能裝資料」純屬 C 的印象，不是 C++ 的規則。
// -----------------------------------------------------------------------------
struct Timestamp {
private:
    long m_epochSec;                       // struct 裡的 private 成員，完全合法

public:
    explicit Timestamp(long sec) : m_epochSec(sec) {}   // struct 裡的建構函式

    long   seconds() const { return m_epochSec; }
    string format() const {
        long h = (m_epochSec / 3600) % 24;
        long m = (m_epochSec / 60) % 60;
        long s = m_epochSec % 60;
        char buf[16];
        snprintf(buf, sizeof(buf), "%02ld:%02ld:%02ld", h, m, s);
        return string(buf);
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】伺服器設定：用 struct 表達「純資料聚合」
//   情境：從設定檔／命令列讀進來的一組參數。
//   這裡「沒有任何不變量」—— 任何欄位組合都是合法的設定，
//   所以全 public 的 struct 是正確選擇，硬包 getter/setter 反而是過度設計。
//   額外好處：它是 aggregate，可以直接用大括號逐欄位初始化。
// -----------------------------------------------------------------------------
struct ServerConfig {
    int    port        = 8080;
    string host        = "0.0.0.0";
    int    maxConn     = 1024;
    bool   enableTls   = false;
};

void printConfig(const ServerConfig& c) {
    cout << "  host=" << c.host << " port=" << c.port
         << " maxConn=" << c.maxConn
         << " tls=" << (c.enableTls ? "on" : "off") << endl;
}

// 對照組：同樣是「連線」，但有不變量（連線數不可為負、不可超過上限）
//        → 這種就該用 class，把狀態關起來
class ConnectionPool {
private:
    int m_active = 0;
    int m_limit;

public:
    explicit ConnectionPool(int limit) : m_limit(limit > 0 ? limit : 1) {}

    bool acquire() {
        if (m_active >= m_limit) {
            cout << "  [拒絕] 連線已達上限 " << m_limit << endl;
            return false;
        }
        ++m_active;
        cout << "  [取得] 目前使用中 " << m_active << "/" << m_limit << endl;
        return true;
    }
    void release() {
        if (m_active > 0) --m_active;      // 不變量：不可為負
        cout << "  [釋放] 目前使用中 " << m_active << "/" << m_limit << endl;
    }
    int active() const { return m_active; }
};

int main() {
    cout << "=== 差異 ①：預設成員存取權 ===" << endl;
    MyClass c;
    // cout << c.x;    // ❌ 編譯錯誤！x 是 private
    cout << "  class : x 預設 private，需透過 getX() → " << c.getX() << endl;

    MyStruct s;
    cout << "  struct: x 預設 public，可直接存取 → " << s.x << endl;

    cout << "\n=== 差異 ②：預設繼承存取權（多型的隱形殺手） ===" << endl;
    DerivedByStruct ds;
    Base* p = &ds;                 // ✅ struct 預設 public 繼承
    cout << "  struct 繼承（預設 public）：可轉成 Base*" << endl;
    p->hello();

    DerivedByClass dc;
    // Base* q = &dc;              // ❌ error: 'Base' is an inaccessible base of 'DerivedByClass'
    cout << "  class  繼承（預設 private）：Base* q = &dc; 會編譯失敗" << endl;
    cout << "  → 所以繼承時一定要明寫 public/private，別依賴預設值。" << endl;
    (void)dc;

    cout << "\n=== struct 一樣能有建構函式、成員函式與 private 成員 ===" << endl;
    Timestamp t(45296);
    cout << "  Timestamp(45296).format() = " << t.format()
         << "（seconds=" << t.seconds() << "）" << endl;

    cout << "\n=== 日常實務：純資料用 struct（可 aggregate 初始化） ===" << endl;
    ServerConfig def;
    cout << "  預設值： ";
    printConfig(def);

    ServerConfig prod{443, "10.0.0.5", 8192, true};   // aggregate initialization
    cout << "  正式機： ";
    printConfig(prod);

    ServerConfig tweak = def;
    tweak.port = 9090;              // 純資料，直接改欄位是完全正當的
    cout << "  微調後： ";
    printConfig(tweak);

    cout << "\n=== 日常實務：有不變量就用 class ===" << endl;
    ConnectionPool pool(2);
    pool.acquire();
    pool.acquire();
    pool.acquire();                 // 被擋下：已達上限
    pool.release();
    pool.acquire();                 // 釋放後又可取得
    cout << "  → 「連線數不可為負、不可超過上限」這兩條不變量" << endl;
    cout << "     只有在狀態 private 時才保得住。" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 11 課：存取修飾符——public、private、protected5.cpp" -o access5

// === 預期輸出 ===
// === 差異 ①：預設成員存取權 ===
//   class : x 預設 private，需透過 getX() → 10
//   struct: x 預設 public，可直接存取 → 10
//
// === 差異 ②：預設繼承存取權（多型的隱形殺手） ===
//   struct 繼承（預設 public）：可轉成 Base*
//   Base::hello(), id=7
//   class  繼承（預設 private）：Base* q = &dc; 會編譯失敗
//   → 所以繼承時一定要明寫 public/private，別依賴預設值。
//
// === struct 一樣能有建構函式、成員函式與 private 成員 ===
//   Timestamp(45296).format() = 12:34:56（seconds=45296）
//
// === 日常實務：純資料用 struct（可 aggregate 初始化） ===
//   預設值：   host=0.0.0.0 port=8080 maxConn=1024 tls=off
//   正式機：   host=10.0.0.5 port=443 maxConn=8192 tls=on
//   微調後：   host=0.0.0.0 port=9090 maxConn=1024 tls=off
//
// === 日常實務：有不變量就用 class ===
//   [取得] 目前使用中 1/2
//   [取得] 目前使用中 2/2
//   [拒絕] 連線已達上限 2
//   [釋放] 目前使用中 1/2
//   [取得] 目前使用中 2/2
//   → 「連線數不可為負、不可超過上限」這兩條不變量
//      只有在狀態 private 時才保得住。
