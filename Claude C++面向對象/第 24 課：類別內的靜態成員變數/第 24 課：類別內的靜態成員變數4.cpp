// =============================================================================
//  第 24 課：類別內的靜態成員變數 4  —  靜態成員的生命週期
// =============================================================================
//
// 【主題資訊 Information】
//   語法:  class C { inline static T obj{...}; };   // C++17
//   儲存期: static storage duration（靜態儲存期）
//   生命期: main 之前建構 → main 之後解構，與任何物件無關
//   標頭檔: <string>
//   標準版本: 靜態成員 C++98；inline static 就地定義並初始化為 C++17
//
// 【詳細解釋 Explanation】
//
// 【1. 三種儲存期，決定「活多久」】
//   C++ 物件的生命期由「儲存期」決定，不是由寫在哪裡決定：
//     * automatic（區域變數）：進入作用域建構，離開作用域解構。
//     * dynamic（new/delete）：由程式設計師決定。
//     * static（靜態成員、全域變數、函式內 static）：
//       整個程式一份，在 main 之前初始化、main 之後才解構。
//   本檔的 staticTracker 是第三種，memberTracker 是第一種（跟著 obj 走）。
//   輸出會清楚顯示：obj 解構後 staticTracker 還能 ping()，
//   而 staticTracker 的 [解構] 出現在 "main 結束" 之後 —— 那已經不在 main 裡了。
//
// 【2. 初始化發生在什麼時候】
//   靜態初始化分兩階段：
//     (a) static initialization：常數初始化 / 零初始化，發生在載入期，
//         不執行任何程式碼（constant initialization）。
//     (b) dynamic initialization：需要呼叫建構子的，
//         在同一個 translation unit 內，依「宣告順序」執行，且都在 main 之前。
//   staticTracker 有建構子，屬於 (b)。所以主控台第一行 [建構] 會出現在
//   "=== 靜態成員的生命週期 ===" 之前 —— 那時 main 根本還沒開始跑。
//
// 【3. 解構順序：反向，但只在同一個 TU 內有保證】
//   同一個 translation unit 內，靜態物件依「建構完成的反向順序」解構，
//   這是標準保證的（[basic.start.term]）。
//   但「跨 translation unit」的初始化與解構順序是 unspecified ——
//   這就是有名的 static initialization / destruction order fiasco：
//   A.cpp 的靜態物件解構時去碰 B.cpp 的靜態物件，
//   而 B 可能已經被解構了，於是存取到已結束生命期的物件。
//
// 【4. 為什麼實務上偏好 Meyers Singleton】
//   把靜態物件包進函式裡：
//       static Logger& logger() { static Logger inst; return inst; }
//   好處是「第一次呼叫時才建構」（lazy），
//   因此建構順序自然等於「第一次使用的順序」，破解了初始化順序問題。
//   C++11 起這種 function-local static 的初始化還保證是 thread-safe 的
//   （所謂 magic static，編譯器會插入 guard 變數）。
//   注意它只解決「初始化順序」，解構順序仍是反向、仍需小心。
//
// 【概念補充 Concept Deep Dive】
//   * inline static（C++17）之前，非 const 靜態成員必須在某個 .cpp 裡
//     再寫一次定義（int C::x = 0;），否則連結期會 undefined reference。
//     inline 讓多個 TU 看到同一份定義而不違反 ODR，因此可以只寫在標頭檔。
//   * 靜態成員不佔物件空間：sizeof(MyClass) 只含 memberTracker。
//   * 靜態物件的解構是透過 __cxa_atexit 註冊的（Itanium ABI）。
//     這也是為什麼 std::exit 會跑解構子，而 std::abort / _Exit 不會 ——
//     後者直接結束行程，靜態物件的解構子根本不執行。
//   * 若解構子裡有 cout 輸出，而 cout 自己也是靜態物件，
//     理論上會有順序問題；標準特別用 std::ios_base::Init 保證
//     只要含入 <iostream>，串流在該 TU 的靜態物件之前初始化、之後才銷毀。
//
// 【注意事項 Pay Attention】
//   1. 靜態成員的解構在 main 之後，任何「main 結束就代表結束」的假設都不成立。
//   2. 跨 TU 的靜態初始化/解構順序是 unspecified，不可依賴。
//   3. std::abort() 與 std::_Exit() 不會執行靜態物件的解構子，
//      靠解構子 flush 檔案的設計在崩潰路徑上不會生效。
//   4. 靜態物件在多執行緒下是共享狀態；non-const 靜態成員需自行同步。
//   5. 靜態物件的建構若丟例外，會直接呼叫 std::terminate（發生在 main 之前，無處可接）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】靜態成員的生命週期
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 類別的靜態成員物件，什麼時候建構、什麼時候解構?
//     答：建構在 main 之前（需要建構子的屬 dynamic initialization，
//         同一個 TU 內依宣告順序），解構在 main 之後（反向順序）。
//         它的生命期與任何物件無關 —— 所有物件都銷毀了它依然活著。
//     追問：那 main 裡 return 之後、程式真正結束之前發生了什麼?
//         → 先跑完 main 的區域物件解構，再依註冊的反向順序
//         跑靜態物件的解構（Itanium ABI 用 __cxa_atexit 註冊）。
//
// 🔥 Q2. 什麼是 static initialization order fiasco?怎麼解?
//     答：跨 translation unit 的靜態物件，初始化順序是 unspecified。
//         若 A.cpp 的靜態物件在建構時使用 B.cpp 的靜態物件，
//         而 B 尚未初始化，就會讀到未建構的物件。
//         解法是 Meyers Singleton：把物件放進函式內的 static，
//         第一次呼叫才建構，順序自然正確；
//         C++11 起這種 function-local static 初始化還保證 thread-safe。
//     追問：Meyers Singleton 有解決「解構」順序嗎?
//         → 沒有。解構仍是反向順序，若解構子之間互相依賴仍會出問題；
//         真正需要時的做法是刻意不解構（leaky singleton）或用 shared_ptr 管相依。
//
// ⚠️ 陷阱. 「解構子裡寫檔案就能保證程式結束時資料一定寫出去。」
//     答：不成立。靜態物件的解構子只在「正常結束」時執行 ——
//         也就是 main 回傳或呼叫 std::exit。
//         std::abort()（含未捕捉例外導致的 terminate）、std::_Exit()、
//         被 SIGKILL 砍掉、斷電，都不會跑解構子。
//     為什麼會錯：把「解構子一定會被呼叫」當成無條件的保證，
//         但那個保證的前提是「物件生命期正常結束」。
//         行程被強制終止時，物件的生命期並沒有「結束」，只是消失了。
//         需要耐久性就必須主動 flush，而不是依賴解構子。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 從缺，理由如下
//   物件生命期與靜態儲存期是語言執行模型，LeetCode 判題不會考。
//   真正相關的是判題環境的副作用：LeetCode 在同一個行程內連續跑多筆測資，
//   類別的 static 狀態不會在測資之間重置，
//   於是出現「單筆測資對、整批跑就錯」。這屬於本課的實務警訊而非題目。
//
// =============================================================================

#include <iostream>
#include <string>
using namespace std;

class Tracker {
private:
    string label_;

public:
    Tracker(const string& label) : label_(label) {
        cout << "  [建構] " << label_ << endl;
    }
    ~Tracker() {
        cout << "  [解構] " << label_ << endl;
    }

    void ping() const {
        cout << "  " << label_ << " 存活中" << endl;
    }
};

class MyClass {
public:
    // 靜態成員：程式開始時初始化
    // 靜態成員變數屬於類別本身，而不是任何特定的對象
    // 這裡我們定義了一個靜態 Tracker 成員，直接在類別內初始化
    inline static Tracker staticTracker{"靜態成員 Tracker"};

    // 普通成員
    // 每個 MyClass 對象都會有自己的 memberTracker
    // 這裡我們在建構函數中初始化 memberTracker，並給它一個獨特的標籤
    Tracker memberTracker;

    MyClass(const string& name) : memberTracker("普通成員 " + name) {
        cout << "  [建構] MyClass " << name << endl;
    }

    ~MyClass() {
        cout << "  [解構] MyClass" << endl;
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】服務關閉時的統計數據落地（shutdown flush）
//   情境：長時間執行的服務會在記憶體裡累積請求計數，
//         希望「行程結束時」把統計寫進 log。很多人直覺就寫在靜態物件的解構子裡。
//   這個範例示範它「確實會在 main 之後執行」，
//   同時也點出它的前提：只有正常結束（main 回傳 / std::exit）才算數。
//   實務建議：解構子當成最後一道保險，
//             真正重要的資料要在明確的 shutdown() 流程主動 flush。
// -----------------------------------------------------------------------------
class MetricsCollector {
private:
    string service_;
    long   requests_ = 0;

public:
    explicit MetricsCollector(const string& svc) : service_(svc) {
        cout << "  [metrics] 收集器啟動：" << service_ << endl;
    }

    void recordRequest() { ++requests_; }

    // 明確的 flush：實務上應該由 shutdown 流程呼叫，而不是只靠解構子
    void flush(const string& reason) const {
        cout << "  [metrics] flush(" << reason << ")："
             << service_ << " 共處理 " << requests_ << " 筆請求" << endl;
    }

    ~MetricsCollector() {
        // 這行會出現在 "main 結束" 之後 —— 證明靜態物件的解構在 main 之後
        flush("解構子");
    }
};

// 靜態成員：整個程式一份，main 之前建構、main 之後解構
class ApiServer {
public:
    inline static MetricsCollector metrics{"api-server"};
};

int main() {
    cout << "=== 靜態成員的生命週期 ===" << endl;
    cout << "(靜態成員已在 main 之前初始化)" << endl;

    cout << "\n--- 創建對象 ---" << endl;
    {
        MyClass obj("測試");

        cout << "\n--- 使用中 ---" << endl;
        MyClass::staticTracker.ping();
        obj.memberTracker.ping();

        cout << "\n--- 作用域結束 ---" << endl;
    }
    // obj 已銷毀，但靜態成員還活著
    // 靜態成員的生命週期超過任何對象，直到程式結束才會被銷毀

    cout << "\n--- obj 已銷毀，靜態成員仍在 ---" << endl;
    MyClass::staticTracker.ping();

    cout << "\n=== 日常實務：服務關閉時的統計落地 ===" << endl;
    for (int i = 0; i < 3; ++i) ApiServer::metrics.recordRequest();
    ApiServer::metrics.flush("主動呼叫");
    cout << "  ↑ 主動 flush 是可靠的；解構子那次要等 main 之後才發生。" << endl;

    cout << "\n--- main 結束 ---" << endl;
    return 0;
}
// 程式結束後，靜態成員才會被銷毀（反向於建構順序）
// 因此下面兩行必然出現在 "--- main 結束 ---" 之後

// 編譯: g++ -std=c++17 -Wall -Wextra 第 24 課：類別內的靜態成員變數4.cpp -o static_member4

// === 預期輸出 ===
//   [建構] 靜態成員 Tracker
//   [metrics] 收集器啟動：api-server
// === 靜態成員的生命週期 ===
// (靜態成員已在 main 之前初始化)
//
// --- 創建對象 ---
//   [建構] 普通成員 測試
//   [建構] MyClass 測試
//
// --- 使用中 ---
//   靜態成員 Tracker 存活中
//   普通成員 測試 存活中
//
// --- 作用域結束 ---
//   [解構] MyClass
//   [解構] 普通成員 測試
//
// --- obj 已銷毀，靜態成員仍在 ---
//   靜態成員 Tracker 存活中
//
// === 日常實務：服務關閉時的統計落地 ===
//   [metrics] flush(主動呼叫)：api-server 共處理 3 筆請求
//   ↑ 主動 flush 是可靠的；解構子那次要等 main 之後才發生。
//
// --- main 結束 ---
//   [metrics] flush(解構子)：api-server 共處理 3 筆請求
//   [解構] 靜態成員 Tracker
