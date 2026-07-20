/*=============================================================================
 * 檔名：11_StaticMember.cpp
 * 主題：static 成員變數 與 static 成員函式
 * 適合：已會 class，想知道「不屬於單一物件、整個類別共用」的資料怎麼處理
 *
 * 【課題介紹】
 *   一般成員是「每個物件各有一份」，但有些東西很自然「整個類別只該有一份」：
 *
 *     - 已建立過幾個物件 (instance counter)
 *     - 全班共用的設定 (config / 常數)
 *     - 專案層級的 logger / id 產生器
 *
 *   這時候就該用 static (靜態) 成員。
 *
 *       「static 成員是『類別層級』的成員，不屬於任何單一物件，
 *        所有物件共用同一份。」
 *
 * 【static 成員變數】
 *   - 在 class 內「宣告」，但通常需要在 class 外「定義」(配置實際儲存空間)。
 *   - C++17 之後可在 class 內加 inline 直接連定義一起寫，更方便。
 *   - 存取方式有兩種：
 *         ClassName::var       (推薦，能看出是類別層級)
 *         object.var           (合法，但容易誤解為物件成員)
 *
 *   範例：
 *       class Foo {
 *       public:
 *           static int count;            // 宣告
 *       };
 *       int Foo::count = 0;              // 在 class 外定義 (傳統寫法)
 *
 *   或 C++17：
 *       class Foo {
 *       public:
 *           inline static int count = 0; // 宣告 + 定義 同時搞定
 *       };
 *
 * 【static 成員函式】
 *   - 屬於整個類別，呼叫時不需要物件。
 *   - 沒有 this 指標。
 *   - 因此「不能存取非 static 成員」 (因為不知道是哪個物件的)。
 *   - 常用於：工廠函式 (factory)、輔助計算、操作 static 成員。
 *
 * 【static 與 const 容易混淆的地方】
 *   - 一個常見場景：類別內部要放一個常數
 *       class Circle { static constexpr double PI = 3.14159; };
 *     這樣 PI 是「整個類別共用、編譯期就決定」的常數，不會佔每個物件記憶體。
 *
 * 【對應 Leetcode】1396. Design Underground System
 *   題目簡述：地鐵打卡系統，要設計三個函式
 *     - checkIn (id, station, time)
 *     - checkOut(id, station, time)
 *     - getAverageTime(start, end) → 從 start 站到 end 站的平均時間
 *   為什麼選這題：
 *     真實情境很適合用一個「全域單例 (整個系統只有一個)」的設計，
 *     而 static 成員就是單例模式 (Singleton) 的基礎 (第 26 篇會深入)。
 *     本篇讓你看見「累計總時間 / 累計趟數 → 求平均」的小設計。
 *
 * 【參考】
 *   https://en.cppreference.com/w/cpp/language/static
 *=============================================================================*/

/*
補充筆記：StaticMember
  - StaticMember 這類 OOP 範例要追蹤物件狀態：建構後是否有效、操作後是否仍符合類別承諾。
  - 如果類別擁有資源，就要檢查 destructor、copy、move 是否表達同一套所有權規則。
  - 繼承、friend、static、operator overload 都應服務於清楚的物件語意，而不是只展示語法。
  - static data member 屬於類別本身，不屬於任何單一物件；所有物件看到的是同一份資料。
  - static member function 沒有 this，因此不能直接讀寫一般成員；它適合放和類別相關、但不依賴特定物件狀態的操作。
  - C++17 以前非 const static data member 通常需要在 class 外提供一次定義；C++17 inline static 讓定義可放在 class 內。
  - static 成員常用於計數器、全域設定、工廠註冊表，但它本質上是共享狀態，測試和多執行緒都要小心。
  - 在多執行緒環境修改 static 資料需要同步；否則多個 thread 同時讀寫會形成 data race。
  - function local static 在第一次執行到該行時初始化；C++11 起初始化本身是 thread-safe，常用於 lazy singleton。
  - 不要把 static 當成逃避物件設計的工具；若狀態其實屬於某個物件，就應該放在物件內，否則依賴關係會變隱形。
  - 命名 static 成員時可用 ClassName::member 明確指出它是類別層級資料，避免讀者誤以為每個物件各有一份。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】static 成員
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. static 成員變數與 static 成員函式各有什麼特性？
//     答：static 資料成員屬於類別本身，所有物件共用同一份；static 成員函式沒有 this，
//     因此不能直接存取非 static 成員，呼叫時也不需要任何物件。
//     追問：static 成員函式可以是 virtual 嗎？（不行 — virtual 分派要從物件取 vptr，
//     沒有 this 就無從分派，兩者語意互斥，編譯錯誤）
//
// 🔥 Q2. static 成員變數要在哪裡定義？
//     答：C++17 之前，非 const 的 static 資料成員必須在 class 外提供一次定義
//     （`int Foo::count = 0;`），只宣告不定義會在連結期得到 undefined reference。
//     C++17 起可寫 `inline static int count = 0;` 在 class 內一次完成；
//     `static constexpr` 資料成員自 C++17 起也隱含 inline。
//
// Q3. function-local static 的初始化是執行緒安全的嗎？
//     答：是。C++11 起標準保證 function-local static 的初始化只會執行一次且是
//     thread-safe（俗稱 magic static），這正是 Meyers Singleton 的基礎；
//     C++11 之前才需要 double-checked locking 那套（而且當年還做不對）。
//     追問：那 static 成員本身也安全嗎？（只保證「初始化」；之後的讀寫仍是共享狀態，
//     多執行緒修改要自己同步，否則就是 data race）
//
// ⚠️ 陷阱. 不同 translation unit 的非區域 static 物件，初始化順序是什麼？
//     答：未定義 — 這就是 static initialization order fiasco。跨 TU 的非區域 static
//     物件之間沒有任何順序保證，若 A 的建構子用到 B，可能讀到還沒初始化的 B。
//     為什麼會錯：以為「檔案順序 / 連結順序」決定初始化順序。
//     解法是改用 function-local static（初次被使用時才建構，順序自然正確）。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>     // std::pair

// -----------------------------------------------------------------------------
// 範例 1：用 static 計算「目前還活著的物件數量」
// -----------------------------------------------------------------------------
class Widget {
public:
    inline static int liveCount = 0;     // C++17 行內靜態：宣告兼定義

    Widget()                  { ++liveCount; }
    Widget(const Widget&)     { ++liveCount; }   // 別忘了複製建構也要算
    ~Widget()                 { --liveCount; }

    // static 成員函式：呼叫時不需要物件，也不能存取非 static 成員
    static int howMany() { return liveCount; }
};

// 若改用 C++11/14：可以這樣寫 (兩段)
// class Widget { public: static int liveCount; ... };
// int Widget::liveCount = 0;

// -----------------------------------------------------------------------------
// 範例 2：對應 Leetcode 1396 - Design Underground System
// -----------------------------------------------------------------------------
// 設計思路：
//   - 用一個 map 存「乘客 id → (進站站名, 進站時間)」
//   - 用一個 map 存「(start,end) → (累計時間總和, 趟數)」
//     getAverageTime 就回傳 累計 / 趟數
class UndergroundSystem {
private:
    // key: 乘客 id ; value: (進站站名, 進站時間)
    std::unordered_map<int, std::pair<std::string, int>> checkInMap_;

    // key: 「start#end」字串 ; value: (累計總時間, 趟數)
    // 真實程式中可以用 pair<string,string> 當 key，但要自寫 hash，這裡用字串簡化。
    std::unordered_map<std::string, std::pair<long long, int>> routeStats_;

    // static 工具函式：把兩站名組成 routeStats_ 的 key
    static std::string makeKey(const std::string& a, const std::string& b) {
        return a + "#" + b;
    }

public:
    void checkIn(int id, const std::string& stationName, int t) {
        checkInMap_[id] = {stationName, t};
    }

    void checkOut(int id, const std::string& stationName, int t) {
        auto it = checkInMap_.find(id);
        if (it == checkInMap_.end()) return;     // 沒有對應的 checkIn，略過

        const std::string& start = it->second.first;
        int                t0    = it->second.second;
        std::string key = makeKey(start, stationName);

        auto& stat = routeStats_[key];           // (sum, count)，不存在會建立 (0,0)
        stat.first  += (t - t0);
        stat.second += 1;
        checkInMap_.erase(it);                   // 對應的 check-in 用過就刪掉
    }

    double getAverageTime(const std::string& startStation,
                          const std::string& endStation) const {
        auto it = routeStats_.find(makeKey(startStation, endStation));
        if (it == routeStats_.end() || it->second.second == 0) return 0.0;
        return static_cast<double>(it->second.first) / it->second.second;
    }
};

// -----------------------------------------------------------------------------
// 範例 3：對應 Leetcode 359 - Logger Rate Limiter
// -----------------------------------------------------------------------------
// 題目簡述：設計 shouldPrintMessage(timestamp, message)；
//           同一訊息在 10 秒內只能印一次，回傳 true 表示這次可以印。
// 重點：static constexpr 常數讓「整個類別共用、不佔每個物件記憶體」。
class LoggerRL {
private:
    std::unordered_map<std::string, int> lastTime_;
    static constexpr int TIMEOUT = 10;                // 整個類別共用的常數
public:
    bool shouldPrintMessage(int timestamp, const std::string& message) {
        auto it = lastTime_.find(message);
        if (it == lastTime_.end() || timestamp - it->second >= TIMEOUT) {
            lastTime_[message] = timestamp;
            return true;
        }
        return false;
    }
};

// -----------------------------------------------------------------------------
// 範例 4：日常實用 - 用 static 寫流水號產生器
// -----------------------------------------------------------------------------
class IdGenerator {
private:
    inline static int next_ = 1000;       // C++17 inline static：宣告兼定義
public:
    static int nextId() { return next_++; }
};

int main() {
    std::cout << "----- 範例 1：Widget 計數 -----" << std::endl;
    std::cout << "目前 Widget 個數 = " << Widget::howMany() << std::endl;   // 0
    {
        Widget a, b, c;
        std::cout << "建了三個後 = " << Widget::liveCount << std::endl;     // 3
        Widget d = a;                                                         // 複製
        std::cout << "再複製一個 = " << Widget::howMany() << std::endl;      // 4
    }
    std::cout << "離開區塊後 = " << Widget::howMany() << std::endl;          // 0

    std::cout << "----- 範例 2：Leetcode 1396 -----" << std::endl;
    UndergroundSystem us;
    // 一段時間內三個人在三個站打卡
    us.checkIn(45, "Leyton",        3);
    us.checkIn(32, "Paradise",     8);
    us.checkIn(27, "Leyton",       10);
    us.checkOut(45, "Waterloo",    15);   // Leyton → Waterloo 用 12
    us.checkOut(27, "Waterloo",    20);   // Leyton → Waterloo 用 10
    us.checkOut(32, "Cambridge",   22);   // Paradise → Cambridge 用 14

    std::cout << "Paradise → Cambridge 平均: "
              << us.getAverageTime("Paradise", "Cambridge") << std::endl;   // 14
    std::cout << "Leyton → Waterloo 平均: "
              << us.getAverageTime("Leyton", "Waterloo") << std::endl;      // 11

    std::cout << "----- 範例 3：Leetcode 359 Logger Rate Limiter -----" << std::endl;
    LoggerRL lg;
    std::cout << lg.shouldPrintMessage(1,  "foo") << std::endl;   // 1
    std::cout << lg.shouldPrintMessage(2,  "bar") << std::endl;   // 1
    std::cout << lg.shouldPrintMessage(3,  "foo") << std::endl;   // 0 (太頻繁)
    std::cout << lg.shouldPrintMessage(8,  "bar") << std::endl;   // 0
    std::cout << lg.shouldPrintMessage(11, "foo") << std::endl;   // 1 (10s 後可印)

    std::cout << "----- 範例 4：日常實用 - IdGenerator -----" << std::endl;
    std::cout << "id1 = " << IdGenerator::nextId() << std::endl;   // 1000
    std::cout << "id2 = " << IdGenerator::nextId() << std::endl;   // 1001
    std::cout << "id3 = " << IdGenerator::nextId() << std::endl;   // 1002
    return 0;
}

/* 預期輸出：
 * ----- 範例 1：Widget 計數 -----
 * 目前 Widget 個數 = 0
 * 建了三個後 = 3
 * 再複製一個 = 4
 * 離開區塊後 = 0
 * ----- 範例 2：Leetcode 1396 -----
 * Paradise → Cambridge 平均: 14
 * Leyton → Waterloo 平均: 11
 * ----- 範例 3：Leetcode 359 Logger Rate Limiter -----
 * 1
 * 1
 * 0
 * 0
 * 1
 * ----- 範例 4：日常實用 - IdGenerator -----
 * id1 = 1000
 * id2 = 1001
 * id3 = 1002
 */

/*=============================================================================
 * 【本篇重點回顧】
 *   1. static 成員變數是「整個類別共用」，不屬於某個物件，存取用 ClassName::name。
 *   2. C++17 起可以用 inline static 在 class 內一次完成宣告 + 定義。
 *   3. static 成員函式沒有 this 指標，無法直接存取非 static 成員。
 *   4. static 常見用途：實例計數、id 產生器、工具函式、單例的內部資料表。
 *
 * 【下一篇預告】
 *   12_Friend.cpp
 *   friend 朋友函式 / 朋友類別 — 怎麼讓「外人」看到你的 private 成員？
 *   為什麼 operator<< 重載常常需要 friend？
 *=============================================================================*/

// 編譯: g++ -std=c++20 -Wall -Wextra 11_StaticMember.cpp -o 11_StaticMember

// === 預期輸出 ===
// ----- 範例 1：Widget 計數 -----
// 目前 Widget 個數 = 0
// 建了三個後 = 3
// 再複製一個 = 4
// 離開區塊後 = 0
// ----- 範例 2：Leetcode 1396 -----
// Paradise → Cambridge 平均: 14
// Leyton → Waterloo 平均: 11
// ----- 範例 3：Leetcode 359 Logger Rate Limiter -----
// 1
// 1
// 0
// 0
// 1
// ----- 範例 4：日常實用 - IdGenerator -----
// id1 = 1000
// id2 = 1001
// id3 = 1002
