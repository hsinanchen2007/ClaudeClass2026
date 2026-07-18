/*=============================================================================
 * 檔名：3_Encapsulation.cpp
 * 主題：封裝 (Encapsulation) - public / private 與「資料藏起來」的觀念
 * 適合：已會 class、想知道為什麼要把成員變數設成 private 的初學者
 *
 * 【課題介紹】
 *   OOP 有三大支柱 (Three Pillars)：
 *     1. 封裝 (Encapsulation)
 *     2. 繼承 (Inheritance)            ← 第 14 篇
 *     3. 多型 (Polymorphism)            ← 第 16 篇
 *
 *   今天先講第一個：封裝。
 *   封裝的核心概念：
 *
 *       「把資料藏起來，外界只能透過特定的入口 (public 函式) 操作。」
 *
 *   為什麼要這樣？想像一下：
 *     - 銀行帳戶的 balance 如果是 public，外人可以一行 acc.balance = -9999
 *       直接把餘額改成負的，這顯然不對。
 *     - 把 balance 設成 private，只開放 deposit() / withdraw() 兩個函式，
 *       函式內可以做檢查（不能存負數、不能透支等等），就能保護資料一致性。
 *
 *   一句話：
 *       「public 是門口的服務窗口；private 是辦公室裡面，閒人勿入。」
 *
 * 【三種存取等級】
 *   - public    任何地方都能存取
 *   - private   只有同一個 class 自己 (跟它的 friend，第 12 篇) 能存取
 *   - protected 同 class + 子類別能存取（繼承時用，第 15 篇詳述）
 *
 * 【常見慣例 (convention)】
 *   - 成員變數通常設為 private
 *   - 提供 public 的「getter (讀取器)」與「setter (設定器)」對外溝通
 *   - 成員變數命名常用底線結尾，如 `balance_`，或前綴 `m_balance`，
 *     用意是跟參數區分開，不會打架。本檔為了簡潔，用底線結尾形式。
 *
 * 【對應 Leetcode】1603. Design Parking System
 *   題目簡述：
 *     設計一個停車系統，有 big / medium / small 三種車位，
 *     建構時給每種車位的數量。提供 addCar(carType) 函式，
 *     如果該種車位還有空位則停進去並回傳 true，否則回傳 false。
 *   為什麼選這題：
 *     車位剩餘數量「絕對不該被外面亂改」，這就是封裝的最佳示範。
 *
 * 【參考】
 *   https://en.cppreference.com/w/cpp/language/access
 *   https://cplusplus.com/doc/tutorial/classes/
 *=============================================================================*/

/*
補充筆記：Encapsulation
  - Encapsulation 這類 OOP 範例要追蹤物件狀態：建構後是否有效、操作後是否仍符合類別承諾。
  - 如果類別擁有資源，就要檢查 destructor、copy、move 是否表達同一套所有權規則。
  - 繼承、friend、static、operator overload 都應服務於清楚的物件語意，而不是只展示語法。
  - 封裝的重點不是把 private 寫滿，而是把資料修改集中在少數成員函式中，讓類別可以檢查輸入、維持不變條件並隱藏內部表示法。
  - private 成員仍然存在於物件記憶體中，只是語法上禁止外部直接存取；這是編譯期的介面限制，不是加密或安全防護。
  - getter 若直接回傳可修改的 reference，等於把 private 後門打開；初學時可先回傳值或 const reference，避免呼叫端繞過檢查。
  - setter 不應只是機械式 setScore(int x){ score=x; }；若資料有合法範圍，setter 應在這裡拒絕或修正不合法值。
  - 封裝讓日後能更換內部實作，例如從 int score 改成等級 enum，而外部仍透過同一組 public 函式使用類別。
  - 過度封裝也會造成樣板程式碼；如果型別只是單純資料載體，例如座標點或回傳結果，使用 public struct 可能更清楚。
  - 判斷封裝好不好，可以看外部程式是否需要知道太多細節；若呼叫者必須照固定順序手動修改多個欄位，通常表示類別責任切得不夠好。
  - 封裝和測試互相配合：測試 public 行為，而不是硬檢查 private 實作，這樣重構內部資料時測試不會不必要地破。
*/
#include <iostream>
#include <string>
#include <vector>

// -----------------------------------------------------------------------------
// 範例 1：封裝過的銀行帳戶
// -----------------------------------------------------------------------------
class BankAccount {
private:                                  // 以下是「私人區域」，外界不准碰
    std::string owner_;                   // 命名慣例：成員變數加底線後綴
    double      balance_;

public:                                   // 以下是「對外窗口」
    // setter：設定戶名 - 用 const 參考避免複製字串
    void setOwner(const std::string& name) { owner_ = name; }

    // getter：取得戶名 - const 表示這個函式不會修改物件 (只讀)
    const std::string& getOwner() const { return owner_; }

    // 初始化餘額（簡化用，正式做法是用建構子，下一篇會學）
    void setInitialBalance(double v) { balance_ = v; }

    // 存款：加上「金額必須 > 0」的檢查 → 這就是封裝的價值！
    bool deposit(double amount) {
        if (amount <= 0) {
            std::cout << "[錯誤] 存款金額必須為正" << std::endl;
            return false;
        }
        balance_ += amount;
        return true;
    }

    // 提款：加上「不能透支」的檢查
    bool withdraw(double amount) {
        if (amount <= 0) {
            std::cout << "[錯誤] 提款金額必須為正" << std::endl;
            return false;
        }
        if (amount > balance_) {
            std::cout << "[錯誤] 餘額不足" << std::endl;
            return false;
        }
        balance_ -= amount;
        return true;
    }

    double getBalance() const { return balance_; }
};

// 試想：如果 balance_ 是 public，外人就可以寫
//     acc.balance_ = -1e9;
// 直接破壞資料正確性。private 把這個風險擋掉了。

// -----------------------------------------------------------------------------
// 範例 2：對應 Leetcode 1603 - Design Parking System
// -----------------------------------------------------------------------------
// 題目本身就是一個典型的「狀態 (剩餘車位數) 必須被保護」的設計題。
class ParkingSystem {
private:
    int big_;        // 大車位剩餘數量
    int medium_;     // 中車位剩餘數量
    int small_;      // 小車位剩餘數量

public:
    // 簡單的初始化函式（下一篇會看到更好的寫法：建構子）
    void init(int big, int medium, int small) {
        big_    = big;
        medium_ = medium;
        small_  = small;
    }

    // 嘗試讓一輛車進場，根據 carType 決定停哪種車位
    //   carType = 1 → big
    //   carType = 2 → medium
    //   carType = 3 → small
    // 回傳 true：成功停入；false：該類型已客滿
    bool addCar(int carType) {
        // 用 switch 比 if/else if 鏈更清楚
        switch (carType) {
            case 1:
                if (big_ > 0)    { --big_;    return true; }
                break;
            case 2:
                if (medium_ > 0) { --medium_; return true; }
                break;
            case 3:
                if (small_ > 0)  { --small_;  return true; }
                break;
            default:
                std::cout << "[錯誤] 未知 carType: " << carType << std::endl;
                break;
        }
        return false;
    }

    // 額外加一個只讀方法，方便 demo 看內部狀態
    void show() const {
        std::cout << "剩餘車位 - big: " << big_
                  << ", medium: " << medium_
                  << ", small: "  << small_ << std::endl;
    }
};

// -----------------------------------------------------------------------------
// 範例 3：對應 Leetcode 705 - Design HashSet
// -----------------------------------------------------------------------------
// 題目簡述：設計一個 HashSet 類別，支援 add / remove / contains 三個操作。
// 為什麼選這題：內部 buckets 是實作細節，必須 private 封閉起來；
//               外部只能透過 add/remove/contains 介面互動 → 封裝典範。
class MyHashSet {
private:
    // 用一個固定 bucket 數的陣列，每個 bucket 是一個 vector (chaining)
    // 真實的 std::unordered_set 也是類似概念但更複雜，這裡求簡單。
    static constexpr int BUCKETS = 769;     // 質數通常 hash 分布更平均
    std::vector<std::vector<int>> table_;

    int hash(int key) const { return key % BUCKETS; }   // private：實作細節不外露

public:
    MyHashSet() : table_(BUCKETS) {}

    void add(int key) {
        auto& bucket = table_[hash(key)];
        for (int x : bucket) if (x == key) return;   // 已存在則不重複放
        bucket.push_back(key);
    }

    void remove(int key) {
        auto& bucket = table_[hash(key)];
        for (auto it = bucket.begin(); it != bucket.end(); ++it) {
            if (*it == key) { bucket.erase(it); return; }
        }
    }

    bool contains(int key) const {
        const auto& bucket = table_[hash(key)];
        for (int x : bucket) if (x == key) return true;
        return false;
    }
};

// -----------------------------------------------------------------------------
// 範例 4：日常實用 - SimpleConfig 設定檔封裝
// -----------------------------------------------------------------------------
// 工作上常見：把一組設定 (host、port、retry) 封裝起來，
// 外部只能透過 getter / setter 取用，setter 內加入合法性檢查。
class SimpleConfig {
private:
    std::string host_;
    int         port_;
    int         retry_;

public:
    SimpleConfig() : host_("localhost"), port_(8080), retry_(3) {}

    const std::string& host() const { return host_; }
    int port()  const { return port_; }
    int retry() const { return retry_; }

    void setHost(const std::string& h) {
        if (h.empty()) {
            std::cout << "[錯誤] host 不能為空" << std::endl;
            return;
        }
        host_ = h;
    }
    void setPort(int p) {
        if (p < 1 || p > 65535) {
            std::cout << "[錯誤] port 必須介於 1~65535" << std::endl;
            return;
        }
        port_ = p;
    }
};

int main() {
    std::cout << "----- 範例 1：封裝過的 BankAccount -----" << std::endl;
    BankAccount acc;
    acc.setOwner("Alice");
    acc.setInitialBalance(0);

    acc.deposit(1000);
    acc.deposit(-50);          // 預期會被擋下
    acc.withdraw(2000);        // 預期會被擋下 (餘額不足)
    acc.withdraw(300);
    std::cout << acc.getOwner() << " 目前餘額: " << acc.getBalance() << std::endl;

    // acc.balance_ = -1e9;     // ← 試開這行會編譯錯誤！因為 balance_ 是 private。

    std::cout << "----- 範例 2：Leetcode 1603 ParkingSystem -----" << std::endl;
    ParkingSystem ps;
    ps.init(/*big=*/1, /*medium=*/1, /*small=*/0);
    // 上面那種 /*xxx=*/ 寫法是「具名參數註解」，方便讀者看清楚每個 1 是什麼意思。

    std::cout << ps.addCar(1) << std::endl;   // big 還有 1 個 → true
    std::cout << ps.addCar(2) << std::endl;   // medium 還有 1 個 → true
    std::cout << ps.addCar(3) << std::endl;   // small = 0 → false
    std::cout << ps.addCar(1) << std::endl;   // big 已用完 → false
    ps.show();

    std::cout << "----- 範例 3：Leetcode 705 Design HashSet -----" << std::endl;
    MyHashSet hs;
    hs.add(1);
    hs.add(2);
    std::cout << hs.contains(1) << std::endl;   // 1 (true)
    std::cout << hs.contains(3) << std::endl;   // 0 (false)
    hs.add(2);                                  // 重複 add 不會出錯
    hs.remove(2);
    std::cout << hs.contains(2) << std::endl;   // 0 (已被 remove)

    std::cout << "----- 範例 4：SimpleConfig 設定 -----" << std::endl;
    SimpleConfig cfg;
    cfg.setHost("api.example.com");
    cfg.setPort(443);
    cfg.setPort(99999);                          // 預期被擋下
    cfg.setHost("");                             // 預期被擋下
    std::cout << "config: " << cfg.host() << ":" << cfg.port()
              << ", retry=" << cfg.retry() << std::endl;
    return 0;
}

/* 預期輸出：
 * ----- 範例 1：封裝過的 BankAccount -----
 * [錯誤] 存款金額必須為正
 * [錯誤] 餘額不足
 * Alice 目前餘額: 700
 * ----- 範例 2：Leetcode 1603 ParkingSystem -----
 * 1
 * 1
 * 0
 * 0
 * 剩餘車位 - big: 0, medium: 0, small: 0
 * ----- 範例 3：Leetcode 705 Design HashSet -----
 * 1
 * 0
 * 0
 * ----- 範例 4：SimpleConfig 設定 -----
 * [錯誤] port 必須介於 1~65535
 * [錯誤] host 不能為空
 * config: api.example.com:443, retry=3
 */

/*=============================================================================
 * 【本篇重點回顧】
 *   1. 封裝 = 把資料藏起來，只透過 public 函式對外，避免狀態被亂改。
 *   2. 慣例：成員變數 private、外界互動函式 public。
 *   3. setter 可以加檢查，避免不合法的值；getter 通常加 const。
 *   4. 寫過封裝的程式比較好維護，因為「規則只寫在類別內部，集中管理」。
 *
 * 【下一篇預告】
 *   4_Constructor.cpp
 *   建構子 (Constructor) — 物件出生時自動執行的特殊函式，
 *   並用 Leetcode 1656. Design an Ordered Stream 練習。
 *=============================================================================*/
