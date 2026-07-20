// =============================================================================
//  第 2.5 章 5  —  量化移動 vs 複製：用「配置次數」而不是「碼錶」
// =============================================================================
//
// 【主題資訊 Information】
//   量測手法 : 覆寫全域 operator new / operator delete 來計數堆積配置
//   標頭檔   : <string>, <utility>, <new>, <cstdlib>, <chrono>
//   複雜度   : std::string 複製為 O(n)（配置 + 拷貝 n bytes）
//              std::string 移動為 O(1)（只搬指標／長度／容量三個欄位）
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼本檔不用「計時」當主要證據】
//   原始的做法是用 std::chrono 量兩個迴圈的毫秒數。這有兩個問題：
//
//   (a) 不可重現：耗時受 CPU 頻率調節、快取狀態、其他行程干擾影響，
//       每次執行都不同。把某一次的毫秒數寫進「預期輸出」，
//       下次跑就對不上，那個數字等於假資料。
//
//   (b) 更嚴重的是「比較本身不公平」。常見的寫法是這樣：
//           for (i < N) { std::string copy = source; }                  // 複製組
//           for (i < N) { std::string temp = source;                    // ← 也複製了！
//                         std::string moved = std::move(temp);
//                         source = moved; }                             // ← 又複製一次！
//       移動組每輪其實做了「一次複製 + 一次移動 + 一次複製回寫」，
//       工作量比複製組還多。量出來的數字無論偏向哪邊，都不能證明任何事。
//
//   本檔改用「堆積配置次數」與「拷貝位元組數」：這兩個量是確定性的，
//   可重現，而且直接對應成本發生的機制——複製之所以慢，就是因為它要
//   向配置器要一塊新記憶體再逐位元組拷貝；移動之所以快，就是因為它
//   一次配置都不做。計時只是這個機制的間接後果。
//
// 【2. 公平的比較該怎麼設計】
//   關鍵是「兩組的前置成本必須相同，只有被測操作不同」。
//   本檔每一輪都先建立一份 temp（兩組共同的前置成本），
//   然後只在「被測操作」前後各取一次配置計數快照：
//       快照 →  std::string x = temp;              ← 複製建構
//       快照 →  std::string x = std::move(temp);   ← 移動建構
//   兩者的前置完全一致，差額就純粹是該操作本身的配置次數。
//
// 【3. std::string 移動時到底搬了什麼】
//   libstdc++ 的 std::string 內部大致是 { 指標, 長度, 容量／SSO 緩衝 }，
//   本機 sizeof(std::string) 實測為 32 bytes。
//   移動就是把這幾個欄位搬過去並把來源置空——不碰堆積，所以是 O(1)，
//   而且與字串長度完全無關。複製則必須配置 n bytes 再 memcpy，
//   成本隨長度線性成長。
//
// 【4. SSO：短字串為什麼移動也不會變快】
//   libstdc++ 對 15 個字元以內的字串採用 SSO（small string optimization），
//   直接存在物件內部、不配置堆積。這種情況下移動與複製都是搬 32 bytes，
//   完全一樣快。所以「移動一定比較快」只在「字串長到需要堆積配置」時成立。
//   本檔特意用 10000 字元，就是要越過 SSO 門檻。
//
// 【概念補充 Concept Deep Dive】
//   覆寫全域 operator new / delete 是標準允許的替換（replaceable function）。
//   要注意：
//     * 必須同時提供對應的 operator delete，否則配置與釋放不成對。
//     * C++14 起有 sized deallocation（operator delete(void*, size_t)），
//       也要一併提供，否則某些呼叫路徑會找不到對應版本。
//     * 計數器本身不是執行緒安全的；本檔是單執行緒，故足夠。
//   另外，本檔刻意用 asm volatile 記憶體屏障阻止編譯器把整段最佳化掉。
//   這是量測程式的必要手段——沒有它，-O2 可能直接刪掉沒有副作用的迴圈，
//   於是量到「0 毫秒」而誤以為快得驚人。
//
// 【注意事項 Pay Attention】
// 1. 耗時數字每次執行都不同，本檔已將它輸出到 stderr，
//    讓 stdout 保持逐位元組穩定。預期輸出區塊只涵蓋 stdout。
// 2. 配置次數是確定性的，但它取決於標準庫實作（SSO 門檻、成長策略）。
//    本機為 g++ 15.2 / libstdc++。
// 3. 「移動比複製快」只在型別持有堆積資源時成立。int、SSO 短字串、
//    trivially copyable 的小型結構完全沒有差別。
// 4. 量測程式一定要防止編譯器最佳化掉被測程式碼，否則量到的是空迴圈。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】量化移動與複製的成本
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 移動比複製快多少？請說出它「快在哪裡」而不是「快幾倍」。
//     答：複製 std::string 要向配置器要一塊 n bytes 的記憶體再拷貝 n bytes，
//         是 O(n)；移動只搬指標、長度、容量三個欄位並把來源置空，
//         零次配置、O(1)，而且與字串長度無關。所以正確的說法不是
//         「快 N 倍」，而是「複製隨長度線性成長，移動是常數」。
//     追問：那什麼情況移動不會比較快？→ 型別沒有堆積資源時。
//         int、指標、走 SSO 的短字串，移動與複製都是搬同樣幾個 bytes。
//
// 🔥 Q2. 要證明「移動比複製快」，你會怎麼設計量測？
//     答：優先量「配置次數」這種確定性指標，而不是牆鐘時間——可重現，
//         而且直接對應成本機制。若要計時，必須確保兩組的前置成本相同、
//         用足夠大的重複次數、加記憶體屏障防止被最佳化掉，
//         並且多次執行取分布而不是只跑一次。
//     追問：為什麼要加屏障？→ 因為被測程式碼沒有可觀察的副作用，
//         -O2 可以合法地整段刪除，於是量到接近 0 的時間而做出錯誤結論。
//
// ⚠️ 陷阱. 下面這段「移動組」的計時為什麼完全沒有意義？
//            for (i<N) { std::string temp = source;
//                        std::string moved = std::move(temp);
//                        source = moved; }
//     答：因為它每輪做了一次複製（建 temp）、一次移動、再一次複製
//         （source = moved 是複製賦值，moved 是左值）。工作量比純複製組
//         還多，量出來的數字無法歸因到「移動」這件事上。
//     為什麼會錯：寫的人想「先準備一份供移動的來源」才加上 temp，
//         卻沒意識到那份準備本身就是一次完整的複製，已經被算進計時區間。
//         量測的鐵律是：計時區間內只能有你想量的那個操作，
//         所有前置準備都必須排除在外，或在兩組間完全相同。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】從缺。
//   理由：本檔主題是「效能量測方法論」與移動／複製的成本模型。
//   LeetCode 只判定演算法正確性與大致執行時間，不會因為多一次字串複製
//   而失敗，清單中也沒有任何一題以資源搬移成本為核心。
//   硬套一題只會模糊焦點，故從缺，改以實務範例呈現這個成本差異
//   在真實系統中的樣子。

#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <utility>
#include <new>
#include <cstdlib>

// -----------------------------------------------------------------------------
// 堆積配置計數器：覆寫全域 operator new / delete
//   這是標準允許的 replaceable function。只計數、不改變配置行為。
//   注意必須同時提供 sized deallocation（C++14 起），否則會有路徑找不到
//   對應的 delete。本檔為單執行緒，計數器未加同步。
// -----------------------------------------------------------------------------
static long g_alloc_count = 0;
static long g_alloc_bytes = 0;

void* operator new(std::size_t n) {
    ++g_alloc_count;
    g_alloc_bytes += static_cast<long>(n);
    void* p = std::malloc(n);
    if (!p) throw std::bad_alloc();
    return p;
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void* operator new[](std::size_t n) {
    ++g_alloc_count;
    g_alloc_bytes += static_cast<long>(n);
    void* p = std::malloc(n);
    if (!p) throw std::bad_alloc();
    return p;
}
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }

// 阻止編譯器把「沒有可觀察副作用」的被測程式碼整段最佳化掉。
// 少了這道屏障，-O2 可能直接刪掉迴圈，於是量到 0 而誤以為快得驚人。
template <typename T>
static inline void doNotOptimize(T& value) {
    asm volatile("" : "+m"(value) : : "memory");
}

// -----------------------------------------------------------------------------
// 【日常實務範例】訊息轉送管線：同一批資料經過三個處理階段
//   場景：接收 → 驗證 → 入佇列。每個階段都會把訊息交給下一個階段。
//   為什麼重要：這種「一路傳下去」的管線是複製成本最容易悄悄累積的地方。
//     每一段都用複製，一則 10 KB 的訊息走完三段就配置了三次、拷貝 30 KB；
//     全程用移動則是零次配置——因為每一段交出去之後就不再需要它了。
//   下面用實際的配置次數對照兩種寫法。
// -----------------------------------------------------------------------------
struct Message {
    std::string payload;
};

// 複製版本：每段都收 const&，然後自己存一份
static Message stageCopy(const Message& in) {
    Message out;
    out.payload = in.payload;      // 深拷貝：配置 + memcpy
    return out;
}

// 移動版本：sink parameter，收下之後把內容搬走
static Message stageMove(Message in) {
    Message out;
    out.payload = std::move(in.payload);   // 只搬指標
    return out;
}

int main() {
    const int N = 100000;
    const std::size_t LEN = 10000;         // 遠超過 SSO 門檻，確保用到堆積

    std::cout << "=== 移動 vs 複製：以「堆積配置次數」量化 ===\n";
    std::cout << "字串長度 " << LEN << " 字元，各執行 " << N << " 次\n";
    std::cout << "sizeof(std::string) = " << sizeof(std::string)
              << " bytes（本機實測，實作定義）\n\n";

    std::string source(LEN, 'x');

    // ---- 複製組：只計「複製建構」本身的配置 ----
    long copy_allocs = 0;
    long copy_bytes  = 0;
    for (int i = 0; i < N; ++i) {
        std::string temp = source;             // 兩組共同的前置成本
        doNotOptimize(temp);

        long a0 = g_alloc_count, b0 = g_alloc_bytes;
        std::string result = temp;             // ← 被測操作：複製建構
        doNotOptimize(result);
        copy_allocs += g_alloc_count - a0;
        copy_bytes  += g_alloc_bytes - b0;
    }

    // ---- 移動組：前置完全相同，只有被測操作換成移動 ----
    long move_allocs = 0;
    long move_bytes  = 0;
    for (int i = 0; i < N; ++i) {
        std::string temp = source;             // 與上面完全相同的前置成本
        doNotOptimize(temp);

        long a0 = g_alloc_count, b0 = g_alloc_bytes;
        std::string result = std::move(temp);  // ← 被測操作：移動建構
        doNotOptimize(result);
        move_allocs += g_alloc_count - a0;
        move_bytes  += g_alloc_bytes - b0;
    }

    std::cout << "複製建構 " << N << " 次 -> 堆積配置 " << copy_allocs
              << " 次，共 " << copy_bytes << " bytes\n";
    std::cout << "移動建構 " << N << " 次 -> 堆積配置 " << move_allocs
              << " 次，共 " << move_bytes << " bytes\n";
    std::cout << "結論：複製的成本隨字串長度線性成長；移動與長度無關，"
                 "是常數時間。\n";

    // ---- SSO：短字串移動不會比較快 ----
    std::cout << "\n=== 對照：SSO 短字串（15 字元以內不配置堆積）===\n";
    std::string shortSrc(5, 'a');
    long s_copy = 0, s_move = 0;
    for (int i = 0; i < N; ++i) {
        std::string temp = shortSrc;
        doNotOptimize(temp);
        long a0 = g_alloc_count;
        std::string result = temp;
        doNotOptimize(result);
        s_copy += g_alloc_count - a0;
    }
    for (int i = 0; i < N; ++i) {
        std::string temp = shortSrc;
        doNotOptimize(temp);
        long a0 = g_alloc_count;
        std::string result = std::move(temp);
        doNotOptimize(result);
        s_move += g_alloc_count - a0;
    }
    std::cout << "短字串複製 " << N << " 次 -> 堆積配置 " << s_copy << " 次\n";
    std::cout << "短字串移動 " << N << " 次 -> 堆積配置 " << s_move << " 次\n";
    std::cout << "兩者都是 0：SSO 下根本沒碰堆積，移動並不會比較快。\n";

    // ---- 日常實務：三段式訊息管線 ----
    std::cout << "\n=== 日常實務: 三段式訊息管線（10000 bytes 訊息）===\n";
    {
        Message m; m.payload = std::string(LEN, 'p');
        long a0 = g_alloc_count;
        Message r = stageCopy(stageCopy(stageCopy(m)));
        doNotOptimize(r);
        std::cout << "全程複製 -> 堆積配置 " << (g_alloc_count - a0) << " 次\n";
    }
    {
        Message m; m.payload = std::string(LEN, 'p');
        long a0 = g_alloc_count;
        Message r = stageMove(stageMove(stageMove(std::move(m))));
        doNotOptimize(r);
        std::cout << "全程移動 -> 堆積配置 " << (g_alloc_count - a0) << " 次\n";
    }

    // ---- 耗時只作參考：送 stderr，因為它每次執行都不同 ----
    auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < N; ++i) { std::string c = source; doNotOptimize(c); }
    auto t1 = std::chrono::steady_clock::now();
    std::cerr << "[stderr] 參考用耗時（每次執行都不同，不列入預期輸出）："
              << N << " 次複製 = "
              << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count()
              << " ms\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第 2.5 章：stdmove — 強制轉換為右值5.cpp -o move_vs_copy_cost

// 【但書】
//   1. 「耗時」刻意輸出到 stderr，不列入下方預期輸出——它受 CPU 頻率、
//      快取狀態與系統負載影響，每次執行都不同。stdout 只放確定性的
//      配置次數與位元組數，本機連跑 5 次逐位元組相同。
//   2. 配置次數雖然是確定性的，仍取決於標準庫實作（SSO 門檻、成長策略）。
//      本機為 g++ 15.2 / libstdc++，sizeof(std::string) = 32 bytes。
//   3. 複製組的位元組數是 1000100000（每次配置 10001 bytes：10000 個字元
//      加上結尾的 '\0'），不是剛好 10^9。

// === 預期輸出 ===
// === 移動 vs 複製：以「堆積配置次數」量化 ===
// 字串長度 10000 字元，各執行 100000 次
// sizeof(std::string) = 32 bytes（本機實測，實作定義）
//
// 複製建構 100000 次 -> 堆積配置 100000 次，共 1000100000 bytes
// 移動建構 100000 次 -> 堆積配置 0 次，共 0 bytes
// 結論：複製的成本隨字串長度線性成長；移動與長度無關，是常數時間。
//
// === 對照：SSO 短字串（15 字元以內不配置堆積）===
// 短字串複製 100000 次 -> 堆積配置 0 次
// 短字串移動 100000 次 -> 堆積配置 0 次
// 兩者都是 0：SSO 下根本沒碰堆積，移動並不會比較快。
//
// === 日常實務: 三段式訊息管線（10000 bytes 訊息）===
// 全程複製 -> 堆積配置 3 次
// 全程移動 -> 堆積配置 0 次
