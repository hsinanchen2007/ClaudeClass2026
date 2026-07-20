/*=============================================================================
 * 檔名：20_UniquePtr.cpp
 * 主題：std::unique_ptr - 獨占所有權的智慧指標
 * 適合：學完 RAII 後，想揮別 new/delete 的人
 *
 * 【課題介紹】
 *   傳統 C++ 寫 heap 物件：
 *
 *       Foo* p = new Foo;
 *       // ... 用 ...
 *       delete p;
 *
 *   問題很多：
 *     - 忘記 delete  → 記憶體洩漏
 *     - 中間拋例外   → 跳過 delete
 *     - 多次 delete  → 雙重釋放，當機
 *     - 指標被別人複製 → 不知道誰該負責 delete
 *
 *   現代 C++ 的解法：智慧指標 (smart pointer)。它是一個物件，內部持有 raw pointer，
 *   並在解構子裡 delete 掉資源 — 完美的 RAII 應用。
 *
 *   智慧指標有三種，這篇先學最常用的 std::unique_ptr：
 *
 *       「unique_ptr 表示『獨占所有權』 (Exclusive Ownership)：
 *        同一時間只有一個 unique_ptr 擁有該資源。
 *        當這個 unique_ptr 被銷毀時，自動 delete 資源。」
 *
 *   兩個重要特性：
 *     1. 不能被「複製 (copy)」：兩份 unique_ptr 同時擁有同一資源是矛盾的。
 *     2. 但可以被「移動 (move)」：把所有權從 A 轉給 B (第 22 篇深入 move semantics)。
 *
 * 【常用 API】
 *   #include <memory>
 *
 *   std::unique_ptr<Foo> p = std::make_unique<Foo>(args...);  // 建立
 *   *p             // 取出物件
 *   p->member      // 像普通指標一樣存取成員
 *   p.get()        // 取得內部的 raw pointer (謹慎使用)
 *   p.reset()      // 提前釋放、或換成新的
 *   p.release()    // 放棄擁有權，回傳 raw pointer (你必須自己 delete!)
 *
 *   std::unique_ptr<Foo> q = std::move(p);   // 把擁有權從 p 轉移給 q
 *
 * 【為什麼推薦 make_unique 而不是 new】
 *   1. 一行搞定，不會忘記 delete。
 *   2. 例外安全：避免「先 new 完資源、傳參數時拋例外」造成的洩漏。
 *   3. 寫起來短：std::make_unique<Foo>(1, 2) vs std::unique_ptr<Foo>(new Foo(1, 2))
 *
 * 【對應 Leetcode】705. Design HashSet
 *   題目簡述：不用內建 hash set，自己設計 add / remove / contains。
 *   為什麼選這題：可以練習用 unique_ptr 持有「自家設計」的容器物件，
 *   也避免一定要寫 delete。
 *
 * 【參考】
 *   https://en.cppreference.com/w/cpp/memory/unique_ptr
 *=============================================================================*/


/*
補充筆記：UniquePtr
  - std::unique_ptr 表示獨占所有權：同一時間只有一個 unique_ptr 負責刪除所指物件。
  - unique_ptr 不可複製，只能 move；這避免兩個 owner 同時以為自己該 delete 同一塊記憶體。
  - std::make_unique<T>(args...) 會配置並建構 T，通常比手寫 new T 更安全，能避免例外發生時資源外洩。
  - move 之後來源 unique_ptr 會變成空指標；只能檢查、重新指定或銷毀，不應再解參考。
  - unique_ptr<T[]> 是陣列版本，但多數可變長資料應優先使用 std::vector，因為 vector 還保存大小並提供演算法介面。
  - 把 unique_ptr 傳入函式時，T* 或 T& 表示借用不取得所有權，std::unique_ptr<T> by value 表示轉移所有權。
  - release() 會放棄所有權並回傳裸指標，不會 delete；若沒有立刻交給另一個 owner，很容易洩漏。
  - reset() 會刪除目前物件並接管新指標；用它比手動 delete 再賦值安全。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::unique_ptr
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. unique_ptr 為什麼不能 copy？那怎麼從函式回傳？
//     答：copy 會造成兩個 unique_ptr 都認為自己擁有資源 → double free，所以 copy
//         constructor 與 copy assignment 被 `= delete`，它是 move-only 型別。
//         從函式回傳沒問題：回傳區域變數時會走 NRVO 或隱式 move（回傳語句中的區域
//         物件會被當成 rvalue 處理），不需要自己寫 std::move。
//     追問：傳進函式該怎麼寫？（只是使用物件 → 傳 T& / const T& / 裸 T*；
//           要接管所有權 → 傳 unique_ptr<T> by value，呼叫端須 std::move，
//           如本檔的 useSet(std::move(set))）
//
// 🔥 Q2. `release()`、`reset()`、`get()` 的差別？
//     答：`release()` 放棄所有權、內部指標置 null、回傳裸指標且「不刪除物件」
//         —— 呼叫端必須自己 delete（本檔範例 1 就示範了這點）。`reset(p)` 會
//         「先刪除目前管理的物件」再接管 p（不給參數就只是刪除並置空）。
//         `get()` 只回傳裸指標、不轉移所有權，用來呼叫 C API。
//     追問：`get()` 拿到的指標可以拿去 delete 或交給另一個 smart pointer 嗎？
//           （絕對不行 —— 會變成兩個擁有者，最後 double free）
//
// Q3. 為什麼推薦 `make_unique` 而不是 `unique_ptr<T>(new T)`？
//     答：① exception safety —— `f(unique_ptr<A>(new A), g())` 中若 g() 在 new A
//         之後、unique_ptr 建構之前拋出，A 就洩漏了（C++17 收緊求值順序後緩解，
//         但仍建議用 make）② 不必重複寫型別名 ③ 一致性。注意 `std::make_unique`
//         是 **C++14** 才加入的（C++11 漏掉了）。
//     追問：make_unique 能指定自訂 deleter 嗎？（不能，和 make_shared 一樣）
//
// ⚠️ 陷阱. `std::unique_ptr<T>` 拿去管理 `new T[n]` 配置的陣列會怎樣？
//     答：undefined behavior。`new[]` 必須配 `delete[]`，而 `unique_ptr<T>` 用的是
//         `delete`。正確做法是陣列特化 `std::unique_ptr<T[]>`（用 delete[] 釋放、
//         提供 operator[]、沒有 operator* / operator->），`make_unique<T[]>(n)`
//         自 C++14 起可用。不過實務上大多數情況直接用 `std::vector` 更好。
//     為什麼會錯：多數人以為 smart pointer 會「看情況」選擇 delete 或 delete[]；
//         實際上這是型別在編譯期就決定的，執行期完全不做判斷。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <memory>      // std::unique_ptr, std::make_unique
#include <vector>
#include <list>
#include <utility>     // std::move
#include <string>

// -----------------------------------------------------------------------------
// 範例 1：unique_ptr 基本用法
// -----------------------------------------------------------------------------
class Resource {
public:
    explicit Resource(const std::string& name) : name_(name) {
        std::cout << "  Resource(" << name_ << ") 建構\n";
    }
    ~Resource() {
        std::cout << "  ~Resource(" << name_ << ") 解構\n";
    }
    void use() { std::cout << "  使用 " << name_ << std::endl; }
private:
    std::string name_;
};

// -----------------------------------------------------------------------------
// 範例 2：對應 Leetcode 705 - Design HashSet
// -----------------------------------------------------------------------------
// 設計：分離鏈結法 (Separate Chaining)
//   - 用 BUCKETS 個桶，把 key % BUCKETS 當索引，
//   - 每個桶內用 std::list<int> 存放衝突的 key。
class MyHashSet {
private:
    static constexpr int BUCKETS = 769;       // 用質數降低碰撞
    std::vector<std::list<int>> table_;       // 769 個桶

    int hash(int key) const { return key % BUCKETS; }

public:
    MyHashSet() : table_(BUCKETS) {}

    void add(int key) {
        auto& bucket = table_[hash(key)];
        for (int v : bucket) if (v == key) return;     // 已存在不重複加
        bucket.push_back(key);
    }

    void remove(int key) {
        auto& bucket = table_[hash(key)];
        bucket.remove(key);     // std::list 的 remove 會把所有相等元素都刪掉
    }

    bool contains(int key) const {
        const auto& bucket = table_[hash(key)];
        for (int v : bucket) if (v == key) return true;
        return false;
    }
};

// 一個輔助函式：示範把 unique_ptr 「按值」傳入 (必須用 std::move 移動)
void useSet(std::unique_ptr<MyHashSet> s) {
    s->add(99);
    std::cout << "  在子函式裡 contains(99) = " << s->contains(99) << std::endl;
    // 離開函式，s 解構，HashSet 自動 delete，不用我們煩惱
}

// -----------------------------------------------------------------------------
// 範例 3：對應 Leetcode 706 - Design HashMap
// -----------------------------------------------------------------------------
// 和範例 2 對偶：除了 key，還要存 value。用 unique_ptr 持有，避免手動 delete。
class MyHashMap {
private:
    static constexpr int BUCKETS = 769;
    std::vector<std::list<std::pair<int, int>>> table_;
    int h(int key) const { return key % BUCKETS; }
public:
    MyHashMap() : table_(BUCKETS) {}
    void put(int key, int value) {
        auto& bucket = table_[h(key)];
        for (auto& kv : bucket) {
            if (kv.first == key) { kv.second = value; return; }
        }
        bucket.push_back({key, value});
    }
    int get(int key) const {
        const auto& bucket = table_[h(key)];
        for (const auto& kv : bucket) if (kv.first == key) return kv.second;
        return -1;
    }
    void remove(int key) {
        auto& bucket = table_[h(key)];
        for (auto it = bucket.begin(); it != bucket.end(); ++it) {
            if (it->first == key) { bucket.erase(it); return; }
        }
    }
};

// -----------------------------------------------------------------------------
// 範例 4：日常實用 - 用 unique_ptr 持有 Shape 多型物件
// -----------------------------------------------------------------------------
// 把不同形狀放到 vector<unique_ptr<...>>，自動釋放、無需手動 delete。
class Shape20 {
public:
    virtual double area() const = 0;
    virtual ~Shape20() = default;
};
class Circle20 : public Shape20 {
    double r_;
public:
    explicit Circle20(double r) : r_(r) {}
    double area() const override { return 3.14159 * r_ * r_; }
};
class Square20 : public Shape20 {
    double s_;
public:
    explicit Square20(double s) : s_(s) {}
    double area() const override { return s_ * s_; }
};

int main() {
    std::cout << "===== 範例 1：unique_ptr 基本 =====" << std::endl;

    {
        std::unique_ptr<Resource> p = std::make_unique<Resource>("DB-Connection");
        p->use();
        // 離開區塊 → p 解構 → 內部 delete → ~Resource 自動跑
    }

    std::cout << "----- 移動 (move) 而非複製 -----" << std::endl;
    {
        std::unique_ptr<Resource> a = std::make_unique<Resource>("A");
        // std::unique_ptr<Resource> b = a;     // ← 編譯錯誤！不能複製
        std::unique_ptr<Resource> b = std::move(a);  // OK：把所有權搬給 b
        if (!a) std::cout << "  a 已經沒有東西了\n";
        b->use();
    }

    std::cout << "----- reset / release -----" << std::endl;
    {
        std::unique_ptr<Resource> p = std::make_unique<Resource>("X");
        p.reset();      // 提前釋放
        std::cout << "  reset 後 p 是空的: " << (p ? "no" : "yes") << "\n";
        p.reset(new Resource("Y"));   // 重新指向新物件 (不建議；用 make_unique 更佳)
        Resource* raw = p.release();  // 放棄擁有權 → 此時你「必須自己 delete」!!
        std::cout << "  release 之後 unique_ptr 為空: " << (p ? "no" : "yes") << "\n";
        delete raw;                   // 不 delete 就洩漏
    }

    std::cout << "===== 範例 2：Leetcode 705 HashSet =====" << std::endl;
    auto set = std::make_unique<MyHashSet>();
    set->add(1);
    set->add(2);
    std::cout << "contains(1) = " << set->contains(1) << std::endl;   // 1
    std::cout << "contains(3) = " << set->contains(3) << std::endl;   // 0
    set->add(2);                                                       // 重複加沒事
    set->remove(2);
    std::cout << "remove(2) 後 contains(2) = " << set->contains(2) << std::endl; // 0

    // 把 set 的所有權搬進子函式
    useSet(std::move(set));
    std::cout << "main 中 set 還在嗎? " << (set ? "yes" : "no")
              << " (move 後預期 no)" << std::endl;

    std::cout << "===== 範例 3：Leetcode 706 HashMap =====" << std::endl;
    auto map = std::make_unique<MyHashMap>();
    map->put(1, 1);
    map->put(2, 2);
    std::cout << "get(1) = " << map->get(1) << std::endl;     // 1
    std::cout << "get(3) = " << map->get(3) << std::endl;     // -1
    map->put(2, 1);                                            // 覆蓋 key=2
    std::cout << "get(2) = " << map->get(2) << std::endl;     // 1
    map->remove(2);
    std::cout << "remove(2) 後 get(2) = " << map->get(2) << std::endl;  // -1

    std::cout << "===== 範例 4：unique_ptr + 多型形狀 =====" << std::endl;
    std::vector<std::unique_ptr<Shape20>> shapes;
    shapes.push_back(std::make_unique<Circle20>(3));
    shapes.push_back(std::make_unique<Square20>(4));
    double total = 0;
    for (const auto& sp : shapes) total += sp->area();
    std::cout << "面積總和 = " << total << std::endl;
    // shapes 解構 → 自動釋放每個物件 (Shape20 解構子是 virtual)
    return 0;
}

/* 預期輸出（順序固定）：
 * ===== 範例 1：unique_ptr 基本 =====
 *   Resource(DB-Connection) 建構
 *   使用 DB-Connection
 *   ~Resource(DB-Connection) 解構
 * ----- 移動 (move) 而非複製 -----
 *   Resource(A) 建構
 *   a 已經沒有東西了
 *   使用 A
 *   ~Resource(A) 解構
 * ----- reset / release -----
 *   Resource(X) 建構
 *   ~Resource(X) 解構
 *   reset 後 p 是空的: yes
 *   Resource(Y) 建構
 *   release 之後 unique_ptr 為空: yes
 *   ~Resource(Y) 解構
 * ===== 範例 2：Leetcode 705 HashSet =====
 * contains(1) = 1
 * contains(3) = 0
 * remove(2) 後 contains(2) = 0
 *   在子函式裡 contains(99) = 1
 * main 中 set 還在嗎? no (move 後預期 no)
 * ===== 範例 3：Leetcode 706 HashMap =====
 * get(1) = 1
 * get(3) = -1
 * get(2) = 1
 * remove(2) 後 get(2) = -1
 * ===== 範例 4：unique_ptr + 多型形狀 =====
 * 面積總和 = 44.2743
 */

/*=============================================================================
 * 【本篇重點回顧】
 *   1. unique_ptr 表示「獨占擁有權」，物件離開作用域時自動 delete。
 *   2. 不能複製 (copy)，但可以移動 (move) 把擁有權交出去。
 *   3. 用 std::make_unique 建立，避免直接 new。
 *   4. .get() 拿 raw pointer 觀察用 / 傳給 C API；release() 放棄管理 (慎用)。
 *   5. 預設用 unique_ptr，需要共享所有權再用 shared_ptr (下一篇)。
 *
 * 【下一篇預告】
 *   21_SharedPtr.cpp
 *   std::shared_ptr — 多個指標共享一個資源，引用計數歸零才釋放。
 *=============================================================================*/

// 編譯: g++ -std=c++20 -Wall -Wextra 20_UniquePtr.cpp -o 20_UniquePtr

// === 預期輸出 (節錄) ===
// ===== 範例 1：unique_ptr 基本 =====
//   Resource(DB-Connection) 建構
//   使用 DB-Connection
//   ~Resource(DB-Connection) 解構
// ----- 移動 (move) 而非複製 -----
//   Resource(A) 建構
//   a 已經沒有東西了
//   使用 A
//   ~Resource(A) 解構
// ----- reset / release -----
//   Resource(X) 建構
//   ~Resource(X) 解構
//   reset 後 p 是空的: yes
//   Resource(Y) 建構
//   release 之後 unique_ptr 為空: yes
//   ~Resource(Y) 解構
// ===== 範例 2：Leetcode 705 HashSet =====
// contains(1) = 1
// contains(3) = 0
// remove(2) 後 contains(2) = 0
// …（後略，完整輸出共 29 行）
