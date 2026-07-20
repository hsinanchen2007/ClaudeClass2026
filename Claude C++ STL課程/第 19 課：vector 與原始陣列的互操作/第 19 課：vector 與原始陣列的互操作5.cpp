// =============================================================================
//  第 19 課：vector 與原始陣列的互操作 5  —  兩階段查詢（two-phase query）慣用法
// =============================================================================
//
// 【主題資訊 Information】
//   慣用法 :
//       int needed = c_api(nullptr, 0);      // 第一階段：只問「要多大」
//       std::vector<T> buf(needed);          // 依答案配置
//       int got = c_api(buf.data(), buf.size());  // 第二階段：真的取資料
//       buf.resize(got);                     // 收斂到實際筆數（重要！）
//
//   標頭檔 : <vector>
//   典型 API: POSIX readlink / sysctl / getgrouplist、
//             Windows GetComputerName / RegQueryValueEx、
//             OpenGL glGetProgramInfoLog、SQLite sqlite3_column_bytes
//   複雜度 : 兩次呼叫，配置一次。相對於「猜大小 + 反覆重試」是最少的往返次數。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼 C API 要設計成兩階段】
//   C 函式想回傳「動態長度的資料」時只有三種選擇：
//     (a) 自己 malloc 回傳指標 → 呼叫端必須知道要用哪個 free（跨 DLL/CRT 會出事）
//     (b) 回傳指向靜態緩衝區的指標 → 不可重入、不執行緒安全（strtok 的惡夢）
//     (c) 由呼叫端配置，函式只填寫 → 所有權清楚，但呼叫端得先知道大小
//   (c) 是最乾淨的，代價就是「大小怎麼知道」。兩階段查詢就是對這個代價的標準回答：
//   先用一次不帶緩衝區的呼叫把大小問出來，再配置，再取一次。
//
// 【2. 呼叫慣例有兩種，一定要看清楚文件】
//   風格 A（本檔示範）：空間不足時「回傳所需大小」，成功時回傳實際筆數。
//   風格 B（POSIX 常見）：空間不足時回傳 -1 並設 errno = ERANGE，
//                        呼叫端自己把緩衝區加倍後重試，直到成功。
//   兩者的錯誤處理完全不同。把 A 的程式碼套到 B 的 API 上，
//   你會把 -1 當成「需要 -1 個元素」，接著 vector<T> buf(-1) ——
//   size_type 是無號的，-1 會變成天文數字，通常導致 std::length_error 或
//   std::bad_alloc。這是本主題最常見的實務事故。
//
// 【3. 為什麼第二階段之後往往還要 resize】
//   兩次呼叫之間，系統狀態可能已經改變（多了一個網路介面、少了一個行程）。
//   所以第二階段的回傳值才是「真正寫進去幾筆」，它可能小於第一階段的答案。
//   如果不 resize，vector 的尾端會殘留值初始化的 0，
//   而使用者看到的 size() 是「配置量」而不是「資料量」——這是安靜的資料錯誤。
//   標準做法：buf.resize(got); 把 size 收斂到實際筆數。
//   縮小 resize 不會重新配置（capacity 不變），所以指標仍然有效、成本是 O(縮減量)。
//
// 【4. 競態（race）與重試迴圈】
//   反過來，第二階段也可能回報「還是不夠」（資料在兩次呼叫之間變多了）。
//   健壯的實作會包成迴圈：問大小 → 配置 → 取 → 若仍不足則放大重試，
//   並設一個重試上限避免無窮迴圈。本檔的 fetchWithRetry() 示範這個形狀。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼第一階段傳 nullptr 是安全的
//   這類 API 的契約明文規定「buffer 可以是 NULL，此時只回報所需大小」。
//   這是 API 自己的承諾，不是通則——不要把它推廣到任何 C 函式。
//   對於沒有這個承諾的 API，傳 nullptr 就是未定義行為。
//
// (B) 為什麼不乾脆一開始就配一個很大的緩衝區
//   「猜一個夠大的值」在資料真的變大時就會截斷，而且很多 API 截斷時
//   不會告訴你（只回傳它寫了多少）。你會拿到看似成功、實則不完整的資料。
//   兩階段查詢的價值就在於「大小由資料的擁有者決定」，而不是由你猜。
//
// (C) 與 C++ 風格 API 的對比
//   C++ 直接回傳 std::vector<T> 就好——RVO / move 讓回傳容器幾乎沒有成本，
//   長度資訊天然跟著資料走。兩階段查詢純粹是為了遷就 C 的 ABI 限制。
//   包裝舊 C API 時，最好的做法就是把兩階段查詢藏進一個回傳 vector 的
//   C++ 函式裡，讓呼叫端再也看不到這個細節（本檔的 fetchSystemData() 就是）。
//
// 【注意事項 Pay Attention】
//   1. 先讀懂 API 的錯誤慣例（回傳所需大小 vs 回傳 -1 + errno），兩者處理方式不同。
//   2. 絕對不要把可能是 -1 的回傳值直接拿去建構 vector——
//      size_type 是無號的，-1 會變成極大值，通常導致丟出例外。
//   3. 第二階段之後要用實際回傳的筆數 resize，避免尾端殘留初始值。
//   4. 兩次呼叫之間資料可能變動，正式程式碼要有重試上限。
//   5. 字串類 API 要留意回傳的長度含不含結尾的 '\0'，差一個就是截斷或多印一格。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】兩階段查詢
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 什麼是兩階段查詢？為什麼 C API 要這樣設計？
//     答：先以空緩衝區呼叫一次取得所需大小，配置後再呼叫一次真正取資料。
//         因為 C 函式若自己 malloc 回傳，呼叫端不知道該用哪個 free
//         （跨 DLL / 不同 CRT 會出事）；若回傳靜態緩衝區則不可重入。
//         「呼叫端配置、被呼叫端填寫」所有權最清楚，代價就是要先知道大小。
//     追問：C++ 為什麼不需要這個模式？
//         → 直接回傳 std::vector<T> 即可，move / RVO 讓成本趨近於零，
//           而且長度資訊天然跟著資料走。
//
// 🔥 Q2. 第二階段拿到資料後，為什麼常常還要 resize 一次？
//     答：因為兩次呼叫之間系統狀態可能改變，第二階段實際寫入的筆數
//         可能少於第一階段回報的所需量。不 resize 的話，vector 尾端會留下
//         值初始化的 0，而 size() 反映的是配置量而非資料量，
//         使用者會拿到多出來的假資料。用 buf.resize(實際筆數) 收斂即可，
//         縮小不會重新配置，指標仍然有效。
//     追問：那如果第二階段回報「還是不夠」呢？
//         → 代表資料在兩次呼叫之間變多了，要放大緩衝區重試，並設重試上限。
//
// ⚠️ 陷阱. 「int n = c_api(nullptr, 0); std::vector<int> buf(n);」這樣寫有什麼風險？
//     答：如果該 API 是 POSIX 風格、失敗時回傳 -1，那 n 就是 -1。
//         vector 的 size_type 是無號型別，-1 會被轉成 SIZE_MAX
//         （本機 64-bit 上是 18446744073709551615），
//         接著 vector 通常會丟 std::length_error 或 std::bad_alloc。
//         正確寫法是先檢查 if (n < 0) 處理錯誤，確認為正數後才建構。
//     為什麼會錯：把「回傳所需大小」當成所有兩階段 API 的通則，
//         但另一半的 API 是用 -1 + errno 報錯的。
//         有號回傳值轉無號參數這一步，是 C/C++ 互操作最常見的沉默災難：
//         編譯器不會警告（-Wall -Wextra 也不會），直到執行期才爆炸。
// ═══════════════════════════════════════════════════════════════════════════

#include <cstddef>
#include <iostream>
#include <vector>

// -----------------------------------------------------------------------------
// 模擬一個 C API：第一次呼叫取得大小，第二次呼叫填入資料
//   契約（風格 A）：buffer 為 nullptr 或空間不足 → 回傳所需筆數；
//                   否則填入資料並回傳實際寫入筆數。
// -----------------------------------------------------------------------------
int get_system_data(int* buffer, int buffer_size) {
    const int data[] = {11, 22, 33, 44, 55, 66, 77};
    const int data_count = 7;

    if (buffer == nullptr || buffer_size < data_count) {
        return data_count;              // 告訴呼叫者需要多大的空間
    }
    for (int i = 0; i < data_count; ++i) {
        buffer[i] = data[i];
    }
    return data_count;                  // 實際寫入筆數
}

// -----------------------------------------------------------------------------
// 把兩階段查詢包成一個乾淨的 C++ 介面：呼叫端再也看不到這個細節
// -----------------------------------------------------------------------------
std::vector<int> fetchSystemData() {
    int needed = get_system_data(nullptr, 0);      // 第一階段
    if (needed <= 0) return {};                    // 防禦：非正數一律當作沒資料

    std::vector<int> buf(static_cast<std::size_t>(needed));
    int got = get_system_data(buf.data(), static_cast<int>(buf.size()));  // 第二階段
    if (got < 0) return {};

    buf.resize(static_cast<std::size_t>(got));     // 收斂到實際筆數
    return buf;
}

// -----------------------------------------------------------------------------
// 帶重試的健壯版本：處理「兩次呼叫之間資料變多」的競態
// -----------------------------------------------------------------------------
std::vector<int> fetchWithRetry(int maxAttempts) {
    std::vector<int> buf;
    int needed = get_system_data(nullptr, 0);

    for (int attempt = 1; attempt <= maxAttempts; ++attempt) {
        if (needed <= 0) return {};
        buf.assign(static_cast<std::size_t>(needed), 0);

        int got = get_system_data(buf.data(), static_cast<int>(buf.size()));
        if (got < 0) return {};

        if (got <= static_cast<int>(buf.size())) {  // 成功寫入
            buf.resize(static_cast<std::size_t>(got));
            std::cout << "  （第 " << attempt << " 次嘗試成功）\n";
            return buf;
        }
        needed = got;                               // 還是不夠，用新值再來
    }
    std::cout << "  （超過重試上限，放棄）\n";
    return {};
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 26. Remove Duplicates from Sorted Array
//   題目：原地移除已排序陣列中的重複元素，回傳新長度 k；
//         呼叫端只保證讀取前 k 個元素，後面的內容不予理會。
//   為什麼用到本主題：這正是 C API 的「回傳實際筆數」慣例在演算法題裡的翻版——
//         緩衝區的物理大小沒有改變，改變的是「有效資料有幾筆」這個回傳值。
//         這也解釋了為什麼真實世界的兩階段查詢一定要拿回傳值去 resize：
//         物理容量和邏輯筆數本來就是兩件事。
//   複雜度：時間 O(n)，空間 O(1)（原地）。
// -----------------------------------------------------------------------------
int removeDuplicates(std::vector<int>& nums) {
    if (nums.empty()) return 0;
    std::size_t k = 1;                       // 已確定不重複的筆數
    for (std::size_t i = 1; i < nums.size(); ++i) {
        if (nums[i] != nums[k - 1]) {
            nums[k] = nums[i];
            ++k;
        }
    }
    return static_cast<int>(k);
}

// -----------------------------------------------------------------------------
// 【日常實務範例】查詢本機所有網路介面的 MTU（模擬 POSIX 風格 ioctl/sysctl）
//   情境：監控程式要列出每張網卡的 MTU。系統 API net_list_mtu() 是 C 寫的，
//         介面數量不固定（USB 網卡、VPN 虛擬介面隨時可能增減），
//         所以只能先問「現在有幾張」，再配置緩衝區去取。
//   為何用兩階段：介面數量由核心決定，呼叫端不可能事先知道；
//         猜一個固定上限（例如 16）在容器/雲端環境會被截斷。
//         這裡也示範第二階段回報數量變少時，用 resize 收斂的必要性。
// -----------------------------------------------------------------------------
int net_list_mtu(int* out, int capacity, int actualCount) {  // 假裝是系統 API
    if (out == nullptr || capacity < actualCount) return actualCount;
    const int mtus[] = {65536, 1500, 1500, 1420, 9000};      // lo / eth / wifi / vpn / jumbo
    for (int i = 0; i < actualCount; ++i) out[i] = mtus[i % 5];
    return actualCount;
}

std::vector<int> listInterfaceMtus(int countAtQuery, int countAtFetch) {
    int needed = net_list_mtu(nullptr, 0, countAtQuery);     // 第一階段
    if (needed <= 0) return {};

    std::vector<int> mtus(static_cast<std::size_t>(needed));
    // 第二階段：模擬期間有一張 VPN 介面斷線，實際筆數變少
    int got = net_list_mtu(mtus.data(), static_cast<int>(mtus.size()), countAtFetch);
    if (got < 0) return {};

    mtus.resize(static_cast<std::size_t>(got));              // 收斂，避免尾端殘留 0
    return mtus;
}

static void printVec(const char* label, const std::vector<int>& v) {
    std::cout << label;
    for (std::size_t i = 0; i < v.size(); ++i) std::cout << (i ? " " : "") << v[i];
    std::cout << "\n";
}

int main() {
    // -------------------------------------------------------------------------
    std::cout << "=== 兩階段查詢的基本流程 ===\n";
    // -------------------------------------------------------------------------
    int needed = get_system_data(nullptr, 0);
    std::cout << "第一階段：需要 " << needed << " 個元素的空間\n";

    std::vector<int> v(static_cast<std::size_t>(needed));
    int got = get_system_data(v.data(), static_cast<int>(v.size()));
    std::cout << "第二階段：實際寫入 " << got << " 筆\n";
    printVec("取得的資料：", v);

    // -------------------------------------------------------------------------
    std::cout << "\n=== 包裝成乾淨的 C++ 介面 ===\n";
    // -------------------------------------------------------------------------
    std::vector<int> data = fetchSystemData();
    std::cout << "fetchSystemData() 回傳 " << data.size() << " 筆\n";
    printVec("內容：", data);

    // -------------------------------------------------------------------------
    std::cout << "\n=== 帶重試上限的健壯版本 ===\n";
    // -------------------------------------------------------------------------
    std::vector<int> retried = fetchWithRetry(3);
    printVec("重試版取得：", retried);

    // -------------------------------------------------------------------------
    std::cout << "\n=== 為什麼一定要 resize：尾端殘留的示範 ===\n";
    // -------------------------------------------------------------------------
    std::vector<int> noResize(7);                 // 依第一階段配置 7 筆
    net_list_mtu(noResize.data(), 7, 3);          // 但實際只有 3 筆
    printVec("忘記 resize：", noResize);
    std::cout << "  ↑ 後面 4 個 0 是值初始化的殘留，不是真實資料\n";

    std::vector<int> withResize(7);
    int realCount = net_list_mtu(withResize.data(), 7, 3);
    withResize.resize(static_cast<std::size_t>(realCount));
    printVec("正確 resize：", withResize);

    // -------------------------------------------------------------------------
    std::cout << "\n=== LeetCode 26. Remove Duplicates from Sorted Array ===\n";
    // -------------------------------------------------------------------------
    std::vector<int> nums1 = {1, 1, 2};
    int k1 = removeDuplicates(nums1);
    std::cout << "[1,1,2] → k = " << k1 << "，前 k 筆：";
    for (int i = 0; i < k1; ++i) std::cout << (i ? " " : "") << nums1[i];
    std::cout << "（vector 物理長度仍是 " << nums1.size() << "）\n";

    std::vector<int> nums2 = {0, 0, 1, 1, 1, 2, 2, 3, 3, 4};
    int k2 = removeDuplicates(nums2);
    std::cout << "[0,0,1,1,1,2,2,3,3,4] → k = " << k2 << "，前 k 筆：";
    for (int i = 0; i < k2; ++i) std::cout << (i ? " " : "") << nums2[i];
    std::cout << "\n";

    // 在真實 C++ 程式裡，拿到 k 之後就該收斂，語意才正確
    nums2.resize(static_cast<std::size_t>(k2));
    std::cout << "resize 之後 size = " << nums2.size() << "（語意才對得上）\n";

    // -------------------------------------------------------------------------
    std::cout << "\n=== 日常實務：列出網路介面 MTU ===\n";
    // -------------------------------------------------------------------------
    std::vector<int> mtus = listInterfaceMtus(5, 4);   // 查詢時 5 張，取用時剩 4 張
    std::cout << "查詢時 5 張介面，實際取得 " << mtus.size() << " 張\n";
    printVec("各介面 MTU：", mtus);

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第\ 19\ 課：vector\ 與原始陣列的互操作5.cpp -o interop5

// === 預期輸出 ===
// === 兩階段查詢的基本流程 ===
// 第一階段：需要 7 個元素的空間
// 第二階段：實際寫入 7 筆
// 取得的資料：11 22 33 44 55 66 77
//
// === 包裝成乾淨的 C++ 介面 ===
// fetchSystemData() 回傳 7 筆
// 內容：11 22 33 44 55 66 77
//
// === 帶重試上限的健壯版本 ===
//   （第 1 次嘗試成功）
// 重試版取得：11 22 33 44 55 66 77
//
// === 為什麼一定要 resize：尾端殘留的示範 ===
// 忘記 resize：65536 1500 1500 0 0 0 0
//   ↑ 後面 4 個 0 是值初始化的殘留，不是真實資料
// 正確 resize：65536 1500 1500
//
// === LeetCode 26. Remove Duplicates from Sorted Array ===
// [1,1,2] → k = 2，前 k 筆：1 2（vector 物理長度仍是 3）
// [0,0,1,1,1,2,2,3,3,4] → k = 5，前 k 筆：0 1 2 3 4
// resize 之後 size = 5（語意才對得上）
//
// === 日常實務：列出網路介面 MTU ===
// 查詢時 5 張介面，實際取得 4 張
// 各介面 MTU：65536 1500 1500 1420
