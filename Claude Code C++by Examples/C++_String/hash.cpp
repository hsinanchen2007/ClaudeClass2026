// =============================================================================
// 檔名: hash.cpp
// 主題: std::hash<std::basic_string> (字串的標準雜湊特化)
// 參考連結 References:
//   cppreference: https://en.cppreference.com/cpp/string/basic_string/hash
//   cplusplus.com: https://cplusplus.com/reference/functional/hash/
// =============================================================================
//
// 【函式資訊 Information】
//   template<class CharT, class Traits, class Alloc>
//   struct hash<std::basic_string<CharT, Traits, Alloc>>;       // since C++11
//
//   // 使用方式:
//   std::hash<std::string> h;
//   size_t v = h(some_string);                                  // size_t 雜湊值
//
// 對應特化亦存在於:
//   std::hash<std::wstring>, std::hash<std::u8string>,
//   std::hash<std::u16string>, std::hash<std::u32string>,
//   std::hash<std::pmr::string> 等。
//
// =============================================================================
//
// 【詳細解釋 Explanation】
//
// (一) 為什麼 STL 要為 string 特化 hash?
// ----------------------------------------------------------------------------
// 因為 std::string 是 std::unordered_set / unordered_map 的 key 中,
// 出現頻率最高的型別之一。如果標準沒有為它特化 hash,使用者每次寫
// unordered_map<string, V> 就要自訂 hash 函式 —— 這顯然不合理。
// C++11 在引入無序容器的同時,就為 string 系列提供現成 hash 特化。
//
// (二) 雜湊與 string_view 的關係 (重要!)
// ----------------------------------------------------------------------------
// 標準保證:對於相同內容的 std::string、std::string_view,雜湊值「相等」。
// 也就是說,std::hash<std::string>{}("abc") 與
//          std::hash<std::string_view>{}("abc") 必須回傳同一個值。
//
// 這是「heterogeneous lookup」(異質查找) 能成立的基礎 —— 你可以拿
// string_view 直接到 unordered_map<string, V> 裡查 (C++20 起,搭配
// 自訂 transparent hash + transparent equal_to)。
//
// (三) 同一份 string 的 hash 是 deterministic 的嗎?
// ----------------------------------------------------------------------------
// 在「同一個程式執行內」一定相同。但「跨程式執行」、「跨平台」、
// 甚至「跨 libstdc++ 版本」**不保證相同**。理由是現代 STL 為了抵禦
// hash flooding (DoS 攻擊),會在程式啟動時加入 random seed。
// 所以:
//   - 不要把 hash 結果寫入持久化檔案、網路協定、cache key。
//   - 真正需要「跨程式同值」的雜湊應使用 SipHash、xxHash、CityHash
//     之類的明確算法 (自己 include / 自己實作)。
//
// (四) 內部演算法概要
// ----------------------------------------------------------------------------
// 標準沒有指定具體演算法。常見實作:
//   - libstdc++:Murmur 衍生 (gcc 7 起 +seed)。
//   - libc++:加 seed 的線性 mix。
//   - MSVC STL:FNV-1a 變體。
// 共通特性:
//   - 對長度線性 O(n)。
//   - 對短字串高度乘法 + xor 混合,避免大量短字串聚集到同一 bucket。
//
// (五) 自訂 struct 想當 unordered_map 的 key 時
// ----------------------------------------------------------------------------
// 沒有預設 hash;你必須:
//   - 寫一個 hash function 物件 (struct with operator()),
//   - 或特化 std::hash<MyType> (用法上更隱式)。
// 常見做法:把 struct 的成員都用 std::hash 算出 size_t,再做 hash_combine
// (混合)。boost::hash_combine 是著名的範本:
//   inline void hash_combine(size_t& seed, size_t v) {
//       seed ^= v + 0x9e3779b9 + (seed << 6) + (seed >> 2);
//   }
//
// (六) C++ 標準演進
// ----------------------------------------------------------------------------
//   C++11    : 加入 std::hash<basic_string> 特化。
//   C++17    : 加入 std::hash<basic_string_view> 特化,且保證同值。
//   C++20    : transparent hash + heterogeneous lookup (透過 is_transparent
//              tag) 變成標準正式做法。
//   C++23    : std::pmr::hash 行為與 std::hash 相容。
//
// =============================================================================
//
// 【概念補充 Concept Deep Dive】
//
// 1) Hash Flooding (DoS) 攻擊
//    若雜湊函式可被預測,攻擊者可故意送一堆「同 hash 但不同內容」的
//    key,讓 unordered_map 退化為 linked list,把 O(1) 變成 O(n)。
//    現代 STL 引入 random seed 就是為了讓攻擊者無法事先計算碰撞集。
//    這也是「不要把 hash 結果跨程式重用」的根本原因。
//
// 2) Transparent Hashing (C++20)
//    給 unordered_set/map 加上 is_transparent tag,可讓 find / contains
//    接受不同型別的 key (例如用 string_view 查 unordered_map<string, V>),
//    避免每次查找都建一個臨時 string。設定:
//        struct sv_hash {
//            using is_transparent = void;   // 標記 transparent
//            size_t operator()(std::string_view sv) const noexcept {
//                return std::hash<std::string_view>{}(sv);
//            }
//        };
//        struct sv_eq {
//            using is_transparent = void;
//            bool operator()(std::string_view a, std::string_view b) const noexcept {
//                return a == b;
//            }
//        };
//        std::unordered_map<std::string, V, sv_hash, sv_eq> m;
//        m.find(std::string_view{"foo"});  // 不再造臨時 string
//
// 3) Hash Quality 與 Bucket 數量
//    unordered_map 內部有 bucket 陣列;優良 hash 會把 key 「均勻」分散
//    到 buckets。可用 m.load_factor()、m.bucket_count() 觀察。
//    如果你的 string 多為「人名 / 短英文 word」,標準 hash 已足夠。
//    若是「URL / SHA / 結構化字串」且效能敏感,可考慮 xxHash3、
//    Google CityHash 等專業函式。
//
// 4) noexcept
//    std::hash 的 operator() 對標準型別都是 noexcept;這對 unordered_map
//    的 strong exception guarantee 很重要。自訂 hash 也應寫 noexcept。
//
// 5) 雜湊與 equality 的對等義務
//    標準合約:a == b 必蘊含 hash(a) == hash(b)。反之不必。
//    如果你的 == 比較忽略大小寫,你的 hash 也必須忽略大小寫,否則
//    unordered_map 會行為錯誤 (找不到應該存在的 key)。
//
// =============================================================================
//
// 【注意事項 Pay Attention】
// 1. hash 結果跨執行/跨平台/跨版本不保證相同 —— 不可持久化。
// 2. std::hash<std::string> 與 std::hash<std::string_view> 對相同內容
//    必相等;這是異質查找的基石。
// 3. 自訂 == 與自訂 hash 必須一致 (前者蘊含後者)。
// 4. noexcept 是強制慣例;為效能著想自訂 hash 也應 noexcept。
// 5. 對極大 key 集合,建議自己 reserve(n / load_factor) 預先擴 bucket。
// 6. 別自己實作劣質 hash (例如「把 char 加總」) —— 會導致大量碰撞。
//
// =============================================================================

/*
補充筆記：std::string::hash
  - std::string::hash 需要同時考慮內容、容量與舊指標失效；字串 API 常會配置或搬移緩衝區。
  - 回傳位置的函式要先處理 npos，回傳 pointer/view 的函式要追蹤來源 string 生命週期。
  - UTF-8 文字下，size 代表位元組數，不代表使用者看到的字元數。
  - std::string::hash 是 std::string 主題的一部分；先分清楚這個操作會讀取、追加、插入、刪除、搜尋，還是只暴露底層字元。
  - std::string 的索引型別是 size_type；和 int 混用時要小心 unsigned underflow，尤其是倒著迴圈或拿 npos 比較。
  - 修改字串內容或容量可能讓先前取得的 iterator、reference、pointer、c_str() 指標失效。
  - operator[] 不做範圍檢查，at() 越界會丟 std::out_of_range；教學範例可比較兩者，工作程式要依錯誤處理需求選。
  - find/rfind/find_first_of 這類搜尋函式失敗回傳 std::string::npos；判斷時應和 npos 比，不要寫成 >= 0。
  - 字串和 C string 互通時要記得 null terminator；std::string 可以保存內含 \0 的資料，但很多 C API 會在第一個 \0 停止。
  - 若只是傳入唯讀文字片段且不需要擁有資料，std::string_view 可避免複製；但它不能延長原字串生命週期。
  - 處理中文或 UTF-8 時，std::string 的 size() 回傳 byte 數，不是人眼看到的字元數。
*/
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

void demoHash() {
    std::cout << "=== std::hash<std::string> 基本用法 ===\n";
    std::hash<std::string> hs;
    std::cout << "hash(\"hello\") = " << hs("hello") << "\n";
    std::cout << "hash(\"world\") = " << hs("world") << "\n";

    std::cout << "\n=== std::hash<std::string> 與 std::hash<string_view> 對相同內容相等 ===\n";
    std::hash<std::string_view> hv;
    std::cout << std::boolalpha
              << (hs("hello") == hv("hello")) << "\n";   // true
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例】LeetCode 49. Group Anagrams (Medium)
//
// 題目敘述:
//   給字串陣列 strs,把彼此為 anagram (字母重排) 的字串歸成同一組。
//   範例: ["eat","tea","tan","ate","nat","bat"]
//        → [["bat"],["nat","tan"],["ate","eat","tea"]]
//
// 為何用 std::hash<std::string>:
//   經典做法:把每個字串排序後當 key,用 unordered_map<string, vector<string>>
//   分組。string 的 hash 由標準提供,我們不必自己寫,unordered_map 即可
//   高效插入與查找。這直接示範了「string 為 key 的最常見容器組合」。
//
// 解題思路:
//   每個字串複製排序 → 用結果當 key → 推進對應 group。
//
// 複雜度: 時間 O(N · K log K) (K 為平均字串長度),空間 O(N · K)
// -----------------------------------------------------------------------------
#include <algorithm>
#include <vector>
std::vector<std::vector<std::string>>
groupAnagrams(std::vector<std::string>& strs) {
    std::unordered_map<std::string, std::vector<std::string>> bucket;
    for (auto& s : strs) {
        std::string key = s;
        std::sort(key.begin(), key.end());
        bucket[key].push_back(s);
    }
    std::vector<std::vector<std::string>> out;
    out.reserve(bucket.size());
    for (auto& [k, v] : bucket) out.push_back(std::move(v));
    return out;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 Daily Work】HTTP header 名稱「忽略大小寫」的高效 dedupe
//
// 為何用 std::hash:
//   HTTP header 名稱是 case-insensitive (RFC 7230 §3.2)。後端 server
//   要把同一個 header 的不同寫法 ("Content-Type" / "content-type")
//   視為同一 key。傳統做法是先 lowercase 再放進 map,但每次 lookup
//   都要造臨時 lowercase string,有額外配置。
//
//   現代做法:自訂一對 transparent ci_hash + ci_eq,讓 unordered_map
//   接受任何 string-like 來源,且 hash/equality 都忽略大小寫。
//   一次配置鍵 (頭一次 insert),之後 lookup 全程零拷貝。
//
//   這個 pattern 是 HTTP server / gRPC metadata / 環境變數 (Windows)
//   等場景的「殺手鐧」做法。
// -----------------------------------------------------------------------------
struct CiHash {
    using is_transparent = void;
    size_t operator()(std::string_view sv) const noexcept {
        // 簡化版:對每字 lowercase 後做 FNV-1a;真實場景請用標準提供的
        // hash 加上 case-fold,以保證 quality。
        size_t h = 1469598103934665603ull;
        for (char c : sv) {
            unsigned char uc = static_cast<unsigned char>(
                std::tolower(static_cast<unsigned char>(c)));
            h ^= uc;
            h *= 1099511628211ull;
        }
        return h;
    }
};
struct CiEq {
    using is_transparent = void;
    bool operator()(std::string_view a, std::string_view b) const noexcept {
        if (a.size() != b.size()) return false;
        for (size_t i = 0; i < a.size(); ++i) {
            if (std::tolower(static_cast<unsigned char>(a[i])) !=
                std::tolower(static_cast<unsigned char>(b[i]))) return false;
        }
        return true;
    }
};

void demoHttpHeaders() {
    std::cout << "\n=== 日常實務: case-insensitive HTTP headers ===\n";
    std::unordered_map<std::string, std::string, CiHash, CiEq> headers;
    headers.emplace("Content-Type", "application/json");
    headers.emplace("Authorization", "Bearer abc.def");

    // 用不同大小寫版本查找,皆能命中,且不需建立臨時 std::string
    std::cout << "find content-type → "
              << headers.find(std::string_view{"content-type"})->second << "\n";
    std::cout << "find AUTHORIZATION → "
              << headers.find(std::string_view{"AUTHORIZATION"})->second << "\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例 2】LeetCode 1location7. 1location7. Unique Number of Occurrences -字串版
// 題目: LeetCode 1207 變奏 ── 給字串陣列,判斷各字串「出現次數」是否互異。
// 為何用 hash: unordered_map<string,int> 是計數最簡寫法;再用 set 看次數有無重複。
// 複雜度: O(N)。
// -----------------------------------------------------------------------------
#include <unordered_set>
bool uniqueOccurrences(const std::vector<std::string>& arr) {
    std::unordered_map<std::string, int> cnt;
    for (const auto& s : arr) ++cnt[s];
    std::unordered_set<int> seen;
    for (auto& [k, v] : cnt) {
        if (!seen.insert(v).second) return false;
    }
    return true;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】Idempotency-Key 去重 (HTTP 服務防重送)
// 為何用 hash: unordered_set<string> O(1) 查表,server 收到請求先看 key 是否存在。
//              金流、Webhook 處理的標準做法。
// -----------------------------------------------------------------------------
class IdempotencyStore {
    std::unordered_set<std::string> keys;
public:
    bool tryRegister(const std::string& key) {
        return keys.insert(key).second;     // 第一次回 true、之後回 false
    }
    size_t size() const { return keys.size(); }
};

int main() {
    demoHash();

    std::cout << "\n=== LeetCode 49 ===\n";
    std::vector<std::string> in = {"eat","tea","tan","ate","nat","bat"};
    auto groups = groupAnagrams(in);
    for (auto& g : groups) {
        std::cout << "[ ";
        for (auto& s : g) std::cout << s << " ";
        std::cout << "]\n";
    }

    std::cout << "\n=== LeetCode 1207 變奏 ===\n";
    std::cout << std::boolalpha
              << uniqueOccurrences({"a","b","a","c","c","c"}) << "\n"   // true (a=2,b=1,c=3)
              << uniqueOccurrences({"a","a","b","b"})         << "\n";  // false (a=2,b=2)

    demoHttpHeaders();

    std::cout << "\n=== 日常實務: IdempotencyStore ===\n";
    IdempotencyStore store;
    std::cout << store.tryRegister("req-001") << "\n";    // 1
    std::cout << store.tryRegister("req-001") << "\n";    // 0 (重送)
    std::cout << store.tryRegister("req-002") << "\n";    // 1
    std::cout << "stored=" << store.size() << "\n";       // 2

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1:同一個字串 "hello",在不同程式 / 平台 / libstdc++ 版本下 hash 值會一樣嗎?
    //    A:不保證。現代 STL 為防 hash flooding DoS 攻擊,會在程式啟動時加
    //       random seed,跨執行就不同。所以「不要把 hash 結果寫入檔案、網路
    //       協定、cache key」 — 真要持久化用 SipHash / xxHash 等明確算法。
    //
    //  Q2:std::hash<std::string>{}("abc") 跟 std::hash<std::string_view>{}("abc")
    //       是否相等?
    //    A:標準保證相等 (C++17 起)。這是 transparent hashing / heterogeneous
    //       lookup 的基礎 — 才能讓 unordered_map<string, V> 用 string_view
    //       直接查找,不用每次造臨時 string。
    //
    //  Q3:自訂 == 與自訂 hash 必須遵守什麼合約?
    //    A:a == b 必蘊含 hash(a) == hash(b)。反向不必。若 == 是大小寫不敏感
    //       (ci_eq),hash 也必須做 case-fold (ci_hash),否則 unordered_map
    //       會找不到應該存在的 key。自訂 hash 也應該標 noexcept 以便容器
    //       維持 strong exception guarantee。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra hash.cpp -o hash

// === 預期輸出 (節錄) ===
// === LeetCode 1207 變奏 ===
// true
// false
// === 日常實務: IdempotencyStore ===
// 1
// 0
// 1
// stored=2
