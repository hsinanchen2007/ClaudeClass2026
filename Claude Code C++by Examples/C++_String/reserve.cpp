// =============================================================================
// 檔名: reserve.cpp
// 主題: std::string::reserve (預先配置記憶體 / 提示容量)
// 參考:
//   - https://en.cppreference.com/cpp/string/basic_string/reserve
//   - https://cplusplus.com/reference/string/string/reserve/
// =============================================================================
//
// 【函式資訊 Information】
//   void reserve(size_type new_cap = 0);          // C++17 之前
//   void reserve(size_type new_cap);              // C++20 起 (移除預設值)
//   void reserve();                               // C++17 起,等效 shrink_to_fit
//                                                 //   (C++20 不再 deprecate,但少用)
//
// 參數:
//   new_cap - 期望的最小 capacity。
// 回傳:
//   無 (void)。
// 例外:
//   - 若 new_cap > max_size() 丟 std::length_error。
//   - 配置失敗丟 std::bad_alloc。
//   - 例外丟出時字串內容不變 (strong exception guarantee)。
//
// =============================================================================
//
// 【詳細解釋 Explanation - 設計理念與底層運作】
//
// 1. 它「保證了什麼」與「沒保證什麼」
//    -----------------------------------------------------------------
//    reserve(n) 是對 std::string 的「容量提示」:
//      - 它「保證」呼叫後 capacity() >= n;
//      - 它「不保證」capacity() 剛好等於 n (實作可給更大,常見會對齊
//        到 2 的次方 / 16 bytes 的倍數 / SSO 內建大小);
//      - 它「不改變」size() 與內容 — 字串看起來與呼叫前一模一樣;
//      - 它「不延伸」結尾 '\0' — 結尾仍位於 size() 處。
//
// 2. 為什麼需要 reserve?— Reallocation 是隱形成本殺手
//    -----------------------------------------------------------------
//    string 內部維持一塊連續的 char buffer。當 size() 即將超過 capacity()
//    時,實作會:
//        (a) 配置一塊新的、更大的 buffer (通常容量 = 舊 capacity * 1.5 或 *2);
//        (b) 把舊內容拷貝/搬移到新 buffer;
//        (c) 釋放舊 buffer。
//    這個流程稱為 reallocation,單次成本 O(N)。如果沒有 reserve,程式
//    在 N 個字元的 push_back/+= 過程會經歷 log_k(N) 次 reallocation
//    (其中 k 是成長係數 1.5 或 2),雖然攤銷後仍是 O(1)/字元,但實際的
//    記憶體配置 + 拷貝 + 快取失效成本相當可觀。
//
//    若預先 reserve(N),整個流程只配置一次,後續 N 次寫入皆是純粹的
//    O(1) memory store,效能差距 2~10 倍很常見。
//
// 3. Capacity 的指數成長策略 (Geometric Growth)
//    -----------------------------------------------------------------
//    為什麼 STL 容器(string、vector)選擇「乘倍」而非「加常數」?
//      - 加常數成長 (例如每次 +16):總拷貝量為 16 + 32 + 48 + ... = O(N^2)
//      - 乘倍成長 (例如每次 *2):  總拷貝量為 1 + 2 + 4 + ... + N = 2N = O(N)
//    結論:乘倍成長讓 push_back 的攤銷成本維持 O(1)。標準雖未強制,
//    但 libstdc++ / libc++ / MSVC STL 三大實作都使用乘倍。
//
// 4. 與 SSO (Small String Optimization) 的互動
//    -----------------------------------------------------------------
//    現代 std::string 通常內嵌一個小 buffer (libstdc++ 為 15 字元、
//    libc++ 為 22 字元、MSVC 為 15 字元),短字串完全不配置 heap。
//    若 reserve(n) 的 n 落在 SSO 範圍內,實作可能:
//      - 真的維持 SSO,只是把 capacity() 報告為 SSO 大小;
//      - 或不做任何事 (因為已經 >= n)。
//    重點:在 SSO 範圍內呼叫 reserve 通常完全免費。
//
// 5. C++20 的語意修正:reserve(0) 不再等於 shrink_to_fit
//    -----------------------------------------------------------------
//    在 C++17 之前,reserve(0) 或 reserve() 隱含「縮減容量」的語意,
//    這讓「想保留容量但不增加」的呼叫變得危險。C++20 起:
//      - reserve(n) 嚴格地 *只增加* capacity,從不減少;
//      - 想縮減請明確呼叫 shrink_to_fit();
//      - reserve() 無參數版被移除/視為 reserve(0) 的 no-op。
//    這是 LWG issue 2122 / P0966R1 的修正,使語意更直觀。
//
// 6. 例外安全 (Exception Safety)
//    -----------------------------------------------------------------
//    reserve 提供 strong exception guarantee:若中途丟例外
//    (bad_alloc / length_error),原字串內容、size、capacity、迭代器
//    全部保持原狀。實作通常先配置新 buffer 再拷貝,只有全部成功才
//    swap 進去。
//
// 7. 迭代器/指標/參考失效規則
//    -----------------------------------------------------------------
//    若 reserve 真的觸發 reallocation:
//      - 所有迭代器、指標、參考全部失效;
//      - 包含先前 c_str() / data() 回傳的指標。
//    若 reserve 沒有觸發 (capacity 已足夠):
//      - 全部仍有效。
//
// 8. 何時不該用 reserve?
//    -----------------------------------------------------------------
//    - 完全不知道最終大小,而 SSO 又夠用時 — 直接 += 即可;
//    - 處理很多 *短* 字串時,reserve 反而打破 SSO,逼迫實作走 heap 配置;
//    - 在 hot loop 中對「每次都會被覆寫」的字串呼叫 reserve — 第一次
//      之後就是 no-op,反而是浪費 CPU 一個 if 判斷。
//
// =============================================================================
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼 push_back 是「攤銷 (amortized) O(1)」?
//     第 i 次 push_back 觸發 reallocation 的機率 = 1 / (k^j),其中 k 是成長
//     係數、j 是「目前 size 對應第幾次成長」。把 N 次操作的總成本算出來:
//        Total = N + (N/k + N/k^2 + N/k^3 + ...) = N + N/(k-1)
//     當 k = 2 時 Total = 2N → 平均 2 ops/字元 → O(1)。
//     reserve 的價值就在於把 N/(k-1) 那一項砍到 0。
//
// (B) reserve 與 vector::reserve 的差異
//     - vector::reserve 完全相同邏輯;
//     - 但 string 多了 SSO 干擾,使得 capacity() 在小字串時可能呈現
//       「奇怪的固定值」(15 / 22 / 23) 而與你呼叫的數值無關;
//     - string 的結尾 '\0' 不算進 size() 也不算進 capacity()
//       (capacity 計算的是「可寫入字元的上限」,'\0' 之外另有空間)。
//
// (C) reserve 不是「配置正好 N」的手段
//     若你需要「正好 N 個字元」的 buffer 用作 C API 回填 (read/recv),
//     正確作法是 resize(N),它會把 size 設為 N、capacity >= N,並可從
//     &s[0] 直接寫入 N 個 byte。reserve(N) 之後 size 仍是 0,寫到
//     &s[0] 是 UB。
//
// (D) 估算技巧
//     - 拼接已知字串:reserve(a.size() + b.size() + 分隔符 + ...);
//     - 重複字元: reserve(count);
//     - 不確定但有上限:reserve(估上限),寧可大一點;
//     - 連線/壓縮場景估不到:每讀 4KB chunk 後 reserve(s.size()*2)。
//
// =============================================================================
//
// 【注意事項 Pay Attention】
// 1. reserve 不會改變 size(),只改變 capacity()。
// 2. reserve 後仍需用 push_back / += / append 等寫入資料才會增加 size。
// 3. C++20 起 reserve(0) 是 no-op,要縮回實際大小請用 shrink_to_fit()。
// 4. reserve 觸發 reallocation 時所有迭代器、指標、參考會失效。
// 5. reserve 超過 max_size() 會丟 std::length_error。
// 6. 想用 string 當 read buffer 給 C API,請用 resize 而非 reserve。
// =============================================================================

/*
補充筆記：std::string::reserve
  - reserve 只改 capacity，不改 size；它是降低多次 append 重配成本的工具。
  - reserve 可能使既有指標、reference、iterator 失效。
  - 不要把 reserve 當成建立可寫元素，真正改變長度要用 resize 或 append。
  - std::string::reserve 是 std::string 主題的一部分；先分清楚這個操作會讀取、追加、插入、刪除、搜尋，還是只暴露底層字元。
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

void demoReserve() {
    std::string s;
    std::cout << "init     : size=" << s.size() << ", capacity=" << s.capacity() << "\n";

    s.reserve(100);
    std::cout << "after reserve(100): size=" << s.size() << ", capacity=" << s.capacity() << "\n";

    // 寫入資料 — size 增加,capacity 已足夠不會 reallocate
    for (int i = 0; i < 50; ++i) s += 'A';
    std::cout << "after 50 appends  : size=" << s.size() << ", capacity=" << s.capacity() << "\n";
}

// -----------------------------------------------------------------------------
// 【效能對比範例】重複 += 一百萬次
// 為何用 reserve: 預先 reserve 可避免 ~20 次 reallocation,效能差數倍。
// -----------------------------------------------------------------------------
#include <chrono>
void benchmarkReserve() {
    constexpr int N = 1'000'000;

    auto t1 = std::chrono::high_resolution_clock::now();
    {
        std::string s;
        for (int i = 0; i < N; ++i) s += 'x';   // 沒有 reserve,會反覆 reallocate
    }
    auto t2 = std::chrono::high_resolution_clock::now();

    auto t3 = std::chrono::high_resolution_clock::now();
    {
        std::string s;
        s.reserve(N);                           // 一次到位,後續零 reallocation
        for (int i = 0; i < N; ++i) s += 'x';
    }
    auto t4 = std::chrono::high_resolution_clock::now();

    auto noReserve   = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    auto withReserve = std::chrono::duration_cast<std::chrono::microseconds>(t4 - t3).count();
    std::cout << "no reserve   : " << noReserve   << " us\n";
    std::cout << "with reserve : " << withReserve << " us\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 38. Count and Say (Medium)
// 題目: 連續描述數列。第 1 項 = "1",第 n 項 = 描述第 n-1 項的「連續數字段」。
//       例如 "1" → "11" (一個 1) → "21" (兩個 1) → "1211" (一個 2、一個 1) → ...
// 為何用 reserve: 結果字串長度成長很快 (大約乘 1.3x/輪),reserve 估算大小
//                 可避免每輪 += 都觸發 reallocation。
// 思路: 雙指標掃過上一輪字串,統計同字元連續長度,輸出「長度+字元」。
// 複雜度: 每輪 O(L_i),總和 O(sum L_i)。
// -----------------------------------------------------------------------------
std::string countAndSay(int n) {
    std::string s = "1";
    for (int k = 2; k <= n; ++k) {
        std::string next;
        next.reserve(s.size() * 2);             // 預估上限,避免反覆 realloc
        size_t i = 0;
        while (i < s.size()) {
            size_t j = i;
            while (j < s.size() && s[j] == s[i]) ++j;
            next += static_cast<char>('0' + (j - i));   // 長度
            next += s[i];                                // 字元
            i = j;
        }
        s = std::move(next);
    }
    return s;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 Daily Work】高效率組合 log line
// 為何用 reserve: 一行 log 通常有「時間戳 + level + module + msg + 換行」,
//                 長度可預估。reserve 一次到位避免多次 reallocation,在
//                 高 throughput 服務 (每秒數千條 log) 累積差距很大。
// -----------------------------------------------------------------------------
std::string formatLogLine(const std::string& timestamp,
                          const std::string& level,
                          const std::string& module,
                          const std::string& msg) {
    std::string line;
    // 估算: [time] [LEVEL] module: msg\n  → 加上 10 個固定符號的緩衝
    line.reserve(timestamp.size() + level.size() + module.size() + msg.size() + 10);
    line += '[';
    line += timestamp;
    line += "] [";
    line += level;
    line += "] ";
    line += module;
    line += ": ";
    line += msg;
    line += '\n';
    return line;
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例 2】LeetCode 1page. 1page. To Lower Case
// 題目: LeetCode 709. To Lower Case
// 把字串內每個英文字母改小寫。
// 為何用 reserve: 雖然 s 長度不變,但若以 += 重新構建,reserve 可省 reallocation。
// 複雜度: O(N)。
// -----------------------------------------------------------------------------
std::string toLowerWithReserve(const std::string& s) {
    std::string out;
    out.reserve(s.size());           // 預配記憶體
    for (char c : s) {
        out += (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
    }
    return out;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】batch SQL builder ── 預估容量後構建大字串
// 為何用 reserve: 多筆 insert / update 一次性產生大字串,先估容量可避免多次 realloc。
// -----------------------------------------------------------------------------
std::string buildBatchInsert(const std::string& table,
                             const std::vector<std::vector<std::string>>& rows) {
    if (rows.empty()) return "";
    // 估算: "INSERT INTO ... VALUES " (~30) + 每筆 ~rowSize 字元
    size_t estimate = 64 + rows.size() * (rows[0].size() * 16 + 4);
    std::string sql;
    sql.reserve(estimate);
    sql += "INSERT INTO ";
    sql += table;
    sql += " VALUES ";
    for (size_t i = 0; i < rows.size(); ++i) {
        if (i > 0) sql += ", ";
        sql += "(";
        for (size_t j = 0; j < rows[i].size(); ++j) {
            if (j > 0) sql += ",";
            sql += "'";
            sql += rows[i][j];
            sql += "'";
        }
        sql += ")";
    }
    return sql;
}

int main() {
    demoReserve();
    std::cout << "\n=== reserve 效能對比 ===\n";
    benchmarkReserve();

    std::cout << "\n=== LeetCode 38 ===\n";
    for (int i = 1; i <= 6; ++i) std::cout << countAndSay(i) << "\n";

    std::cout << "\n=== LeetCode 709 ===\n";
    std::cout << toLowerWithReserve("Hello World") << "\n";   // hello world

    std::cout << "\n=== 日常實務: log line ===\n";
    std::cout << formatLogLine("2026-05-04 10:30:00", "INFO", "auth",
                                "user login: alice@example.com");

    std::cout << "\n=== 日常實務: batch SQL ===\n";
    std::cout << buildBatchInsert("users",
                                  {{"1","alice"},{"2","bob"},{"3","carol"}}) << "\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1:reserve 與 resize 的關鍵差別?
    //    A:reserve(N) 只動 capacity,size() 不變、元素不存在;從 &s[0]
    //      寫入 N byte 是 UB。resize(N) 會把 size 設為 N、預設 fill '\0',
    //      然後可從 &s[0] 安全寫入。要當 read/recv 的 buffer 用 resize,
    //      事先預配記憶體用 reserve。
    //
    //  Q2:C++20 的 reserve(0) 與 reserve() 行為改了什麼?
    //    A:C++17 之前 reserve(0) 與無參數 reserve() 是「縮容量」的隱含
    //      指令;C++20 (LWG 2122 / P0966R1) 起 reserve 嚴格只增不減,
    //      reserve(0) 變 no-op。要縮減請改用 shrink_to_fit(),語意更清楚。
    //
    //  Q3:為什麼 capacity 用「乘倍」而非「加常數」成長?
    //    A:加常數成長會讓 N 次 push_back 累計 O(N^2) 拷貝;乘倍 (k=1.5
    //      或 2) 累計 O(N),平均每次 O(1)。等比級數的總和 1+2+4+...+N
    //      = 2N-1。標準雖未強制,但三大實作都採乘倍。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -O2 -Wall -Wextra reserve.cpp -o reserve

// === 預期輸出 (節錄) ===
// === LeetCode 709 ===
// hello world
// === 日常實務: batch SQL ===
// INSERT INTO users VALUES ('1','alice'), ('2','bob'), ('3','carol')
