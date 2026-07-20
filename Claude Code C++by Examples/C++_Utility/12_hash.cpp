/*
================================================================================
主題:std::hash —— 給型別一個雜湊值,讓它能放進 unordered_map / unordered_set
標準:C++11 起
標頭:<functional>
參考:https://en.cppreference.com/w/cpp/utility/hash
================================================================================

【一、課題介紹】
  std::unordered_map<K, V>、std::unordered_set<K> 內部用「雜湊表」實作,
  必須有辦法把 K 轉成一個整數位置(slot)。負責這件事的就是 std::hash<K>。

  C++ 標準函式庫已經為大多數內建型別提供 std::hash 的特化:
    - 整數家族:bool、char、short、int、long、long long(及其 unsigned)
    - 浮點:float、double、long double
    - 指標:T*
    - 字串:std::string、std::wstring、std::string_view 等
    - 智慧指標:std::shared_ptr / std::unique_ptr

  但對「自訂型別」(例如 struct Point),沒有預設雜湊函式;這時你需要自己
  特化 std::hash<Point>(或提供一個自訂 Hasher 函式物件)。

【二、觀念解釋】
  1. 標頭:<functional>。
  2. 用法(對內建型別):
       std::hash<int> h;
       std::size_t v = h(42);         // 拿到 size_t 雜湊值
  3. 對自訂型別,有兩種做法:
       (a) 提供 std::hash 的特化(放在 std 命名空間內,規範允許):
            namespace std {
              template<> struct hash<Point> {
                std::size_t operator()(const Point& p) const noexcept {
                  return hash<int>()(p.x) ^ (hash<int>()(p.y) << 1);
                }
              };
            }
       (b) 寫一個 Hasher 結構,當作 unordered_map 的第三個樣板參數:
            struct PointHash { std::size_t operator()(const Point&) const; };
            std::unordered_map<Point, V, PointHash> m;
  4. 組合多個欄位的雜湊:
       簡單合成可用 XOR + shift,但碰撞率不佳。生產環境推薦類似 boost::hash_combine:
         h ^= hasher(value) + 0x9e3779b9 + (h << 6) + (h >> 2);
  5. 雜湊函式必須對「相等的 key」回傳相同雜湊值,而且要 noexcept。

【三、常見陷阱】
  - 兩物件相等(operator==)就一定要雜湊相同,否則 unordered_map 會壞掉。
  - 直接 XOR 各欄位雜湊容易碰撞(例如 (1,2) 與 (2,1) 變成同雜湊),
    對性能敏感的程式應使用 hash_combine。
  - std::hash<T> 對相同輸入的回傳值「不保證跨平台一致」,別拿來當持久化雜湊。

【四、與其他 utility 的比較】
  - vs std::map(紅黑樹):map 要求 key 可排序、查詢 O(log n);
    unordered_map 要求 key 可雜湊、平均查詢 O(1)。
  - vs 自寫整數編碼:對小型 struct 直接 (x * P + y) 編碼也行,但用 std::hash
    更通用、可讀性高。

【五、Leetcode 對應題目】
  題號:1. Two Sum(兩數之和)
  難度:Easy
  連結:https://leetcode.com/problems/two-sum/
  選用理由:Two Sum 標準解法用 unordered_map<int,int>,正好證明 std::hash<int>
            是預設可用的,不需要任何特化。

【六、日常工作實用範例】
  情境:用 (host, port) 當 unordered_set 的 key,需要為 std::pair 提供雜湊。
        這是日常會遇到、又很容易卡住的問題,因為 std 沒有為 pair 預設提供 hash!
================================================================================
*/

/*
補充筆記：std::hash
  - std::hash 屬於 utility 類工具；這些型別與函式常用來表達小型資料組合、可選值、型別安全聯集或值類別轉換。
  - pair/tuple 適合簡短聚合結果，但欄位語意複雜時應定義具名 struct，避免 first/second 或 get<0> 難讀。
  - optional 表示可能沒有值，使用前要檢查 has_value 或使用 value_or；value() 在無值時會丟例外。
  - variant 表示多選一型別，應用 visit 或 holds_alternative/get_if 安全存取目前替代項。
  - any 提供執行期任意型別保存，但取回需要知道正確型別；過度使用會失去靜態型別檢查優勢。
  - std::move/std::forward/std::exchange/as_const 都是表達意圖的工具；它們本身不一定搬移或複製資料。
  - std::hash 提供 unordered 容器的預設 hash；自訂 key 要讓相等物件產生相同 hash。
  - hash 不要求跨執行保持穩定，不能把 std::hash 結果當持久化檔案格式。
  - 好的 hash 應混合所有參與 equality 的欄位；漏欄位會造成大量 collision 或語意不一致。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::hash 與 unordered 容器
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 要把自訂型別放進 unordered_map／unordered_set，需要提供什麼？
//     答：兩樣東西：① hash 函式 ② 相等比較。做法是特化 std::hash<MyType>（需提供
//     size_t operator()(const MyType&) const noexcept），或把 hasher 當作容器的第三個
//     模板參數傳入；相等比較通常直接提供 operator==。
//     追問：兩者的分工是什麼？（hash 決定放進哪個 bucket，operator== 在同一個 bucket
//     內區分不同的鍵）
//
// 🔥 Q2. 多個欄位的 hash 怎麼組合？為什麼不能直接 XOR？
//     答：直接 h1 ^ h2 是對稱的，{1, 2} 與 {2, 1} 會得到相同的雜湊值，讓大量鍵擠進同一
//     個 bucket。常見做法是類似 boost 的混合式：
//     seed ^= h + 0x9e3779b9 + (seed << 6) + (seed >> 2);
//     重點是引入「位置相關」與擴散，而不只是把位元疊在一起。
//
// ⚠️ 陷阱. 兩個 operator== 相等的物件，hash 值不同會怎樣？
//     答：這會破壞容器的不變式，行為未定義——同一個鍵可能被算到不同 bucket，於是
//     find() 找不到剛剛 insert 進去的東西。反過來「hash 相同但物件不等」則完全正常，
//     那只是碰撞，由 operator== 解決。
//     為什麼會錯：直覺覺得「hash 只是加速用的提示，錯了頂多慢一點」；但容器是靠
//     「相等 ⇒ 同 hash」這條不變式在做查找的，違反它就是真 bug。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

// ---------------------------------------------------------------------------
// 範例 1:對內建型別呼叫 std::hash
// ---------------------------------------------------------------------------
void demo_basic() {
    std::cout << "[demo_basic]\n";
    std::hash<int>         h_int;
    std::hash<std::string> h_str;
    std::cout << "  hash(42)        = " << h_int(42) << "\n";
    std::cout << "  hash(\"hello\")   = " << h_str("hello") << "\n";
    std::cout << "  hash(\"hello\")   = " << h_str("hello") << "  (same input -> same hash)\n";
}

// ---------------------------------------------------------------------------
// 範例 1.5:更多內建型別的 std::hash + std::hash 特化(法 a)
//
// 標準函式庫對「大多數常見型別」都已提供 std::hash 特化,本段先把常見幾個跑過。
// 接著示範「在 std 命名空間裡特化 std::hash<Point>」這條路 —— 之後 Point
// 就可以「直接」當 unordered_map 的 key,不必傳第三個樣板參數。
// ---------------------------------------------------------------------------
void demo_hash_for_more_builtins() {
    std::cout << "[demo_hash_for_more_builtins]\n";

    std::cout << "  hash(true)      = " << std::hash<bool>{}(true)            << "\n";
    std::cout << "  hash('A')       = " << std::hash<char>{}('A')             << "\n";
    std::cout << "  hash(123LL)     = " << std::hash<long long>{}(123LL)      << "\n";
    std::cout << "  hash(3.14)      = " << std::hash<double>{}(3.14)          << "\n";

    int x = 42;
    int* px = &x;
    std::cout << "  hash(int*)      = " << std::hash<int*>{}(px) << "\n";

    // string_view 在 C++17 起亦有預設特化
    std::string_view sv = "hello";
    std::cout << "  hash(string_view)=" << std::hash<std::string_view>{}(sv) << "\n";
}

// 自訂型別:Point —— 在 std 命名空間特化 std::hash<Point>
struct Point {
    int x, y;
    bool operator==(const Point& o) const noexcept { return x == o.x && y == o.y; }
};
namespace std {
    template <>
    struct hash<Point> {
        std::size_t operator()(const Point& p) const noexcept {
            std::size_t h = std::hash<int>{}(p.x);
            // hash_combine
            h ^= std::hash<int>{}(p.y) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
            return h;
        }
    };
}

void demo_hash_specialization() {
    std::cout << "[demo_hash_specialization]\n";

    // 因為 std::hash<Point> 已被特化,Point 可直接當 unordered_set 的 key,
    // 不用再傳第三個樣板參數(這正是「特化 std::hash」相對「自訂 Hasher」
    // 最直觀的好處)。
    std::unordered_set<Point> visited;
    visited.insert({0, 0});
    visited.insert({1, 2});
    visited.insert({0, 0});                          // 重複,不會增加
    std::cout << "  visited.size = " << visited.size() << "\n";

    // 直接當函式物件用也可以
    Point p{3, 4};
    std::cout << "  hash(Point{3,4}) = " << std::hash<Point>{}(p) << "\n";
}

// ---------------------------------------------------------------------------
// 範例 2:Leetcode #1 Two Sum 用 unordered_map<int,int>
//
// 這證明 std::hash<int> 是預設可用,不需要做任何事。
// ---------------------------------------------------------------------------
std::pair<int,int> twoSum(const std::vector<int>& nums, int target) {
    std::unordered_map<int, int> seen;
    for (int i = 0; i < static_cast<int>(nums.size()); ++i) {
        int need = target - nums[i];
        auto it = seen.find(need);
        if (it != seen.end()) return {it->second, i};
        seen[nums[i]] = i;
    }
    return {-1, -1};
}

void demo_leetcode_two_sum() {
    std::cout << "[demo_leetcode_two_sum]\n";
    std::vector<int> nums = {2, 7, 11, 15};
    auto [i, j] = twoSum(nums, 9);
    std::cout << "  index pair = (" << i << ", " << j << ")\n";
}

// ---------------------------------------------------------------------------
// 範例 3:日常工作實用範例 —— 為 std::pair<string,int> 提供 Hasher
//
// 情境:把網路端點 (host, port) 放進 unordered_set 表示「已連線清單」。
//       std 沒有為 pair 預設提供 hash,所以我們提供一個自訂 Hasher。
//
// 這裡示範比 XOR 更穩的「hash_combine」(取自 boost 的常見手法)。
// ---------------------------------------------------------------------------
struct PairHash {
    template <class A, class B>
    std::size_t operator()(const std::pair<A, B>& p) const noexcept {
        std::size_t h1 = std::hash<A>{}(p.first);
        std::size_t h2 = std::hash<B>{}(p.second);
        // boost::hash_combine 的常見實作
        h1 ^= h2 + 0x9e3779b97f4a7c15ULL + (h1 << 6) + (h1 >> 2);
        return h1;
    }
};

void demo_practical_endpoint_set() {
    std::cout << "[demo_practical_endpoint_set]\n";
    using Endpoint = std::pair<std::string, int>;
    std::unordered_set<Endpoint, PairHash> connected;

    connected.insert({"127.0.0.1", 8080});
    connected.insert({"192.168.0.5", 22});
    connected.insert({"127.0.0.1", 8080});        // 重複,不會被加入

    std::cout << "  connected.size = " << connected.size() << "\n";
    for (const auto& ep : connected) {
        std::cout << "    " << ep.first << ":" << ep.second << "\n";
    }
}

// ---------------------------------------------------------------------------
// 實用範例 (額外):cache key —— 把多欄位組成穩定 hash
//
// 工作中常見:對 (userId, route, locale) 三欄位查 cache。我們建一個 CacheKey
// struct, 實作 operator== 與 std::hash 特化, 便可直接當 unordered_map 的 key。
// ---------------------------------------------------------------------------
struct CacheKey {
    int userId;
    std::string route;
    std::string locale;
    bool operator==(const CacheKey& o) const noexcept {
        return userId == o.userId && route == o.route && locale == o.locale;
    }
};
namespace std {
    template <> struct hash<CacheKey> {
        std::size_t operator()(const CacheKey& k) const noexcept {
            std::size_t h = std::hash<int>{}(k.userId);
            h ^= std::hash<std::string>{}(k.route)  + 0x9e3779b97f4a7c15ULL + (h<<6) + (h>>2);
            h ^= std::hash<std::string>{}(k.locale) + 0x9e3779b97f4a7c15ULL + (h<<6) + (h>>2);
            return h;
        }
    };
}

void demo_practical_cache_key() {
    std::cout << "[demo_practical_cache_key]\n";
    std::unordered_map<CacheKey, std::string> cache;
    cache[{1, "/home", "zh-TW"}] = "home_zh";
    cache[{1, "/home", "en-US"}] = "home_en";
    cache[{2, "/about","en-US"}] = "about_en";
    std::cout << "  cache size = " << cache.size() << "\n";
    auto it = cache.find({1, "/home", "zh-TW"});
    if (it != cache.end()) std::cout << "  hit: " << it->second << "\n";
}

int main() {
    demo_basic();
    demo_hash_for_more_builtins();
    demo_hash_specialization();
    demo_leetcode_two_sum();
    demo_practical_endpoint_set();
    demo_practical_cache_key();
    return 0;
}

/*
================================================================================
編譯與執行:
    g++ -std=c++17 -Wall -Wextra 12_hash.cpp -o 12_hash && ./12_hash
================================================================================
*/

// 編譯: g++ -std=c++20 -Wall -Wextra 12_hash.cpp -o 12_hash

// === 預期輸出 ===
// [demo_basic]
//   hash(42)        = 42
//   hash("hello")   = 2762169579135187400
//   hash("hello")   = 2762169579135187400  (same input -> same hash)
// [demo_hash_for_more_builtins]
//   hash(true)      = 1
//   hash('A')       = 65
//   hash(123LL)     = 123
//   hash(3.14)      = 5464867211497793177
//   hash(int*)      = 140737271011796
//   hash(string_view)=2762169579135187400
// [demo_hash_specialization]
//   visited.size = 2
//   hash(Point{3,4}) = 11400714819323198682
// [demo_leetcode_two_sum]
//   index pair = (0, 1)
// [demo_practical_endpoint_set]
//   connected.size = 2
//     192.168.0.5:22
//     127.0.0.1:8080
// [demo_practical_cache_key]
//   cache size = 3
//   hit: home_zh
