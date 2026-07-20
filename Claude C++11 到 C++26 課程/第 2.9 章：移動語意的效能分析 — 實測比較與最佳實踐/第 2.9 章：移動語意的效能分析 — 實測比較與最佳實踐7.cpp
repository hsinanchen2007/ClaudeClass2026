// =============================================================================
//  第 2.9 章 範例 7  —  移動「無效」的兩種情況：基本型別與 SSO 短字串
// =============================================================================
//
// 【主題資訊 Information】
//   標準版本：C++11 起（移動語意）；本檔以 C++17 編譯
//   標頭檔  ：<string>、<utility>、<type_traits>、<cstdint>、<chrono>
//   核心結論：移動的收益 =「省下的那一次 heap 配置 + 逐元素複製」。
//             沒有 heap 配置的型別，移動就沒有任何收益。
//
// 【詳細解釋 Explanation】
//
// 【1. 移動語意到底在省什麼】
//   很多人把 std::move 理解成「比較快的複製」，這是錯的心智模型。
//   移動之所以快，唯一的原因是它「不做那件昂貴的事」：
//       複製：配置一塊新 heap → 把 n 個位元組搬過去      → O(n) + 一次配置
//       移動：把指標抄過來、來源設成空殼                  → O(1)、零配置
//   所以真正被省下的是「配置 + 逐元素複製」。
//   一個型別如果本來就不做 heap 配置，那移動就沒有東西可以省。
//
// 【2. 情況一：基本型別（int / double / 指標 / enum）】
//   int 只是 4 個位元組的值，沒有任何外部資源。
//   std::move(x) 對它做的事是：把 x 轉成 int&&，然後……用它初始化一個新的 int。
//   「用一個 int 初始化另一個 int」就是位元複製，沒有第二種做法。
//   所以 int 的「移動」與「複製」產生完全相同的機器碼。
//   另一個關鍵差別：移動基本型別「不會改動來源」。
//   類別型別被移動後是「有效但未指定」狀態，但 int 被 move 之後值原封不動 ——
//   因為根本沒有發生任何搬移。
//
// 【3. 情況二：SSO 短字串】
//   libstdc++ 的 std::string 對長度 ≤ 15 的字串採用 Small String Optimization：
//   字元直接存在 string 物件自己的記憶體裡（一小塊內建緩衝區），完全不碰 heap。
//       長字串： [ ptr | size | cap ] ──► heap: "xxxxxxxx..."   ← 有東西可以偷
//       短字串： [ ptr | size | "Hi\0............" ]            ← 資料就在物件內，偷不走
//   既然沒有 heap 緩衝區，移動就沒有指標可以接管，只能把那 16 個位元組
//   照樣複製一次。所以短字串的移動與複製成本幾乎相同。
//   注意：SSO 門檻 15 是 libstdc++ 的實作定義值（libc++ 為 22），不可寫死。
//
// 【4. 那什麼時候移動才有效】
//   當物件持有「指向外部資源的控制區塊」時：長字串、vector、unique_ptr、
//   檔案 handle、socket、鎖……。判斷準則很簡單：
//       這個型別的複製建構會不會做 heap 配置或系統呼叫？
//       會 → 移動有意義；不會 → 移動是零收益。
//
// 【概念補充 Concept Deep Dive】
//   本檔刻意不用碼錶量時間，改用「可重現的觀測」，理由有二：
//     (a) 前一版用 -O2 量 int 迴圈，量到 0 ms —— 因為那個迴圈沒有副作用，
//         整段被最佳化器當死碼刪掉了。「0 ms」不代表移動很快，
//         而是「這段程式根本沒被執行」。這是效能量測最經典的假象。
//     (b) 牆鐘時間不可重現，教材無法貼出穩定的預期輸出。
//   改用的觀測是「字串資料是否落在物件自己的記憶體範圍內」與
//   「移動後目的地是否接管了來源原本的緩衝區」，兩者都是布林值，
//   由語言與實作規則決定，跑幾次都一樣，而且直接命中「有沒有東西可偷」這個本質。
//
// 【注意事項 Pay Attention】
// 1. 不可印出被移動後字串的「內容」—— 那是有效但未指定的狀態。
//    本檔只印「資料是否位於物件內部」與 size() 這類有定義的觀測。
// 2. 對基本型別而言，移動後來源的值不變，這是有保證的（因為它就是複製）；
//    不要把這個結論套用到類別型別上。
// 3. 指標比較一律先轉成 std::uintptr_t 再比整數，並且只輸出布林值：
//    跨不同物件的原始指標做關係比較屬未指定行為，而原始位址每次執行都不同。
// 4. Timer 的成員初始化順序必須與宣告順序一致，否則觸發 -Wreorder
//    （-Wall 預設開啟）。本檔前一版即有此問題，已修正。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】什麼時候 std::move 是白寫的
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 對一個 int 呼叫 std::move 會發生什麼事？有沒有效能差別？
//     答：沒有任何差別。std::move(x) 只是把 x 轉型成 int&&，
//         接著用它初始化新的 int —— 而「用 int 初始化 int」就是位元複製。
//         移動與複製會產生完全相同的機器碼。附帶一提，來源的值不會被改動，
//         這點與類別型別「移動後為有效但未指定」的行為不同。
//     追問：那對 std::unique_ptr 呢？
//         → 完全不同。unique_ptr 不能複製，只能移動；移動會把來源設為
//           nullptr（這是有明確保證的，不是未指定），所有權真的轉移了。
//
// 🔥 Q2. 為什麼短字串用 std::move 幾乎沒有好處？
//     答：因為 SSO。libstdc++ 對長度 ≤ 15 的字串把字元直接存在 string
//         物件內部，不做 heap 配置。移動的收益來自「接管對方的 heap 緩衝區」，
//         短字串根本沒有那塊緩衝區可以接管，只能照樣把內建緩衝區複製一次。
//     追問：那你會因此建議「短字串不要用 std::move」嗎？
//         → 不會。收益是零，但成本也是零，而且你通常無法在編譯期得知
//           執行時字串多長。維持一致的寫法比微觀最佳化重要。
//
// ⚠️ 陷阱. 「我量過了，int 的移動迴圈是 0 ms，複製是 30 ms，
//           所以移動連基本型別都比較快。」
//     答：那個 0 ms 代表迴圈被最佳化器整段刪除了，不代表它跑得快。
//         `int m = std::move(s); (void)m;` 沒有任何可觀測的副作用，
//         -O2 之下編譯器可以合法地把整個迴圈移除。你量到的是「沒有執行」。
//     為什麼會錯：把「時間變成 0」直接讀成「速度變成無限快」，
//         沒有懷疑過那段程式碼還在不在。要避免這件事，
//         必須讓結果流向可觀測的地方（累加成 checksum 並輸出），
//         或者像本檔一樣，乾脆改用不會被最佳化掉的結構性觀測。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <utility>
#include <type_traits>
#include <cstdint>
#include <chrono>

// -----------------------------------------------------------------------------
// 觀測工具：只回傳布林，不輸出原始位址（位址每次執行都不同）
//   轉成 std::uintptr_t 再比較：跨不同物件的原始指標做關係比較屬未指定行為，
//   轉成整數後的比較則是實作定義且可攜的常見做法。
// -----------------------------------------------------------------------------
static inline std::uintptr_t addr_of(const void* p) {
    return reinterpret_cast<std::uintptr_t>(p);
}

// 字串資料是否就在 string 物件自己的記憶體範圍內？（是 = 命中 SSO，未用 heap）
static inline bool data_is_inside_object(const std::string& s) {
    const std::uintptr_t obj  = addr_of(&s);
    const std::uintptr_t data = addr_of(s.data());
    return data >= obj && data < obj + sizeof(std::string);
}

// -----------------------------------------------------------------------------
// Timer：耗時輸出到 stderr，stdout 保持逐位元組穩定
//   宣告順序 = 初始化順序（label_ 在前），否則 -Wreorder 會警告。
// -----------------------------------------------------------------------------
class Timer {
    const char* label_;                                        // 宣告第 1
    std::chrono::high_resolution_clock::time_point start_;     // 宣告第 2
public:
    explicit Timer(const char* label)
        : label_(label),
          start_(std::chrono::high_resolution_clock::now()) {}
    ~Timer() {
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(
                      std::chrono::high_resolution_clock::now() - start_).count();
        std::cerr << "  [timing] " << label_ << ": " << us << " us\n";
    }
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 本檔不附
//   理由：本檔談的是「某些型別為什麼從移動語意得不到好處」，
//   屬於記憶體佈局與資源持有的性質，不是演算法問題。
//   LeetCode 沒有任何一題的正確性或複雜度會因為「這裡是移動還是複製」而改變，
//   硬掛一題只會誤導。依規格寧缺勿濫，改附一個真實情境。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】快取項目：鍵是短字串，值是大內容 —— 只有一邊值得 move
//   情境：一個 HTTP 回應快取。key 是 "GET:/api/v1/user"（多半 ≤ 15 字元或略長），
//         value 是整份回應主體（動輒數 KB）。
//   要點：把項目搬進快取時，value 用 std::move 是真的省下一次大複製；
//         key 用不用 std::move 幾乎沒差（SSO 內就沒東西可偷）。
//         真實專案裡值得花力氣檢查的，永遠是那個「大的成員」。
// -----------------------------------------------------------------------------
struct CacheEntry {
    std::string key;    // 短：多半落在 SSO 內
    std::string body;   // 長：一定在 heap

    CacheEntry(std::string k, std::string b)
        : key(std::move(k)), body(std::move(b)) {}   // by-value + move 慣用法
};

int main() {
    std::cout << "=== 情況一：基本型別，移動就是複製 ===\n";
    {
        int source = 42;
        int copied = source;                 // 複製
        int moved  = std::move(source);      // 對 int 而言仍是位元複製

        std::cout << "  複製結果 = " << copied << "，移動結果 = " << moved << "\n";
        std::cout << "  移動後來源的值 = " << source
                  << "（基本型別不會被掏空，這點有保證）\n";
        std::cout << "  is_trivially_copyable<int> = " << std::boolalpha
                  << std::is_trivially_copyable_v<int> << "\n";
        std::cout << "  → 沒有外部資源可以接管，移動與複製產生相同的機器碼\n";
    }

    std::cout << "\n=== 情況二：SSO 短字串，沒有 heap 緩衝區可偷 ===\n";
    {
        std::string shortSrc = "Hi";              // 長度 2，遠在 SSO 門檻內
        std::string copied   = shortSrc;
        std::string moved    = std::move(shortSrc);

        std::cout << "  短字串長度 = " << copied.size() << "\n";
        std::cout << "  複製後資料位於物件內部（未用 heap）？ "
                  << (data_is_inside_object(copied) ? "是" : "否") << "\n";
        std::cout << "  移動後資料位於物件內部（未用 heap）？ "
                  << (data_is_inside_object(moved) ? "是" : "否") << "\n";
        std::cout << "  → 兩者都在物件內部：沒有指標可以接管，\n";
        std::cout << "    移動只能把內建緩衝區照樣複製一次，成本與複製相同\n";
        std::cout << "  （SSO 門檻 15 為 libstdc++ 實作定義值；libc++ 為 22）\n";
    }

    std::cout << "\n=== 對照組：長字串才真的有東西可以偷 ===\n";
    {
        std::string longSrc(10000, 'x');          // 遠超 SSO 門檻，必在 heap
        const auto  srcBuf = addr_of(longSrc.data());

        std::cout << "  長字串資料位於物件內部？ "
                  << (data_is_inside_object(longSrc) ? "是" : "否")
                  << "（否 = 資料在 heap 上）\n";

        std::string moved = std::move(longSrc);
        std::cout << "  移動後 目的地緩衝區 == 來源原本的緩衝區？ "
                  << (addr_of(moved.data()) == srcBuf ? "是" : "否")
                  << "（是 = 整塊 heap 被接管，只搬了指標）\n";
        std::cout << "  移動後 目的地長度 = " << moved.size() << "\n";
        std::cout << "  → 這才是移動語意真正發揮作用的場景\n";
    }

    std::cout << "\n=== 日常實務：快取項目，只有大的那個成員值得在意 ===\n";
    {
        std::string key  = "GET:/api/user";       // 13 字元，落在 SSO 內
        std::string body(8192, 'B');              // 8 KB 回應主體，必在 heap

        const bool keyInObject = data_is_inside_object(key);
        const auto bodyBuf     = addr_of(body.data());

        CacheEntry entry(std::move(key), std::move(body));

        std::cout << "  key 長度 = " << entry.key.size()
                  << "，body 長度 = " << entry.body.size() << "\n";
        std::cout << "  key 原本就在物件內部（SSO）？        "
                  << (keyInObject ? "是" : "否") << "\n";
        std::cout << "  body 的 heap 緩衝區被原封接管？      "
                  << (addr_of(entry.body.data()) == bodyBuf ? "是" : "否") << "\n";
        std::cout << "  → move(key) 省下 0 次配置；move(body) 省下一次 8 KB 的配置與複製\n";
        std::cout << "    最佳化要花在大的成員上，短字串維持一致寫法即可\n";
    }

    // 保留一段牆鐘量測作為對照，但輸出到 stderr，且累加 checksum 阻止死碼消除
    {
        std::cerr << "\n[以下耗時輸出到 stderr，每次執行都不同，僅供參考]\n";
        const int N = 2000000;
        unsigned long long checksum = 0;
        {
            Timer t("短字串 複製 2000000 次");
            for (int i = 0; i < N; ++i) {
                std::string s = "Hi";
                std::string c = s;
                checksum += c.size();
            }
        }
        {
            Timer t("短字串 移動 2000000 次");
            for (int i = 0; i < N; ++i) {
                std::string s = "Hi";
                std::string m = std::move(s);
                checksum += m.size();
            }
        }
        std::cerr << "  [checksum] " << checksum << "（僅為阻止死碼消除）\n";
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 2.9 章：移動語意的效能分析 — 實測比較與最佳實踐7.cpp" -o move_noop
// 執行: ./move_noop             （stdout 穩定；stderr 另有耗時參考）
//       ./move_noop 2>/dev/null （只看可重現的結構性觀測）

// ── 輸出但書 ────────────────────────────────────────────────────────────
// 1. 下方預期輸出只含 stdout，全部是布林值與長度，逐位元組可重現
//    （實測連跑 10 次 md5 相同）。stderr 的 [timing]／[checksum]
//    每次執行都不同，刻意不納入預期輸出，也不可拿來當驗收條件。
// 2.「資料是否位於物件內部」為 libstdc++ 的 SSO 行為（門檻 15，實作定義）。
//    在 libc++（門檻 22）或 MSVC 上，短字串同樣命中 SSO，但門檻數值不同。
// 3. 本檔不印任何被移動後字串的內容 —— 那是有效但未指定的狀態。
//    只印 size() 與「資料位置」這類有明確定義的觀測。
// 4. 基本型別那一段印出移動後的 source 是安全的：對 int 而言移動即複製，
//    來源不會被改動；這個結論不可套用到類別型別。
// 5. 本機環境：GCC 15.2.0 (Ubuntu 15.2.0-16ubuntu1) / libstdc++ / x86-64。

// === 預期輸出 ===
// === 情況一：基本型別，移動就是複製 ===
//   複製結果 = 42，移動結果 = 42
//   移動後來源的值 = 42（基本型別不會被掏空，這點有保證）
//   is_trivially_copyable<int> = true
//   → 沒有外部資源可以接管，移動與複製產生相同的機器碼
//
// === 情況二：SSO 短字串，沒有 heap 緩衝區可偷 ===
//   短字串長度 = 2
//   複製後資料位於物件內部（未用 heap）？ 是
//   移動後資料位於物件內部（未用 heap）？ 是
//   → 兩者都在物件內部：沒有指標可以接管，
//     移動只能把內建緩衝區照樣複製一次，成本與複製相同
//   （SSO 門檻 15 為 libstdc++ 實作定義值；libc++ 為 22）
//
// === 對照組：長字串才真的有東西可以偷 ===
//   長字串資料位於物件內部？ 否（否 = 資料在 heap 上）
//   移動後 目的地緩衝區 == 來源原本的緩衝區？ 是（是 = 整塊 heap 被接管，只搬了指標）
//   移動後 目的地長度 = 10000
//   → 這才是移動語意真正發揮作用的場景
//
// === 日常實務：快取項目，只有大的那個成員值得在意 ===
//   key 長度 = 13，body 長度 = 8192
//   key 原本就在物件內部（SSO）？        是
//   body 的 heap 緩衝區被原封接管？      是
//   → move(key) 省下 0 次配置；move(body) 省下一次 8 KB 的配置與複製
//     最佳化要花在大的成員上，短字串維持一致寫法即可
