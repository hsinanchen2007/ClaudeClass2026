/*=============================================================================
 * 檔名：7_InitializerList.cpp
 * 主題：建構子初始化列表 (Member Initializer List)
 * 適合：已會寫建構子，但都還在大括號內 x = 1; y = 2; 賦值的初學者
 *
 * 【課題介紹】
 *   你已經學過建構子，可能寫過這樣的版本：
 *
 *       Point(double x, double y) {
 *           x_ = x;
 *           y_ = y;
 *       }
 *
 *   它能跑，但不夠好。C++ 提供另一種更專業的寫法：
 *
 *       Point(double x, double y) : x_(x), y_(y) {}
 *                                ↑↑↑↑↑↑↑↑↑↑↑↑↑↑
 *                                這段叫「成員初始化列表」
 *
 *   兩種寫法的差別不是「美觀」而已，而是「真的有功能差異」：
 *
 *   1. 大括號內賦值 = 「先用預設值建構成員，再用 = 蓋過去」 (兩步)
 *   2. 初始化列表    = 「直接用你給的值建構成員」              (一步)
 *
 *   對 int / double 這種基本型別影響不大，但對「有建構成本的物件」
 *   (例如 std::string, std::vector) 就有效能差異 — 少做了一次預設建構。
 *
 * 【何時「必須」用初始化列表？非寫不可的場景】
 *   有三種成員，沒辦法在大括號內賦值，你「只能」用初始化列表：
 *
 *     A. const 成員  ─ 一旦建出來就不能改，所以必須在「建出來那一刻」就給值。
 *     B. 參考成員 (T&) ─ reference 一定要在誕生時綁定到某個對象。
 *     C. 沒有預設建構子的物件成員 ─ 因為大括號內賦值前會先呼叫預設建構子，
 *                                  該成員根本沒有預設建構子可呼叫。
 *
 * 【初始化順序的陷阱】
 *   成員的初始化「順序」是看「在 class 內宣告的順序」，
 *   不是看你在初始化列表寫的順序！
 *   例：
 *     class A {
 *         int b_;
 *         int a_;
 *     public:
 *         A(int x) : a_(x), b_(a_) {}   // 危險！b_ 比 a_ 先初始化，這時 a_ 還沒值
 *     };
 *   建議：初始化列表寫的順序 = 宣告順序，避免踩雷。
 *
 * 【對應 Leetcode】535. Encode and Decode TinyURL
 *   題目簡述：
 *     設計一個短網址服務，提供 encode(longUrl) 與 decode(shortUrl)
 *     兩個方法，要求 decode(encode(url)) == url。
 *   為什麼選這題：
 *     物件需要持有一個 unordered_map (對應表) 與一個計數器，
 *     這些成員適合在初始化列表設好，是練習的好場景。
 *
 * 【參考】
 *   https://en.cppreference.com/w/cpp/language/constructor   (Member init list 段落)
 *=============================================================================*/

/*
補充筆記：InitializerList
  - InitializerList 這類 OOP 範例要追蹤物件狀態：建構後是否有效、操作後是否仍符合類別承諾。
  - 如果類別擁有資源，就要檢查 destructor、copy、move 是否表達同一套所有權規則。
  - 繼承、friend、static、operator overload 都應服務於清楚的物件語意，而不是只展示語法。
  - member initializer list 位於建構子參數列後、建構子本體前，用來指定每個成員如何初始化；這是 C++ 建構物件最重要的語法之一。
  - 初始化和賦值不同：初始化是在物件出生時給初值，賦值是物件已存在後改值。對昂貴物件來說，先預設建構再賦值可能多一次成本。
  - reference 成員、const 成員和沒有預設建構子的成員只能在初始化列表處理，因為進入建構子本體時它們必須已經存在。
  - 成員實際初始化順序依 class 內宣告順序；若列表順序不同，編譯器可能警告，但實際順序不會因此改變。
  - base class 也在初始化列表初始化，且會早於 derived class 成員；理解這點才能看懂繼承中的建構順序。
  - 大括號初始化可避免某些 narrowing conversion，例如 double 到 int；但 initializer_list overload 可能優先被選中，遇到容器建構子要特別看語意。
  - 若有多個建構子重複初始化邏輯，可考慮 delegating constructor，讓一個建構子呼叫另一個建構子集中處理。
  - 初始化列表不適合塞複雜流程；需要檢查或計算的值可先由 helper function 產生，再把結果交給成員初始化。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】成員初始化列表（Member Initializer List）
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼要用初始化列表，而不是在建構子本體內指派？
//     答：本體內是「先預設建構成員、再指派」兩步；初始化列表是直接初始化一步，對有
//     建構成本的型別（string、vector）較快。更關鍵的是有些成員「只能」用初始化列表：
//     const 成員、reference 成員、沒有預設建構子的成員，以及 base class 的初始化。
//
// 🔥 Q2. 成員的初始化順序由什麼決定？
//     答：由成員在 class 中的「宣告順序」決定，與初始化列表的書寫順序完全無關。
//     所以 `A(int x) : a_(x), b_(a_) {}` 若 b_ 宣告在 a_ 前面，b_ 會先被初始化，
//     此時讀到的 a_ 還沒有值。
//     追問：怎麼避免？（讓列表順序與宣告順序一致，並開啟 -Wreorder 警告）
//
// Q3. delegating constructor 是什麼？
//     答：C++11 起，一個建構子可以在初始化列表中呼叫同類別的另一個建構子
//     （`A() : A(0) {}`），把共用的初始化邏輯集中一處。
//     注意：一旦委派給別的建構子，該列表就不能再同時初始化其他成員。
//
// ⚠️ 陷阱. `std::vector<int> v{3, 0};` 與 `std::vector<int> v(3, 0);` 一樣嗎？
//     答：不一樣。`{}` 會優先匹配 std::initializer_list 的建構子 → 得到 `{3, 0}`
//     兩個元素；`()` 呼叫的是 (count, value) 版本 → 得到 `{0, 0, 0}` 三個元素。
//     為什麼會錯：記住了「現代 C++ 一律用大括號」的口號，卻忽略在重載決議中
//     initializer_list 建構子擁有最高優先權。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <unordered_map>

// -----------------------------------------------------------------------------
// 範例 1：基本對比 - 大括號賦值 vs 初始化列表
// -----------------------------------------------------------------------------
class Point {
private:
    double x_;
    double y_;

public:
    // (A) 大括號內賦值：x_, y_ 先預設建構 (這裡是 0)，再被賦值蓋過去
    // Point(double x, double y) { x_ = x; y_ = y; }

    // (B) 初始化列表：直接用 (x) 與 (y) 把成員建構出來，少一步
    Point(double x, double y) : x_(x), y_(y) {
        // 大括號內可以放「初始化之外」的事，例如除錯訊息
        std::cout << "Point(" << x_ << ", " << y_ << ") 建構完成\n";
    }
};

// -----------------------------------------------------------------------------
// 範例 2：必須用初始化列表的三種成員
// -----------------------------------------------------------------------------
class MustUseList {
private:
    const int  id_;       // const 成員：誕生後不能改
    int&       refToX_;   // 參考成員：必須在誕生時就綁定
    std::string name_;    // 一般成員：兩種寫法都行，但用列表更佳

public:
    // 為什麼三個成員都得在這裡初始化？
    //   id_      : const，無法事後賦值
    //   refToX_  : reference，無法事後綁定
    //   name_    : 雖然可以事後賦值，但這樣會多一次 default 建構，沒效率
    MustUseList(int id, int& x, const std::string& n)
        : id_(id), refToX_(x), name_(n) {}

    void show() const {
        std::cout << "MustUseList{ id=" << id_
                  << ", refToX=" << refToX_
                  << ", name=" << name_ << " }" << std::endl;
    }
};

// -----------------------------------------------------------------------------
// 範例 3：對應 Leetcode 535 - Encode and Decode TinyURL
// -----------------------------------------------------------------------------
class TinyURL {
private:
    std::unordered_map<std::string, std::string> shortToLong_;  // 對應表
    int counter_;        // 用流水號當短碼，最簡單實作

public:
    // 注意：如果不寫初始化列表，counter_ 是「未定義值」(int 沒有預設 0)，
    //       而 unordered_map 雖然會被預設建構為空，但寫在列表更明確。
    TinyURL() : shortToLong_(), counter_(0) {}

    // 把長網址換成短網址；同樣的長網址多次呼叫，會得到不同短碼 (簡化處理)
    std::string encode(const std::string& longUrl) {
        std::string code = "http://tiny.url/" + std::to_string(counter_++);
        shortToLong_[code] = longUrl;
        return code;
    }

    // 反查回原始長網址
    std::string decode(const std::string& shortUrl) const {
        auto it = shortToLong_.find(shortUrl);
        if (it == shortToLong_.end()) return "(not found)";
        return it->second;
    }
};

// -----------------------------------------------------------------------------
// 範例 4：對應 Leetcode 1396 - Design Underground System  (難度: medium)
// -----------------------------------------------------------------------------
// 題目簡述：
//   設計地鐵系統，紀錄旅客 checkIn(id, station, time) 與 checkOut(id, station, time)，
//   提供 getAverageTime(start, end) 回傳「從 start 到 end」的平均耗時。
// 為什麼選這題：類別內部用 unordered_map 紀錄「進站」與「累計時間/次數」，
//               很適合用初始化列表設好成員。
class UndergroundSystem {
private:
    // 每個旅客目前的進站紀錄 (id → {站名, 時間})
    std::unordered_map<int, std::pair<std::string, int>> checkInData_;
    // 「起點|終點」 → {累計總時間, 出現次數}
    std::unordered_map<std::string, std::pair<long, int>> stats_;

public:
    // 用初始化列表把兩個 map 預設建構為空 (其實不寫也行，這裡示範意圖明確)
    UndergroundSystem() : checkInData_(), stats_() {}

    void checkIn(int id, const std::string& stationName, int t) {
        checkInData_[id] = {stationName, t};
    }

    void checkOut(int id, const std::string& stationName, int t) {
        auto& [startStation, startTime] = checkInData_[id];
        std::string key = startStation + "|" + stationName;     // 用 "|" 分隔
        stats_[key].first  += (t - startTime);
        stats_[key].second += 1;
        checkInData_.erase(id);
    }

    double getAverageTime(const std::string& startStation,
                          const std::string& endStation) const {
        std::string key = startStation + "|" + endStation;
        auto it = stats_.find(key);
        if (it == stats_.end()) return 0.0;
        return static_cast<double>(it->second.first) / it->second.second;
    }
};

// -----------------------------------------------------------------------------
// 範例 5：日常實用 - Logger 帶 const 等級資訊
// -----------------------------------------------------------------------------
// const 成員必須在初始化列表設定 — 本範例是 const 成員的典型場景。
class Logger {
private:
    const std::string moduleName_;   // 一旦建好就不能改
    const int         level_;        // 同上
    int               count_;

public:
    Logger(const std::string& name, int level)
        : moduleName_(name), level_(level), count_(0) {}

    void log(const std::string& msg) {
        ++count_;
        std::cout << "[" << moduleName_ << "][L" << level_ << "][#"
                  << count_ << "] " << msg << std::endl;
    }
};

int main() {
    std::cout << "----- 範例 1：Point 用初始化列表 -----" << std::endl;
    Point p(3.0, 4.0);

    std::cout << "----- 範例 2：必須用初始化列表的三種成員 -----" << std::endl;
    int x = 100;
    MustUseList m(1, x, "demo");
    m.show();

    // 改外面的 x 看會不會影響 refToX_，因為它是 reference
    x = 999;
    m.show();    // 預期 refToX 會跟著變成 999

    std::cout << "----- 範例 3：Leetcode 535 TinyURL -----" << std::endl;
    TinyURL t;
    std::string longUrl = "https://leetcode.com/problems/design-tinyurl/";
    std::string shortUrl = t.encode(longUrl);
    std::cout << "短網址: " << shortUrl << std::endl;
    std::cout << "還原:   " << t.decode(shortUrl) << std::endl;

    // 多測一個
    std::string s2 = t.encode("https://example.com/very/long/path?with=query");
    std::cout << "短網址: " << s2 << std::endl;
    std::cout << "還原:   " << t.decode(s2) << std::endl;

    // 驗證 decode(encode(x)) == x
    std::cout << (t.decode(t.encode("abc")) == "abc" ? "驗證通過" : "驗證失敗") << std::endl;

    std::cout << "----- 範例 4：Leetcode 1396 Underground System -----" << std::endl;
    UndergroundSystem us;
    us.checkIn(45, "Leyton", 3);
    us.checkOut(45, "Waterloo", 15);            // 耗時 12
    us.checkIn(27, "Leyton", 10);
    us.checkOut(27, "Waterloo", 20);            // 耗時 10
    // Leyton → Waterloo 平均: (12 + 10) / 2 = 11.0
    std::cout << "Leyton→Waterloo 平均: "
              << us.getAverageTime("Leyton", "Waterloo") << std::endl;

    std::cout << "----- 範例 5：Logger 帶 const 成員 -----" << std::endl;
    Logger lg("auth", 2);
    lg.log("使用者登入");
    lg.log("使用者登出");
    return 0;
}

/* 預期輸出：
 * ----- 範例 1：Point 用初始化列表 -----
 * Point(3, 4) 建構完成
 * ----- 範例 2：必須用初始化列表的三種成員 -----
 * MustUseList{ id=1, refToX=100, name=demo }
 * MustUseList{ id=1, refToX=999, name=demo }
 * ----- 範例 3：Leetcode 535 TinyURL -----
 * 短網址: http://tiny.url/0
 * 還原:   https://leetcode.com/problems/design-tinyurl/
 * 短網址: http://tiny.url/1
 * 還原:   https://example.com/very/long/path?with=query
 * 驗證通過
 * ----- 範例 4：Leetcode 1396 Underground System -----
 * Leyton→Waterloo 平均: 11
 * ----- 範例 5：Logger 帶 const 成員 -----
 * [auth][L2][#1] 使用者登入
 * [auth][L2][#2] 使用者登出
 */

/*=============================================================================
 * 【本篇重點回顧】
 *   1. 初始化列表 :成員(值), 成員(值)... 寫在「建構子大括號的前面」。
 *   2. 它直接「建構」成員，比在大括號內賦值少做一次預設建構，更有效率。
 *   3. const 成員、參考成員、無預設建構子的物件 → 必須用初始化列表。
 *   4. 成員初始化順序 = 在 class 內宣告的順序。建議列表順序保持一致避免踩雷。
 *
 * 【下一篇預告】
 *   8_CopyConstructor.cpp
 *   複製建構子 (Copy Constructor) — 物件被複製時發生什麼事？
 *   為什麼有時候會踩到「淺拷貝 vs 深拷貝」的坑。
 *=============================================================================*/

// 編譯: g++ -std=c++20 -Wall -Wextra 7_InitializerList.cpp -o 7_InitializerList

// === 預期輸出 ===
// ----- 範例 1：Point 用初始化列表 -----
// Point(3, 4) 建構完成
// ----- 範例 2：必須用初始化列表的三種成員 -----
// MustUseList{ id=1, refToX=100, name=demo }
// MustUseList{ id=1, refToX=999, name=demo }
// ----- 範例 3：Leetcode 535 TinyURL -----
// 短網址: http://tiny.url/0
// 還原:   https://leetcode.com/problems/design-tinyurl/
// 短網址: http://tiny.url/1
// 還原:   https://example.com/very/long/path?with=query
// 驗證通過
// ----- 範例 4：Leetcode 1396 Underground System -----
// Leyton→Waterloo 平均: 11
// ----- 範例 5：Logger 帶 const 成員 -----
// [auth][L2][#1] 使用者登入
// [auth][L2][#2] 使用者登出
