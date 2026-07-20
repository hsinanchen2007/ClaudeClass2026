// =============================================================================
//  第 23 課：mutable 關鍵字 2  —  用 mutable 實作「計算快取 (memoization)」
// =============================================================================
//
// 【主題資訊 Information】
//   語法    ： mutable <型別> <成員名>;      // 只能用在非 static、非 const、非 reference 的資料成員
//   標準版本： C++98 起即有 mutable;C++11 起 mutable 另有第二個意義(lambda 的 mutable)
//   標頭檔  ： 無(語言關鍵字)
//   複雜度  ： 本檔的 getArea() 第一次 O(1) 計算,之後 O(1) 讀快取
//             ——重點不是複雜度,而是「省掉重複計算」
//
// 【詳細解釋 Explanation】
//
// 【1. const 成員函式到底承諾了什麼】
//   const 成員函式的機制是:在函式內部,this 的型別從 Circle* 變成 const Circle*。
//   因此透過 this 存取的每個成員都成了 const,不能寫入。這個承諾很有價值:
//   它讓 const Circle& 這種參數可以安全地被呼叫端信任。
//   但這個承諾是「語法層級」的——編譯器擋的是「有沒有寫入成員」,
//   而使用者真正在意的是「這個物件從外面看起來有沒有變」。
//   這兩件事在「快取」這種情境下就分岔了。
//
// 【2. bitwise const vs logical const(本課的核心)】
//   * bitwise const(位元層級不變):物件的每一個 byte 都沒被改動。編譯器檢查的是這個。
//   * logical const(邏輯層級不變):從類別的公開介面觀察,物件的「意義」沒變。
//   本檔的 getArea() 修改了 cachedArea_ / areaCached_,所以它不是 bitwise const,
//   但半徑沒變、任何呼叫者看到的面積都一樣,所以它是 logical const。
//   mutable 的用途正是:「我知道這個成員被改了,但它不屬於物件的邏輯狀態,請放行」。
//
// 【3. 為什麼需要兩個成員(旗標 + 值),而不是用哨兵值】
//   常見的偷懶寫法是 cachedArea_ = -1 代表「還沒算」。這在本例剛好可行(面積恆正),
//   但一旦快取的值域涵蓋所有可能數值(例如快取「溫度」可以是 -1),哨兵值就失效了。
//   用獨立的 bool 旗標是可組合、不依賴值域的做法。
//   C++17 之後也可用 mutable std::optional<double> 一次表達「有沒有值 + 值是多少」。
//
// 【4. 快取失效(invalidation)才是真正的難點】
//   setRadius() 必須把兩個旗標都設回 false。這行如果漏掉,getArea() 會回傳
//   「舊半徑算出來的面積」——而且不會有任何編譯錯誤或崩潰,只是答案默默地錯。
//   這就是那句名言的由來:電腦科學只有兩件難事,快取失效與命名。
//   實務上的防呆做法是「只留一個修改入口」:所有會影響快取的 setter 都呼叫同一個
//   私有的 invalidateCache(),而不是各自複製貼上清除程式碼。
//
// 【概念補充 Concept Deep Dive】
//   (A) mutable 對記憶體佈局沒有任何影響。
//       mutable 純粹是編譯期的存取檢查標記,不改變成員的大小、順序或對齊,
//       也不會產生任何額外指令。它不像 volatile 會影響最佳化。
//
//   (B) 編譯器仍可能把 const 物件放進唯讀區段(.rodata)。
//       但只要類別含有 mutable 成員,該物件就不會被整個放進唯讀記憶體——
//       因為標準允許修改 mutable 成員。這正是 mutable 合法、而對真正的 const
//       物件做 const_cast 修改是 UB 的根本差別(見本課檔案 6)。
//
//   (C) mutable 與執行緒安全:C++11 起的重要慣例。
//       標準函式庫要求「const 成員函式必須是 thread-safe 的」(可同時被多執行緒呼叫)。
//       本檔這種「無鎖快取」的 const 函式其實 不是 thread-safe:兩條執行緒同時
//       呼叫 getArea() 會同時寫入 cachedArea_,構成 data race。
//       正式做法是再加一個 mutable std::mutex,或改用 std::once_flag。
//       這也是 mutable 最經典的用途之一——mutex 一定得是 mutable,
//       因為 lock() 會修改 mutex 的狀態,而加鎖的動作本身出現在 const 函式裡。
//
//   (D) 本檔用 static constexpr double PI 而不是 mutable。
//       PI 是「所有 Circle 共用、且永不改變」的常數,屬於下一課(static 成員)的主題。
//       C++17 起 static constexpr 資料成員隱含 inline,不需要在 .cpp 另外定義。
//
// 【注意事項 Pay Attention】
//   1. mutable 不能和 const 一起用:mutable const int x; 是編譯錯誤
//      (「永不改變」與「隨時可改」本來就矛盾)。
//   2. mutable 不能用於 static 成員,也不能用於 reference 成員。
//   3. mutable 會讓 const 的保護力打折。只把「不影響外部觀察結果」的成員標為
//      mutable(快取、計數器、mutex、lazy 旗標),核心資料絕不要標(見本課檔案 4)。
//   4. 輸出順序的陷阱:cout << "面積 = " << c.getArea() 這一行,
//      getArea() 內部的 cout 會夾在中間印出。C++17 起標準保證 << 的左運算元
//      先於右運算元求值,所以「面積 = 」一定先印;但在 C++14 以前這是
//      unspecified(本機 g++ 15.2 實測 C++14 也是同樣順序,但那是實作行為,不可依賴)。
//      要避免這種交錯,應先把值存進區域變數再輸出。
//   5. 本檔的快取沒有加鎖,只適用於單執行緒。多執行緒請見上面 (C)。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】mutable 與快取
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. const 成員函式裡為什麼可以修改 mutable 成員?編譯器做了什麼?
//     答：const 成員函式的實質是把 this 從 T* 變成 const T*,所以透過 this
//         存取到的成員全都帶 const。mutable 的作用就是在這個轉換中被豁免——
//         編譯器對 mutable 成員不套用那層 const,因此寫入是合法的。
//         它是純編譯期的存取控制,不產生任何執行期成本,也不改變物件佈局。
//     追問：那 mutable 和 const_cast 差在哪?→ mutable 是標準祝福的合法路徑;
//         對「本來就宣告為 const 的物件」用 const_cast 去寫入則是 UB,
//         因為編譯器可能把它放在唯讀記憶體。差別在於物件本身的常數性,
//         不在於你用什麼語法繞過去。
//
// 🔥 Q2. 什麼叫 logical const?為什麼標準委員會不乾脆讓 const 只檢查 logical const?
//     答：logical const 指「從公開介面觀察不到差異」。編譯器無法自動判斷這件事——
//         它不知道 cachedArea_ 是快取還是核心資料,那是設計者的語意。
//         所以標準的做法是:編譯器守 bitwise const(可機械檢查),
//         再由設計者用 mutable 明確標註哪些成員不算邏輯狀態。
//     追問：那要怎麼驗證一個成員該不該是 mutable?→ 拿掉它之後,
//         用同一串公開呼叫去觀察,回傳值序列是否完全相同?相同才可以是 mutable。
//         本檔拿掉快取只會變慢,答案不變,所以合格。
//
// ⚠️ 陷阱. 「getArea() 是 const,所以多執行緒同時呼叫一定安全。」
//     答：錯。本檔的 getArea() 在快取未命中時會寫入 cachedArea_ 與 areaCached_,
//         兩條執行緒同時進來就是 data race,屬於 UB(而不是「頂多算兩次」)。
//         要真的安全,需要 mutable std::mutex 搭配 lock_guard,或 std::call_once。
//     為什麼會錯：多數人把「const」直接等同於「唯讀所以執行緒安全」。
//         但 const 只保證「沒有修改邏輯狀態」,mutable 成員照樣在被寫入。
//         標準函式庫的慣例是「const 成員函式應該做到 thread-safe」——
//         那是對實作者的要求,不是編譯器的保證。自己寫的類別若沒加鎖,就沒有這個性質。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <unordered_map>
#include <list>
using namespace std;

class Circle {
private:
    double radius_;

    // 快取相關——mutable 因為它們只是效能優化，不影響邏輯狀態
    // 這裡用 mutable 是因為即使在 const 函數中，我們也想更新快取狀態
    // areaCached_ 和 circumCached_ 用來記錄快取是否有效
    // cachedArea_ 和 cachedCircum_ 存儲計算結果
    mutable bool areaCached_;
    mutable double cachedArea_;
    mutable bool circumCached_;
    mutable double cachedCircum_;

    static constexpr double PI = 3.14159265358979;

public:
    Circle(double r)
        : radius_(r > 0 ? r : 1.0)
        , areaCached_(false), cachedArea_(0)
        , circumCached_(false), cachedCircum_(0)
    {
    }

    // getter：邏輯上只讀，但會更新快取
    // const 函數：允許在 const 對象上調用，並且可以修改 mutable 成員
    // 這裡的 getArea() 和 getCircumference() 是 const 的，因為它們不修改 Circle 的邏輯狀態（半徑不變），但它們會修改快取狀態，這就是 mutable 的典型用法
    // const 函數：邏輯上只讀, 但可以修改快取狀態
    double getArea() const {
        if (!areaCached_) {
            cout << "    [計算面積...]" << endl;
            cachedArea_ = PI * radius_ * radius_;   // ✅ mutable 可修改
            areaCached_ = true;                      // ✅ mutable 可修改
        } else {
            cout << "    [使用快取]" << endl;
        }
        return cachedArea_;
    }

    double getCircumference() const {
        if (!circumCached_) {
            cout << "    [計算周長...]" << endl;
            cachedCircum_ = 2 * PI * radius_;
            circumCached_ = true;
        } else {
            cout << "    [使用快取]" << endl;
        }
        return cachedCircum_;
    }

    // setter：修改半徑時，快取失效
    void setRadius(double r) {
        if (r <= 0) return;
        radius_ = r;
        areaCached_ = false;     // 清除快取
        circumCached_ = false;   // 清除快取
        cout << "  半徑改為 " << radius_ << "，快取已清除" << endl;
    }

    double getRadius() const { return radius_; }
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 146. LRU Cache
//   題目：設計一個容量固定的 LRU(Least Recently Used)快取,get 與 put 都要 O(1)。
//   為什麼用到本主題：這題正是「讀取操作會修改內部狀態」的教科書案例——
//     get(key) 從呼叫者的角度是純查詢,但它必須把該節點搬到「最近使用」的一端,
//     否則淘汰順序就錯了。如果把 get 宣告成 const 成員函式,那條搬動 list 的程式碼
//     就必須靠 mutable(或乾脆不要宣告成 const)。
//     這裡刻意示範「誠實的設計」:get 會改變淘汰順序,那是使用者觀察得到的行為
//     (它影響之後誰被淘汰),屬於邏輯狀態,所以 不該 用 mutable 硬包成 const。
//     對照 Circle 的快取——那個拿掉後行為完全一樣,才是 mutable 的正當用途。
//   複雜度：get / put 皆為 O(1);list 存放順序,unordered_map 存放 key -> list 迭代器。
// -----------------------------------------------------------------------------
class LRUCache {
private:
    size_t capacity_;
    list<pair<int, int>> order_;                                   // front = 最近使用
    unordered_map<int, list<pair<int, int>>::iterator> index_;

public:
    explicit LRUCache(size_t capacity) : capacity_(capacity) {}

    // 注意這裡「不是」const:搬動順序會影響未來的淘汰結果,屬於邏輯狀態
    int get(int key) {
        auto it = index_.find(key);
        if (it == index_.end()) return -1;
        order_.splice(order_.begin(), order_, it->second);          // 搬到最前面
        return it->second->second;
    }

    void put(int key, int value) {
        auto it = index_.find(key);
        if (it != index_.end()) {
            it->second->second = value;
            order_.splice(order_.begin(), order_, it->second);
            return;
        }
        if (index_.size() == capacity_) {
            index_.erase(order_.back().first);                      // 淘汰最久未使用
            order_.pop_back();
        }
        order_.emplace_front(key, value);
        index_[key] = order_.begin();
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】商品含稅價的快取,以及「單一失效入口」的寫法
//   情境：電商後台的商品物件。含稅價 = 未稅價 * (1 + 稅率),計算本身不貴,
//         但真實系統裡這類衍生欄位常常要查匯率表、跑折扣規則,成本很高。
//   重點：稅率或未稅價任一改變,快取都必須失效。
//         這裡把清除動作收斂到唯一的 invalidate(),而不是在每個 setter 各寫一次——
//         日後新增第三個影響因素時,只會漏掉一個地方的機率大幅降低。
// -----------------------------------------------------------------------------
class Product {
private:
    string sku_;
    double netPrice_;
    double taxRate_;

    mutable bool grossCached_ = false;   // 只是效能優化,不是邏輯狀態
    mutable double grossCache_ = 0.0;
    mutable int computeCount_ = 0;       // 用來證明「快取真的有生效」

    void invalidate() { grossCached_ = false; }   // 唯一的失效入口

public:
    Product(const string& sku, double net, double rate)
        : sku_(sku), netPrice_(net), taxRate_(rate) {}

    double grossPrice() const {
        if (!grossCached_) {
            grossCache_ = netPrice_ * (1.0 + taxRate_);
            grossCached_ = true;
            ++computeCount_;
        }
        return grossCache_;
    }

    void setNetPrice(double net) { netPrice_ = net; invalidate(); }
    void setTaxRate(double rate) { taxRate_ = rate; invalidate(); }

    const string& sku() const { return sku_; }
    int computeCount() const { return computeCount_; }
};

int main() {
    cout << "=== 快取範例 ===" << endl;

    Circle c(5.0);

    // 第一次調用——觸發計算
    cout << "\n--- 第一次查詢 ---" << endl;
    cout << "  面積 = " << c.getArea() << endl;
    cout << "  周長 = " << c.getCircumference() << endl;

    // 第二次調用——使用快取
    cout << "\n--- 第二次查詢（快取命中）---" << endl;
    cout << "  面積 = " << c.getArea() << endl;
    cout << "  周長 = " << c.getCircumference() << endl;

    // 修改半徑——快取失效
    cout << "\n--- 修改半徑 ---" << endl;
    c.setRadius(10.0);

    // 再次查詢——重新計算
    cout << "\n--- 修改後查詢 ---" << endl;
    cout << "  面積 = " << c.getArea() << endl;
    cout << "  周長 = " << c.getCircumference() << endl;

    // 再次查詢——又是快取
    cout << "\n--- 再次查詢（快取命中）---" << endl;
    cout << "  面積 = " << c.getArea() << endl;

    // 示範注意事項 4：先取值再輸出，就不會有交錯
    cout << "\n--- 先取值再輸出（避免交錯）---" << endl;
    double area = c.getArea();          // getArea() 的訊息先獨立印完
    cout << "  面積 = " << area << endl;

    cout << "\n=== LeetCode 146. LRU Cache ===" << endl;
    LRUCache lru(2);
    lru.put(1, 100);
    lru.put(2, 200);
    cout << "  get(1) = " << lru.get(1) << endl;   // 100，同時把 key 1 變成最近使用
    lru.put(3, 300);                                // 容量滿，淘汰最久未使用的 key 2
    cout << "  get(2) = " << lru.get(2) << endl;   // -1，已被淘汰
    cout << "  get(3) = " << lru.get(3) << endl;   // 300
    cout << "  get(1) = " << lru.get(1) << endl;   // 100，因為剛才 get 過所以留著

    cout << "\n=== 日常實務: 含稅價快取與失效 ===" << endl;
    Product p("SKU-1024", 1000.0, 0.05);
    cout << "  含稅價 = " << p.grossPrice() << endl;
    cout << "  含稅價 = " << p.grossPrice() << "（走快取）" << endl;
    cout << "  實際計算次數 = " << p.computeCount() << endl;

    p.setTaxRate(0.10);                 // 稅率改變 -> 快取失效
    cout << "  調整稅率後 含稅價 = " << p.grossPrice() << endl;
    cout << "  實際計算次數 = " << p.computeCount() << endl;

    p.setNetPrice(2000.0);              // 未稅價改變 -> 快取也要失效
    cout << "  調整未稅價後 含稅價 = " << p.grossPrice() << endl;
    cout << "  實際計算次數 = " << p.computeCount() << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第\ 23\ 課：mutable\ 關鍵字2.cpp -o mutable2

// 【輸出說明】
//   1. 「面積 = 」與「[計算面積...]」交錯,是因為 getArea() 內部也在輸出。
//      C++17 起保證 << 左運算元先求值,所以「面積 = 」必定先印出;
//      C++14 以前此順序為 unspecified(本機 g++ 15.2 恰好相同,但不可依賴)。
//   2. 面積/周長為 double 預設 6 位有效數字輸出(78.5398 而非 78.539816)。
//   3. 本檔輸出完全決定性,連續執行 3 次 md5 相同。

// === 預期輸出 ===
// === 快取範例 ===
//
// --- 第一次查詢 ---
//   面積 =     [計算面積...]
// 78.5398
//   周長 =     [計算周長...]
// 31.4159
//
// --- 第二次查詢（快取命中）---
//   面積 =     [使用快取]
// 78.5398
//   周長 =     [使用快取]
// 31.4159
//
// --- 修改半徑 ---
//   半徑改為 10，快取已清除
//
// --- 修改後查詢 ---
//   面積 =     [計算面積...]
// 314.159
//   周長 =     [計算周長...]
// 62.8319
//
// --- 再次查詢（快取命中）---
//   面積 =     [使用快取]
// 314.159
//
// --- 先取值再輸出（避免交錯）---
//     [使用快取]
//   面積 = 314.159
//
// === LeetCode 146. LRU Cache ===
//   get(1) = 100
//   get(2) = -1
//   get(3) = 300
//   get(1) = 100
//
// === 日常實務: 含稅價快取與失效 ===
//   含稅價 = 1050
//   含稅價 = 1050（走快取）
//   實際計算次數 = 1
//   調整稅率後 含稅價 = 1100
//   實際計算次數 = 2
//   調整未稅價後 含稅價 = 2200
//   實際計算次數 = 3
