// =============================================================================
// 檔名: capacity.cpp
// 主題: std::string::capacity (取得目前已配置的容量)
// 參考: https://en.cppreference.com/cpp/string/basic_string/capacity
//       https://cplusplus.com/reference/string/string/capacity/
// =============================================================================
//
// 【函式資訊 Information】
//   size_type capacity() const noexcept;
//
// 【詳細解釋 Explanation】
//
// (1) 設計理念:讓使用者控制「成長 vs 重新配置」的權衡
//   動態長度的字串實作有兩個基本選擇:
//     a. 每次寫入都剛剛好配置 — 寫入次數多時會無數次 realloc + 複製,O(N²)
//     b. 預先多配置一些 — 用「攤銷 O(1)」換取「短期記憶體浪費」
//   STL 選了 (b),因此 std::string 內部維護兩個欄位:
//     - size:目前實際存了幾個字元
//     - capacity:已配置的 buffer 能容納幾個字元 (不需 realloc)
//   當 size() 接近 capacity(),下一次寫入若超過,就會觸發 reallocation:
//   配置新的、更大的緩衝區,把舊資料複製過去,釋放舊緩衝區。
//
// (2) Capacity Growth Strategy (成長策略)
//   多數實作採用「指數成長」(geometric growth):
//     - GCC libstdc++:大致每次 2 倍
//     - Clang libc++:大致每次 2 倍 (但有時是 1.5 倍)
//     - MSVC:大約 1.5 倍
//   為什麼指數?因為這保證「N 次 push_back 攤銷下來總成本 O(N)」,
//   而非每次都 +1 配置會變 O(N²)。
//   證明思路:每次 grow 時把所有舊字元 copy 一次,總 copy 次數 ≈ N + N/2 +
//   N/4 + ... ≤ 2N,所以攤銷 O(1) per push_back。
//
// (3) SSO (Small String Optimization)
//   現代 std::string 實作通常使用 SSO:
//     - 短字串 (典型 15 bytes 以下) 直接存在 string 物件本身的 inline buffer
//     - 不需要動態配置記憶體 (no malloc)
//     - 不需要釋放 (no free)
//     - 沒有 cache miss 風險
//   觀察:對短字串,capacity() 通常顯示固定數字 (libstdc++ 是 15)。
//   超過 SSO 上限後才會 fallback 到 heap allocation。
//   為什麼是 15 bytes?因為 64-bit 平台上 string 物件本身大約 24-32 bytes,
//   扣掉 size、flag 等 metadata 後剩 15-22 bytes 給 inline buffer。
//
// (4) capacity / size / max_size 的層級
//     0  ≤  size()  ≤  capacity()  ≤  max_size()
//   - size():實際儲存量
//   - capacity():已配置容量 (不需 realloc 就能達到)
//   - max_size():實作能達到的理論上限 (絕對天花板)
//   合法操作:resize(n) 在 n ≤ capacity() 時不需 realloc;n > capacity()
//   時會自動 grow capacity 並 realloc。
//
// (5) 與 reserve / shrink_to_fit 的互動
//   - reserve(n):確保 capacity() >= n,不會縮小
//   - shrink_to_fit():請求 capacity() == size() (非綁定請求,實作可拒絕)
//   - clear():把 size 設成 0,但通常不縮 capacity
//   設計原則:預知最終長度時就 reserve,避免迴圈中反覆 realloc。
//
// (6) 歷史背景
//   - C++98:capacity() 已存在
//   - C++11:noexcept、shrink_to_fit 加入
//   - C++11 是 SSO 開始普遍化的轉折點 (規範允許 + 主流實作都導入)
//   - C++23:basic_string 加入 contains 等新 API,但 capacity 邏輯不變
//
// (7) 複雜度
//   capacity() 本身 O(1)、noexcept、不會配置或釋放記憶體。
//
// 【注意事項 Pay Attention】
// 1. capacity() 與 size() / max_size() 的差別:
//      size()      : 目前字元數
//      capacity()  : 已配置的記憶體可容納上限(不需 realloc)
//      max_size()  : 理論上限(整個系統能給的)
// 2. 別假設 capacity() 是某個固定數字,不同實作差異很大。
// 3. 配合 reserve / shrink_to_fit 控制 capacity。
// 4. clear() 不會減少 capacity()。
// 5. 任何讓 capacity 改變的操作都會讓所有 iterator / pointer / reference 失效。
// 6. capacity() 不包含結尾 '\0' 的 byte (那是 buffer 內部多保留的)。
//
// 【概念補充 Concept Deep Dive】
//
// ★ 為什麼 reserve 重要?
//   迴圈中拼接字串的常見錯誤:
//     std::string s;
//     for (int i = 0; i < 1000000; ++i) s += "x";  // 多次 realloc
//   即使有指數成長,realloc 帶來的 copy 仍是不必要的。寫:
//     std::string s;
//     s.reserve(1000000);   // 一次配足
//     for (int i = 0; i < 1000000; ++i) s += "x";  // 全程 zero realloc
//   差距通常是 5-10 倍以上。日常處理 log、JSON、SQL 字串時 reserve 是
//   標配優化。
//
// ★ Growth Factor 的學問
//   理論研究指出 growth factor 應介於 (1, 2):
//     - 太小 (1.1) → 攤銷 O(1) 但 realloc 次數仍多
//     - 太大 (2.0) → realloc 次數少但記憶體浪費高,新 buffer 永遠無法
//                    重用之前釋放的空間 (新空間 = N + N/2 + N/4 + ... < 舊空間總和 × 2)
//   黃金比例 1.618 (φ) 在某些理論模型最佳;但 1.5 與 2.0 是實務主流。
//   MSVC 採 1.5,libstdc++/libc++ 採 2.0,各有取捨。
//
// ★ 為何 shrink_to_fit() 是「非綁定請求」?
//   標準允許實作拒絕 shrink_to_fit,因為:
//     - 縮小一定要 realloc + copy,有成本
//     - SSO 之內的字串本來就不需要縮
//     - 實作可能想保留 capacity 給之後重複使用
//   結果:呼叫後 capacity() 不一定真的等於 size()。但呼叫絕不會增加
//   capacity,且不會讓 size 改變。
//
// ★ SSO 的隱藏代價
//   SSO 雖然省去 heap allocation,但:
//     - string 物件本身變大 (~32 bytes 而非 ~16 bytes)
//     - move 操作不再是純粹的指標交換,SSO 範圍內必須真的 copy bytes
//     - 嚴格的 noexcept move 在 SSO 字串上仍 noexcept,但 cost 略高
//   多數情況優點遠大於缺點,主流實作都用 SSO。
//
// ★ capacity() == 0 的字串存在嗎?
//   非預設建構的空字串 (例如 reserve(0) 後 shrink_to_fit) 理論上 capacity
//   可以是 0,但實作通常仍保留 SSO inline buffer 而給出非零值 (例如 15)。
//   完全相依實作,不要寫測試斷言 capacity() == 0。
//
// =============================================================================

/*
補充筆記：std::string::capacity
  - std::string::capacity 需要同時考慮內容、容量與舊指標失效；字串 API 常會配置或搬移緩衝區。
  - 回傳位置的函式要先處理 npos，回傳 pointer/view 的函式要追蹤來源 string 生命週期。
  - UTF-8 文字下，size 代表位元組數，不代表使用者看到的字元數。
  - std::string::capacity 是 std::string 主題的一部分；先分清楚這個操作會讀取、追加、插入、刪除、搜尋，還是只暴露底層字元。
  - std::string 的索引型別是 size_type；和 int 混用時要小心 unsigned underflow，尤其是倒著迴圈或拿 npos 比較。
  - 修改字串內容或容量可能讓先前取得的 iterator、reference、pointer、c_str() 指標失效。
  - operator[] 不做範圍檢查，at() 越界會丟 std::out_of_range；教學範例可比較兩者，工作程式要依錯誤處理需求選。
  - find/rfind/find_first_of 這類搜尋函式失敗回傳 std::string::npos；判斷時應和 npos 比，不要寫成 >= 0。
  - 字串和 C string 互通時要記得 null terminator；std::string 可以保存內含 \0 的資料，但很多 C API 會在第一個 \0 停止。
  - 若只是傳入唯讀文字片段且不需要擁有資料，std::string_view 可避免複製；但它不能延長原字串生命週期。
  - 處理中文或 UTF-8 時，std::string 的 size() 回傳 byte 數，不是人眼看到的字元數。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】capacity / SSO
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 什麼是 SSO(Small String Optimization)?為什麼 std::string 不一定在堆上?
//     答:短字串直接存放在 string 物件自身的內嵌 buffer(隨物件位於 stack 或所屬
//     物件內),完全不呼叫 operator new;長度超過門檻才改為 heap 配置。動機是真實
//     程式的字串多半很短,而 heap allocation 相對昂貴(鎖、cache miss、碎片)。
//     所以「std::string 一定會 new」是錯的。
//     追問:怎麼驗證有沒有走 SSO?→ 看 s.data() 是否落在 [&s, &s+sizeof(s)) 區間內。
//
// 🔥 Q2. SSO 的門檻是多少?
//     答:先講「這是 implementation-defined,標準未規定任何數字」,再舉實例:
//     libstdc++ 15 字元(sizeof(std::string)==32)、libc++ 22 字元(sizeof==24)、
//     MSVC STL 15。libstdc++ 是 16 bytes 的 buffer 留 1 byte 給 '\0',所以是 15。
//     追問:為什麼 libc++ 物件更小卻裝更多?→ 它用 union 把 capacity 欄位也讓給
//     buffer 用,只留 1 bit 當 long/short 旗標。
//
// 🔥 Q3. capacity() / reserve() / shrink_to_fit() 各自做什麼?
//     答:capacity() 是「不重新配置就能容納的字元數」(保證 >= size());
//     reserve(n) 確保 capacity() >= n,不會改變 size();shrink_to_fit() 只是
//     非約束性請求,實作可以完全忽略。成長因子同樣是 implementation-defined
//     (libstdc++ 約 2 倍,實測 15 → 30 → 60;MSVC 約 1.5 倍),幾何成長讓
//     += / append 維持攤還 O(1)。
//
// ⚠️ 陷阱. capacity() 大於 0 就代表已經發生 heap 配置嗎?
//     答:不一定。空字串在 libstdc++ 上 capacity() 就已經是 15,那是內嵌 buffer,
//     一次 malloc 都沒有發生。
//     為什麼會錯:多數人腦中的模型是「capacity > 0 ⇒ 有動態記憶體」,忽略了 SSO
//     讓前面那幾個字元的容量是白送的。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <vector>

void demoCapacity() {
    std::string s;
    std::cout << "init      capacity = " << s.capacity() << " (常見 SSO 大小)\n";

    s = "Hello";
    std::cout << "Hello     capacity = " << s.capacity() << "\n";

    // 推進到超過 SSO
    s = std::string(50, 'A');
    std::cout << "50 chars  capacity = " << s.capacity() << "\n";

    // 觀察成長策略
    std::string t;
    size_t prev = t.capacity();
    std::cout << "成長過程: " << prev;
    for (int i = 0; i < 200; ++i) {
        t += 'x';
        if (t.capacity() != prev) {
            prev = t.capacity();
            std::cout << " -> " << prev;
        }
    }
    std::cout << "\n";
}

// -----------------------------------------------------------------------------
// 【實務範例】根據 capacity 決定字串拼接策略
// 為何用 capacity: 在迴圈中拼接時,若 capacity 不夠,提前 reserve 避免反覆 realloc。
// -----------------------------------------------------------------------------
std::string buildJoinedString(const std::vector<std::string>& parts,
                              const std::string& sep) {
    // 估算最終長度
    size_t total = 0;
    for (const auto& p : parts) total += p.size();
    total += sep.size() * (parts.empty() ? 0 : parts.size() - 1);

    std::string result;
    if (total > result.capacity()) result.reserve(total);

    for (size_t i = 0; i < parts.size(); ++i) {
        if (i > 0) result += sep;
        result += parts[i];
    }
    return result;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1768. Merge Strings Alternately (Easy)
//
// 題目敘述:
//   給兩個字串 word1、word2,從 word1 起交替取字元合併;若一邊先用完,
//   把另一邊剩下的字元接到尾端。
//   範例:
//     word1="abc",   word2="pqr"   → "apbqcr"
//     word1="ab",    word2="pqrs"  → "apbqrs"
//     word1="abcd",  word2="pq"    → "apbqcd"
//
// 為何用 capacity / reserve:
//   結果長度可預先計算為 word1.size() + word2.size()。直接 += 字元雖然
//   能正確產生答案,但若不 reserve 預估容量,字串會在合併過程中觸發多次
//   reallocation(典型 SSO → 32 → 64 ...);呼叫 reserve(total) 一次配足
//   capacity,迴圈中 += 都不會 realloc — 是「事先知道最終長度時 reserve」
//   的最教科書範例,也是面試官想看到的「字串拼接優化思維」。
//
// 解題思路:
//   1. total = word1.size() + word2.size(),reserve(total)。
//   2. 雙指針 i, j 交替從 word1、word2 取字元 push_back。
//   3. 任一字串先到尾就把另一邊剩下的 append 進去。
//
// 複雜度:時間 O(N+M)、空間 O(N+M)(只配置一次 buffer)。
// -----------------------------------------------------------------------------
std::string mergeAlternately(const std::string& word1, const std::string& word2) {
    std::string result;
    size_t total = word1.size() + word2.size();
    result.reserve(total);   // 預估 capacity,避免迴圈中反覆 realloc

    size_t i = 0, j = 0;
    while (i < word1.size() && j < word2.size()) {
        result += word1[i++];
        result += word2[j++];
    }
    // 把剩下的接上去 (append 比迴圈 += 更快)
    if (i < word1.size()) result.append(word1, i, std::string::npos);
    if (j < word2.size()) result.append(word2, j, std::string::npos);

    // 此時 result.size() == total,result.capacity() 至少為 total
    return result;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】根據已知值列建立 SQL IN 子句
// 為何用 capacity: 大 batch 任務(如批次刪除/批次查詢)會處理上千個 ID。
//                  預估 capacity 後一次配足,比反覆成長快非常多。
//                  這是 ORM / DB 工具實作 IN 子句最常用的優化模式。
// -----------------------------------------------------------------------------
std::string buildSqlInClause(const std::vector<int>& ids) {
    std::string out;
    // 估算: 每個 id 最多 10 位數 + ", "
    out.reserve(ids.size() * 12 + 2);
    out += '(';
    for (size_t i = 0; i < ids.size(); ++i) {
        if (i > 0) out += ", ";
        out += std::to_string(ids[i]);
    }
    out += ')';
    return out;
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例 2】LeetCode 1page. 1page0. Generate a String With Characters That Have Odd Counts
// 題目: LeetCode 1374. Generate a String With Characters That Have Odd Counts
// 給 n 回傳一個長度 n、且每個出現字元次數都是奇數的字串。
// 為何用 capacity: 我們可預先 reserve(n),確保只配置一次。然後填入字元 'a',
//                  若 n 是偶數,結尾改成 'b'。展示「先 reserve 再 fill」的標準寫法。
// 思路: 全填 'a';偶數 → 最後一個改 'b';奇數 → 直接全 'a'。
// 複雜度: O(N)。
// -----------------------------------------------------------------------------
std::string generateTheString(int n) {
    std::string out;
    out.reserve(static_cast<size_t>(n));         // 預先配置,避免成長
    out.assign(static_cast<size_t>(n), 'a');
    if (n % 2 == 0) out.back() = 'b';
    return out;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】寫 log 緩衝區:估算容量後一次配置
// 為何用 capacity: server log 每行格式固定 "[YYYY-MM-DD HH:MM:SS] [LEVEL] msg\n",
//                  總長可估;先 reserve 可保證寫入過程零 realloc,效能穩定。
// -----------------------------------------------------------------------------
std::string formatLogLine(const std::string& level, const std::string& msg) {
    // 估算: "[2024-05-18 10:00:00] " (22) + "[ERROR] " (~8) + msg + "\n"
    std::string out;
    out.reserve(22 + 8 + msg.size() + 1);
    out += "[2024-05-18 10:00:00] [";
    out += level;
    out += "] ";
    out += msg;
    out += '\n';
    return out;
}

int main() {
    demoCapacity();
    std::cout << "\n=== 拼接示範 ===\n";
    std::cout << buildJoinedString({"red", "green", "blue"}, ", ") << "\n";

    std::cout << "\n=== 日常實務: SQL IN 子句 ===\n";
    std::cout << buildSqlInClause({1, 2, 3, 42, 100}) << "\n";

    std::cout << "\n=== LeetCode 1768 (reserve 預估容量) ===\n";
    auto m1 = mergeAlternately("abc", "pqr");
    std::cout << "\"abc\"+\"pqr\"   = \"" << m1 << "\" (size=" << m1.size()
              << ", capacity>=" << m1.capacity() << ")\n";              // apbqcr
    std::cout << "\"ab\"+\"pqrs\"   = \"" << mergeAlternately("ab", "pqrs") << "\"\n"; // apbqrs
    std::cout << "\"abcd\"+\"pq\"   = \"" << mergeAlternately("abcd", "pq") << "\"\n"; // apbqcd

    std::cout << "\n=== LeetCode 1374 ===\n";
    std::cout << "n=4 -> [" << generateTheString(4) << "]\n";    // aaab
    std::cout << "n=5 -> [" << generateTheString(5) << "]\n";    // aaaaa

    std::cout << "\n=== 日常實務: formatLogLine ===\n";
    std::cout << formatLogLine("INFO",  "server started on port 8080");
    std::cout << formatLogLine("ERROR", "connection refused");

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：capacity() 與 size() 有什麼差別?三者 (含 max_size) 的關係?
    //    A：size() 是「實際存了幾個字元」, capacity() 是「不需 realloc 就
    //       能容納的最大字元數」。永遠 0 ≤ size() ≤ capacity() ≤ max_size()。
    //       capacity() 是「實際已配置的 buffer 大小 - 1」(扣掉結尾 '\0'),
    //       max_size() 才是實作的理論天花板。
    //
    //  Q2：std::string 的 capacity 成長因子是 2 倍還是 1.5 倍?
    //    A：實作而定。GCC libstdc++ 與 Clang libc++ 大致 2 倍,MSVC 約 1.5 倍。
    //       關鍵不是因子大小,而是「指數成長」── 這保證 N 次 push_back 攤銷
    //       下來 O(N) 而非 O(N²)。1.5 倍的好處是新 buffer 可能能重用釋放的
    //       舊 buffer 區域 (記憶體 fragmentation 較低),代價是 realloc 次數多。
    //
    //  Q3：SSO (Small String Optimization) 對 capacity() 有什麼影響?
    //    A：短字串 (libstdc++ 是 ≤15 byte、libc++ 是 ≤22 byte) 直接存在
    //       string 物件本身的 inline buffer,不配置 heap。此時 capacity()
    //       會回傳一個固定值 (例如 15),不會是 0。觀察:對 std::string{}
    //       呼叫 capacity() 多半得到 SSO 的最大內嵌容量,而非 0。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra capacity.cpp -o capacity

// === 預期輸出 (節錄) ===
// === LeetCode 1374 ===
// n=4 -> [aaab]
// n=5 -> [aaaaa]
// === 日常實務: formatLogLine ===
// [2024-05-18 10:00:00] [INFO] server started on port 8080
// [2024-05-18 10:00:00] [ERROR] connection refused
