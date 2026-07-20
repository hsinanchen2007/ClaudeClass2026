// =============================================================================
//  lesson32_performance.cpp  —  移動 vs 拷貝：用「配置次數」量化移動語意的價值
// =============================================================================
//
// 【主題資訊 Information】
//   主題      : Move Constructor 的效能意義（為什麼移動比拷貝快）
//   標準版本  : C++11 起（右值引用、移動建構、移動賦值）
//   標頭檔    : <string> <vector> <memory> <utility> <chrono> <cstddef>
//   複雜度    : 拷貝一個長度 L 的字串 = O(L) + 1 次 heap 配置
//               移動一個字串         = O(1)  + 0 次 heap 配置
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼「拷貝一個字串」很貴？】
//   std::string 的物件本體很小（libstdc++ 實測 sizeof 為 32 bytes：一個指標、
//   一個長度、外加 16 bytes 的小字串緩衝區），但它「擁有」一塊 heap 上的
//   字元陣列。拷貝時必須做兩件事：
//     (a) 向 allocator 要一塊新的 heap 記憶體  ← 這一步才是主要成本
//     (b) 把 L 個位元組複製過去
//   (a) 貴在它可能要走 free-list 搜尋、在多執行緒下爭搶 malloc arena 的鎖，
//   甚至向 OS 要新頁面（觸發 page fault）。這些成本與字串長度無關，是「固定
//   的一大筆」，所以即使字串不長，配置本身也不便宜。
//
// 【2. 移動做了什麼？】
//   移動建構只把「指標、長度、容量」這幾個純量欄位搬過去，再把來源歸零。
//   完全不碰 heap。所以不論字串是 1000 字元還是 100 MB，移動的成本都相同。
//   這是「常數時間 vs 線性時間」的差別，也是移動語意最核心的價值。
//
// 【3. 為什麼本檔用「配置次數」而不是只看毫秒數？】
//   計時器量到的數字受 CPU 頻率調節、cache 冷熱、記憶體壓力與 OS 排程影響，
//   每次執行都不一樣；寫進教材就成了無法重現的宣稱。
//   「allocator 被呼叫了幾次」則是由程式語意決定的確定值：
//     - 拷貝 N 個超過 SSO 門檻的字串 → 剛好 N 次配置
//     - 移動 N 個字串                → 剛好 0 次配置
//   這個數字每次執行都一樣，而且比毫秒數更能說明「省下的到底是什麼」。
//   本檔仍保留計時，但印到 stderr，讓 stdout 保持逐位元組穩定。
//
// 【4. 怎麼數？—— 用 counting allocator，而不是換掉全域 operator new】
//   std::string 其實是 std::basic_string<char, char_traits<char>,
//   allocator<char>> 的別名。本檔只把「最後一個模板參數」換成一個會計數的
//   allocator，其餘完全不動：
//       using CString = std::basic_string<char, std::char_traits<char>,
//                                         CountingAlloc<char>>;
//   因為 SSO、容量成長策略等行為都寫在 basic_string 本身、與 allocator 無關，
//   CString 的行為與 std::string 完全一致（本檔實測 sizeof 兩者都是 32）。
//   相較於改寫全域 operator new，這個做法只攔截「字串自己的配置」，不會把
//   vector 緩衝區、iostream 內部配置一起算進來，量測目標更精準。
//
// 【5. 為什麼要 reserve()？】
//   若不先 reserve，vector 會在成長過程中重新配置並搬移既有元素。那些搬移
//   會讓「拷貝 vs 移動」的對比失焦。先 reserve 之後，迴圈裡發生的每一次
//   字串配置都確定只來自 push_back 的那一次拷貝或移動。
//
// 【概念補充 Concept Deep Dive】
//   * SSO（Small String Optimization）：短字串直接存在物件內部的緩衝區，
//     完全不配置 heap。門檻是實作定義的：本檔實測 libstdc++ 為 15
//     （長度 15 → 0 次配置，長度 16 → 1 次配置）；libc++ 是 22，MSVC 是 15。
//     本檔主測試刻意用 1000 字元，確保一定走 heap，SSO 不會干擾計數。
//   * 無狀態 allocator 與移動：CountingAlloc 沒有成員、任兩個實例恆相等
//     （always_equal），所以移動建構可以直接把指標搬走，不需要「用目標的
//     allocator 重新配置再逐一搬移」。若 allocator 有狀態且不相等，移動就
//     可能退化成逐元素搬移——這是自訂 allocator 常見的效能陷阱。
//   * 為什麼移動後來源還能安全解構：移動建構會讓來源回到一個合法狀態
//     （通常是空字串），來源進入「valid but unspecified」——解構它是安全的，
//     但不該再讀它的值。
//
// 【注意事項 Pay Attention】
//   1. 移動「不保證」比拷貝快。對沒有 heap 資源的型別（int、std::array<int,N>、
//      POD struct），移動就是拷貝，兩者完全一樣。移動只對「擁有資源」的型別有意義。
//   2. 被移動後的物件處於 valid but unspecified 狀態。可以對它賦新值、可以解構，
//      但不該讀它原本的值。本檔在移動後只印「還剩幾個元素」這種有明確保證的資訊。
//   3. 計時數字（stderr）每次執行都不同，取決於 CPU 頻率、cache 與記憶體壓力，
//      不可寫死進教材當作結論。配置次數（stdout）才是確定的。
//   4. SSO 門檻是實作定義的，換編譯器/標準庫就會變。任何「長度 N 以下不配置」
//      的說法都必須標明是哪一家實作。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】移動語意的效能
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 移動一個 std::string 比拷貝快多少？為什麼？
//     答：拷貝是 O(L) 且需要 1 次 heap 配置；移動是 O(1) 且 0 次配置。
//         快多少取決於字串長度——越長差距越大，因為移動的成本是常數。
//         真正省下的不只是複製那 L 個位元組，更是那次 heap 配置（可能觸發
//         鎖競爭與 page fault），這往往才是主要成本。
//     追問：那移動一個 std::array<int, 1000> 會比拷貝快嗎？
//         → 不會，兩者完全一樣。array 把資料直接存在物件裡、沒有 heap 資源
//           可以「偷」，移動建構只能逐元素移動，而 int 的移動就是拷貝。
//
// 🔥 Q2. 為什麼量測移動效能時一定要先 reserve()？
//     答：不 reserve 的話，vector 成長時會重新配置緩衝區並搬移既有元素，
//         這些成本會混進量測結果，讓你分不清成本來自「push_back 的拷貝/移動」
//         還是「vector 擴容」。reserve 之後，量到的就只有元素本身的成本。
//     追問：擴容時 vector 會拷貝還是移動既有元素？
//         → 只有當元素的移動建構是 noexcept 時才會移動；否則為了維持強例外
//           保證會退化成拷貝。這正是移動建構一定要標 noexcept 的理由。
//
// ⚠️ 陷阱. 「我用 std::chrono 量到移動比拷貝快 35 倍，所以移動快 35 倍」——
//         這個結論錯在哪？
//     答：35 倍只在「這台機器、這個字串長度、這次執行」成立。換成長度 10 的
//         字串（落在 SSO 範圍、根本不配置 heap），移動可能完全沒有優勢；
//         換一台記憶體頻寬不同的機器，倍率也會變。可重現的說法是
//         「拷貝 N 次配置、移動 0 次配置」這種由語意決定的計數事實。
//     為什麼會錯：把「一次量測結果」當成「語言性質」。效能倍率是環境的函數，
//         複雜度與配置次數才是程式語意本身決定的不變量。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <chrono>
#include <vector>
#include <string>
#include <memory>
#include <utility>
#include <cstddef>

// -----------------------------------------------------------------------------
// 會計數的 allocator
//   只攔截「字串自己的 heap 配置」，不影響 vector 緩衝區與 iostream 內部配置。
//   無狀態（沒有成員）、任兩實例恆相等，所以容器的移動可以直接偷指標。
// -----------------------------------------------------------------------------
namespace {
    std::size_t g_allocCount = 0;   // 累計配置次數
}

template <class T>
struct CountingAlloc {
    using value_type = T;

    CountingAlloc() noexcept = default;
    template <class U> CountingAlloc(const CountingAlloc<U>&) noexcept {}

    T* allocate(std::size_t n) {
        ++g_allocCount;
        return std::allocator<T>{}.allocate(n);
    }
    void deallocate(T* p, std::size_t n) noexcept {
        std::allocator<T>{}.deallocate(p, n);
    }
};
template <class A, class B>
bool operator==(const CountingAlloc<A>&, const CountingAlloc<B>&) noexcept { return true; }
template <class A, class B>
bool operator!=(const CountingAlloc<A>&, const CountingAlloc<B>&) noexcept { return false; }

// 與 std::string 唯一的差別就是 allocator；SSO、容量策略等行為完全相同。
using CString = std::basic_string<char, std::char_traits<char>, CountingAlloc<char>>;

// 量測區間的小工具：回傳這段期間內發生的配置次數
struct AllocScope {
    std::size_t start = g_allocCount;
    std::size_t stop() const { return g_allocCount - start; }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】把一批 log 行從 producer buffer 交棒給 consumer 佇列
//   情境：log 收集器每隔一段時間，把累積的行「整批交出去」給寫檔執行緒。
//         這些行的所有權要轉移，producer 交出後就不再需要它們——
//         正是移動語意的教科書用途。
//   重點：drainByCopy 是常見的效能地雷寫法（每行都重新配置一塊 heap）；
//         drainByMove 只搬指標。兩者搬的行數相同，配置次數差距卻是確定的。
// -----------------------------------------------------------------------------
std::size_t drainByCopy(std::vector<CString>& src, std::vector<CString>& dst) {
    dst.reserve(dst.size() + src.size());
    for (const auto& line : src) dst.push_back(line);        // 每行一次 heap 配置
    src.clear();
    return dst.size();
}

std::size_t drainByMove(std::vector<CString>& src, std::vector<CString>& dst) {
    dst.reserve(dst.size() + src.size());
    for (auto& line : src) dst.push_back(std::move(line));   // 只搬指標，零配置
    src.clear();
    return dst.size();
}

int main() {
    const int N = 500000;
    const std::size_t LEN = 1000;   // 遠大於 SSO 門檻，確保每個字串都走 heap

    // ── 先實測本機的 SSO 門檻（實作定義，不能用背的）──
    std::cout << "=== SSO 門檻實測（實作定義）===\n";
    std::cout << "sizeof(CString) = " << sizeof(CString)
              << "，sizeof(std::string) = " << sizeof(std::string)
              << "（只換 allocator，佈局不變）\n";
    for (std::size_t len : {std::size_t{15}, std::size_t{16}}) {
        AllocScope sc;
        { CString probe(len, 'x'); }
        std::cout << "  長度 " << len << " 的字串 → 配置 " << sc.stop() << " 次\n";
    }
    std::cout << "→ 本機 libstdc++ 的 SSO 門檻是 15（≤15 不配置 heap）。\n\n";

    // ── 建立一批很長的字串 ──
    std::vector<CString> source;
    source.reserve(N);
    for (int i = 0; i < N; ++i) {
        source.push_back(CString(LEN, static_cast<char>('A' + (i % 26))));
    }

    std::cout << "=== 主測試設定 ===\n";
    std::cout << "字串個數 N = " << N << "，每個長度 = " << LEN << " 字元\n";
    std::cout << "(長度遠大於 SSO 門檻，故每個字串都持有一塊 heap 記憶體)\n\n";

    // ──── 測試拷貝：計數 + 計時 ────
    std::vector<CString> copied;
    copied.reserve(N);                       // 先 reserve，排除擴容造成的干擾
    AllocScope copyScope;
    auto t1 = std::chrono::high_resolution_clock::now();
    for (const auto& s : source) {
        copied.push_back(s);                 // 拷貝：每次配置 1000 bytes + 複製
    }
    auto t2 = std::chrono::high_resolution_clock::now();
    std::size_t copyAllocs = copyScope.stop();

    // ──── 測試移動：計數 + 計時 ────
    std::vector<CString> moved;
    moved.reserve(N);
    AllocScope moveScope;
    auto t3 = std::chrono::high_resolution_clock::now();
    for (auto& s : source) {
        moved.push_back(std::move(s));       // 移動：只搬指標，零配置
    }
    auto t4 = std::chrono::high_resolution_clock::now();
    std::size_t moveAllocs = moveScope.stop();

    // ──── 確定性結果（stdout，每次執行都一樣）────
    std::cout << "=== 確定性指標：heap 配置次數 ===\n";
    std::cout << "拷貝 " << N << " 個字串的配置次數：" << copyAllocs << "\n";
    std::cout << "移動 " << N << " 個字串的配置次數：" << moveAllocs << "\n";
    std::cout << "省下的配置次數：" << (copyAllocs - moveAllocs) << "\n";
    std::cout << "說明：拷貝每個字串各需 1 次配置，故等於 N；移動只搬指標，故為 0。\n\n";

    // ──── 正確性檢查（順便確保上面的迴圈不會被優化掉）────
    std::cout << "=== 正確性檢查 ===\n";
    std::cout << "copied.size() = " << copied.size()
              << "，moved.size() = " << moved.size() << "\n";
    std::cout << "copied[0] 前 5 字元 = " << std::string(copied[0].substr(0, 5).c_str()) << "\n";
    std::cout << "moved[0]  前 5 字元 = " << std::string(moved[0].substr(0, 5).c_str())  << "\n";
    // 注意：source 的元素已被移走，處於 valid but unspecified 狀態，
    //       這裡只印「還剩幾個元素」這種有明確保證的資訊，不讀其內容。
    std::cout << "source 仍有 " << source.size() << " 個元素（內容已被移走，不可再讀）\n\n";

    // ──── 實務範例：批次交棒 ────
    std::cout << "=== 日常實務：log 批次交棒（拷貝 vs 移動）===\n";
    {
        std::vector<CString> producer, consumer;
        producer.reserve(1000);
        for (int i = 0; i < 1000; ++i) {
            producer.push_back(CString("2026-07-19 10:00:00 [INFO] request handled id=")
                               + CString(std::to_string(i).c_str()));
        }
        std::vector<CString> backup = producer;   // 留一份給第二次測試

        // 先把 consumer 的容量配好，讓兩次量測都只反映「元素本身的配置」。
        consumer.reserve(1000);

        AllocScope s1;
        std::size_t n1 = drainByCopy(producer, consumer);
        std::size_t a1 = s1.stop();

        consumer.clear();
        AllocScope s2;
        std::size_t n2 = drainByMove(backup, consumer);
        std::size_t a2 = s2.stop();

        std::cout << "drainByCopy：交出 " << n1 << " 行，配置 " << a1 << " 次\n";
        std::cout << "drainByMove：交出 " << n2 << " 行，配置 " << a2 << " 次\n";
        std::cout << "結論：兩者搬的行數相同，但移動版完全不碰 heap。\n";
    }

    // ──── 計時資訊送 stderr（每次執行都不同，不屬於預期輸出）────
    auto copy_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    auto move_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t3).count();
    std::cerr << "[計時，僅供參考，每次執行都不同] 拷貝 " << copy_ms
              << " ms / 移動 " << move_ms << " ms\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -O2 "第 32 課：移動建構函數（Move Constructor）4.cpp" -o lesson32d
// 執行: ./lesson32d            （計時資訊印在 stderr，可用 2>/dev/null 濾掉）

// 【預期輸出的但書】
//   1. 下方 stdout 內容在本機（GCC 15.2 / libstdc++）是確定的，三次執行逐位元組相同。
//      配置次數由程式語意決定：拷貝 N 個超過 SSO 門檻的字串必然是 N 次、移動必然是 0 次。
//   2. 「SSO 門檻 = 15」是 libstdc++ 的實作定義值，由本檔實測得到（長度 15 → 0 次、
//      長度 16 → 1 次）。libc++ 為 22、MSVC 為 15，換標準庫這兩行數字就會不同。
//   3. sizeof 為 32 也是實作定義值（libstdc++ x86-64）。
//   4. 計時那一行印在 stderr，數值每次執行都不同（本機 -O2 多次實測拷貝約 160~175 ms、
//      移動約 4~5 ms），故意不列入下方預期輸出。
//   5. source 被移走後元素處於 valid but unspecified 狀態，本檔只印元素個數，不印內容。

// === 預期輸出 ===
// === SSO 門檻實測（實作定義）===
// sizeof(CString) = 32，sizeof(std::string) = 32（只換 allocator，佈局不變）
//   長度 15 的字串 → 配置 0 次
//   長度 16 的字串 → 配置 1 次
// → 本機 libstdc++ 的 SSO 門檻是 15（≤15 不配置 heap）。
//
// === 主測試設定 ===
// 字串個數 N = 500000，每個長度 = 1000 字元
// (長度遠大於 SSO 門檻，故每個字串都持有一塊 heap 記憶體)
//
// === 確定性指標：heap 配置次數 ===
// 拷貝 500000 個字串的配置次數：500000
// 移動 500000 個字串的配置次數：0
// 省下的配置次數：500000
// 說明：拷貝每個字串各需 1 次配置，故等於 N；移動只搬指標，故為 0。
//
// === 正確性檢查 ===
// copied.size() = 500000，moved.size() = 500000
// copied[0] 前 5 字元 = AAAAA
// moved[0]  前 5 字元 = AAAAA
// source 仍有 500000 個元素（內容已被移走，不可再讀）
//
// === 日常實務：log 批次交棒（拷貝 vs 移動）===
// drainByCopy：交出 1000 行，配置 1000 次
// drainByMove：交出 1000 行，配置 0 次
// 結論：兩者搬的行數相同，但移動版完全不碰 heap。
