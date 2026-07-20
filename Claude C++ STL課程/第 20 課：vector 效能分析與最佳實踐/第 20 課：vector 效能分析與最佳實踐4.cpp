// =============================================================================
//  第 20 課：vector 效能分析與最佳實踐 4
//  —  emplace_back 到底什麼時候才真的比較快？（實測）
// =============================================================================
//
// 【主題資訊 Information】
//   void push_back(const T&);  void push_back(T&&);        // C++11 起有 T&& 版本
//   template<class... Args> reference emplace_back(Args&&...);  // C++11（C++17 起回傳 reference）
//
//   標準版本：兩者的現代形式皆為 C++11
//   複雜度  ：均攤 O(1)（本檔全部先 reserve，排除重新配置變因）
//   標頭檔  ：<vector>、<chrono>、<string>
//
// 【詳細解釋 Explanation】
//
// 【1. 這個實驗在量什麼】
//   第 3 檔用印訊息的方式看「發生了幾次建構／移動」；這一檔改用時間，
//   回答一個更實際的問題：**那些差異換算成時間，值得在意嗎？**
//   四組測試都先 reserve(N)，所以重新配置成本被完全排除，
//   量到的差異只來自「建構元素的方式」本身。
//
// 【2. 為什麼 string 那組會有差、int 那組沒差】
//   * std::vector<std::string> 的 push_back(std::string("..."))
//       ① 用字面值建構一個臨時 std::string（本例 23 字元 -> 要 malloc）
//       ② 移動建構到容器（便宜：搬指標，不複製字元）
//       ③ 臨時物件解構
//     emplace_back("...") 只有 ①，直接在容器記憶體上建構。
//     省下的是「一次移動建構 + 一次解構」——不是省下 malloc，
//     malloc 兩邊都要做一次。
//   * std::vector<int> 的 push_back(i) 與 emplace_back(i)
//     兩者都只是把 4 bytes 寫進去。emplace_back 走的是變參模板 +
//     完美轉發 + placement new；這條呼叫鏈在 -O2 會被完全 inline 消除，
//     但在 -O0 會原封不動地保留下來。結果非常反直覺——見第 4 點。
//
// 【3. SSO 讓這個實驗更有層次（本機 libstdc++ 實測）】
//   libstdc++ 的 std::string 有 15 字元的小字串最佳化（SSO）門檻：
//       長度 <= 15 -> 字元直接放在 string 物件裡，不配置 heap
//       長度 >= 16 -> 配置 heap
//   本檔用的 "hello_world_test_string" 是 **23 字元 -> 走 heap**（本機實測）。
//   這代表每一次建構都伴隨一次 malloc，放大了「建構」在總時間中的比重。
//   如果把字串改成 8 個字元（走 SSO），兩組的差距會明顯縮小 ——
//   因為那時 push_back 多出來的「移動」只是複製 32 bytes 的物件本體，非常便宜。
//   ★ 15 這個門檻是 libstdc++ 的實作定義值，libc++ 是 22、MSVC 是 15，
//     標準完全沒有規定。
//
// 【4. ★ 最佳化等級會讓結論「反轉」——本檔最重要的一課】
//   同一份程式碼、同一台機器，只差一個 -O2，本機實測結果如下：
//
//                        -O0（無 -O 旗標）        -O2
//       push_back  string    106435 us          22208 us
//       emplace_back string    77870 us         21147 us   -> 由「快 1.4 倍」變成「幾乎相同」
//       push_back  int          3828 us            575 us
//       emplace_back int        9871 us            218 us   -> 由「慢 2.6 倍」變成「快 2.6 倍」
//
//   請仔細看 int 那兩列：**結論的方向整個顛倒了。**
//     * -O0 時，emplace_back 的變參模板 + std::forward + placement new
//       全部是真實的函式呼叫，一個都沒被 inline，所以比單純的 push_back 慢。
//     * -O2 時，這條鏈被完全展開消除，反而比 push_back 少一層轉換而更快。
//   這就是為什麼「在 debug build 上量效能」是沒有意義的——
//   你量到的是抽象層的呼叫開銷，不是演算法的成本。
//
//   紀律：
//     * 效能結論一定要連同「編譯旗標」一起講，否則那句話不完整。
//     * 只能寫「本機在 -O0 實測約 X 倍」，不能寫「一定快 X 倍」。
//     * 真正要下結論，請在 -O2/-O3 下跑多次取中位數。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼移動一個 std::string 很便宜
//     libstdc++ 的 std::string 是 32 bytes（本機 sizeof 實測）：
//     一個指標、一個長度、加上 16 bytes 的 SSO 緩衝區（同時也是 capacity 欄位）。
//     移動長字串只要搬走指標與長度、把來源改成空字串，字元本身完全不動。
//     所以 push_back 多出來的那一次移動，成本是「常數」而不是 O(字串長度）。
//     這就是為什麼 string 那組雖然有差，卻不會差到數量級。
//
// (B) 為什麼不用 -O2 的數字當教材輸出
//     開 -O2 之後，這種「建了就丟」的 microbenchmark 很容易被
//     dead-code elimination 整段刪掉，量到的可能是「什麼都沒做」的時間。
//     本檔用 sink 變數把結果留下來降低這個風險，但更嚴謹的做法是用
//     Google Benchmark 的 benchmark::DoNotOptimize()。
//     教材選擇貼「未最佳化」的數字，是因為它至少保證迴圈真的跑了。
//
// (C) 什麼時候 emplace_back 反而比較慢
//     幾乎沒有——但有一個真實情況：容器是 vector<std::unique_ptr<T>> 時，
//         v.emplace_back(new T);        // 危險
//     如果配置成功但 vector 擴容丟出 bad_alloc，那個裸指標就洩漏了。
//         v.push_back(std::make_unique<T>());   // 安全
//     這裡 push_back 比較好不是因為快，而是因為例外安全。
//
// 【注意事項 Pay Attention】
//
// 1. ★ 本檔四個耗時數字「每次執行都不同」。換機器、換編譯器、換系統負載
//    都會不同，絕對值完全不可跨機器比較。
// 2. ★★ 預期輸出是用檔尾「編譯:」那行取得的，**沒有 -O 旗標（等同 -O0）**。
//    改用 -O2 之後不只是「數字變小」——int 那組的**勝負會整個反過來**
//    （見上面【4】的對照表：-O0 時 emplace 慢 2.6 倍，-O2 時快 2.6 倍）。
//    任何「快幾倍」的說法沒有附上編譯旗標就是不完整的。
// 3. 不要因為這個實驗就把所有 push_back 改成 emplace_back。
//    對 int/double 這種型別沒有實質效益，反而降低可讀性
//    （emplace_back 看不出來你要放的是什麼型別）。
// 4. 本檔的 Timer 成員宣告順序是 label_ -> start_，初始化列表同序，
//    避免 -Wreorder 警告，也確保「讀時鐘」是建構的最後一步。
// 5. 15 字元的 SSO 門檻是 libstdc++ 實作定義值，不是標準保證。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】emplace_back 的效能真相
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. emplace_back 對 std::vector<int> 會比 push_back 快嗎？
//     答：「要看你開不開最佳化」——這才是完整的答案。
//         本機實測：-O0 時 emplace_back 反而**慢** 2.6 倍（變參模板與
//         std::forward 的呼叫鏈完全沒被 inline）；-O2 時反過來**快** 2.6 倍
//         （呼叫鏈被消除，且少一層轉換）。
//         實務上的結論是：對 int 這種型別，選哪個都不該是你的效能瓶頸，
//         應該用可讀性來決定。
//     追問：那什麼型別才穩定看得出差別？
//         → 建構成本高、而且你手上只有「建構參數」不是現成物件的時候，
//           例如 vector<std::string> 直接用字面值 emplace_back。
//           不過本機 -O2 實測連 string 那組也只差約 5%。
//
// 🔥 Q2. push_back(std::string("abc")) 比 emplace_back("abc") 多做了什麼？
//     答：多了「一次移動建構（臨時 string -> 容器）」與「一次臨時物件解構」。
//         注意 malloc 兩者都各做一次（若字串超過 SSO 門檻），
//         省下的不是配置，而是那次移動與解構。
//     追問：如果字串只有 5 個字元呢？
//         → 走 SSO 不配置 heap，那次移動只是複製 32 bytes 的物件本體，
//           差距會小很多。
//
// ⚠️ 陷阱. 「emplace_back 永遠是更好的選擇」——哪裡不對？
//     答：兩個地方。(1) 傳入現成的同型別物件時兩者完全等價，沒有效益。
//         (2) v.emplace_back(new T) 這種寫法在 vector 擴容丟例外時會洩漏記憶體，
//             應該用 v.push_back(std::make_unique<T>())。
//         另外 emplace_back 會繞過 explicit 檢查，可能悄悄選到你沒想到的建構子。
//     為什麼會錯：把「原地建構」直接等同於「一定比較好」，
//         忽略了它其實只在「用參數現場建構昂貴物件」這個特定情境才有優勢，
//         而且它放寬的型別檢查本身就是一種風險。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <chrono>
#include <string>

// -----------------------------------------------------------------------------
// RAII 計時器
// 宣告順序 label_ -> start_ 與初始化列表一致（避免 -Wreorder），
// 且讓讀時鐘成為建構的最後一步。
// -----------------------------------------------------------------------------
struct Timer {
    std::string label_;
    std::chrono::high_resolution_clock::time_point start_;

    explicit Timer(const std::string& label)
        : label_(label),
          start_(std::chrono::high_resolution_clock::now()) {}

    ~Timer() {
        auto end = std::chrono::high_resolution_clock::now();
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(end - start_).count();
        std::cout << "  " << label_ << ": " << us << " us" << std::endl;
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例 1】結構化 log 緩衝區
//   情境：高流量服務不能每筆 log 都直接寫檔（syscall 太貴），
//     而是先累積在記憶體，滿了再一次 flush。
//   每筆 log 的欄位（等級、來源、訊息）是現場產生的散裝值，
//   不是現成的 LogEntry 物件 -> 用 emplace_back 直接原地建構，
//   避免「先做一個 LogEntry 再搬進去」的來回。
//   緩衝區在建構時就 reserve，flush 用 clear()（保留 capacity）達成零配置重用。
// -----------------------------------------------------------------------------
struct LogEntry {
    std::string level;
    std::string source;
    std::string message;

    LogEntry(std::string lv, std::string src, std::string msg)
        : level(std::move(lv)), source(std::move(src)), message(std::move(msg)) {}
};

class LogBuffer {
    std::vector<LogEntry> entries_;
    std::size_t flushed_ = 0;

public:
    explicit LogBuffer(std::size_t capacity) {
        entries_.reserve(capacity);          // 一次配置，往後重複使用
    }

    void log(const std::string& level, const std::string& source, const std::string& msg) {
        entries_.emplace_back(level, source, msg);   // 原地建構，無臨時 LogEntry
    }

    // flush 後用 clear()：元素銷毀但 capacity 留著 -> 下一輪完全不配置
    std::size_t flush() {
        std::size_t n = entries_.size();
        flushed_ += n;
        entries_.clear();
        return n;
    }

    std::size_t pending()   const { return entries_.size(); }
    std::size_t capacity()  const { return entries_.capacity(); }
    std::size_t flushed()   const { return flushed_; }
};

int main() {
    const int N = 500'000;
    std::size_t sink = 0;   // 留住結果，降低整段被最佳化消除的機率

    std::cout << "=== 1. std::string（建構成本高：23 字元，超過 SSO 門檻）===" << std::endl;
    {
        Timer t("push_back  string ");
        std::vector<std::string> v;
        v.reserve(N);
        for (int i = 0; i < N; ++i) v.push_back(std::string("hello_world_test_string"));
        sink += v.back().size();
    }
    {
        Timer t("emplace_back string");
        std::vector<std::string> v;
        v.reserve(N);
        for (int i = 0; i < N; ++i) v.emplace_back("hello_world_test_string");
        sink += v.back().size();
    }

    std::cout << "\n=== 2. int（建構成本極低）===" << std::endl;
    {
        Timer t("push_back  int    ");
        std::vector<int> v;
        v.reserve(N);
        for (int i = 0; i < N; ++i) v.push_back(i);
        sink += static_cast<std::size_t>(v.back());
    }
    {
        Timer t("emplace_back int   ");
        std::vector<int> v;
        v.reserve(N);
        for (int i = 0; i < N; ++i) v.emplace_back(i);
        sink += static_cast<std::size_t>(v.back());
    }
    std::cout << "  (檢查值 sink = " << sink << "，僅為防止迴圈被最佳化掉)" << std::endl;

    std::cout << "\n=== 3. SSO 門檻確認（本機 libstdc++ 實作定義值）===" << std::endl;
    {
        std::string shortStr(15, 'x');
        std::string longStr(16, 'x');
        auto insideObject = [](const std::string& s) {
            const char* p = s.data();
            const char* b = reinterpret_cast<const char*>(&s);
            return p >= b && p < b + sizeof(std::string);
        };
        std::cout << "  sizeof(std::string) = " << sizeof(std::string) << " bytes" << std::endl;
        std::cout << "  長度 15 -> 走 SSO（不配置 heap）: "
                  << (insideObject(shortStr) ? "是" : "否") << std::endl;
        std::cout << "  長度 16 -> 走 SSO（不配置 heap）: "
                  << (insideObject(longStr) ? "是" : "否") << std::endl;
        std::cout << "  -> 本機門檻為 15；libc++ 是 22、MSVC 是 15，標準未規定" << std::endl;
    }

    std::cout << "\n=== 4. 日常實務：結構化 log 緩衝區 ===" << std::endl;
    {
        LogBuffer buf(128);
        buf.log("INFO",  "auth",    "user 4711 logged in");
        buf.log("WARN",  "storage", "disk usage above 80%");
        buf.log("ERROR", "payment", "gateway timeout after 3000ms");
        std::cout << "  待寫出 " << buf.pending() << " 筆，capacity=" << buf.capacity() << std::endl;

        std::size_t n = buf.flush();
        std::cout << "  flush 了 " << n << " 筆；flush 後 pending=" << buf.pending()
                  << "，capacity 仍是 " << buf.capacity() << std::endl;

        buf.log("INFO", "auth", "user 4711 logged out");   // 第二輪：零配置
        std::cout << "  第二輪再寫 1 筆，累計 flush " << buf.flushed()
                  << " 筆，capacity 依然 " << buf.capacity() << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第 20 課：vector 效能分析與最佳實踐4.cpp -o lesson20_4

// ※ 第 1、2 段的耗時「每次執行都不同」；int 那兩組差異本來就在雜訊範圍，

// === 預期輸出 ===
// ※ 本機實測（Ubuntu 26.04 / g++ 15.2.0 / 上面那行編譯指令，未加 -O 旗標）。
//    順序甚至可能互換。請看趨勢，不要背數字。
//
// TIMING_OUTPUT_PLACEHOLDER
