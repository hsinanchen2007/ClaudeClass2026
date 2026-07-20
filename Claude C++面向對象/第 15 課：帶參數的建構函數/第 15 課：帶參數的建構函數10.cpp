// =============================================================================
//  第 15 課：帶參數的建構函數 10  —  explicit：禁止隱式轉換
// =============================================================================
//
// 【主題資訊 Information】
//   語法：  explicit ClassName(T param);
//   標準版本：C++98 起可用於單參數建構函式；
//             C++11 起可用於多參數建構函式與轉換運算子；
//             C++20 起支援 explicit(bool)（條件式 explicit）
//   標頭檔：<type_traits>（本檔用 static_assert 驗證）
//   核心作用：禁止「編譯器自動用這個建構函式做型別轉換」
//
// 【詳細解釋 Explanation】
//
// 【1. 沒有 explicit 時，單參數建構函式是「轉換建構函式」】
//   任何可以用單一引數呼叫的建構函式，預設都會被編譯器當成
//   「從參數型別轉換到本類別」的轉換途徑。於是：
//       Distance d = 100.0;          // 編譯器偷偷呼叫 Distance(100.0)
//       showDistance(200.0);         // 也自動轉換了
//   這在少數情況方便（例如 std::string s = "hello";），但更多時候是災難。
//
// 【2. 為什麼隱式轉換是災難？—— 它讓型別錯誤靜默通過】
//   型別系統的價值在於「把錯誤擋在編譯期」。隱式轉換等於在型別之間開了後門：
//       void applyDiscount(Distance d);
//       applyDiscount(0.15);     // 作者想傳「15% 折扣」，卻建出了「0.15 公尺」
//   這段程式碼順利編譯、順利執行，只是結果完全錯誤。
//   加上 explicit 之後，這一行變成編譯錯誤 —— 錯誤被抓在最便宜的時間點。
//   單位混淆（公尺 vs 英尺、秒 vs 毫秒）在真實工程中造成過重大事故，
//   explicit 正是防止這類錯誤的第一道防線。
//
// 【3. explicit 到底擋掉了什麼？—— 精確的界線】
//   被禁止（copy-initialization 語境）：
//       Distance d = 100.0;              // ❌
//       showDistance(200.0);             // ❌ 函式引數的隱式轉換
//       return 300.0;                    // ❌ 回傳值的隱式轉換
//       vector<Distance> v = {1.0, 2.0}; // ❌ 初始化列表
//   仍然允許（direct-initialization 語境）：
//       Distance d1(100.0);              // ✅ 直接初始化
//       Distance d2{100.0};              // ✅ 直接列表初始化
//       showDistance(Distance(200.0));   // ✅ 明確寫出轉換
//       static_cast<Distance>(300.0);    // ✅ 明確轉換
//   一句話：explicit 要求你「把轉換寫出來」，而不是禁止轉換本身。
//
// 【4. 什麼時候「不該」加 explicit？】
//   當「這個轉換在語意上就是同一個東西的另一種表示」時，隱式反而更自然：
//       std::string s = "hello";                    // const char* → string
//       std::chrono::seconds s = 5s;                // 使用者定義字面值
//   判準：轉換之後「概念上還是同一個東西」就可以隱式；
//   若是「用一個數字產生一個有單位／有語意的物件」，一律加 explicit。
//   Core Guidelines C.46 的建議是：預設就加 explicit，除非你有明確理由不加。
//
// 【概念補充 Concept Deep Dive】
//   * C++11 起 explicit 也能用在多參數建構函式上，因為 `{1, 2}` 這種
//     大括號初始化可以觸發多參數的隱式轉換：
//       void f(Rect r);   f({1, 2});   // 若 Rect(int,int) 非 explicit 就合法
//   * explicit 也可用於轉換運算子：`explicit operator bool() const;`
//     這是智慧指標的標準做法 —— 讓 `if (ptr)` 可行，
//     但擋掉 `int x = ptr;` 這種荒謬的隱式轉換。
//   * C++20 的 `explicit(bool)` 允許依條件決定是否 explicit，
//     主要用於樣板程式庫（例如 std::pair 的建構函式）。
//   * 隱式轉換最多只會發生「一次」使用者定義的轉換。
//     所以即使沒有 explicit，`showDistance("100")` 仍會失敗
//     （const char* → string → Distance 需要兩次使用者定義轉換）。
//
// 【注意事項 Pay Attention】
//   1. 單參數建構函式預設就是轉換建構函式 —— 這是預設開啟的後門。
//   2. explicit 不是禁止轉換，而是要求你明確寫出來。
//   3. 有預設值使得「可以用單一引數呼叫」的多參數建構函式同樣受影響
//      （例如 `Color(int r = 0, int g = 0, int b = 0)`），也該考慮加 explicit。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】explicit 關鍵字
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. explicit 的作用是什麼？什麼時候該加？
//     答：禁止編譯器用這個建構函式做隱式型別轉換，呼叫端必須明確寫出轉換
//         （`Distance d(100.0)` 或 `static_cast<Distance>(100.0)`）。
//         凡是「用一個數字或字串產生一個有單位、有語意的物件」都該加，
//         Core Guidelines C.46 建議「預設加，除非有明確理由不加」。
//     追問：什麼情況不該加？→ 當轉換後「概念上還是同一個東西」時，
//           例如 `std::string s = "hello";`。強制寫成 string("hello") 反而囉唆。
//
// 🔥 Q2. 加了 explicit 之後，哪些寫法還能用？
//     答：直接初始化的全部都能用：`Distance d(100.0);`、`Distance d{100.0};`、
//         `showDistance(Distance(200.0));`、`static_cast<Distance>(300.0)`。
//         被擋掉的是複製初始化語境：`Distance d = 100.0;`、
//         直接把 double 傳給吃 Distance 的函式、以及從 double 直接 return。
//     追問：多參數建構函式需要 explicit 嗎？→ C++11 起需要考慮，
//           因為 `f({1, 2})` 這種大括號寫法可以觸發多參數的隱式轉換。
//
// ⚠️ 陷阱. 沒加 explicit 的單參數建構函式，最實際的危害是什麼？
//     答：讓「單位錯誤」這類語意 bug 靜默通過編譯。
//         `applyTimeout(30)` 作者以為是 30 秒，型別卻是毫秒的 Duration ——
//         程式順利編譯執行，只是逾時設成了 30 毫秒。
//         這種 bug 沒有任何編譯期線索，只能靠測試或線上事故發現。
//     為什麼會錯：把隱式轉換當成「方便的功能」，沒意識到它等於
//         「主動關閉型別系統對這個型別的保護」。型別存在的意義正是
//         讓「公尺」與「英尺」、「秒」與「毫秒」無法互相代入。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <vector>
#include <type_traits>
using namespace std;

class SafeDistance {
private:
    double meters;

public:
    // explicit：禁止隱式轉換。
    // 沒有它的話 `SafeDistance d = 100.0;` 會靜默成立 ——
    // 而那正是「單位混淆」這類 bug 的溫床。
    explicit SafeDistance(double m) : meters(m) {
        cout << "  建構 SafeDistance: " << meters << " m" << endl;
    }

    void print() const {
        cout << "  距離: " << meters << " 公尺" << endl;
    }
    double value() const { return meters; }
};

// 對照組：沒有 explicit —— 開啟了 double → UnsafeDistance 的自動轉換
class UnsafeDistance {
private:
    double meters;
public:
    UnsafeDistance(double m) : meters(m) {}      // 沒有 explicit！
    double value() const { return meters; }
};

void showDistance(SafeDistance d) {
    d.print();
}

// 編譯期驗證 explicit 的效果
static_assert(!is_convertible<double, SafeDistance>::value,
              "explicit 阻止了 double → SafeDistance 的隱式轉換");
static_assert(is_convertible<double, UnsafeDistance>::value,
              "沒有 explicit 就會開啟隱式轉換");
static_assert(is_constructible<SafeDistance, double>::value,
              "但明確建構仍然可行 —— explicit 只擋隱式，不擋明確");

// -----------------------------------------------------------------------------
// 【日常實務範例】強型別的時間單位 —— explicit 防止「秒 / 毫秒」混淆
//   情境：設定 HTTP 用戶端的逾時。這是單位混淆最常出事的地方之一：
//   有人以為參數是秒、有人以為是毫秒，而 int 對兩者一視同仁。
//   `setTimeout(30)` 到底是 30 秒還是 30 毫秒？從呼叫端完全看不出來，
//   而且傳錯了也不會有任何編譯錯誤 —— 直到線上出現大量逾時才發現。
//   解法是為每個單位建立獨立型別並加上 explicit：
//     * 型別不同 → 編譯器擋住互相代入
//     * explicit → 擋住「直接傳一個裸數字」
//   之後 `setTimeout(Seconds(30))` 的意圖在呼叫端一目了然，
//   而 `setTimeout(30)` 直接編譯失敗。這正是 std::chrono 的設計哲學。
// -----------------------------------------------------------------------------
class Milliseconds {
public:
    explicit Milliseconds(long v) : m_value(v) {}
    long count() const { return m_value; }
private:
    long m_value;
};

class Seconds {
public:
    explicit Seconds(long v) : m_value(v) {}
    long count() const { return m_value; }
    // 提供「明確的」單位轉換，而不是讓編譯器亂猜
    Milliseconds toMilliseconds() const { return Milliseconds(m_value * 1000); }
private:
    long m_value;
};

class HttpClientConfig {
public:
    // 參數型別就寫死了單位，呼叫端不可能傳錯
    HttpClientConfig(const string& url, Milliseconds timeout)
        : m_url(url), m_timeoutMs(timeout) {}

    void describe() const {
        cout << "    " << m_url << " timeout=" << m_timeoutMs.count() << " ms" << endl;
    }

private:
    string       m_url;
    Milliseconds m_timeoutMs;
};

int main() {
    cout << "=== 明確建構：全部合法 ===" << endl;
    SafeDistance d1(100.0);              // 直接初始化
    d1.print();

    SafeDistance d2{50.0};               // 直接列表初始化
    d2.print();

    showDistance(SafeDistance(200.0));   // 明確寫出轉換
    SafeDistance d3 = static_cast<SafeDistance>(300.0);   // 明確轉換
    d3.print();

    cout << "\n=== explicit 擋掉的寫法（已註解）===" << endl;
    // SafeDistance bad1 = 50.0;      // ❌ copy-initialization
    // showDistance(200.0);            // ❌ 函式引數的隱式轉換
    // vector<SafeDistance> v = {1.0, 2.0};   // ❌ 初始化列表
    cout << "  SafeDistance d = 50.0;   → 編譯錯誤" << endl;
    cout << "  showDistance(200.0);      → 編譯錯誤" << endl;
    cout << "  vector<SafeDistance> v = {1.0, 2.0};  → 編譯錯誤" << endl;
    cout << "  ↑ explicit 不是禁止轉換，而是要求你把轉換「寫出來」" << endl;

    cout << "\n=== ⚠️ 沒有 explicit 的危險 ===" << endl;
    UnsafeDistance u = 0.15;             // 作者可能想表達「15% 折扣」！
    cout << "  `UnsafeDistance u = 0.15;` 靜默成立，建出 "
         << u.value() << " 公尺" << endl;
    cout << "  ↑ 型別錯誤完全沒有被攔截 —— 這正是 explicit 要防的事" << endl;

    cout << "\n=== 編譯期驗證 ===" << endl;
    cout << boolalpha;
    cout << "  is_convertible<double, SafeDistance>   = "
         << is_convertible<double, SafeDistance>::value   << "  （explicit 擋下）" << endl;
    cout << "  is_convertible<double, UnsafeDistance> = "
         << is_convertible<double, UnsafeDistance>::value << "   （後門敞開）" << endl;
    cout << "  is_constructible<SafeDistance, double> = "
         << is_constructible<SafeDistance, double>::value << "   （明確建構仍可行）" << endl;

    cout << "\n=== 實務：強型別時間單位 ===" << endl;
    HttpClientConfig fast("https://api.example.com", Milliseconds(500));
    fast.describe();

    HttpClientConfig slow("https://report.example.com", Seconds(30).toMilliseconds());
    slow.describe();

    // 下面這些全部無法編譯，而這正是我們要的：
    // HttpClientConfig bad1("https://x.com", 30);              // ❌ 裸數字：單位不明
    // HttpClientConfig bad2("https://x.com", Seconds(30));     // ❌ 型別不符：秒不是毫秒
    cout << "  `HttpClientConfig(url, 30)` 無法編譯 —— 30 是秒還是毫秒？" << endl;
    cout << "  `HttpClientConfig(url, Seconds(30))` 也無法編譯 —— 型別明確不符" << endl;
    cout << "  必須寫 Seconds(30).toMilliseconds()，轉換意圖一目了然" << endl;
    cout << "  ↑ 這正是 std::chrono 的設計哲學：讓單位錯誤成為編譯錯誤" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 15 課：帶參數的建構函數10.cpp" -o demo10
// 註：explicit 用於單參數建構函式 C++98 即可；多參數需 C++11。

// === 預期輸出 ===
// === 明確建構：全部合法 ===
//   建構 SafeDistance: 100 m
//   距離: 100 公尺
//   建構 SafeDistance: 50 m
//   距離: 50 公尺
//   建構 SafeDistance: 200 m
//   距離: 200 公尺
//   建構 SafeDistance: 300 m
//   距離: 300 公尺
//
// === explicit 擋掉的寫法（已註解）===
//   SafeDistance d = 50.0;   → 編譯錯誤
//   showDistance(200.0);      → 編譯錯誤
//   vector<SafeDistance> v = {1.0, 2.0};  → 編譯錯誤
//   ↑ explicit 不是禁止轉換，而是要求你把轉換「寫出來」
//
// === ⚠️ 沒有 explicit 的危險 ===
//   `UnsafeDistance u = 0.15;` 靜默成立，建出 0.15 公尺
//   ↑ 型別錯誤完全沒有被攔截 —— 這正是 explicit 要防的事
//
// === 編譯期驗證 ===
//   is_convertible<double, SafeDistance>   = false  （explicit 擋下）
//   is_convertible<double, UnsafeDistance> = true   （後門敞開）
//   is_constructible<SafeDistance, double> = true   （明確建構仍可行）
//
// === 實務：強型別時間單位 ===
//     https://api.example.com timeout=500 ms
//     https://report.example.com timeout=30000 ms
//   `HttpClientConfig(url, 30)` 無法編譯 —— 30 是秒還是毫秒？
//   `HttpClientConfig(url, Seconds(30))` 也無法編譯 —— 型別明確不符
//   必須寫 Seconds(30).toMilliseconds()，轉換意圖一目了然
//   ↑ 這正是 std::chrono 的設計哲學：讓單位錯誤成為編譯錯誤
