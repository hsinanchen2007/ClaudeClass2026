// =============================================================================
//  第 19 課：vector 與原始陣列的互操作 13  —  vector<char> 當作 C API 的輸出緩衝區
// =============================================================================
//
// 【主題資訊 Information】
//   T*       vector<T>::data() noexcept;         // C++11 起
//   const T* vector<T>::data() const noexcept;
//
//   標頭檔：<vector>、<cstring>（strlen/strcpy）、<string>
//   複雜度：data() 為 O(1)；resize() 為 O(新舊 size 差)
//   本檔標準：C++17
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼要用 vector<char> 而不是 char buf[256]】
//   C API 的典型長相是「你給我一塊記憶體和它的大小，我把結果寫進去」。
//   用固定大小的區域陣列 char buf[256] 有三個問題：
//     (a) 大小寫死。需要 1MB 時就爆堆疊（典型堆疊只有 8MB，本機 ulimit -s 可查）。
//     (b) 無法在執行期依 API 回報的需求量調整。
//     (c) 沒有 RAII——若中途 return 或丟例外，你得自己記得處理（用 new[] 時尤甚）。
//   vector<char> 三個問題全解：大小可在執行期決定、可 resize、解構自動釋放。
//   而且因為 vector 保證元素連續，buffer.data() 拿到的就是一塊道地的 char 陣列，
//   C 函式完全分辨不出它來自 STL。
//
// 【2. data() 與 &v[0] 的差別】
//   兩者在 v 非空時等價，但 v 為空時：
//       v.data()  → 合法，回傳一個不可解參考的指標（可能是 nullptr）
//       &v[0]     → UB，因為 v[0] 本身就是越界存取
//   所以取底層指標一律用 data()，這也是 C++11 特地補上這個成員函式的原因。
//
// 【3. 「兩階段呼叫」慣用法】
//   許多 OS API（Windows 的 GetComputerName、POSIX 的 readlink、
//   甚至 snprintf）都支援「先問長度，再要資料」：
//       int need = api(nullptr, 0);        // 第一次：只問需要多大
//       std::vector<char> buf(need + 1);   // 依需求配置（+1 給 '\0'）
//       api(buf.data(), buf.size());       // 第二次：真的取資料
//   這個 pattern 只有在緩衝區大小可於執行期決定時才成立，正是 vector 的價值。
//
// 【4. 從 vector<char> 轉回 std::string 的兩種寫法】
//       std::string s(buffer.data());        // 讀到第一個 '\0' 為止
//       std::string s(buffer.data(), len);   // 明確指定長度（推薦）
//   後者才是正確做法：它不依賴 '\0'，也能保留內含 '\0' 的二進位資料，
//   而且省掉一次 strlen 掃描。
//
// 【概念補充 Concept Deep Dive】
//   ● 為什麼要 vector<char> buffer(256, '\0') 而不是 buffer.reserve(256)
//     reserve 只動 capacity，size 仍是 0。此時 buffer.data() 指向的記憶體
//     雖然已配置，但那些位置**尚未建構任何元素**，寫入它們是 UB
//     （實務上通常「看起來能動」，但這正是最危險的地方——它不會當場出錯）。
//     必須用建構子或 resize() 讓 size 真的變成 256，元素才算存在。
//
//   ● 為什麼多留一格給 '\0'
//     C 字串以 '\0' 結尾。若 API 寫入 len 個字元再加一個 '\0'，
//     緩衝區至少要 len + 1 格。本檔的 os_get_hostname 就是這樣檢查的：
//     buffer_size < len + 1 就回報失敗。少算這 1 格是經典的 off-by-one 溢位。
//
//   ● size_t 與 int 的轉換
//     buffer.size() 是 std::size_t（無號 64-bit），C API 常吃 int。
//     直接傳會有隱式轉換，開 -Wconversion 就會警告。
//     本檔明確寫 static_cast<int>，並且先檢查沒有溢位，是比較誠實的寫法。
//
// 【注意事項 Pay Attention】
//   1. 只有 size() 涵蓋的範圍才是「已建構的元素」；reserve 出來的空間不可直接寫。
//   2. 任何可能改變 capacity 的操作（push_back / resize 變大 / insert）之後，
//      先前取得的 data() 指標即失效，必須重新取得。
//   3. 傳給 C API 的長度務必是「格數」而不是「位元組數」——對 vector<char>
//      兩者相同，但對 vector<int> 就差 4 倍，這是很常見的錯誤。
//   4. 用 buffer.data() 當 C 字串印出前，要確定裡面真的有 '\0'。
//      本檔一開始就把整塊填成 '\0'，所以安全。
//   5. vector<char> 與 std::string 都能當緩衝區；C++17 起 string 的 data()
//      也回傳非 const 指標，可直接給 C API 寫。選哪個看語意：
//      裝「文字」用 string，裝「位元組」用 vector<char> 或 vector<std::byte>。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】vector<char> 作為 C API 緩衝區
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼要寫 vector<char> buf(256) 而不是 buf.reserve(256)？
//     答：reserve 只增加 capacity，size 仍為 0，那塊記憶體上「還沒有元素存在」，
//         把它交給 C API 寫入是 UB。必須用建構子或 resize() 讓 size 真的變成 256。
//     追問：那 reserve 完直接寫 data()[0] 實際上會崩潰嗎？→ 不一定，
//         通常「看起來能跑」，這正是它危險的原因——錯誤不會當場暴露，
//         但 size() 仍是 0，後續 for 迴圈、複製、序列化全都會漏掉這些資料。
//
// 🔥 Q2. data() 和 &v[0] 差在哪？
//     答：v 非空時兩者等價。v 為空時 data() 合法（回傳不可解參考的指標），
//         &v[0] 則是 UB，因為 v[0] 已經越界。C++11 補上 data() 就是為了這個。
//     追問：空 vector 的 data() 一定回傳 nullptr 嗎？→ 不保證。
//         標準只說「回傳的指標不可解參考」，是否為 nullptr 由實作決定。
//
// ⚠️ 陷阱. std::string s(buffer.data()) 和 std::string s(buffer.data(), len)
//         哪個對？很多人覺得都一樣。
//     答：後者才穩。前者靠 '\0' 決定長度——若 API 沒寫 '\0'、
//         或資料本身含有 '\0'（二進位），長度就錯了。
//         明確給 len 既正確又省掉一次 strlen 掃描。
//     為什麼會錯：腦中把 vector<char> 當成「就是 C 字串」，
//         但它其實是「一塊位元組」，是否為合法 C 字串取決於內容有沒有終止符。
//         二進位協定（例如封包裡的欄位）幾乎一定會踩到這個坑。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <cstring>
#include <string>
#include <cstdio>     // std::snprintf

// 模擬一個作業系統 API（類似 Windows 的 GetComputerName 或 POSIX 的 read）
// 把字串寫入 buffer，回傳寫入的位元組數
int os_get_hostname(char* buffer, int buffer_size) {
    const char* hostname = "my-workstation";
    int len = static_cast<int>(std::strlen(hostname));

    if (buffer_size < len + 1) {
        return -1;  // 緩衝區太小
    }

    std::strcpy(buffer, hostname);
    return len;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 1】「兩階段呼叫」：先問長度，再配置，再取資料
//   情境：這是 Win32 / POSIX API 最常見的介面設計。
//         傳 nullptr 進去代表「我只是問你需要多大」，回傳所需長度；
//         第二次才給真正的緩衝區。vector 讓第二步的「依需求配置」變得無痛。
// -----------------------------------------------------------------------------
int os_get_config_path(char* buffer, int buffer_size) {
    const char* path = "/etc/myapp/config.yaml";
    int len = static_cast<int>(std::strlen(path));
    if (buffer == nullptr || buffer_size == 0) {
        return len;                 // 第一階段：只回報需要多少格（不含 '\0'）
    }
    if (buffer_size < len + 1) {
        return -1;                  // 空間不足
    }
    std::strcpy(buffer, path);
    return len;                     // 第二階段：實際寫入長度
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】用 snprintf 的回傳值精準配置緩衝區，組出 log 行
//   情境：snprintf 有個很有用的性質——即使緩衝區不夠，它也會回報
//         「完整輸出需要多少字元」。搭配 vector<char> 就能保證一次到位、
//         不截斷、不浪費，這是伺服器程式組 log / 組 SQL 的標準做法。
// -----------------------------------------------------------------------------
std::string format_log_line(const char* level, const char* msg, int code) {
    const char* fmt = "[%s] %s (code=%d)";
    // 第一階段：傳 nullptr、大小 0，只問需要多長
    int need = std::snprintf(nullptr, 0, fmt, level, msg, code);
    if (need < 0) return {};
    // 第二階段：配置 need + 1（含結尾 '\0'），一次寫完
    std::vector<char> buf(static_cast<std::size_t>(need) + 1);
    std::snprintf(buf.data(), buf.size(), fmt, level, msg, code);
    return std::string(buf.data(), static_cast<std::size_t>(need));
}

// 註：本檔不附 LeetCode 範例。「把 vector<char> 當 C API 輸出緩衝區」
//     是系統程式設計的介面問題，LeetCode 一律用 std::string / vector<int>
//     直接傳值，沒有等價題型；硬掛一題只會模糊本課重點。

int main() {
    std::cout << "=== 1. vector<char> 當 C API 緩衝區 ===\n";
    // 用 vector<char> 當作動態緩衝區
    // 注意是建構子 (256, '\0') 而不是 reserve(256)：
    // 必須讓 size 真的變成 256，元素才存在，才可以交給 C 函式寫入
    std::vector<char> buffer(256, '\0');
    std::cout << "buffer.size()     = " << buffer.size() << "\n";
    std::cout << "buffer.capacity() = " << buffer.capacity() << "\n";

    int len = os_get_hostname(buffer.data(), static_cast<int>(buffer.size()));

    if (len > 0) {
        // 可以直接用 buffer.data() 當作 C 字串（因為整塊預先填了 '\0'）
        std::cout << "主機名稱：" << buffer.data() << std::endl;
        std::cout << "長度：" << len << std::endl;

        // 也可以轉成 std::string——建議用「明確長度」版本
        std::string hostname(buffer.data(), static_cast<std::size_t>(len));
        std::cout << "std::string：" << hostname
                  << "（size=" << hostname.size() << "）\n";
    }

    std::cout << "\n=== 2. 緩衝區太小時 API 回報失敗 ===\n";
    std::vector<char> tiny(4, '\0');
    int r = os_get_hostname(tiny.data(), static_cast<int>(tiny.size()));
    std::cout << "用 4 格緩衝區呼叫 → 回傳 " << r
              << "（-1 代表空間不足，沒有發生溢位）\n";

    std::cout << "\n=== 3. data() 對空 vector 也安全 ===\n";
    std::vector<char> empty;
    std::cout << "empty.size() = " << empty.size()
              << "，empty.data() 可安全呼叫（但不可解參考）\n";
    std::cout << "data() == nullptr ? " << std::boolalpha
              << (empty.data() == nullptr)
              << "（是否為 nullptr 由實作決定，標準不保證）\n";

    std::cout << "\n=== 日常實務 1：兩階段呼叫取得設定檔路徑 ===\n";
    // 第一階段：只問需要多大
    int need = os_get_config_path(nullptr, 0);
    std::cout << "第一階段：API 回報需要 " << need << " 格（不含結尾 '\\0'）\n";

    // 第二階段：依需求精準配置
    std::vector<char> pathBuf(static_cast<std::size_t>(need) + 1, '\0');
    int got = os_get_config_path(pathBuf.data(), static_cast<int>(pathBuf.size()));
    std::cout << "第二階段：實際寫入 " << got << " 格\n";
    std::cout << "設定檔路徑：" << std::string(pathBuf.data(),
                                               static_cast<std::size_t>(got)) << "\n";
    std::cout << "緩衝區大小 " << pathBuf.size() << " 格，剛好不多不少\n";

    std::cout << "\n=== 日常實務 2：snprintf 精準配置組 log 行 ===\n";
    std::cout << format_log_line("ERROR", "資料庫連線逾時", 504) << "\n";
    std::cout << format_log_line("INFO", "服務啟動完成", 0) << "\n";
    std::string line = format_log_line("WARN", "磁碟使用率偏高", 85);
    std::cout << line << "（字串長度 " << line.size() << "，沒有截斷也沒有浪費）\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 19 課：vector 與原始陣列的互操作13.cpp" -o demo13

// === 預期輸出 ===
// === 1. vector<char> 當 C API 緩衝區 ===
// buffer.size()     = 256
// buffer.capacity() = 256
// 主機名稱：my-workstation
// 長度：14
// std::string：my-workstation（size=14）
//
// === 2. 緩衝區太小時 API 回報失敗 ===
// 用 4 格緩衝區呼叫 → 回傳 -1（-1 代表空間不足，沒有發生溢位）
//
// === 3. data() 對空 vector 也安全 ===
// empty.size() = 0，empty.data() 可安全呼叫（但不可解參考）
// data() == nullptr ? true（是否為 nullptr 由實作決定，標準不保證）
//
// === 日常實務 1：兩階段呼叫取得設定檔路徑 ===
// 第一階段：API 回報需要 22 格（不含結尾 '\0'）
// 第二階段：實際寫入 22 格
// 設定檔路徑：/etc/myapp/config.yaml
// 緩衝區大小 23 格，剛好不多不少
//
// === 日常實務 2：snprintf 精準配置組 log 行 ===
// [ERROR] 資料庫連線逾時 (code=504)
// [INFO] 服務啟動完成 (code=0)
// [WARN] 磁碟使用率偏高 (code=85)（字串長度 38，沒有截斷也沒有浪費）
