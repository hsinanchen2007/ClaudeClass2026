// =============================================================================
//  第 14 課：預設建構函數（Default Constructor）  —  總複習
// =============================================================================
//
// 【主題資訊 Information】
//   定義：「不需要提供任何引數就能呼叫的建構函式」——注意不是「參數列為空」
//   三種形式：
//       X() { ... }                       // 自己寫（user-provided）
//       X() = default;                    // 要編譯器的預設版本（非 user-provided）
//       X(int a = 0, int b = 0) { ... }   // 全部參數都有預設值，同樣算
//   標準版本：`= default` / `= delete` / NSDMI 皆為 C++11；其餘 C++98 起即有
//   標頭檔：<string>、<vector>、<type_traits>
//
// 【詳細解釋 Explanation】
//
// 【1. 編譯器什麼時候會幫你生成？生成的版本做了什麼？】
//   只在「使用者一個建構函式都沒宣告」時才隱式宣告。它的行為是：
//     * class type 的成員 → 呼叫它們各自的預設建構函式（所以 std::string 安全）
//     * 內建型別的成員   → 什麼都不做，值不確定（讀取是 UB）
//   這個「一半安全一半危險」的行為源自 zero-overhead 原則：
//   語言不為你沒要求的清零付出代價。
//
// 【2. 它為什麼會「消失」，以及三種補救】
//   一旦你宣告了任何建構函式（連純宣告都算），隱式的預設建構函式就不再存在。
//   這是刻意的：你已經表明「建立這個物件需要資料」，語言不該留後門。
//   補救方式：
//     (a) 手寫無參版本 —— 可自訂初值，但別寫成空的 `X() {}`（見下方陷阱）
//     (b) `X() = default;` —— 要編譯器原本的行為，建議搭配 NSDMI
//     (c) 讓帶參版本的所有參數都有預設值 —— 但不可再另寫 `X()`（會歧義）
//
// 【3. 本課最核心的區別：user-provided vs user-declared】
//       X() = default;   → user-declared，但「不是」user-provided
//       X() { }          → user-declared，「而且是」user-provided
//   這個區別決定了值初始化的行為：
//       A a{};   // A() = default → 先零初始化整個物件 → 成員保證為 0
//       B b{};   // B() {}        → 直接呼叫空建構函式 → 成員值不確定
//   所以「手寫一個空的建構函式」其實比 `= default` 更危險。
//   `= default` 另外還保留 trivial 性質（可 memcpy、type traits 為 true）。
//
// 【4. `= delete`：把「不該存在」寫進程式碼】
//   當「無參建立」在語意上不成立時（資料庫連線、已驗證的 Session、檔案 handle），
//   應該明確 `X() = delete;` 而不是靠「只寫帶參版本」讓它隱式消失。
//   前者的錯誤訊息是明確的 "use of deleted function"，
//   也讓維護者知道這是設計決策而非漏寫。
//   附帶妙用：deleted 函式仍參與多載解析，可用 `void f(char) = delete;`
//   精準攔截「型別對但語意錯」的呼叫。
//
// 【5. 誰需要預設建構函式？】
//   需要：`T arr[N]`、`std::array<T,N>`、`vector<T> v(n)`、`v.resize(n)`、
//         `map<K,T>::operator[]`
//   不需要：`vector<T> v;`（空容器）、`v.reserve(n)`（只配置記憶體）、
//           `push_back` / `emplace_back`（明確提供了建構方式）
//   reserve 與 resize 的差別是常見考點。
//
// 【概念補充 Concept Deep Dive】
//   * 真正可靠的初值保證只有 NSDMI 與初始化列表 —— 它們屬於類別作者。
//     「值初始化的零初始化」要靠呼叫端寫那對大括號，而且只在
//     預設建構函式非 user-provided 時成立，兩個前提都可能被破壞。
//   * `= default` 若寫在類別外（`X::X() = default;`）會變成 user-provided，
//     失去 trivial 性質與零初始化行為 —— 一律寫在類別內。
//   * 若某成員無法預設建構（const 成員無 NSDMI、參考成員），
//     `X() = default;` 的結果會是 deleted，編譯器不報錯，
//     直到你真的建立物件時才失敗。
//
// 【注意事項 Pay Attention】
//   1. 別手寫空的建構函式 `X() {}` —— 用 `= default` 取代，永遠更好。
//   2. `= default` 不會初始化內建型別成員；要保證初值請搭配 NSDMI。
//   3. 全預設參數的建構函式不可與無參建構函式並存（`X x;` 會歧義）。
//   4. 讀取未初始化的成員是 UB，觀察到的任何值都不可推廣。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】預設建構函數
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 什麼是預設建構函式？編譯器什麼時候會自動生成？
//     答：能「不提供任何引數就被呼叫」的建構函式，包含無參版本與
//         全部參數都有預設值的版本。編譯器只在「使用者一個建構函式都沒宣告」
//         時才隱式宣告它；生成的版本會呼叫 class type 成員的預設建構函式，
//         但對內建型別什麼都不做。
//     追問：那 std::string 成員為什麼是空字串？→ 它是 class type，
//           有自己的預設建構函式，一定會被呼叫。安全的是它，不是整個類別。
//
// 🔥 Q2. `X() = default;` 和 `X() {}` 差在哪？（高頻）
//     答：三點：① 前者非 user-provided、後者是；
//         ② 前者保留 trivial 性質，後者失去；
//         ③ 值初始化 `X x{}` 對前者會先零初始化整個物件，對後者不會。
//         實務結論：需要「什麼都不做的預設建構函式」時一律用 `= default`。
//     追問：那要保證成員為 0 該怎麼做？→ 用 NSDMI。
//           它是類別作者的保證，不依賴呼叫端寫不寫大括號。
//
// 🔥 Q3. 哪些用法需要 T 可預設建構？
//     答：`T arr[N]`、`std::array<T,N>`、`vector<T> v(n)`、`v.resize(n)`、
//         `map<K,T>::operator[]`。
//         不需要的有：空 vector、`reserve(n)`、`push_back`、`emplace_back`。
//     追問：reserve 和 resize 為什麼不同？→ reserve 只配置記憶體、不建構元素；
//           resize 要讓 size() 等於 n，擴充時必須真的建構出新元素。
//
// ⚠️ 陷阱. 下面的程式印出什麼？
//         struct A { int v; A() = default; };
//         struct B { int v; B() {}         };
//         A a{};  B b{};  cout << a.v << " " << b.v;
//     答：a.v 保證是 0；b.v 的值不確定，讀取它是 UB —— 可能印出 0，
//         也可能印出任何值，換個編譯選項或執行環境就會變。
//     為什麼會錯：以為「兩個建構函式本體都是空的，行為當然一樣」。
//         實際上關鍵不在本體寫了什麼，而在「它是不是 user-provided」——
//         這個性質決定了值初始化要不要先零初始化整個物件。
//         在特殊成員函式上，「怎麼宣告的」比「本體寫什麼」更重要。
// ═══════════════════════════════════════════════════════════════════════════

/*
 * ================================================================
 * 【第 14 課：預設建構函數（Default Constructor）】總複習 summary.cpp
 * ================================================================
 * 編譯方式：g++ -std=c++17 -o summary summary.cpp
 *
 * 本課重點：
 * 1. 預設建構函數的定義：不需要傳入任何參數就能調用的建構函數
 * 2. 編譯器自動生成的預設建構函數及其行為
 * 3. 預設建構函數消失的情況（定義了任何建構函數後）
 * 4. 解決方案：手動加回 vs = default（C++11）
 * 5. = default 與手動寫空建構函數的微妙差異（值初始化 / 平凡性）
 * 6. 帶預設參數的建構函數也算預設建構函數（及歧義陷阱）
 * 7. 預設建構函數與對象陣列的關係
 * 8. = delete：明確禁止預設建構（C++11）
 * ================================================================
 */

#include <iostream>
#include <string>
#include <vector>
#include <map>
using namespace std;

// ================================================================
// 重點一：什麼是預設建構函數？
// ================================================================
// 預設建構函數 = 不需要傳入任何參數就能調用的建構函數
// 它有兩種形式：
//   形式 1：MyClass() { }                    // 無參數
//   形式 2：MyClass(int x = 0) { }          // 所有參數都有預設值
// 只要可以用 MyClass obj; 不帶任何參數創建對象，那就是預設建構函數。

// ================================================================
// 重點二：編譯器自動生成的預設建構函數
// ================================================================
// 如果完全沒有定義任何建構函數，編譯器會自動生成一個預設建構函數。
//
// ┌────────────────────────────────────┬────────────────────────┐
// │ 成員類型                           │ 自動生成的建構函數行為  │
// ├────────────────────────────────────┼────────────────────────┤
// │ 基本型別（int, double, float 等）  │ 不做任何初始化（垃圾值）│
// │ 類別型別（string, vector 等）      │ 調用該類別的預設建構函數│
// │ 陣列                               │ 按照上述規則處理每個元素│
// └────────────────────────────────────┴────────────────────────┘
// 符合 C++ 的「零開銷原則」：你沒要求初始化，就不花時間初始化。

class AutoGenerated {
public:
    int number;        // 基本型別 → 不會初始化（垃圾值）
    double value;      // 基本型別 → 不會初始化（垃圾值）
    string text;       // 類別型別 → 調用 string 的預設建構函數 → 空字串

    // 沒有定義任何建構函數 → 編譯器自動生成預設建構函數

    void print() const {
        cout << "  number = " << number << " (可能是垃圾值)" << endl;
        cout << "  value  = " << value << " (可能是垃圾值)" << endl;
        cout << "  text   = [" << text << "] (空字串)" << endl;
    }
};

// ================================================================
// 重點三：預設建構函數消失的情況
// ================================================================
// 規則：只要定義了「任何一個」建構函數（無論帶參還是不帶參），
//       編譯器就「不再自動生成」預設建構函數。
//
// 原因：編譯器的邏輯是——你既然自己定義了建構函數，
//       說明你有特殊的初始化需求，不應該擅自生成一個可能不符合意圖的預設建構函數。

class PointNoDefault {
public:
    int x, y;

    // 只定義了帶參數的建構函數
    PointNoDefault(int px, int py) {
        x = px;
        y = py;
    }
    // Point p;  → 編譯錯誤！沒有預設建構函數！
};

// ================================================================
// 重點四：解決方案
// ================================================================

// 方式一：手動寫一個無參數建構函數
class PointManual {
public:
    int x, y;

    PointManual() {
        x = 0;
        y = 0;
        cout << "  手動預設建構: Point(0, 0)" << endl;
    }

    PointManual(int px, int py) {
        x = px;
        y = py;
        cout << "  帶參建構: Point(" << x << ", " << y << ")" << endl;
    }
};

// 方式二（C++11）：使用 = default
class PointDefault {
public:
    int x, y;

    // 告訴編譯器：請幫我生成預設建構函數
    // 注意：x 和 y 仍然是未初始化的垃圾值（和自動生成的行為一樣）
    PointDefault() = default;

    PointDefault(int px, int py) {
        x = px;
        y = py;
    }
};

// ================================================================
// 重點五：= default 與手動寫空建構函數的微妙差異
// ================================================================
// 這是一個微妙但重要的區別，影響「值初始化」行為：
//
// ┌────────────────────┬───────────────────┬────────────────────┐
// │ 場景                │ = default          │ 手動寫 { }          │
// ├────────────────────┼───────────────────┼────────────────────┤
// │ A a; / B b;         │ 基本型別不初始化   │ 基本型別不初始化     │
// │ A a{}; / B b{};     │ 觸發值初始化，歸零 │ 只調用空建構，不歸零 │
// │ 語義                │ 平凡(trivial)建構  │ 使用者定義的建構     │
// └────────────────────┴───────────────────┴────────────────────┘
//
// = default 保留了「平凡性（triviality）」，
// 這讓類別可以享受某些編譯器優化（如 memcpy 優化、POD 類型判斷）。

class A_Default {
public:
    int value;
    A_Default() = default;  // 平凡建構函數
};

class B_UserDefined {
public:
    int value;
    B_UserDefined() { }     // 使用者定義的空建構函數
};

// ================================================================
// 重點六：帶預設參數的建構函數也是預設建構函數
// ================================================================
// 所有參數都有預設值 → 可以不帶參數調用 → 算預設建構函數
//
// 注意：不能同時有無參建構函數和全預設參數建構函數，否則會歧義！
//   class Bad {
//       Bad() { }              // 建構函數 A
//       Bad(int x = 0) { }    // 建構函數 B（也是預設建構函數）
//   };
//   Bad b;  // 編譯錯誤！歧義：應該調用 A 還是 B？

class Color {
private:
    int r, g, b;

public:
    // 所有參數都有預設值 → 這也是預設建構函數！
    Color(int red = 0, int green = 0, int blue = 0) {
        r = (red >= 0 && red <= 255) ? red : 0;
        g = (green >= 0 && green <= 255) ? green : 0;
        b = (blue >= 0 && blue <= 255) ? blue : 0;
    }

    void print() const {
        cout << "  RGB(" << r << ", " << g << ", " << b << ")" << endl;
    }
};

// ================================================================
// 重點七：預設建構函數與對象陣列
// ================================================================
// Enemy enemies[5]; → 要一次創建 5 個對象，
// 但語法上沒有位置讓你傳參數，所以每個元素都必須用預設建構函數初始化。
//
// 如果沒有預設建構函數：
//   NoDefault arr[3];  // 編譯錯誤！
//   解決方法：NoDefault arr[3] = { NoDefault(1), NoDefault(2), NoDefault(3) };

class Enemy {
private:
    string type;
    int health;

public:
    // 必須有預設建構函數，否則無法創建陣列
    Enemy() {
        type = "小怪";
        health = 100;
    }

    Enemy(string t, int hp) {
        type = t;
        health = hp;
    }

    void print() const {
        cout << "  " << type << " (HP: " << health << ")" << endl;
    }

    void setType(string t) { type = t; }
    void setHealth(int hp) { health = hp; }
};

// ================================================================
// 重點八：= delete 明確禁止預設建構（C++11）
// ================================================================
// 當對象在沒有特定資訊的情況下不應該存在時，
// 用 = delete 表達設計意圖。
// 比「忘記定義預設建構函數」更明確——
// 告訴閱讀程式碼的人：「我故意不允許預設建構。」

class DatabaseConnection {
private:
    string host;
    int port;

public:
    DatabaseConnection() = delete;  // 沒有連接資訊就不能創建

    DatabaseConnection(string h, int p) {
        host = h;
        port = p;
        cout << "  連接到 " << host << ":" << port << endl;
    }

    void print() const {
        cout << "  資料庫連接: " << host << ":" << port << endl;
    }
};

// ================================================================
// 綜合範例類別
// ================================================================

// 範例 1：使用 = default
class Config {
public:
    int width;
    int height;
    bool fullscreen;

    Config() = default;  // 請編譯器生成預設建構函數

    Config(int w, int h, bool fs)
        : width(w), height(h), fullscreen(fs) { }

    void print() const {
        cout << "  " << width << "x" << height
             << (fullscreen ? " [全螢幕]" : " [視窗]") << endl;
    }
};

// 範例 2：全預設參數的建構函數
class Timer {
private:
    int hours;
    int minutes;
    int seconds;

public:
    Timer(int h = 0, int m = 0, int s = 0) {
        hours = (h >= 0 && h < 24) ? h : 0;
        minutes = (m >= 0 && m < 60) ? m : 0;
        seconds = (s >= 0 && s < 60) ? s : 0;
    }

    void print() const {
        cout << "  ";
        if (hours < 10) cout << "0";
        cout << hours << ":";
        if (minutes < 10) cout << "0";
        cout << minutes << ":";
        if (seconds < 10) cout << "0";
        cout << seconds << endl;
    }
};

// 範例 3：= delete 禁止預設建構
class FileHandle {
private:
    string filename;
    bool isOpen;

public:
    FileHandle() = delete;  // 沒有檔名就不能創建

    FileHandle(string fname) {
        filename = fname;
        isOpen = true;
        cout << "  打開檔案: " << filename << endl;
    }

    void print() const {
        cout << "  檔案: " << filename
             << " [" << (isOpen ? "開啟" : "關閉") << "]" << endl;
    }
};

// =============================================================================
// 【LeetCode 實戰範例】LeetCode 705. Design HashSet
//   題目：不使用內建雜湊表，自行設計一個 HashSet，支援 add / remove / contains。
//   為什麼用到本主題：這題的骨架正是本課的核心 ——
//     * 建構子必須把「所有桶子」預先配置好，這需要元素型別可預設建構
//       （這裡用 vector<vector<int>>，內層 vector 的預設狀態就是空桶）
//     * 「空桶」是一個有意義的預設值：代表「這個桶還沒有任何元素」，
//       而不是不確定的垃圾狀態 —— 正是本課第 7 檔強調的重點
//     * 若把桶子換成一個沒有預設建構函式的型別，`vector<Bucket> b(N);`
//       這行就會編譯失敗，必須改用 emplace_back 逐一建構
//   複雜度：平均 O(1)；最壞 O(n/BUCKETS)（同一桶內線性搜尋）。
// =============================================================================
class MyHashSet {
public:
    // 建構子在此建立不變條件：永遠有 BUCKETS 個桶，每個桶都是合法的空 vector
    MyHashSet() : m_buckets(BUCKETS) {}
    //             ↑ vector<vector<int>> b(769) 要求內層型別可預設建構；
    //               std::vector 的預設狀態「空容器」正是我們要的有意義預設值

    void add(int key) {
        vector<int>& bucket = m_buckets[hash(key)];
        for (int v : bucket) if (v == key) return;   // 已存在就不重複加
        bucket.push_back(key);
    }

    void remove(int key) {
        vector<int>& bucket = m_buckets[hash(key)];
        for (size_t i = 0; i < bucket.size(); ++i) {
            if (bucket[i] == key) {
                bucket[i] = bucket.back();           // 與最後一個交換再 pop，O(1)
                bucket.pop_back();
                return;
            }
        }
    }

    bool contains(int key) const {
        const vector<int>& bucket = m_buckets[hash(key)];
        for (int v : bucket) if (v == key) return true;
        return false;
    }

private:
    // 用質數當桶數可減少雜湊碰撞
    static const size_t BUCKETS = 769;
    static size_t hash(int key) { return static_cast<size_t>(key) % BUCKETS; }

    vector<vector<int>> m_buckets;
};

// =============================================================================
// 【日常實務範例】功能開關（feature flag）表
//   情境：服務啟動時載入一份功能開關設定。實務上這裡有個經典陷阱 ——
//   查詢一個「設定檔裡沒有提到的開關」時，應該回傳什麼？
//     * 若用 `map<string,bool>::operator[]`，找不到 key 時會「自動插入一筆」
//       並用預設建構函式建出 false —— 這正是本課「map operator[] 需要
//       預設建構函式」的實際後果。它有兩個副作用：
//         (1) 悄悄改變了 map 的內容（在 const 情境下甚至無法編譯）
//         (2) 讓「沒設定」與「明確設為 false」變得無法區分
//     * 正解是用 find()（唯讀、不插入），並讓呼叫端明確指定 fallback。
//   這是「有意義的預設值」與「誠實表達『沒有值』」之間的取捨，
//   也是本課第 8 檔 optional 主題的延伸。
// =============================================================================
class FeatureFlags {
public:
    FeatureFlags() = default;          // 空的開關表是合法的初始狀態

    void set(const std::string& name, bool enabled) {
        m_flags[name] = enabled;
    }

    // 唯讀查詢：用 find 而非 operator[]，不會意外插入資料
    bool isEnabled(const std::string& name, bool fallback = false) const {
        auto it = m_flags.find(name);
        return (it == m_flags.end()) ? fallback : it->second;
    }

    // 誠實回報「這個開關到底有沒有被設定過」
    bool isConfigured(const std::string& name) const {
        return m_flags.find(name) != m_flags.end();
    }

    size_t size() const { return m_flags.size(); }

private:
    std::map<std::string, bool> m_flags;
};

int main() {
    cout << "=================================================" << endl;
    cout << "   第 14 課：預設建構函數（Default Constructor）" << endl;
    cout << "=================================================" << endl;

    // --- 重點二：編譯器自動生成的預設建構函數 ---
    cout << "\n【1】編譯器自動生成的預設建構函數" << endl;
    AutoGenerated obj;
    obj.print();

    // --- 重點四：手動加回預設建構函數 ---
    cout << "\n【2】手動加回預設建構函數" << endl;
    PointManual pm1;           // 調用手動寫的預設建構函數
    PointManual pm2(3, 4);     // 調用帶參建構函數

    // --- 重點四：= default ---
    cout << "\n【3】使用 = default" << endl;
    PointDefault pd1;          // 使用編譯器生成的預設建構函數
    PointDefault pd2(5, 6);
    cout << "  pd1: x=" << pd1.x << ", y=" << pd1.y << " (可能是垃圾值)" << endl;
    cout << "  pd2: x=" << pd2.x << ", y=" << pd2.y << " (正確值)" << endl;

    // --- 重點五：= default 與手動空建構的差異 ---
    cout << "\n【4】= default vs 手動空建構（值初始化差異）" << endl;
    A_Default a1;       // value 是垃圾值
    B_UserDefined b1;   // value 也是垃圾值

    A_Default a2{};     // 值初始化：value 被歸零為 0！
    B_UserDefined b2{}; // 只調用空建構，不歸零

    cout << "  一般宣告：" << endl;
    cout << "    A_Default   a1.value = " << a1.value << " (垃圾值)" << endl;
    cout << "    B_UserDef   b1.value = " << b1.value << " (垃圾值)" << endl;
    cout << "  值初始化 {}：" << endl;
    cout << "    A_Default   a2.value = " << a2.value << " (應該是 0)" << endl;
    cout << "    B_UserDef   b2.value = " << b2.value << " (可能不是 0)" << endl;

    // --- 重點六：帶預設參數的建構函數 ---
    cout << "\n【5】帶預設參數的建構函數也是預設建構函數" << endl;
    Color c1;               // 全部使用預設值 → (0, 0, 0)
    Color c2(255);           // green 和 blue 用預設值 → (255, 0, 0)
    Color c3(0, 128);        // blue 用預設值 → (0, 128, 0)
    Color c4(100, 200, 50);  // 全部指定
    cout << "c1: "; c1.print();   // 黑色
    cout << "c2: "; c2.print();   // 紅色
    cout << "c3: "; c3.print();   // 綠色
    cout << "c4: "; c4.print();   // 自定義

    // --- 重點七：預設建構函數與對象陣列 ---
    cout << "\n【6】預設建構函數與對象陣列" << endl;
    Enemy enemies[5];  // 5 個 Enemy，每個都調用預設建構函數
    for (int i = 0; i < 5; i++) {
        enemies[i].print();
    }
    cout << "  修改後：" << endl;
    enemies[0].setType("Boss");
    enemies[0].setHealth(5000);
    enemies[0].print();

    // --- 重點八：= delete ---
    cout << "\n【7】= delete 禁止預設建構" << endl;
    // DatabaseConnection db;  // 編譯錯誤！預設建構函數被 delete
    DatabaseConnection db("localhost", 5432);
    db.print();

    // --- 綜合範例 ---
    cout << "\n【8】綜合範例" << endl;

    cout << "Config (= default):" << endl;
    Config cfg1{};                       // 值初始化：全部歸零
    Config cfg2(1920, 1080, true);       // 帶參建構
    cout << "cfg1: "; cfg1.print();
    cout << "cfg2: "; cfg2.print();

    cout << "Timer (全預設參數):" << endl;
    Timer t1;                // 全部預設 → 00:00:00
    Timer t2(14);            // 只給小時 → 14:00:00
    Timer t3(8, 30);         // 給小時和分鐘 → 08:30:00
    Timer t4(23, 59, 59);   // 全部指定
    cout << "t1: "; t1.print();
    cout << "t2: "; t2.print();
    cout << "t3: "; t3.print();
    cout << "t4: "; t4.print();

    cout << "FileHandle (= delete):" << endl;
    // FileHandle fh;  // 編譯錯誤！
    FileHandle f1("data.txt");
    FileHandle f2("config.json");
    f1.print();
    f2.print();

    // --- LeetCode 705. Design HashSet ---
    cout << "\n=== LeetCode 705. Design HashSet ===" << endl;
    MyHashSet hs;
    hs.add(1);
    hs.add(2);
    cout << "  add(1), add(2)" << endl;
    cout << "  contains(1) = " << (hs.contains(1) ? "true" : "false") << "（預期 true）" << endl;
    cout << "  contains(3) = " << (hs.contains(3) ? "true" : "false") << "（預期 false）" << endl;
    hs.add(2);
    cout << "  add(2) 重複加入後 contains(2) = " << (hs.contains(2) ? "true" : "false") << endl;
    hs.remove(2);
    cout << "  remove(2) 之後 contains(2) = " << (hs.contains(2) ? "true" : "false")
         << "（預期 false）" << endl;
    cout << "  ↑ 建構子預先配置 769 個桶，每個桶的預設值「空 vector」" << endl;
    cout << "    是有意義的初始狀態，不是垃圾值。" << endl;

    // --- 實務：功能開關表 ---
    cout << "\n=== 實務：功能開關（feature flag）===" << endl;
    FeatureFlags flags;
    flags.set("new_checkout", true);
    flags.set("dark_mode", false);
    cout << "  設定了 " << flags.size() << " 個開關" << endl;
    cout << "  new_checkout  -> " << (flags.isEnabled("new_checkout") ? "開" : "關") << endl;
    cout << "  dark_mode     -> " << (flags.isEnabled("dark_mode") ? "開" : "關") << endl;
    cout << "  beta_search（未設定，用預設 false）-> "
         << (flags.isEnabled("beta_search") ? "開" : "關") << endl;
    cout << "  beta_search（未設定，指定 fallback=true）-> "
         << (flags.isEnabled("beta_search", true) ? "開" : "關") << endl;
    cout << "  查詢過未設定的鍵之後，開關數仍是 " << flags.size()
         << "（用 find 不會像 operator[] 那樣自動插入）" << endl;
    cout << "  dark_mode 有被明確設定嗎? "
         << (flags.isConfigured("dark_mode") ? "有" : "沒有") << endl;
    cout << "  beta_search 有被明確設定嗎? "
         << (flags.isConfigured("beta_search") ? "有" : "沒有")
         << "  ← 能區分「沒設定」與「設為 false」" << endl;

    // --- 重點回顧 ---
    cout << "\n=================================================" << endl;
    cout << "本課重點回顧：" << endl;
    cout << "  1. 預設建構函數 = 不需要參數就能調用的建構函數" << endl;
    cout << "  2. 完全沒定義建構函數時，編譯器才會自動生成" << endl;
    cout << "  3. 自動生成的不初始化基本型別，但會調用類別型別的建構函數" << endl;
    cout << "  4. = default：明確要求編譯器生成，保留平凡性(triviality)" << endl;
    cout << "  5. = delete：明確禁止預設建構函數" << endl;
    cout << "  6. 全預設參數的建構函數也算預設建構函數（注意歧義）" << endl;
    cout << "  7. 對象陣列 T arr[N]; 需要預設建構函數" << endl;
    cout << "  8. T obj{}; 配合 = default 會將基本型別歸零（值初始化）" << endl;
    cout << "=================================================" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary

// === 預期輸出 ===
// ⚠️ 標示「垃圾值 / 可能是垃圾值」的數值來自讀取未初始化成員，屬 undefined behavior。
//    下方為某一次實際執行的結果，每次執行、每台機器、每組編譯選項都可能不同
//    （本次某些欄位剛好是 0，那正是這個坑最危險的地方）。
//    標示為值初始化 {}、= default + NSDMI 的結果才是標準保證、可重現的。
// =================================================
//    第 14 課：預設建構函數（Default Constructor）
// =================================================
//
// 【1】編譯器自動生成的預設建構函數
//   number = 0 (可能是垃圾值)
//   value  = 0 (可能是垃圾值)
//   text   = [] (空字串)
//
// 【2】手動加回預設建構函數
//   手動預設建構: Point(0, 0)
//   帶參建構: Point(3, 4)
//
// 【3】使用 = default
//   pd1: x=1, y=0 (可能是垃圾值)
//   pd2: x=5, y=6 (正確值)
//
// 【4】= default vs 手動空建構（值初始化差異）
//   一般宣告：
//     A_Default   a1.value = -1 (垃圾值)
//     B_UserDef   b1.value = 1 (垃圾值)
//   值初始化 {}：
//     A_Default   a2.value = 0 (應該是 0)
//     B_UserDef   b2.value = 1 (可能不是 0)
//
// 【5】帶預設參數的建構函數也是預設建構函數
// c1:   RGB(0, 0, 0)
// c2:   RGB(255, 0, 0)
// c3:   RGB(0, 128, 0)
// c4:   RGB(100, 200, 50)
//
// 【6】預設建構函數與對象陣列
//   小怪 (HP: 100)
//   小怪 (HP: 100)
//   小怪 (HP: 100)
//   小怪 (HP: 100)
//   小怪 (HP: 100)
//   修改後：
//   Boss (HP: 5000)
//
// 【7】= delete 禁止預設建構
//   連接到 localhost:5432
//   資料庫連接: localhost:5432
//
// 【8】綜合範例
// Config (= default):
// cfg1:   0x0 [視窗]
// cfg2:   1920x1080 [全螢幕]
// Timer (全預設參數):
// t1:   00:00:00
// t2:   14:00:00
// t3:   08:30:00
// t4:   23:59:59
// FileHandle (= delete):
//   打開檔案: data.txt
//   打開檔案: config.json
//   檔案: data.txt [開啟]
//   檔案: config.json [開啟]
//
// === LeetCode 705. Design HashSet ===
//   add(1), add(2)
//   contains(1) = true（預期 true）
//   contains(3) = false（預期 false）
//   add(2) 重複加入後 contains(2) = true
//   remove(2) 之後 contains(2) = false（預期 false）
//   ↑ 建構子預先配置 769 個桶，每個桶的預設值「空 vector」
//     是有意義的初始狀態，不是垃圾值。
//
// === 實務：功能開關（feature flag）===
//   設定了 2 個開關
//   new_checkout  -> 開
//   dark_mode     -> 關
//   beta_search（未設定，用預設 false）-> 關
//   beta_search（未設定，指定 fallback=true）-> 開
//   查詢過未設定的鍵之後，開關數仍是 2（用 find 不會像 operator[] 那樣自動插入）
//   dark_mode 有被明確設定嗎? 有
//   beta_search 有被明確設定嗎? 沒有  ← 能區分「沒設定」與「設為 false」
//
// =================================================
// 本課重點回顧：
//   1. 預設建構函數 = 不需要參數就能調用的建構函數
//   2. 完全沒定義建構函數時，編譯器才會自動生成
//   3. 自動生成的不初始化基本型別，但會調用類別型別的建構函數
//   4. = default：明確要求編譯器生成，保留平凡性(triviality)
//   5. = delete：明確禁止預設建構函數
//   6. 全預設參數的建構函數也算預設建構函數（注意歧義）
//   7. 對象陣列 T arr[N]; 需要預設建構函數
//   8. T obj{}; 配合 = default 會將基本型別歸零（值初始化）
// =================================================
