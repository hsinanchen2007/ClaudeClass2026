// =============================================================================
//  第 10 課：vector 的宣告與初始化方式 1  —  預設建構：空的 vector
// =============================================================================
//
// 【主題資訊 Information】
//   vector() noexcept(noexcept(Allocator()));                    // (1) C++11 起 noexcept
//   explicit vector(const Allocator& alloc) noexcept;            // (2)
//
//   標頭檔　：<vector>
//   標準版本：vector() 自 C++98 就有；{} 與 = {} 的統一初始化語法是 C++11
//   複雜度　：O(1)，且「不配置任何動態記憶體」
//
//   三種等價寫法：
//     std::vector<int>         v1;        // 預設初始化
//     std::vector<double>      v2{};      // 值初始化（C++11 統一初始化）
//     std::vector<std::string> v3 = {};   // 以空的 braced-init-list 初始化
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼這三種寫法結果相同？】
//   對 std::vector 這種「有 user-provided 建構子」的 class type 來說：
//     * `v1;`      → 預設初始化 → 呼叫 default constructor
//     * `v2{};`    → 值初始化   → 因為有 user-provided default ctor，一樣走它
//     * `v3 = {};` → 以空的 braced-init-list 複製初始化 → 同樣落到 default ctor
//   （注意：空的 {} 在多載決議中優先選 default ctor，不是 initializer_list ctor。）
//   三者最終都執行同一段程式碼，所以 size() 與 capacity() 都是 0。
//
//   ⚠️ 但對「內建型別」三者天差地遠：`int a;` 是未初始化（讀取即 UB），
//      `int b{};` 才保證是 0。這正是「一律用 {}」這個 modern C++ 建議的由來。
//
// 【2. 為什麼預設建構不配置記憶體？（lazy allocation）】
//   標準沒有規定 default-constructed vector 的 capacity 必須是 0，但
//   幾乎所有實作都選擇「先不要配置」。理由：
//     * 很多 vector 宣告出來後根本沒被填入資料（例如只有錯誤分支才用到），
//       預先配置等於白花一次配置 + 之後一次釋放。
//     * vector 常被大量建立（例如 vector<vector<int>> 的每一列），
//       預配置會讓建構成本從 O(1) 變成 O(n) 次 heap 操作。
//   代價是：第一次 push_back 一定會觸發一次配置。若你已知大小，
//   應該在宣告後立刻 reserve(n)，把「多次成長」壓成「一次配置」。
//
// 【3. 為什麼 default ctor 是 noexcept？】
//   因為它不配置記憶體，所以不會丟 std::bad_alloc。這個 noexcept 保證很重要：
//   它讓 vector 可以被安全地用在 move constructor、swap 等要求不丟例外的場合，
//   也影響容器成長時能否用 move 取代 copy（見第 11 課容量管理）。
//
// 【概念補充 Concept Deep Dive】
//   libstdc++ 的 vector 內部就是「三根指標」：
//
//       ┌──────────────┬──────────────┬───────────────────┐
//       │ _M_start     │ _M_finish    │ _M_end_of_storage │
//       └──────────────┴──────────────┴───────────────────┘
//         ↑ 資料開頭      ↑ size 結尾     ↑ capacity 結尾
//
//     size()     == _M_finish        - _M_start
//     capacity() == _M_end_of_storage - _M_start
//
//   預設建構時三根指標全是 nullptr，所以 size 與 capacity 自然都是 0。
//   sizeof(std::vector<int>) 在本機（x86-64 / libstdc++ / g++ 15.2）實測是
//   24 bytes（3 × 8 bytes）——這是【實作定義】，MSVC 的 debug iterator 版本更大。
//   注意 sizeof 與元素個數無關：vector 物件本身永遠這麼大，資料在 heap。
//
// 【注意事項 Pay Attention】
//   1. capacity() == 0 是【實作定義】的觀察結果，不是標準保證。標準只保證
//      size() == 0。寫測試時請斷言 empty()，不要斷言 capacity() == 0。
//   2. 空 vector 呼叫 front()、back()、v[0] 都是 UB（不是丟例外）。
//      at(0) 才保證丟 std::out_of_range。
//   3. 空 vector 的 begin() == end()，兩者都是合法可比較的 iterator，
//      range-for 會安全地跑 0 次——不必特別加 if 判斷。
//   4. 判斷是否為空請用 empty() 而非 size() == 0：對 vector 兩者都是 O(1)，
//      但 empty() 是所有容器的通用寫法（歷史上 std::list::size() 曾是 O(n)）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】vector 的預設建構
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::vector<int> v; 之後，v.size() 與 v.capacity() 各是多少？
//     答：size() 標準保證是 0。capacity() 在 libstdc++／libc++／MSVC 實測都是 0，
//         但這是【實作定義】——標準並未規定預設建構後不得配置記憶體。
//         實務上可以依賴「不會配置」這個行為，但測試不該寫死 capacity()==0。
//     追問：那為什麼實作都選擇不配置？→ 避免對「宣告了卻沒用到」的 vector
//         付出配置成本；建 vector<vector<T>> 的 n 列時差距是 n 次 heap 操作。
//
// 🔥 Q2. vector<int> v1; 、vector<int> v2{}; 、vector<int> v3 = {};
//        三者有差別嗎？換成 int 呢？
//     答：對 vector 完全相同，都呼叫 default constructor（空的 {} 在多載決議中
//         優先選 default ctor，不是 initializer_list ctor）。
//         但對內建型別差很多：int a; 未初始化，讀取是 UB；int b{}; 保證為 0。
//     追問：那 vector<int> v(); 呢？→ 這是 most vexing parse，
//         它宣告了一個「回傳 vector<int>、不吃參數的函式」，不是物件。
//
// ⚠️ 陷阱. 「空的 vector 不能取 begin()，要先判斷 if (!v.empty())」——對嗎？
//     答：不對。空 vector 的 begin() 與 end() 都是合法的，而且兩者相等，
//         所以 range-for、std::sort、std::accumulate 都能安全處理空容器（跑 0 次）。
//         真正不能碰的是 front()、back()、v[0]——那些才是 UB。
//     為什麼會錯：把「不能解參考」和「不能取得 iterator」混為一談。
//         end() 本來就是不可解參考的 past-the-end iterator，空容器只是讓
//         begin() 剛好也等於它而已，取得動作本身完全合法。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <vector>

// -----------------------------------------------------------------------------
// 【日常實務範例】逐行讀取設定檔：先宣告空 vector，再累積內容
//   情境：讀取 app.conf，略過空行與 # 註解行，把有效設定收集起來。
//   為什麼用預設建構：讀檔前根本不知道會有幾行有效設定，無法用 vector(n)。
//   這正是「預設建構 + push_back」最典型的場景。
//   小技巧：若能估出上限（例如檔案總行數），先 reserve() 可避免多次重新配置。
// -----------------------------------------------------------------------------
std::vector<std::string> parseConfigLines(const std::vector<std::string>& rawLines) {
    std::vector<std::string> settings;      // 預設建構：size=0, capacity=0
    settings.reserve(rawLines.size());      // 已知上限，一次配置到位

    for (const std::string& line : rawLines) {
        if (line.empty()) continue;         // 略過空行
        if (line[0] == '#') continue;       // 略過註解
        settings.push_back(line);
    }
    return settings;                        // NRVO／move，不會複製整份資料
}

int main() {
    std::cout << "=== 三種預設建構寫法 ===\n";
    std::vector<int>         v1;            // 預設初始化
    std::vector<double>      v2{};          // 值初始化（C++11）
    std::vector<std::string> v3 = {};       // 空的 braced-init-list

    std::cout << std::boolalpha;
    std::cout << "v1  size=" << v1.size() << " capacity=" << v1.capacity()
              << " empty=" << v1.empty() << "\n";
    std::cout << "v2  size=" << v2.size() << " capacity=" << v2.capacity()
              << " empty=" << v2.empty() << "\n";
    std::cout << "v3  size=" << v3.size() << " capacity=" << v3.capacity()
              << " empty=" << v3.empty() << "\n";
    std::cout << "（capacity 為 0 是【實作定義】的觀察，標準只保證 size 為 0）\n";

    std::cout << "\n=== 空 vector 的 iterator 是合法的 ===\n";
    std::cout << "v1.begin() == v1.end() ? " << (v1.begin() == v1.end()) << "\n";
    int loopCount = 0;
    for (int x : v1) { (void)x; ++loopCount; }
    std::cout << "range-for 跑了 " << loopCount << " 次（安全，不需先判斷 empty）\n";

    std::cout << "\n=== sizeof：vector 物件本身的大小與元素個數無關 ===\n";
    std::cout << "sizeof(vector<int>)         = " << sizeof(std::vector<int>) << " bytes\n";
    std::cout << "sizeof(vector<std::string>) = " << sizeof(std::vector<std::string>) << " bytes\n";
    std::cout << "（本機 x86-64 / libstdc++ 實測 = 3 根指標；此為【實作定義】）\n";

    std::cout << "\n=== 預設建構後第一次 push_back 才配置記憶體 ===\n";
    std::vector<int> grow;
    std::cout << "push_back 前 : capacity=" << grow.capacity() << "\n";
    grow.push_back(1);
    std::cout << "push_back 後 : capacity=" << grow.capacity() << "（本機 libstdc++ 實測）\n";

    std::cout << "\n=== 日常實務：解析設定檔 ===\n";
    std::vector<std::string> raw = {
        "# 資料庫設定", "host=127.0.0.1", "", "port=5432", "# 快取", "cache_ttl=300"
    };
    std::vector<std::string> conf = parseConfigLines(raw);
    std::cout << "原始 " << raw.size() << " 行，有效設定 " << conf.size() << " 筆：\n";
    for (const std::string& s : conf) std::cout << "  " << s << "\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 10 課：vector 的宣告與初始化方式1.cpp" -o lesson10_1

// === 預期輸出 ===
// === 三種預設建構寫法 ===
// v1  size=0 capacity=0 empty=true
// v2  size=0 capacity=0 empty=true
// v3  size=0 capacity=0 empty=true
// （capacity 為 0 是【實作定義】的觀察，標準只保證 size 為 0）
//
// === 空 vector 的 iterator 是合法的 ===
// v1.begin() == v1.end() ? true
// range-for 跑了 0 次（安全，不需先判斷 empty）
//
// === sizeof：vector 物件本身的大小與元素個數無關 ===
// sizeof(vector<int>)         = 24 bytes
// sizeof(vector<std::string>) = 24 bytes
// （本機 x86-64 / libstdc++ 實測 = 3 根指標；此為【實作定義】）
//
// === 預設建構後第一次 push_back 才配置記憶體 ===
// push_back 前 : capacity=0
// push_back 後 : capacity=1（本機 libstdc++ 實測）
//
// === 日常實務：解析設定檔 ===
// 原始 6 行，有效設定 3 筆：
//   host=127.0.0.1
//   port=5432
//   cache_ttl=300
