// =============================================================================
// 檔名: get_allocator.cpp
// 主題: std::string::get_allocator (取得字串使用的 allocator)
// 參考連結 References:
//   cppreference: https://en.cppreference.com/cpp/string/basic_string/get_allocator
//   cplusplus.com: https://cplusplus.com/reference/string/string/get_allocator/
// =============================================================================
//
// 【函式資訊 Information】
//   allocator_type get_allocator() const noexcept;
//
// 回傳: 該 string 物件目前所使用的 allocator (一份 copy)。
//
// =============================================================================
//
// 【詳細解釋 Explanation】
//
// (一) 為什麼會有 get_allocator?
// ----------------------------------------------------------------------------
// std::basic_string 完整宣告其實是:
//   template<class CharT,
//            class Traits = std::char_traits<CharT>,
//            class Allocator = std::allocator<CharT>>
//   class basic_string;
// 第三個 template 參數 Allocator 決定「字元所佔的記憶體從哪裡來」。
// 大多數人寫 std::string 時都用預設的 std::allocator,但在某些情境下
// (高效能伺服器、嵌入式、共享記憶體、自訂 memory pool、PMR/std::pmr) 你
// 會給它一個自訂的 allocator,此時 get_allocator() 就是「拿回那一份
// allocator」的標準入口。
//
// (二) 為什麼回傳「一份 copy」而非 reference?
// ----------------------------------------------------------------------------
// Allocator 在 STL 中是「value-like」的物件,常常不持狀態 (stateless),
// 拷貝便宜。標準故意設計成 by-value 回傳,理由有二:
//   1. 對 stateless allocator 而言,複製等於什麼都沒做,零成本。
//   2. 對 stateful allocator (如 PMR 的 polymorphic_allocator) 而言,
//      持有 copy 比持有 reference 安全 —— 即使原 string 被銷毀,
//      你手上的 allocator 仍然可以使用 (因為它通常只是包了一個
//      memory_resource* 而 memory_resource 的生命週期由你管理)。
//
// (三) 何時你會真的呼叫 get_allocator?
// ----------------------------------------------------------------------------
// 在「自製 container 並想跟某個 string 共用 allocator」的情境最常見:
//   std::pmr::string s = ...;
//   std::pmr::vector<int> v(s.get_allocator());  // 共用同一個 memory pool
// 這在「熱迴圈內配置大量小物件」時可大幅減少 malloc 次數與 fragmentation,
// 是高效能 C++ 經典技巧。
//
// 一般應用程式 95% 的時間用不到 get_allocator,但 STL 規格要求每個
// allocator-aware 容器都必須提供它,所以它一直在 API 中佔有一席之地。
//
// (四) Allocator-aware container 的「allocator 傳遞規則」
// ----------------------------------------------------------------------------
// allocator 在 string 之間如何傳遞,由 allocator_traits 三個 trait 決定:
//   - propagate_on_container_copy_assignment (POCCA)
//   - propagate_on_container_move_assignment (POCMA)
//   - propagate_on_container_swap            (POCS)
// 預設 std::allocator 三者都是 false (因為它無狀態,複製不複製沒差)。
// PMR 的 polymorphic_allocator 也是 false_type,且 select_on_container_copy
// 行為獨特 —— copy 出來的 PMR string 會用「預設 memory_resource」而非
// 來源那個。get_allocator 是觀察這些行為的最佳工具。
//
// (五) C++ 標準演進
// ----------------------------------------------------------------------------
//   C++03 起即存在
//   C++11 標記為 noexcept
//   C++17 std::pmr::string 出現,get_allocator() 變成串接 PMR 的關鍵
//   C++20 constexpr basic_string 中也是 constexpr
//
// =============================================================================
//
// 【概念補充 Concept Deep Dive】
//
// 1) Stateless vs Stateful allocator
//    - Stateless (如 std::allocator):所有實例可互換,無內部資料,sizeof
//      通常為 1 (空 class optimization 後甚至為 0)。
//    - Stateful (如 polymorphic_allocator):持有指向 memory_resource 的
//      指標。兩個 string 即便型別相同,如果 allocator 指向不同的 resource,
//      它們不能互相 swap (除非 POCS 為 true)。
//
// 2) Empty Base Optimization (EBO)
//    std::string 通常用 EBO 把 stateless allocator 「壓平」進 string 物件,
//    讓 sizeof(string) 不會因為多了 allocator 成員而變大。
//
// 3) PMR (Polymorphic Memory Resource, C++17)
//    std::pmr::string 內部用 polymorphic_allocator<char>,可以動態選擇
//    memory pool。一個 std::pmr::string 在 stack 上拿出 monotonic_buffer
//    可獲得比 malloc 快數倍的字串配置。get_allocator 可拿到該 allocator
//    並重用到其他 PMR 容器。
//
// 4) 自訂 allocator 的「最少介面」
//    一個合法 allocator 至少要提供:
//      using value_type = T;
//      T* allocate(size_t);
//      void deallocate(T*, size_t);
//      bool operator==/operator!= 與其他 allocator 比較
//    其餘介面 (rebind、construct、destroy) 由 allocator_traits 補完。
//
// 5) 不要過度設計自製 allocator
//    一般專案不需要自訂 allocator;profile 確認瓶頸在配置才考慮。
//    PMR 已能解決絕大多數場景,且 std 對它有完整支援。
//
// =============================================================================
//
// 【注意事項 Pay Attention】
// 1. 回傳的是 copy,不是 reference;對 stateful allocator 要確認背後資源
//    的生命週期足夠長。
// 2. get_allocator 是 noexcept;但拿到後若呼叫其 allocate() 仍可能 throw
//    bad_alloc。
// 3. 不同 string 的 allocator 是否「相等」會影響 swap、move 的行為,務必
//    測試你的自訂 allocator 的 operator==。
// 4. std::string 與 std::pmr::string 是不同型別 (allocator 不同),不能
//    互相 assign,只能透過值複製 (走 const char* 介面)。
// 5. 若你的 allocator 不是 stateless,要嚴格遵守 allocator_traits 的傳遞
//    規則,否則會出現 use-after-free。
//
// =============================================================================

/*
補充筆記：std::string::get_allocator
  - std::string::get_allocator 需要同時考慮內容、容量與舊指標失效；字串 API 常會配置或搬移緩衝區。
  - 回傳位置的函式要先處理 npos，回傳 pointer/view 的函式要追蹤來源 string 生命週期。
  - UTF-8 文字下，size 代表位元組數，不代表使用者看到的字元數。
  - std::string::get_allocator 是 std::string 主題的一部分；先分清楚這個操作會讀取、追加、插入、刪除、搜尋，還是只暴露底層字元。
  - std::string 的索引型別是 size_type；和 int 混用時要小心 unsigned underflow，尤其是倒著迴圈或拿 npos 比較。
  - 修改字串內容或容量可能讓先前取得的 iterator、reference、pointer、c_str() 指標失效。
  - operator[] 不做範圍檢查，at() 越界會丟 std::out_of_range；教學範例可比較兩者，工作程式要依錯誤處理需求選。
  - find/rfind/find_first_of 這類搜尋函式失敗回傳 std::string::npos；判斷時應和 npos 比，不要寫成 >= 0。
  - 字串和 C string 互通時要記得 null terminator；std::string 可以保存內含 \0 的資料，但很多 C API 會在第一個 \0 停止。
  - 若只是傳入唯讀文字片段且不需要擁有資料，std::string_view 可避免複製；但它不能延長原字串生命週期。
  - 處理中文或 UTF-8 時，std::string 的 size() 回傳 byte 數，不是人眼看到的字元數。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】get_allocator 與 allocator-aware 容器
// ───────────────────────────────────────────────────────────────────────────
// Q1. get_allocator() 為什麼回傳 copy 而不是 reference?
//     答:allocator 在 STL 中被設計成 value-like 物件,複製便宜。對 stateless
//     allocator(如 std::allocator)複製等於什麼都沒做;對 stateful allocator
//     (如 pmr::polymorphic_allocator)持有 copy 比持有 reference 安全——即使
//     來源 string 已被解構,手上那份仍可用,因為它通常只包了一個
//     memory_resource*,而該 resource 的生命週期由你自己管理。
//     追問:那 allocator 本身的 allocate() 是 noexcept 嗎?→ get_allocator() 是
//     noexcept,但拿到後呼叫 allocate() 仍可能丟 bad_alloc。
//
// Q2. stateless 與 stateful allocator 差在哪?為什麼會影響 swap?
//     答:stateless allocator 沒有內部資料,所有實例可互換,sizeof 通常為 1,
//     且能靠 EBO(Empty Base Optimization)壓進容器裡不增加 sizeof(string)。
//     stateful allocator 持有狀態(例如指向某個 memory pool);兩個型別相同但
//     allocator 指向不同 resource 的 string,除非 POCS
//     (propagate_on_container_swap)為 true,否則不能安全互 swap。
//     追問:std::allocator 的 POCCA/POCMA/POCS 預設是什麼?→ 都是 false,
//     因為它無狀態,傳不傳遞沒有差別。
//
// Q3. 實務上什麼時候真的會用到 get_allocator()?
//     答:最常見的是「讓自己的容器跟某個既有物件共用同一個 memory pool」:
//     std::pmr::vector<int> v(s.get_allocator());,讓兩者從同一個 memory_resource
//     取記憶體,在熱迴圈中大量配置小物件時能顯著減少 malloc 次數與碎片。
//     一般應用程式幾乎用不到,但 allocator-aware 容器規格要求必須提供它。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <cstring>
#include <cstddef>
#include <memory>
#include <memory_resource>
#include <type_traits>

void demoGetAllocator() {
    std::cout << "=== 預設 std::allocator ===\n";
    std::string s = "Hello";
    auto a = s.get_allocator();
    std::cout << "sizeof(allocator) = " << sizeof(a) << " (stateless 通常為 1)\n";
    std::cout << "is_always_equal   = " << std::boolalpha
              << std::allocator_traits<decltype(a)>::is_always_equal::value << "\n";

    // 用拿到的 allocator 自己配置一塊原始 char 記憶體
    char* raw = a.allocate(16);
    std::strcpy(raw, "manual");
    std::cout << "manual buffer: " << raw << "\n";
    a.deallocate(raw, 16);
}

void demoPmr() {
    std::cout << "\n=== PMR 字串共用 memory_resource ===\n";

    // 在 stack 上開一塊 1KB 的 buffer,當作 monotonic memory pool
    std::array<std::byte, 1024> buf{};
    std::pmr::monotonic_buffer_resource pool{buf.data(), buf.size()};

    std::pmr::string s{"hello pmr", &pool};
    std::cout << "s = " << s << "\n";

    // 取出 s 的 allocator,讓其他容器共用同一個 pool
    auto pa = s.get_allocator();
    std::pmr::string s2{"world", pa};
    std::cout << "s2 also lives in same pool: " << s2 << "\n";

    // 觀察 allocator 內部的 memory_resource 指標確實一致
    std::cout << "same resource? " << std::boolalpha
              << (s.get_allocator().resource() == s2.get_allocator().resource())
              << "\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例】LeetCode 14. Longest Common Prefix (Easy)
//
// 題目敘述:
//   給字串陣列 strs,回傳所有字串的最長共同前綴;若沒有共同前綴回傳 ""。
//   範例: ["flower","flow","flight"] → "fl"
//
// 為何用 get_allocator:
//   結果字串希望「跟輸入用同一個 allocator」(在 PMR 應用裡這代表
//   結果與輸入共用同一塊 memory pool,不會跨 pool 拷貝)。
//   get_allocator + 建構子 (alloc) 的組合是 PMR 容器互通的標準做法。
//
//   題目本身用一般 std::string 也能解,但若呼叫者傳 pmr::string,
//   我們透過 get_allocator() 取出 allocator 並用以建構結果,可避免
//   不必要的 pool 切換。
//
// 解題思路:
//   以第一個字串為候選,從第二個開始逐字比對,直到不符或耗盡。
//
// 複雜度: 時間 O(N · L) (L 為最短字串長度),空間 O(L)
// -----------------------------------------------------------------------------
template <class StringT>
StringT longestCommonPrefix(const std::vector<StringT>& strs) {
    if (strs.empty()) return StringT{};
    // 用 strs[0] 的 allocator 建構同型別的結果
    StringT prefix(strs[0], strs[0].get_allocator());

    for (size_t i = 1; i < strs.size(); ++i) {
        size_t j = 0;
        while (j < prefix.size() && j < strs[i].size() &&
               prefix[j] == strs[i][j]) ++j;
        prefix.resize(j);
        if (prefix.empty()) break;
    }
    return prefix;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 Daily Work】RPC handler 的 per-request memory pool
//
// 為何用 get_allocator:
//   後端 RPC server 每個請求生命週期短、配置量大 (header parse、JSON
//   field、暫存字串)。每個請求用 monotonic_buffer_resource 起一個
//   pool,handler 結束時整塊釋放,不需 free 個別物件 → 大幅降低
//   malloc/free 開銷與 fragmentation。
//
//   字串解析時,我們會取請求字串的 get_allocator() 餵給其他暫存
//   pmr 容器,確保所有臨時資料都活在同一個 pool 內。
// -----------------------------------------------------------------------------
void perRequestPoolDemo() {
    std::cout << "\n=== 日常實務: per-request PMR pool ===\n";

    std::array<std::byte, 4096> stackBuf{};
    std::pmr::monotonic_buffer_resource req{stackBuf.data(), stackBuf.size()};

    // 以 req 為 allocator 建立請求中的 token / header 等字串
    std::pmr::string authHeader{"Bearer abc.def.ghi", &req};

    // 後續的暫存容器自動共用 req,無需另外傳遞 resource 指標。
    // 注意:對 pmr::vector<pmr::string>,push_back 時新元素自動使用 vector
    // 的 allocator 來配置內部 buffer (這就是 uses-allocator construction)。
    std::pmr::vector<std::pmr::string> parts{authHeader.get_allocator()};
    parts.push_back(std::pmr::string{"Bearer", authHeader.get_allocator()});
    parts.push_back(std::pmr::string{"abc.def.ghi", authHeader.get_allocator()});

    std::cout << "authHeader = " << authHeader << "\n";
    std::cout << "parts[0]   = " << parts[0] << "\n";
    std::cout << "parts[1]   = " << parts[1] << "\n";
    // 整塊 req 在離開 scope 時一起回收 — 不需要逐個 free。
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例 2】LeetCode 1page. 1page0. Truncate Sentence - PMR 版
// 題目: LeetCode 1816. Truncate Sentence
// 給句子 s 與 k,回傳前 k 個單字組成的句子。
// 為何用 get_allocator: 我們用同一個 pmr 容器系統,結果 string 沿用 vector 的 allocator,
//                       省掉一次配置。教學 propagate_on_container_copy_assignment 的精神。
// 複雜度: O(N)。
// -----------------------------------------------------------------------------
std::string truncateSentenceK(const std::string& s, int k) {
    int spaces = 0;
    size_t cut = s.size();
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == ' ' && ++spaces == k) { cut = i; break; }
    }
    std::string out;
    out.reserve(cut);
    out.assign(s, 0, cut);
    // 教學要點: out.get_allocator() == 預設 std::allocator
    return out;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】用 pmr::monotonic_buffer_resource 在 stack 上處理一批短字串
// 為何用 get_allocator: 對 pmr::string 而言,get_allocator() 才能取得當下用的
//                       memory_resource;後續建立同 pool 的字串時必須傳入此 allocator。
// 場景: 每個 HTTP 請求都用同一個 stack pool,結束釋放全清空,避免 heap 碎片化。
// -----------------------------------------------------------------------------
void splitWithSamePool(std::pmr::string& src,
                       std::pmr::vector<std::pmr::string>& out) {
    auto alloc = src.get_allocator();           // 取出 allocator,後續同池
    size_t i = 0;
    while (i < src.size()) {
        size_t j = src.find(',', i);
        if (j == std::pmr::string::npos) j = src.size();
        out.emplace_back(std::pmr::string{src.substr(i, j - i), alloc});
        i = j + 1;
    }
}

int main() {
    demoGetAllocator();
    demoPmr();

    std::cout << "\n=== LeetCode 14 ===\n";
    std::vector<std::string> in = {"flower", "flow", "flight"};
    std::cout << "\"" << longestCommonPrefix(in) << "\"\n";   // "fl"

    std::cout << "\n=== LeetCode 1816 (truncateSentenceK) ===\n";
    std::cout << truncateSentenceK("Hello how are you Contestant", 4) << "\n";

    perRequestPoolDemo();

    std::cout << "\n=== 日常實務: splitWithSamePool ===\n";
    {
        std::array<std::byte, 1024> stackBuf{};
        std::pmr::monotonic_buffer_resource pool{stackBuf.data(), stackBuf.size()};
        std::pmr::string src{"a,bb,ccc,dddd", &pool};
        std::pmr::vector<std::pmr::string> parts{&pool};
        splitWithSamePool(src, parts);
        for (auto& p : parts) std::cout << "[" << p << "] ";
        std::cout << "\n";
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1:get_allocator() 為什麼回傳 by-value 而非 reference?
    //    A:Allocator 在 STL 中是「value-like」物件,通常 stateless 拷貝零成本;
    //       對 stateful (例如 polymorphic_allocator) 而言,持有 copy 比 ref 安全 —
    //       即使原 string 銷毀,copy 仍可繼續使用底層 memory_resource。
    //
    //  Q2:std::pmr::string 跟 std::string 是同一個型別嗎?能互相 assign 嗎?
    //    A:不是。std::pmr::string = std::basic_string<char, char_traits<char>,
    //       std::pmr::polymorphic_allocator<char>>,allocator 型別不同 → 整個
    //       basic_string 特化也不同。不能直接 assign,只能透過 const char*
    //       或 string_view 做值複製。
    //
    //  Q3:何時才該自訂 allocator?C++17 PMR 解決了什麼?
    //    A:大多數專案永遠不需要;profile 確認瓶頸在 malloc/free 才考慮。
    //       PMR (polymorphic memory resource) 把「allocator 型別」與「memory
    //       pool」解耦 — 同一個 std::pmr::string 在 runtime 切換不同 pool
    //       (monotonic_buffer / unsynchronized_pool / synchronized_pool),
    //       適合短生命週期大量配置的 RPC handler / parser 場景。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra get_allocator.cpp -o get_allocator

// === 預期輸出 (節錄) ===
// === LeetCode 1816 (truncateSentenceK) ===
// Hello how are you
// === 日常實務: splitWithSamePool ===
// [a] [bb] [ccc] [dddd]
