// ============================================================================
//  23_policy_based_design.cpp  ──  Policy-Based Design
// ============================================================================
//
//  【本篇目標】
//    Policy-Based Design 由 Andrei Alexandrescu 在《Modern C++ Design》發
//    揚光大。核心想法：
//      把類別的「不同變化點 (varying behaviors)」拆成獨立的 policy class，
//      最終類別由多個 policy 組合而成。
//
//    例如一個 thread-safe 容器：
//      - 同步策略：Mutex / SpinLock / NoLock
//      - 配置策略：Heap / Stack / Pool
//      - 比較策略：Less / Greater / Custom
//    把這些做成 policy，使用者按需求挑選。
//
//  【與 Strategy Pattern 的差別】
//    GoF Strategy Pattern：
//      - runtime 透過介面 (虛擬函式) 注入策略
//      - 換策略不必重編譯
//
//    Policy-Based Design：
//      - 編譯期 template 注入策略
//      - 完全 inline，零 runtime cost
//      - 換策略要重編譯
//
//  【典型語法】
//
//        template <typename T,
//                  typename ThreadingPolicy = SingleThreaded,
//                  typename AllocPolicy     = HeapAlloc>
//        class Cache : public ThreadingPolicy, public AllocPolicy {
//            ...
//        };
//
//    多重繼承讓 policy 的成員直接「混入」最終類別。某些 policy 沒有狀態
//    時 (e.g. SingleThreaded 是空 struct)，編譯器會 EBO (empty base
//    optimization) 不佔空間。
//
//  【設計提示】
//    - Policy 要有「明確的單一責任」(single responsibility)。
//    - 預設 policy 給 sensible default，使用者不必每個都填。
//    - 避免太多 policy ─ 讀者會迷路。3~5 個是上限。
//
//  參考：
//    Modern C++ Design, Andrei Alexandrescu, 2001
//    https://en.cppreference.com/cpp/language/templates (Policy 不是 C++ 專有
//    術語，只是業界習慣用法)
// ============================================================================

/*
補充筆記：policy_based_design
  - policy_based_design 涉及模板實例化；請先判斷哪些型別由呼叫端推導，哪些型別由程式指定。
  - 泛型程式要把型別需求寫清楚，例如可比較、可移動、可呼叫或符合某個 concept。
  - 模板技巧的價值在減少重複且保留型別安全，不是讓錯誤訊息變得更長。
  - policy_based_design 是 template 主題；template 的重點是讓型別或值在編譯期決定，產生對應的具體程式碼。
  - template 定義通常需要放在 header 或使用點可見的位置，否則編譯器無法實例化需要的版本。
  - 錯誤訊息常出現在實例化深處；閱讀時先找第一個 substitution 或 constraint 不成立的位置。
  - type trait、SFINAE、concepts 都是在表達「這個型別必須具備什麼能力」；C++20 後 concepts 通常更清楚。
  - perfect forwarding 需要 T&& 搭配 std::forward<T>，不要把所有 && 都誤認為 move。
  - template 可提升零成本抽象，但也可能造成編譯時間上升和二進位膨脹；共通實作可用非 template helper 收斂。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】Policy-Based Design
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 什麼是 policy-based design？標準庫有現成例子嗎？
//     答：把類別的「變化點」拆成獨立的 policy class，用模板參數在編譯期組合注入，
//         由 Alexandrescu 在《Modern C++ Design》發揚。標準庫到處都是這個思想：
//         std::vector<T, Allocator>、std::set<K, Compare>、
//         std::basic_string<C, Traits, Alloc> 的第二、三個參數都是 policy。
//
// 🔥 Q2. 和 GoF 的 Strategy pattern 差在哪？
//     答：同一個想法的兩個時間點。Strategy 靠介面 + 指標在 runtime 注入，可以執行
//         中切換、換策略不必重編譯，但每次呼叫都有 virtual dispatch 成本；
//         policy 是編譯期注入，完全 inline、無額外執行期成本，但換策略要重編譯。
//
// 🔥 Q3. 為什麼 policy 常用「繼承」而不是用成員變數持有？
//     答：為了 empty base optimization——沒有狀態的 policy（例如本檔的
//         SingleThreaded）當基底時可以完全不佔空間，當成員則至少佔 1 byte，
//         還可能因對齊再放大。注意 EBO 是編譯器/ABI 層級的最佳化，主流實作都做，
//         但標準只是允許、並非一律保證。C++20 起也可改用 [[no_unique_address]]。
//
// ⚠️ 陷阱. Cache<int, SingleThreaded> 和 Cache<int, MultiThreaded> 是同一個型別嗎？
//     答：不是，是兩個完全獨立的型別。不能互相賦值、不能放進同一個容器，
//         也不能用同一個非模板函式簽名接收。
//     為什麼會錯：把 policy 想成「設定值」；它其實參與型別身分，每多一個 policy
//         維度就多一組實例化，這也是型別簽名爆炸與錯誤訊息難讀的根源。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <list>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

// ─── 1. Threading Policy：Single 與 Multi ───────────────────────────────
struct SingleThreaded {
    struct Lock { Lock(SingleThreaded&) {} };   // no-op
};

struct MultiThreaded {
    std::mutex m_;
    struct Lock {
        std::lock_guard<std::mutex> g_;
        Lock(MultiThreaded& p) : g_(p.m_) {}
    };
};

// ─── 2. Storage Policy：Vector vs List ──────────────────────────────────
//   兩者介面一致 (push_back / size)，效率特性不同。
//   實際上更實用的策略可能是「Heap allocator vs Pool allocator」。
template <typename T>
struct VectorStorage {
    std::vector<T> data;
    void add(const T& v) { data.push_back(v); }
    std::size_t size() const { return data.size(); }
};

template <typename T>
struct ListStorage {
    std::list<T> data;
    void add(const T& v) { data.push_back(v); }
    std::size_t size() const { return data.size(); }
};

// ─── 3. Cache：Threading + Storage 兩種 policy 組合 ──────────────────────
template <typename T,
          typename ThreadingPolicy = SingleThreaded,
          typename StoragePolicy   = VectorStorage<T>>
class Cache : public ThreadingPolicy {
public:
    void put(const T& v) {
        typename ThreadingPolicy::Lock lock(*this);
        storage_.add(v);
    }
    std::size_t size() const {
        return storage_.size();
    }
private:
    StoragePolicy storage_;
};

// ─── 4. Leetcode 706 ── Design HashMap ───────────────────────────────────
//   題目：實作一個 HashMap：put(k,v) / get(k) / remove(k)。
//
//   解法：開放式 hash 表 (separate chaining)，buckets 用 vector<list<pair<K,V>>>。
//
//   Policy 化的好處：
//     - HashPolicy   ：不同 hash 演算法 (std::hash / 自製 fnv1a / custom)
//     - BucketPolicy ：bucket 數量 (固定 / 動態 rehash)
//   雖然 LC 只要求 0..1e6 整數，這裡示範把 hash function 做成 policy，更
//   接近真實工作場景。
//
//   時間：put/get/remove 平均 O(1)
//   空間：O(n + buckets)
//
//   為了精簡，BucketPolicy 用固定大小 NTTP，其他 policy 拆開示範即可。

template <typename K>
struct StdHashPolicy {
    std::size_t operator()(const K& k) const { return std::hash<K>{}(k); }
};

// FNV-1a 雜湊 (示範自製 hash policy)
struct FnvHashPolicy {
    std::size_t operator()(int k) const {
        std::size_t h = 1469598103934665603ULL;
        unsigned char* p = reinterpret_cast<unsigned char*>(&k);
        for (std::size_t i = 0; i < sizeof(k); ++i) {
            h ^= p[i];
            h *= 1099511628211ULL;
        }
        return h;
    }
};

template <typename K, typename V,
          typename HashPolicy = StdHashPolicy<K>,
          std::size_t Buckets = 1024>
class HashMap : private HashPolicy {
public:
    void put(const K& k, const V& v) {
        auto& bucket = data_[hash(k)];
        for (auto& kv : bucket) if (kv.first == k) { kv.second = v; return; }
        bucket.emplace_back(k, v);
    }

    bool get(const K& k, V& out) const {
        auto& bucket = data_[hash(k)];
        for (auto& kv : bucket) if (kv.first == k) { out = kv.second; return true; }
        return false;
    }

    void remove(const K& k) {
        auto& bucket = data_[hash(k)];
        for (auto it = bucket.begin(); it != bucket.end(); ++it)
            if (it->first == k) { bucket.erase(it); return; }
    }

private:
    std::size_t hash(const K& k) const { return HashPolicy::operator()(k) % Buckets; }
    mutable std::list<std::pair<K, V>> data_[Buckets];
};

// ─── 5. Leetcode 169 ── Majority Element (用 policy 選擇演算法策略) ──────
//   難度: easy
//   題目：陣列出現次數 > n/2 的元素必存在，找出它。
//   範例：[3,2,3] → 3；[2,2,1,1,1,2,2] → 2
//
//   兩個經典演算法：
//     - HashMap 計數 O(n) 時間 / O(n) 空間
//     - Boyer-Moore 投票 O(n) 時間 / O(1) 空間 (極簡天才解法)
//
//   為什麼放在這裡？
//     用 policy-based design：caller 可選 CountPolicy 或 VotePolicy，編譯期
//     換策略，零 runtime 成本。
struct CountPolicy {
    template <typename T>
    static T find(const std::vector<T>& v) {
        std::unordered_map<T, int> cnt;
        for (const T& x : v) ++cnt[x];
        for (auto& kv : cnt) if (kv.second > (int)v.size() / 2) return kv.first;
        return T{};
    }
};

struct VotePolicy {
    template <typename T>
    static T find(const std::vector<T>& v) {
        T candidate = T{};
        int count = 0;
        for (const T& x : v) {
            if (count == 0) candidate = x;
            count += (x == candidate) ? 1 : -1;
        }
        return candidate;
    }
};

template <typename T, typename Policy = VotePolicy>
T majority_element(const std::vector<T>& v) {
    return Policy::template find<T>(v);
}

// ─── 6. 工作實用：Retry 策略 (固定 vs 指數退避) ─────────────────────────
//   許多 API client / RPC 框架的「重試策略」就是 policy。
//   兩個 policy：FixedRetry (固定延遲) 與 ExpRetry (指數退避)。
struct FixedRetry {
    static int delay_ms(int /*attempt*/) { return 100; }
};

struct ExpRetry {
    static int delay_ms(int attempt) { return 100 * (1 << attempt); }   // 100,200,400,...
};

template <typename Policy>
void describe_retry_strategy() {
    std::cout << "retry delays: ";
    for (int i = 0; i < 4; ++i) std::cout << Policy::delay_ms(i) << "ms ";
    std::cout << "\n";
}

// ─── main ────────────────────────────────────────────────────────────────
int main() {
    // (1)(3) Cache 不同組合
    Cache<int> c1;                                          // 預設：single + vector
    c1.put(1); c1.put(2); c1.put(3);
    std::cout << "Cache(single,vector) size = " << c1.size() << "\n";

    Cache<std::string, MultiThreaded, ListStorage<std::string>> c2;
    c2.put("alpha"); c2.put("beta");
    std::cout << "Cache(multi,list) size = " << c2.size() << "\n";

    // (4) HashMap 兩種 hash policy
    HashMap<int, std::string> mp;
    mp.put(1, "one");
    mp.put(17, "seventeen");
    std::string s;
    if (mp.get(1, s))  std::cout << "mp[1] = "  << s << "\n";
    if (mp.get(17, s)) std::cout << "mp[17] = " << s << "\n";
    mp.remove(1);
    std::cout << "after remove(1), get(1)? " << std::boolalpha << mp.get(1, s) << "\n";

    HashMap<int, std::string, FnvHashPolicy> mp2;
    mp2.put(42, "answer");
    if (mp2.get(42, s)) std::cout << "mp2[42] = " << s << "\n";

    // (5) Leetcode 169 Majority Element — 比較兩個 policy
    std::vector<int> nums{2, 2, 1, 1, 1, 2, 2};
    std::cout << "majority(VotePolicy)  = "
              << majority_element<int, VotePolicy>(nums) << "\n";
    std::cout << "majority(CountPolicy) = "
              << majority_element<int, CountPolicy>(nums) << "\n";

    // (6) Retry policy
    std::cout << "Fixed: "; describe_retry_strategy<FixedRetry>();
    std::cout << "Exp:   "; describe_retry_strategy<ExpRetry>();

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：Policy-Based Design 跟 GoF 的 Strategy pattern 差在哪？
    //    A：Strategy 是 runtime polymorphism：用 interface + 持有 pointer，能執行
    //       中切換策略，但有 virtual call 成本。Policy-Based 是 compile-time：策略
    //       是 template parameter，配置只能在編譯期決定，但效能等同手寫，且不同
    //       policy 組合產生的型別在型別系統中是不同型別，能被 overload 區分。
    //
    //  Q2：Policy-Based 的「編譯期成本」與缺點？
    //    A：每多一個 policy 維度就多一條 instantiate；不同組合間 binary 不可
    //       互換；錯誤訊息冗長（一坨 template 名）。實務上要小心 policy 介面
    //       約束（建議用 concepts），並把 hot path 跟 cold path 分檔避免重編。
    //
    //  Q3：std::vector 的 Allocator、std::set 的 Compare 是不是 policy？
    //    A：是。它們都是 template parameter 上的 policy：vector<T, Allocator>、
    //       set<T, Compare, Allocator>。標準函式庫大量採用 policy-based design
    //       讓使用者能客製記憶體管理與比較行為，又不犧牲 inline 效能。
    //
    return 0;
}

// ============================================================================
//  【小結】
//    1. Policy-Based Design 把「會變的部分」拆成 template 參數，編譯期注入。
//    2. 與 Strategy Pattern 同源，差別在 compile-time vs run-time。
//    3. 多重繼承 + EBO 讓空 policy 不佔空間。
//    4. 預設 policy 要 sensible，避免使用者填一堆參數。
//    5. 適度使用，太多 policy 會降低可讀性。
//
//  【下一篇】
//    24_type_erasure.cpp ── 用 template 接任意型別、用虛擬介面收斂 runtime。
// ============================================================================

// 編譯: g++ -std=c++20 -Wall -Wextra 23_policy_based_design.cpp -o 23_policy_based_design

// === 預期輸出 ===
// Cache(single,vector) size = 3
// Cache(multi,list) size = 2
// mp[1] = one
// mp[17] = seventeen
// after remove(1), get(1)? false
// mp2[42] = answer
// majority(VotePolicy)  = 2
// majority(CountPolicy) = 2
// Fixed: retry delays: 100ms 100ms 100ms 100ms
// Exp:   retry delays: 100ms 200ms 400ms 800ms
// ⚠️ 上面的位址／執行緒 id／耗時每次執行都不同，數值僅供對照，不是固定結果。
