// =============================================================================
//  summary.cpp  —  std::move：強制轉換為右值（第 2.5 章總結）
// =============================================================================
//
// 【主題資訊 Information】
//   template<class T>
//   constexpr std::remove_reference_t<T>&& move(T&& t) noexcept;
//     標準版本 : C++11（constexpr 為 C++14 起）
//     標頭檔   : <utility>
//     複雜度   : O(1)，且編譯後不產生任何指令——它只是一次型別轉換
//   相關      : std::forward<T>(x)                    [C++11, <utility>]
//               std::move_if_noexcept<T>(x)           [C++11, <utility>]
//               std::is_nothrow_move_constructible_v  [C++17, <type_traits>]
//   本機實測  : sizeof(std::string) = 32 bytes、sizeof(std::unique_ptr<int>) = 8 bytes
//               （皆為實作定義）
//
// 【詳細解釋 Explanation】
//
// 【1. std::move 不移動任何東西——先把這句話刻進腦裡】
//   它的完整實作只有一行 static_cast：
//       template<typename T>
//       remove_reference_t<T>&& move(T&& arg) noexcept {
//           return static_cast<remove_reference_t<T>&&>(arg);
//       }
//   編譯後它不產生任何機器碼。真正搬移資源的是「因此被多載決議選中的
//   移動建構子或移動賦值運算子」。正確的心智模型是：
//       std::move   = 我對編譯器說「這個物件我不要了，可以拿走」
//       移動建構子  = 真正動手搬的人
//   若對方沒有移動建構子、或多載決議選不到它，就什麼都不會被搬走，
//   而且不會有任何警告。
//
// 【2. 為什麼回傳型別是 remove_reference_t<T>&& 而不是 T&&】
//   這是參考摺疊（reference collapsing）的必然結果。傳入左值時，
//   T 會被推導成 U&，於是 T&& 摺疊回 U&——變成回傳左值參考，功能完全失效。
//   先用 remove_reference_t 把參考剝掉，再補上 &&，才能保證任何輸入
//   都得到右值參考。本檔的 my_move 就是照這個邏輯手寫一遍來驗證。
//
// 【3. 移動不會發生的三種情況】
//   (a) const 物件：std::move(c) 得到 const T&&。移動建構子 T(T&&)
//       綁不了它（會丟掉 const），於是選中 T(const T&)——複製建構子。
//       程式正確但悄悄變慢，零警告。這是最常見的失效。
//   (b) 基本型別：int、double、指標沒有外部資源，移動與複製都是位元組拷貝，
//       而且來源的值有明確保證（x 依然是 42）。
//   (c) SSO 短字串：libstdc++ 對 15 字元以內的字串直接存在物件內部、
//       不碰堆積。此時移動與複製都是搬 32 bytes，完全一樣快。
//   結論：「移動比較快」只在「型別持有堆積資源」時才成立。
//
// 【4. std::move vs std::forward：不要混用】
//   std::move    無條件轉成右值。用於「我確定不再需要這個物件」。
//   std::forward 有條件轉換，保持原本的值類別。只用於轉發參考 T&&。
//   在轉發參考上誤用 std::move，會把呼叫端傳進來的左值也掏空——
//   呼叫端完全沒有授權你這麼做，這是很嚴重且難查的 bug。
//
// 【5. unique_ptr：用型別系統強制獨佔所有權】
//   unique_ptr 把複製建構與複製賦值 delete 掉，只留移動。
//   於是「同一份資源不能有兩個擁有者」從「靠自律」變成「編譯器強制」。
//   而且標準明文規定：移動後來源的 get() == nullptr——
//   這是有保證的，與一般型別的「有效但未指定」不同。
//
// 【概念補充 Concept Deep Dive】
//
// (A) moved-from 到底是什麼狀態
//     標準用語是「valid but unspecified」：可以安全解構、可以重新賦值後
//     正常使用，但內容不保證、不可依賴。讀它並不是 UB，但讀到什麼是未指定的。
//     libstdc++ 目前會把 moved-from 的長字串留成空，那是實作行為，
//     不是標準保證——所以本檔刻意不把它的內容寫進預期輸出。
//     唯一的例外是 unique_ptr / shared_ptr 等標準有明文規定的型別。
//
// (B) 為什麼 return std::move(local) 是反效果
//     直接 return local; 時編譯器會先嘗試 NRVO，把 local 直接建構在
//     回傳值的位置，一次搬移都不做。寫成 return std::move(local); 之後，
//     回傳運算式變成右值參考而非具名物件，NRVO 失去適用資格，
//     只能實際做一次移動建構。std::move 是用來「表達意圖」，不是「加速」。
//
// (C) 怎麼「量」移動與複製的差別才算數
//     牆鐘時間不可重現（受 CPU 頻率、快取、系統負載影響），
//     而且常見的比較寫法根本不公平——「移動組」為了準備來源，
//     往往每輪先做了一次完整複製，工作量比複製組還多。
//     本檔改用「堆積配置次數」：確定性、可重現，而且直接對應成本機制。
//     計時只留作參考並輸出到 stderr。
//
// 【注意事項 Pay Attention】
// 1. 不要把 moved-from 物件的內容當成固定值。讀取合法，但值未指定。
// 2. const 會讓 std::move 靜默退化成複製，沒有任何警告。
//    想被移動的區域變數與成員就不要加多餘的 const。
// 3. 回傳區域變數時不要寫 return std::move(local);——會抑制 NRVO。
// 4. 自訂型別的移動建構子請標 noexcept，否則 vector 擴容時會退化成複製。
// 5. SSO 門檻、sizeof(std::string) 都是實作定義（本機 libstdc++：15 字元、32 bytes）。
// 6. 對 trivially copyable 型別（int、double、指標）用 std::move 完全沒有效果。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::move
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::move 到底做了什麼？請用一句話說清楚。
//     答：它只是一個 static_cast，把左值轉成右值參考型別，編譯後不產生
//         任何指令。真正搬移資源的是因此被選中的移動建構子／移動賦值。
//         它是「授權標記」，不是「動作」。
//     追問：那為什麼要 remove_reference_t？→ 參考摺疊。傳左值時 T 推導成
//         U&，T&& 會摺疊回 U&，就拿不到右值參考了，必須先剝掉參考。
//
// 🔥 Q2. 請說出三種「寫了 std::move 卻沒有真的移動」的情況。
//     答：(1) 對 const 物件——const T&& 只能綁到 const T&，選中複製建構子；
//         (2) 基本型別——沒有資源可搬，移動就是複製；
//         (3) SSO 短字串——資料在物件內部、沒碰堆積，移動與複製一樣。
//     追問：這三種裡哪一種最危險？→ const 那一種。另外兩種本來就沒有
//         效能可言，但 const 是「你以為在移動、其實在深拷貝」，
//         而且完全沒有警告，效能問題會長期潛伏。
//
// 🔥 Q3. std::move 和 std::forward 有什麼不同？
//     答：std::move 無條件轉成右值，用於「我確定不再需要它」；
//         std::forward 有條件轉換、保持原本的值類別，只用於轉發參考 T&&
//         的完美轉發場景。
//     追問：在 T&& 參數上寫 std::move 會怎樣？→ 呼叫端若傳的是左值，
//         你會把對方還要用的物件掏空。對方從未授權，這是嚴重的 bug。
//
// ⚠️ 陷阱 1. 「std::string s = "Hi"; std::string t = std::move(s);
//             之後 s 一定是空字串」——為什麼這句話是錯的？
//     答：標準只保證 moved-from 物件「有效但未指定」。實務上更直接的
//         反例就是 SSO：短字串根本沒有堆積資源可搬，實作往往就地複製、
//         來源保持原值。所以 s 可能仍然是 "Hi"。
//     為什麼會錯：因為拿長字串測試時「看起來就是會清空」，於是把觀察到的
//         實作行為當成標準規則記住。一換成短字串（或換標準庫）就破功。
//
// ⚠️ 陷阱 2. 用 std::chrono 量「複製 N 次」與「移動 N 次」，
//            結果發現移動反而比較慢，於是下結論說移動語意沒用。
//     答：多半是量測本身寫錯了。常見的「移動組」為了準備可移動的來源，
//         每輪會先 std::string temp = source;（這是一次完整複製），
//         移動完可能還 source = moved; 再複製一次。工作量比複製組還多，
//         量出來的數字無法歸因到「移動」。
//     為什麼會錯：只盯著「被測的那一行」，忽略了同一個計時區間裡的前置
//         準備成本。量測的鐵律是：計時區間內只能有你要量的操作，
//         前置準備必須排除，或在兩組之間完全相同。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】從缺。
//   理由：std::move 屬於資源管理與值類別的語言規則，LeetCode 判定的是
//   演算法正確性，不會因為少一次移動而失敗；本清單中的題目
//   （設計題 146/155/705/707、陣列字串題 20/26/88/283 等）核心全在
//   資料結構與演算法，沒有一題以「值類別轉換」為主題。
//   硬套一題會傳達錯誤印象，故從缺，改以實務範例呈現 std::move
//   在真實系統中真正被需要的地方。

// ============================================================
// 第 2.5 章 總結：std::move — 強制轉換為右值
// 編譯：g++ -std=c++17 -o summary summary.cpp
// ============================================================
// 【核心觀念】std::move 本身不移動任何東西！
//   std::move(x) ≡ static_cast<T&&>(x)
//   只是把左值轉成右值引用，讓接收端可以選擇移動建構/賦值
//
// 【std::move 的實作原理】
//   template<typename T>
//   typename std::remove_reference<T>::type&& move(T&& arg) noexcept {
//       return static_cast<typename std::remove_reference<T>::type&&>(arg);
//   }
//   關鍵：remove_reference 去掉引用後再加 &&，無論傳入什麼都得到 T&&
//
// 【三個移動不會發生的情況】
//   1. const 物件：const T&& 只能匹配 const T&（複製建構）
//      → const string c = "x"; string d = move(c); // 複製！不是移動
//   2. 基本型別（int, double...）：沒有資源可搬，移動 = 複製
//   3. SSO 短字串：string 的 Short String Optimization，資料在棧上，移動也是複製
//
// 【unique_ptr 的所有權轉移】
//   unique_ptr 禁止複製，只能移動
//   take_ownership(std::move(p));  // 明確轉移所有權
// ============================================================

#include <iostream>
#include <string>
#include <utility>
#include <memory>
#include <type_traits>
#include <chrono>
#include <new>
#include <cstdlib>

// ============================================================
// 自己實作 my_move
// ============================================================
template<typename T>
typename std::remove_reference<T>::type&& my_move(T&& arg) noexcept {
    return static_cast<typename std::remove_reference<T>::type&&>(arg);
}

// 參數刻意不具名：我們只關心「多載決議選中了哪一個」，
// 不使用參數內容。具名但未使用會觸發 -Wunused-parameter。
void test(const std::string&) { std::cout << "  [左值版本]\n"; }
void test(std::string&&)      { std::cout << "  [右值版本]\n"; }

// -----------------------------------------------------------------------------
// 堆積配置計數器：覆寫全域 operator new / delete（標準允許的替換函式）
//   用來以「確定性的配置次數」取代不可重現的牆鐘計時。
//   注意 C++14 起要一併提供 sized deallocation，否則某些路徑找不到對應的 delete。
//   本檔為單執行緒，計數器未加同步。
// -----------------------------------------------------------------------------
static long g_alloc_count = 0;

void* operator new(std::size_t n) {
    ++g_alloc_count;
    void* p = std::malloc(n);
    if (!p) throw std::bad_alloc();
    return p;
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void* operator new[](std::size_t n) {
    ++g_alloc_count;
    void* p = std::malloc(n);
    if (!p) throw std::bad_alloc();
    return p;
}
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }

// 阻止編譯器把沒有可觀察副作用的被測程式碼整段最佳化掉
template <typename T>
static inline void doNotOptimize(T& value) {
    asm volatile("" : "+m"(value) : : "memory");
}

// -----------------------------------------------------------------------------
// 【日常實務範例】設定載入器：把解析結果「交出去」而不是複製一份
//   場景：服務啟動時讀取設定檔，解析成 Config 物件後交給各個子系統持有。
//         Config 內含多個字串欄位（連線字串、路徑、憑證內容…）。
//   為什麼用 std::move：載入器解析完就功成身退，那些字串不需要留在它手上。
//     用複製等於把每個欄位都深拷貝一次；用移動則只搬指標。
//     這正是 sink parameter 與「回傳值直接建構」的典型應用場景。
//   注意 buildConfig 回傳的是具名區域變數，直接 return cfg; 就好——
//     寫成 return std::move(cfg); 反而會抑制 NRVO、多一次移動。
// -----------------------------------------------------------------------------
struct Config {
    std::string dsn;
    std::string cert_pem;
    std::string log_path;
};

static Config buildConfig() {
    Config cfg;
    cfg.dsn      = std::string(400, 'd');   // 刻意超過 SSO 門檻
    cfg.cert_pem = std::string(400, 'c');
    cfg.log_path = std::string(400, 'l');
    return cfg;          // 直接回傳具名區域變數 → NRVO，不要加 std::move
}

// 複製版：子系統自己留一份副本
class SubsystemCopy {
    Config cfg_;
public:
    explicit SubsystemCopy(const Config& c) : cfg_(c) {}          // 深拷貝三個字串
    const Config& config() const { return cfg_; }
};

// 移動版：sink parameter，收下之後把內容搬進來
class SubsystemMove {
    Config cfg_;
public:
    explicit SubsystemMove(Config c) : cfg_(std::move(c)) {}      // 只搬指標
    const Config& config() const { return cfg_; }
};

int main() {
    // ============================================================
    // 1. std::move ≡ static_cast<T&&>
    // ============================================================
    std::cout << "===== 1. move vs static_cast =====\n";
    {
        std::string s = "Hello, World!";

        std::string a = std::move(s);              // 用 std::move
        s = "Hello, World!";
        std::string b = static_cast<std::string&&>(s);  // 用 static_cast
        // 兩者行為完全相同
        std::cout << "  a = \"" << a << "\"\n";
        std::cout << "  b = \"" << b << "\"\n";
    }
    std::cout << "\n";

    // ============================================================
    // 2. 自製 my_move 驗證
    // ============================================================
    std::cout << "===== 2. 自製 my_move =====\n";
    {
        std::string s = "Hello";

        std::cout << "  std::move:  "; test(std::move(s));
        s = "Hello";
        std::cout << "  my_move:    "; test(my_move(s));
        std::cout << "  my_move(rv):"; test(my_move(std::string("temp")));
    }
    std::cout << "\n";

    // ============================================================
    // 3. 移動不會發生的三種情況
    // ============================================================
    std::cout << "===== 3. 移動不會發生的情況 =====\n";

    // 情況 1：const 物件
    std::cout << "  【const 物件】\n";
    {
        const std::string c = "World";
        std::string d = std::move(c);  // const string&& → 匹配複製建構！
        std::cout << "    c = \"" << c << "\" (仍然是 World！沒有被移動)\n";
        std::cout << "    d = \"" << d << "\"\n";
    }

    // 情況 2：基本型別
    std::cout << "  【基本型別 int】\n";
    {
        int x = 42;
        int y = std::move(x);  // int 沒有資源可搬
        std::cout << "    x = " << x << " (仍然是 42，這對 int 有保證)\n";
        std::cout << "    y = " << y << "\n";
    }

    // 情況 3：SSO 短字串
    std::cout << "  【SSO 短字串】\n";
    {
        std::string s = "Hi";  // 短字串 → SSO，資料在物件內部、不碰堆積
        long before = g_alloc_count;
        std::string t = std::move(s);
        long after = g_alloc_count;
        std::cout << "    t = \"" << t << "\"\n";
        std::cout << "    這次「移動」造成的堆積配置次數: " << (after - before)
                  << "（SSO 下本來就沒有堆積資源可搬）\n";
        // 刻意「不印」s 的內容：moved-from 是「有效但未指定」，
        // 讀它不是 UB，但值不可依賴，不該寫進預期輸出。
        // 這裡改用確定性的證據：長字串才有堆積可搬，短字串沒有。
        std::string longSrc(400, 'x');           // 超過 SSO 門檻
        long b2 = g_alloc_count;
        std::string longDst = std::move(longSrc);
        long a2 = g_alloc_count;
        doNotOptimize(longDst);
        std::cout << "    對照：長字串移動造成的配置次數: " << (a2 - b2)
                  << "（同樣是 0，但它真的搬走了堆積緩衝區）\n";
    }
    std::cout << "\n";

    // ============================================================
    // 4. unique_ptr 所有權轉移
    // ============================================================
    std::cout << "===== 4. unique_ptr 所有權轉移 =====\n";
    {
        auto p = std::make_unique<int>(42);
        std::cout << "  *p = " << *p << "\n";

        // auto q = p;           // ❌ unique_ptr 不可複製！
        auto q = std::move(p);   // ✅ 轉移所有權
        std::cout << "  *q = " << *q << "\n";
        std::cout << "  p = " << (p ? "非空" : "nullptr") << "\n";
    }
    std::cout << "\n";

    // ============================================================
    // 5. 效能比較：用「堆積配置次數」而不是碼錶
    // ============================================================
    // 注意：這裡刻意不用 std::chrono 當主要證據。
    //   (a) 牆鐘時間不可重現，不能寫進預期輸出；
    //   (b) 常見的「移動組」寫法（先 temp = source 再 move，最後又
    //       source = moved 複製回去）其實比複製組做了更多事，
    //       量出來的數字無法歸因到「移動」這個操作。
    // 配置次數是確定性的，而且直接對應成本發生的機制。
    std::cout << "===== 5. 效能比較（以堆積配置次數量化）=====\n";
    {
        const int N = 100000;
        const std::size_t LEN = 10000;      // 遠超過 SSO 門檻，確保用到堆積
        std::string source(LEN, 'x');

        // 兩組的前置成本完全相同（都先建一份 temp），
        // 只在「被測操作」前後取計數快照，差額就是該操作本身的配置次數。
        long copy_allocs = 0;
        for (int i = 0; i < N; ++i) {
            std::string temp = source;
            doNotOptimize(temp);
            long a0 = g_alloc_count;
            std::string result = temp;                 // ← 被測：複製建構
            doNotOptimize(result);
            copy_allocs += g_alloc_count - a0;
        }

        long move_allocs = 0;
        for (int i = 0; i < N; ++i) {
            std::string temp = source;                 // 與上面完全相同的前置
            doNotOptimize(temp);
            long a0 = g_alloc_count;
            std::string result = std::move(temp);      // ← 被測：移動建構
            doNotOptimize(result);
            move_allocs += g_alloc_count - a0;
        }

        std::cout << "  字串長度 " << LEN << " 字元，各執行 " << N << " 次\n";
        std::cout << "  複製建構 -> 堆積配置 " << copy_allocs << " 次\n";
        std::cout << "  移動建構 -> 堆積配置 " << move_allocs << " 次\n";
        std::cout << "  複製成本隨長度線性成長；移動與長度無關，是常數時間。\n";

        // 耗時只作參考，送 stderr（每次執行都不同，不列入預期輸出）
        auto t1 = std::chrono::steady_clock::now();
        for (int i = 0; i < N; ++i) { std::string c = source; doNotOptimize(c); }
        auto t2 = std::chrono::steady_clock::now();
        std::cerr << "[stderr] 參考用耗時（每次執行都不同）：" << N << " 次複製 = "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count()
                  << " ms\n";
    }
    std::cout << "\n";

    // ============================================================
    // 6. 日常實務：設定載入器把結果交給子系統
    // ============================================================
    std::cout << "===== 6. 日常實務：設定交接（複製 vs 移動）=====\n";
    {
        {
            Config cfg = buildConfig();
            long a0 = g_alloc_count;
            SubsystemCopy sub(cfg);                    // 三個字串各深拷貝一次
            doNotOptimize(sub);
            std::cout << "  交給子系統（複製）-> 堆積配置 "
                      << (g_alloc_count - a0) << " 次\n";
        }
        {
            Config cfg = buildConfig();
            long a0 = g_alloc_count;
            SubsystemMove sub(std::move(cfg));         // sink parameter，只搬指標
            doNotOptimize(sub);
            std::cout << "  交給子系統（移動）-> 堆積配置 "
                      << (g_alloc_count - a0) << " 次\n";
        }
        std::cout << "  載入器解析完就不再需要那些字串，移動是這裡的正解。\n";
    }

    std::cout << "\n=== 重點整理 ===\n";
    std::cout << "  std::move 只是 static_cast<T&&>，不做實際移動\n";
    std::cout << "  const 物件 move → 退化為複製（const T&& → const T&）\n";
    std::cout << "  基本型別 / SSO 短字串 → 移動 = 複製，無效能差異\n";
    std::cout << "  unique_ptr 只能 move 不能 copy → 明確轉移所有權\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary

// 【但書】
//   1. 本檔刻意「不輸出」任何 moved-from 物件的內容（例如 SSO 那段的 s）：
//      moved-from 是「有效但未指定」，讀它不是 UB，但值不可依賴，
//      不適合寫進預期輸出。改以確定性的「堆積配置次數」呈現。
//   2. 「耗時」輸出到 stderr，不列入下方預期輸出——它每次執行都不同。
//      stdout 全部是確定性的計數，本機連跑 5 次逐位元組相同。
//   3. SSO 門檻（15 字元）、sizeof(std::string) = 32 bytes 皆為實作定義，
//      本機為 g++ 15.2 / libstdc++。
//   4. unique_ptr 移動後 p 為 nullptr 是標準明文保證，不是實作巧合。

// === 預期輸出 ===
// ===== 1. move vs static_cast =====
//   a = "Hello, World!"
//   b = "Hello, World!"
//
// ===== 2. 自製 my_move =====
//   std::move:    [右值版本]
//   my_move:      [右值版本]
//   my_move(rv):  [右值版本]
//
// ===== 3. 移動不會發生的情況 =====
//   【const 物件】
//     c = "World" (仍然是 World！沒有被移動)
//     d = "World"
//   【基本型別 int】
//     x = 42 (仍然是 42，這對 int 有保證)
//     y = 42
//   【SSO 短字串】
//     t = "Hi"
//     這次「移動」造成的堆積配置次數: 0（SSO 下本來就沒有堆積資源可搬）
//     對照：長字串移動造成的配置次數: 0（同樣是 0，但它真的搬走了堆積緩衝區）
//
// ===== 4. unique_ptr 所有權轉移 =====
//   *p = 42
//   *q = 42
//   p = nullptr
//
// ===== 5. 效能比較（以堆積配置次數量化）=====
//   字串長度 10000 字元，各執行 100000 次
//   複製建構 -> 堆積配置 100000 次
//   移動建構 -> 堆積配置 0 次
//   複製成本隨長度線性成長；移動與長度無關，是常數時間。
//
// ===== 6. 日常實務：設定交接（複製 vs 移動）=====
//   交給子系統（複製）-> 堆積配置 3 次
//   交給子系統（移動）-> 堆積配置 0 次
//   載入器解析完就不再需要那些字串，移動是這裡的正解。
//
// === 重點整理 ===
//   std::move 只是 static_cast<T&&>，不做實際移動
//   const 物件 move → 退化為複製（const T&& → const T&）
//   基本型別 / SSO 短字串 → 移動 = 複製，無效能差異
//   unique_ptr 只能 move 不能 copy → 明確轉移所有權
