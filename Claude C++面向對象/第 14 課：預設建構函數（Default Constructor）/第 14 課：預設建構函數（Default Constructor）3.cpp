// =============================================================================
//  第 14 課：預設建構函數 3  —  自己手寫一個 default constructor 補回來
// =============================================================================
//
// 【主題資訊 Information】
//   主題      : 手寫 default constructor（user-provided）與帶參版本並存
//   標準版本  : C++98 起
//   標頭檔    : <iostream>、<type_traits>
//   對照      : 4.cpp 用 = default，5.cpp 比較兩者的關鍵差異
//
// 【詳細解釋 Explanation】
//
// 【1. 手寫版本解決了「能不能編譯」，但沒解決「值對不對」】
//   本檔的 Point() { x = 0; y = 0; } 是 user-provided default constructor。
//   它做兩件事：讓 Point p; 編得過，並且明確把座標設成 (0,0)。
//   第二點很重要——如果你寫成空的 Point() { }，
//   它一樣能編譯，但 x、y 依然是不確定值，等於只解決了一半的問題。
//   這是初學者最常見的假修復：以為「有 constructor 就安全了」。
//
// 【2. user-provided 這個詞的份量】
//   「使用者提供的」指的是「你寫了函式本體」，它與 = default 有實質差異：
//       * user-provided 的 default constructor 讓類別**不再**是 aggregate
//       * 更關鍵的是：Point p{}; 這種值初始化，對 user-provided 版本
//         **不會**先做 zero-initialization，只會呼叫你的 constructor
//   所以若你寫的是空的 Point() { }，連 Point p{}; 都救不了你。
//   這正是 5.cpp 要專門處理的主題，本檔先建立這個概念。
//
// 【3. 賦值 vs 初始化：本檔埋的效能伏筆】
//   Point() { x = 0; y = 0; } 的實際順序是：
//       1. 對 x、y 做 default-initialization（int → 什麼都不做）
//       2. 進入 constructor 本體
//       3. 執行 x = 0; y = 0;
//   對 int 而言差異微乎其微（編譯器最佳化後通常一模一樣）。
//   但若成員是 std::string 或 std::vector，第 1 步會真的呼叫一次 constructor，
//   第 3 步再呼叫一次 operator=，就是實實在在的兩次工作。
//   正解是成員初始化列表：Point() : x(0), y(0) {}（第 15 課主題）。
//
// 【4. 三種寫法的比較（本檔全部示範）】
//   ┌────────────────────────────┬──────────┬────────────────────┐
//   │ 寫法                        │ 能否編譯 │ x、y 的值           │
//   ├────────────────────────────┼──────────┼────────────────────┤
//   │ Point() { }                 │ 可以     │ 不確定（假修復！）  │
//   │ Point() { x = 0; y = 0; }   │ 可以     │ 0（本檔作法）       │
//   │ Point() : x(0), y(0) {}     │ 可以     │ 0（最推薦）         │
//   └────────────────────────────┴──────────┴────────────────────┘
//
// 【概念補充 Concept Deep Dive】
//   ▍為什麼 constructor 裡的 cout 不影響 constructor 的「身分」
//     不論本體裡寫什麼，只要你寫了 { }，它就是 user-provided。
//     編譯器不會因為「本體是空的」就把它當成 = default 那種版本——
//     這兩者在語言規則上是不同的東西，即使肉眼看起來效果一樣。
//
//   ▍多載並存不會互相干擾
//     Point() 與 Point(int, int) 是兩個獨立的多載，
//     由參數個數決定用哪一個，編譯期就完成解析，沒有執行期成本。
//
//   ▍delegating constructor 可以避免重複
//     若兩個 constructor 有共同的初始化邏輯，C++11 起可以委派：
//         Point() : Point(0, 0) {}
//     本檔示範了這個寫法。它保證邏輯只有一份，之後改動不會漏改。
//
// 【注意事項 Pay Attention】
//   1. 空的 Point() { } 能編譯但不初始化成員——是最常見的假修復。
//   2. user-provided 的 default constructor 會讓 T obj{}; 失去 zero-init 保證。
//   3. constructor 本體內是「賦值」不是「初始化」；重量級成員要用初始化列表。
//   4. const 成員與 reference 成員**只能**在初始化列表初始化，不能在本體賦值。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】手寫 default constructor
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 空的 Point() { } 和 Point() : x(0), y(0) {} 差在哪？
//     答：前者只讓程式編得過，x、y 依然是不確定值——這是假修復。
//         後者才真的把成員初始化成 0。而且前者是 user-provided，
//         會讓 Point p{}; 也失去 zero-initialization 保證，等於兩條路都堵死。
//     追問：那 Point() = default; 呢？→ 它不是 user-provided，
//         所以 Point p{}; 仍會零初始化；但 Point p; 依然是不確定值（見 4.cpp、5.cpp）。
//
// 🔥 Q2. constructor 本體裡的 x = 0 和初始化列表的 : x(0) 有什麼實質差別？
//     答：本體內是「先 default-initialize、再賦值」兩個步驟；
//         初始化列表是「直接建構成目標值」一個步驟。
//         對 int 幾乎沒差（最佳化後常相同），對 std::string／vector 就是
//         多一次建構加一次賦值。更關鍵的是 const 成員與 reference 成員
//         **只能**用初始化列表，本體賦值直接編譯失敗。
//     追問：初始化列表的順序重要嗎？→ 實際執行順序永遠是成員的宣告順序，
//         不是你寫的順序；寫反會拿到 -Wreorder 警告。
//
// ⚠️ 陷阱. 「我補了 default constructor，所以物件一定被初始化好了」
//     答：不一定。補的若是空的 { }，成員完全沒被碰過。
//         「有 constructor」和「成員有值」是兩件獨立的事——
//         constructor 只是一個會被自動呼叫的函式，它做不做事完全看你怎麼寫。
//     為什麼會錯：把 constructor 想成「語言會順便幫我初始化」的機制。
//         實際上它只是保證「這個函式一定會被呼叫」，內容還是你的責任。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <type_traits>
using namespace std;

class Point {
public:
    int x, y;

    // 手動加回預設建構函數，並且真的把成員設好
    Point() {
        x = 0;
        y = 0;
        cout << "預設建構: Point(0, 0)" << endl;
    }

    Point(int px, int py) {
        x = px;
        y = py;
        cout << "帶參建構: Point(" << x << ", " << y << ")" << endl;
    }
};

// ❌ 假修復：能編譯，但什麼也沒初始化
class PointFake {
public:
    int x, y;
    PointFake() { }                    // 空的！x、y 仍是不確定值
    PointFake(int a, int b) : x(a), y(b) {}
};

// ✅ 最推薦：初始化列表（第 15 課主題），或 NSDMI + = default
class PointBest {
public:
    int x, y;
    PointBest() : x(0), y(0) {}                    // 直接建構成 0
    PointBest(int a, int b) : x(a), y(b) {}
};

// C++11 委派：共同邏輯只寫一份
class PointDelegating {
public:
    int x, y;
    PointDelegating() : PointDelegating(0, 0) {}   // 委派給帶參版本
    PointDelegating(int a, int b) : x(a), y(b) {}
};

// 編譯期事實查核：三者都可預設建構，但「是否 user-provided」不同
static_assert(std::is_default_constructible<Point>::value, "");
static_assert(std::is_default_constructible<PointFake>::value, "");
static_assert(!std::is_trivially_default_constructible<Point>::value,
              "user-provided 的 default constructor 必定不是 trivial");

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】從缺
//   理由：本檔比較的是「同一個類別的三種 default constructor 寫法」對
//         初始化結果與 aggregate 性質的影響，是語言規則的細節。
//         LeetCode 題目只在意演算法輸出，不會因為 constructor 寫法不同而有差異。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】連線設定（ConnectionOptions）：「合理的預設值」本身就是功能
//   情境：一個網路客戶端的設定物件，大多數呼叫端只想用預設值，
//         少數才需要細調。此時 default constructor 不只是「讓它能編譯」，
//         它承載了「這個函式庫建議的預設行為」這個產品決策。
//   要點：預設值應該是「安全且大多數情況正確」的——
//         例如逾時給 30 秒而不是 0（等於永不逾時），重試給 3 次而不是無限。
//         把這些決定寫進 default constructor，呼叫端才不必每次都想一遍。
// -----------------------------------------------------------------------------
class ConnectionOptions {
private:
    int    timeoutMs_;
    int    maxRetries_;
    bool   verifyTls_;
    string userAgent_;

public:
    // 預設建構：一組「安全且合理」的預設值
    ConnectionOptions()
        : timeoutMs_(30000), maxRetries_(3), verifyTls_(true),
          userAgent_("myclient/1.0") {}

    // 需要細調時用這個
    ConnectionOptions(int timeoutMs, int maxRetries, bool verifyTls)
        : timeoutMs_(timeoutMs), maxRetries_(maxRetries), verifyTls_(verifyTls),
          userAgent_("myclient/1.0") {}

    void print() const {
        cout << "  timeout=" << timeoutMs_ << "ms"
             << ", retries=" << maxRetries_
             << ", TLS驗證=" << (verifyTls_ ? "開啟" : "關閉")
             << ", UA=" << userAgent_ << endl;
    }
};

int main() {
    cout << "=== 手寫 default constructor：兩個多載並存 ===" << endl;
    Point p1;          // OK：呼叫預設建構函數
    Point p2(3, 4);    // OK：呼叫帶參建構函數
    cout << "  p1 = (" << p1.x << ", " << p1.y << ")" << endl;
    cout << "  p2 = (" << p2.x << ", " << p2.y << ")" << endl;

    cout << "\n=== 假修復：空的 constructor 什麼也沒做 ===" << endl;
    PointFake fake;
    cout << "  fake = (" << fake.x << ", " << fake.y << ")"
         << "  ← 不確定值，每次執行都可能不同" << endl;
    PointFake fake2{};   // 連值初始化都救不了：user-provided 沒有 zero-init 保證
    cout << "  fake2{} = (" << fake2.x << ", " << fake2.y << ")"
         << "  ← 一樣不確定，{} 對 user-provided 版本無效" << endl;

    cout << "\n=== 推薦寫法：初始化列表 ===" << endl;
    PointBest best;
    cout << "  best = (" << best.x << ", " << best.y << ")  ← 保證是 0" << endl;

    cout << "\n=== C++11 委派：共同邏輯只寫一份 ===" << endl;
    PointDelegating d1;
    PointDelegating d2(9, 9);
    cout << "  d1 = (" << d1.x << ", " << d1.y << ")" << endl;
    cout << "  d2 = (" << d2.x << ", " << d2.y << ")" << endl;

    cout << "\n=== 日常實務：連線設定的預設值 ===" << endl;
    cout << "大多數呼叫端：直接用預設" << endl;
    ConnectionOptions defaults;
    defaults.print();

    cout << "需要細調時：" << endl;
    ConnectionOptions tuned(5000, 1, true);   // 內網服務：快速失敗
    tuned.print();

    cout << "  ↑ default constructor 承載的是「函式庫建議的預設行為」，" << endl;
    cout << "    不只是讓程式編得過而已。" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 14 課：預設建構函數（Default Constructor）3.cpp" -o def3

// === 預期輸出 ===
// ⚠️ PointFake 的 x / y 是不確定值（讀取即 UB），下方為某一次執行的結果，每次執行都可能不同；
//    Point、PointBest、PointDelegating 與實務範例的數值才是有保證的。
// === 手寫 default constructor：兩個多載並存 ===
// 預設建構: Point(0, 0)
// 帶參建構: Point(3, 4)
//   p1 = (0, 0)
//   p2 = (3, 4)
//
// === 假修復：空的 constructor 什麼也沒做 ===
//   fake = (1516320128, 29589)  ← 不確定值，每次執行都可能不同
//   fake2{} = (1, 0)  ← 一樣不確定，{} 對 user-provided 版本無效
//
// === 推薦寫法：初始化列表 ===
//   best = (0, 0)  ← 保證是 0
//
// === C++11 委派：共同邏輯只寫一份 ===
//   d1 = (0, 0)
//   d2 = (9, 9)
//
// === 日常實務：連線設定的預設值 ===
// 大多數呼叫端：直接用預設
//   timeout=30000ms, retries=3, TLS驗證=開啟, UA=myclient/1.0
// 需要細調時：
//   timeout=5000ms, retries=1, TLS驗證=開啟, UA=myclient/1.0
//   ↑ default constructor 承載的是「函式庫建議的預設行為」，
//     不只是讓程式編得過而已。
