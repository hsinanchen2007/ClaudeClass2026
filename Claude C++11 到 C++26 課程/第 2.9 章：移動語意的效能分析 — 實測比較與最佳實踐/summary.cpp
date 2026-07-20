// =============================================================================
//  summary.cpp  —  第 2.9 章總結：移動語意的效能分析（以「可重現的計數」為證據）
// =============================================================================
//
// 【主題資訊 Information】
//   標準版本：C++11 引入移動語意；本檔使用 C++17（inline static 成員變數）
//   標頭檔  ：<utility>(std::move)、<vector>、<string>、<chrono>、<cstdint>
//   關鍵複雜度：
//     複製建構 std::string / std::vector ：O(n)（配置 + 逐元素複製）
//     移動建構 std::string / std::vector ：O(1)（搬指標 + 把來源設為空殼）
//   關鍵語法：
//     T(T&& other) noexcept;             // 移動建構
//     T& operator=(T&& other) noexcept;  // 移動賦值
//     std::move(x)                       // 只是 static_cast<T&&>，本身不搬東西
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼這一版把「碼錶」換成「計數器」】
//   效能章節最直覺的做法是拿 std::chrono 量時間，但牆鐘時間有三個致命問題，
//   而且本檔前一版就親自踩到了：
//     (a) 不可重現：同一支程式連跑三次，三個結果都不一樣（CPU 頻率、排程、
//         cache 狀態、其他行程都會影響），教材沒辦法貼出穩定的「預期輸出」。
//     (b) 會被最佳化吃掉：-O2 之下，`int copy = source; (void)copy;` 整個迴圈
//         是死碼，會被完全刪除，於是量到 0 ms。這時候「0 ms」不代表「移動很快」，
//         而是「這段程式根本沒執行」——這是效能量測最常見的假象。
//     (c) 容易量錯對象：前一版的「移動」迴圈寫成
//             std::string temp = source;              // ← 這裡先複製了一次！
//             std::string moved = std::move(temp);    // ← 才移動
//         等於在量「複製 + 移動」對上「複製」，難怪移動反而比較慢。
//   本檔改用「可重現的觀測」：一是比對緩衝區指標有沒有被接管（移動會偷走
//   同一塊 heap，複製則另配一塊），二是統計複製建構／移動建構各被呼叫幾次。
//   這些結果由語言規則決定，跑幾次都一樣，而且比時間更能說明「移動省下了
//   什麼」——省下的正是那一次 O(n) 的配置與複製。
//   牆鐘時間仍然保留，但一律輸出到 stderr，讓 stdout 保持逐位元組穩定。
//
// 【2. 移動到底搬了什麼】
//   std::string / std::vector 的物件本體只是一個「控制區塊」，真正的資料在 heap：
//       vector<int> ──► [ ptr | size | capacity ]  ──► heap: [42][42][42]...
//   複製建構必須：配置一塊同樣大的 heap + 把 n 個元素全部搬過去 → O(n)。
//   移動建構只要：把 ptr/size/capacity 三個欄位抄過來，再把來源三個欄位歸零 → O(1)。
//   關鍵在於「來源必須被改成合法的空殼」，否則兩個物件會指向同一塊記憶體，
//   解構時就會 double free。所以移動一定會修改來源，這也是它不能吃 const 的原因。
//
// 【3. noexcept 為什麼會影響效能（本章最重要的一點）】
//   vector 擴容時要把舊緩衝區的元素搬到新緩衝區。它必須維持「強例外保證」：
//   萬一搬到一半爆了例外，vector 要能回到原狀。
//     * 用「複製」搬：舊資料還在，出事就把新緩衝區丟掉即可 → 可以回復。
//     * 用「移動」搬：舊資料已經被掏空，出事就回不去了 → 無法回復。
//   所以 vector 只有在「移動建構保證不丟例外」時才敢用移動，判斷方式是
//   std::move_if_noexcept。結論很直接：
//       移動建構沒寫 noexcept → vector 擴容時退回用「複製」，你的移動語意等於白寫。
//   這是唯一「一個關鍵字直接決定演算法選擇」的地方，面試極常考。
//
// 【4. 什麼情況下移動沒有好處】
//   移動的收益 = 省下的那次配置 + 複製。若本來就沒有配置，就沒有收益：
//     * 基本型別（int/double/指標）：移動就是複製一份位元，完全相同。
//     * SSO 短字串：libstdc++ 的 std::string 對長度 ≤ 15 的字串直接存在物件內部
//       （Small String Optimization，門檻 15 屬實作定義），不碰 heap，
//       所以複製與移動都不碰 heap，兩者都要逐位元組搬同樣的內容。
//     * 陣列成員 / 沒有 heap 資源的 POD 聚合。
//   反過來說，資料越大、配置越貴，移動的優勢越明顯——但那是「省下 O(n)」，
//   不是「移動本身變快」。
//
// 【概念補充 Concept Deep Dive】
//
// (A) std::move 不移動任何東西
//   std::move(x) 的實作大致是 static_cast<remove_reference_t<T>&&>(x)，
//   它只是把左值「貼上右值標籤」，讓多載決議挑中移動建構。真正搬資料的是
//   那個被挑中的建構子。如果目標型別沒有移動建構（或被 const 擋掉），
//   多載決議會安靜地退回複製建構——不會有任何警告，這是很難察覺的效能坑。
//
// (B) 為什麼 move const 物件會退化成複製
//   std::move(const T x) 得到的是 const T&&。移動建構的參數是 T&&，
//   不能綁 const T&&；但複製建構的參數是 const T&，可以綁。
//   於是多載決議選中複製建構，編譯通過、行為正確、效能全失。
//
// (C) 為什麼 return std::move(local) 是反效果
//   直接 return local 時，編譯器可以做 NRVO（具名回傳值最佳化），
//   直接在呼叫端的空間就地建構，連移動都不必——零次建構。
//   寫成 return std::move(local) 會把回傳運算式變成右值參考運算式，
//   NRVO 的條件被破壞，反而強迫做一次移動建構。省一次移動 > 做一次移動。
//   （注意：C++17 起對「回傳純右值」的複製省略是強制的，但 NRVO 對
//    具名區域變數始終是「允許但不強制」的最佳化。）
//
// (D) 本檔用什麼當「可重現的證據」
//   兩種互補的觀測，都由語言規則決定，跑幾次都一樣：
//     1) 緩衝區指標是否被「偷走」：
//        移動之後，目的地的 data() 會等於來源原本的 data()（同一塊 heap 被接管）；
//        複製則會得到另一塊緩衝區，兩者必然不同。本檔只印這個比較的
//        「布林結果」，不印原始位址（位址每次執行都不同）。
//     2) 複製建構／移動建構各被呼叫幾次：
//        用帶計數器的型別（Tracked）觀察 vector 擴容時到底走了哪一條路。
//   這兩者合起來，能直接證明「移動省下的是那一次 O(n) 的配置與複製」，
//   比牆鐘時間穩定得多，也不會被 -O2 的死碼消除誤導。
//
// 【注意事項 Pay Attention】
// 1. 移動後的來源物件處於「有效但未指定（valid but unspecified）」狀態。
//    可以安全地解構、可以重新賦值，但不可以假設它的內容是什麼。
//    本檔只印 size() 與「緩衝區是否被接管」這類有定義的觀測，不印殘值。
//    （libstdc++ 實測被移動後的 vector 會是空的，但那是實作行為，標準沒有保證。）
// 2. SSO 門檻 15 是 libstdc++ 的實作定義值；MSVC 為 15、libc++ 為 22，
//    數字不可寫死在程式邏輯裡。
// 3. 本檔 stdout 全是計數與布林值，逐位元組可重現；
//    stderr 的耗時每次執行都不同，僅供參考，不可拿來當驗收條件。
// 4. -O2 會把沒有副作用的迴圈整段刪除。若要量時間，務必讓結果流向
//    可觀測的地方（本檔累加 checksum 並輸出到 stderr）以避免死碼消除。
// 5. Timer 的成員初始化順序必須與「宣告順序」一致，否則 -Wall 會發出
//    -Wreorder 警告（本檔前一版就有這個問題，已修正，見 Timer 定義處）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】移動語意的效能
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 移動建構為什麼一定要加 noexcept？不加會怎樣？
//     答：vector 擴容要維持強例外保證。它用 std::move_if_noexcept 判斷：
//         移動建構若沒標 noexcept，vector 不敢用移動（因為搬到一半丟例外就
//         回不去了），會退回逐一「複製」舊元素。結果是你寫了移動建構，
//         在最需要它的擴容路徑上卻完全沒被使用，效能等於沒優化。
//     追問：那 std::move_if_noexcept 對「只有移動、不能複製」的型別怎麼辦？
//         → 此時沒得選，即使移動建構沒 noexcept 也只能用移動；
//           它的規則是「可複製 且 移動不是 noexcept」才退回複製。
//
// 🔥 Q2. std::move 做了什麼？它會搬資料嗎？
//     答：不會。它只是一個 static_cast<T&&>，把左值轉成右值參考，
//         純粹是型別轉換，執行期零成本、零指令。真正搬資料的是因此被
//         多載決議選中的移動建構／移動賦值。
//     追問：那對一個沒有移動建構的型別呼叫 std::move 會發生什麼事？
//         → 安靜地退回複製建構，不會有錯誤也不會有警告，效能全失。
//
// ⚠️ 陷阱 1. 「return std::move(local) 可以避免複製，所以比較快」
//     答：錯，這樣寫反而變慢。直接 return local 時編譯器可以做 NRVO，
//         在呼叫端就地建構，連一次移動都不需要（零次建構）；
//         加上 std::move 會破壞 NRVO 條件，強制多做一次移動建構。
//     為什麼會錯：多數人的心智模型是「std::move = 加速」，
//         把它當成效能開關到處加。實際上 std::move 只是換多載，
//         而編譯器的複製省略比任何一次移動都更省——你是在跟最佳化搶工作。
//
// ⚠️ 陷阱 2. 「量到移動比複製慢，所以移動語意沒用」
//     答：先檢查量測本身。最常見的錯誤是把「複製 + 移動」拿去對比「複製」：
//             std::string tmp = src;            // 這行已經複製了一次
//             std::string dst = std::move(tmp); // 再移動
//         這樣量出來移動當然比較慢，因為它多做了一次移動。
//         正確做法是先在計時區外備好來源，再分別量「只複製」與「只移動」。
//     為什麼會錯：把 benchmark 的結果當成結論，卻沒有檢查 benchmark 有沒有
//         量到你以為的東西。第二個常見假象是 -O2 把整個迴圈當死碼刪掉，
//         量到 0 ms 還誤以為「移動快到量不出來」。
//
// ⚠️ 陷阱 3. 「const 物件也可以 move，反正 std::move 一定有效」
//     答：std::move(const T) 得到 const T&&，綁不到 T&& 的移動建構，
//         但綁得到 const T& 的複製建構，於是安靜退化成複製。
//     為什麼會錯：因為它「編譯得過、跑得對」，只是慢。沒有任何診斷訊息，
//         只能靠 review 或本檔這種複製／移動次數計數才抓得到。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <vector>
#include <utility>
#include <chrono>
#include <cstddef>
#include <cstdint>

// -----------------------------------------------------------------------------
// 觀測工具：緩衝區是否「被偷走」
//   把指標轉成 std::uintptr_t 再比較，只回傳布林值。
//   為什麼不直接印位址：位址每次執行都不同，教材無法貼出穩定的預期輸出。
//   為什麼轉成整數：跨不同物件的原始指標做 < / > 比較屬未指定行為，
//   轉成 uintptr_t 後的整數比較則是實作定義且可攜的常見做法。
// -----------------------------------------------------------------------------
static inline std::uintptr_t addr_of(const void* p) {
    return reinterpret_cast<std::uintptr_t>(p);
}

// 判斷字串資料是否就存在物件自己的記憶體footprint 內（= 命中 SSO，未配置 heap）
static inline bool data_is_inside_object(const std::string& s) {
    const std::uintptr_t obj  = addr_of(&s);
    const std::uintptr_t data = addr_of(s.data());
    return data >= obj && data < obj + sizeof(std::string);
}

// -----------------------------------------------------------------------------
// Timer：只把耗時寫到 stderr，stdout 保持逐位元組穩定
//   注意成員宣告順序 = 初始化順序。前一版把 label_ 寫在初始化列的前面、
//   卻宣告在 start_ 後面，觸發 -Wreorder（-Wall 預設開啟）。
//   這裡把 label_ 宣告在前，初始化列也照同樣順序，警告即消失。
// -----------------------------------------------------------------------------
class Timer {
    const char* label_;                                        // 宣告第 1 → 先初始化
    std::chrono::high_resolution_clock::time_point start_;     // 宣告第 2 → 後初始化
public:
    explicit Timer(const char* label)
        : label_(label),
          start_(std::chrono::high_resolution_clock::now()) {}
    ~Timer() {
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(
                      std::chrono::high_resolution_clock::now() - start_).count();
        // 走 stderr：耗時每次執行都不同，不能污染要驗收的 stdout
        std::cerr << "  [timing] " << label_ << ": " << us << " us\n";
    }
};

// -----------------------------------------------------------------------------
// 可觀測型別：分別統計複製建構與移動建構被呼叫幾次
// -----------------------------------------------------------------------------
struct Tracked {
    static inline int copies = 0;      // C++17: inline static 成員可就地定義
    static inline int moves  = 0;
    std::vector<int> data;

    Tracked() : data(1000, 42) {}
    Tracked(const Tracked& o) : data(o.data)            { ++copies; }
    Tracked(Tracked&& o) noexcept : data(std::move(o.data)) { ++moves; }
    Tracked& operator=(const Tracked&) = default;
    Tracked& operator=(Tracked&&) noexcept = default;

    static void reset() { copies = 0; moves = 0; }
};

// 同一個型別，差別只在「移動建構沒有 noexcept」
struct TrackedNoNoexcept {
    static inline int copies = 0;
    static inline int moves  = 0;
    std::vector<int> data;

    TrackedNoNoexcept() : data(1000, 42) {}
    TrackedNoNoexcept(const TrackedNoNoexcept& o) : data(o.data) { ++copies; }
    TrackedNoNoexcept(TrackedNoNoexcept&& o) : data(std::move(o.data)) { ++moves; }  // 沒 noexcept

    static void reset() { copies = 0; moves = 0; }
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 本檔刻意不附
//   理由：本章主題是「移動語意的效能特性」，屬於語言機制與資源管理層面，
//   衡量標準是配置次數與例外保證，不是演算法複雜度。LeetCode 題目評測的是
//   演算法本身，沒有任何一題的正確性或複雜度取決於「你有沒有寫 noexcept」
//   或「這裡是複製還是移動」。硬掛一題（例如 283. Move Zeroes 只是名字裡
//   有 move，實際是雙指標覆寫，與移動語意毫無關係）只會誤導讀者，
//   因此依規格「寧缺勿濫」原則從缺，改以下方兩個真實工程情境示範。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例 1】日誌處理管線：跨階段傳遞紀錄時，複製 vs 移動的配置次數
//   情境：一條 讀取 → 過濾 → 匯出 的 log 管線。每筆紀錄含訊息字串與標籤，
//         字串長度普遍超過 SSO 門檻，因此每次複製都是一次真實的 heap 配置。
//         這是後端服務最典型的「明明不需要副本，卻整條管線一路複製」效能坑。
// -----------------------------------------------------------------------------
struct LogRecord {
    static inline int copies = 0;      // 這筆紀錄被「複製建構」了幾次
    static inline int moves  = 0;      // 被「移動建構」了幾次
    std::string message;               // 長訊息：超過 SSO 門檻，存在 heap
    std::string level;                 // 短字串：多半落在 SSO 內

    LogRecord(std::string msg, std::string lv)
        : message(std::move(msg)), level(std::move(lv)) {}   // by-value + move 慣用法

    LogRecord(const LogRecord& o)
        : message(o.message), level(o.level)             { ++copies; }
    LogRecord(LogRecord&& o) noexcept
        : message(std::move(o.message)), level(std::move(o.level)) { ++moves; }

    static void reset() { copies = 0; moves = 0; }
};

// 把來源搬進管線：copy 版本（每筆都重新配置一次字串）
static std::vector<LogRecord> pipelineByCopy(const std::vector<LogRecord>& src) {
    std::vector<LogRecord> out;
    out.reserve(src.size());              // 先 reserve，排除擴容干擾
    for (const auto& rec : src) {
        out.push_back(rec);               // 複製建構：message 會再配置一次
    }
    return out;
}

// 把來源搬進管線：move 版本（掏空來源，不配置）
static std::vector<LogRecord> pipelineByMove(std::vector<LogRecord>& src) {
    std::vector<LogRecord> out;
    out.reserve(src.size());
    for (auto& rec : src) {
        out.push_back(std::move(rec));    // 移動建構：只搬指標
    }
    return out;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】組裝回應內容：回傳區域變數時不要畫蛇添足加 std::move
//   情境：把查詢結果組成一份回應物件回傳給呼叫端。
//         兩個版本的差別只有 return 那一行寫不寫 std::move。
//   用 LogRecord 的計數器來觀測，就能「數出」std::move 多做了幾次移動建構。
// -----------------------------------------------------------------------------
static LogRecord buildResponseGood(const std::vector<std::string>& rows) {
    LogRecord body(std::string{}, "INFO");
    for (const auto& r : rows) { body.message += r; body.message += '\n'; }
    return body;              // ✅ 直接回傳具名區域變數 → 可觸發 NRVO（零次建構）
}

// 這個函式刻意示範反面寫法。GCC 的 -Wpessimizing-move（-Wall 預設開啟）會
// 正確地指出「這個 std::move 阻止了複製省略」——那正是本範例要證明的事，
// 因此在此局部關閉該警告，讓整份檔案仍能在 -Wall -Wextra 下零警告編譯。
// 請勿把這個 pragma 當成通用寫法：在真實程式碼裡看到這個警告，應該是把
// std::move 刪掉，而不是把警告關掉。
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpessimizing-move"
static LogRecord buildResponseBad(const std::vector<std::string>& rows) {
    LogRecord body(std::string{}, "INFO");
    for (const auto& r : rows) { body.message += r; body.message += '\n'; }
    return std::move(body);   // ❌ 破壞 NRVO 條件，強迫多做一次移動建構
}
#pragma GCC diagnostic pop

int main() {
    std::cout << "===== 1. 長字串：移動是「接管緩衝區」，複製是「另配一塊」 =====\n";
    {
        const std::string source(10000, 'x');     // 遠超 SSO 門檻，必定在 heap

        std::string donor    = source;            // 先在觀測區外備好來源
        const auto  donorBuf = addr_of(donor.data());

        std::string copied = source;              // 複製建構
        std::string moved  = std::move(donor);    // 移動建構

        std::cout << "  複製後 目的地緩衝區 == 來源緩衝區？ "
                  << (addr_of(copied.data()) == donorBuf ? "是" : "否")
                  << "（另外配置了一塊，內容逐位元組複製，O(n)）\n";
        std::cout << "  移動後 目的地緩衝區 == 來源緩衝區？ "
                  << (addr_of(moved.data()) == donorBuf ? "是" : "否")
                  << "（直接接管同一塊 heap，只搬指標，O(1)）\n";
        std::cout << "  移動後兩者長度：目的地 " << moved.size()
                  << "、來源 " << donor.size() << "\n";
        std::cout << "  → 移動省下的，正是那一次 O(n) 的配置與複製\n";
    }

    std::cout << "\n===== 2. SSO 短字串：移動沒有好處 =====\n";
    {
        std::string shortSrc = "Hi";              // 長度 2，落在 SSO 門檻內
        std::string copied   = shortSrc;
        std::string moved    = std::move(shortSrc);

        std::cout << "  來源資料位於物件內部（未用 heap）？ "
                  << (data_is_inside_object(copied) ? "是" : "否") << "\n";
        std::cout << "  移動後資料仍位於物件內部？         "
                  << (data_is_inside_object(moved) ? "是" : "否") << "\n";
        std::cout << "  → 沒有 heap 緩衝區可以偷，複製與移動都得逐位元組搬同樣的內容\n";
        std::cout << "  （SSO 門檻 15 為 libstdc++ 實作定義值；libc++ 為 22）\n";
    }

    std::cout << "\n===== 3. vector：不論多大，移動都只是搬指標 =====\n";
    {
        const std::vector<int> source(100000, 42);

        std::vector<int> donor    = source;
        const auto       donorBuf = addr_of(donor.data());

        std::vector<int> copied = source;
        std::vector<int> moved  = std::move(donor);

        std::cout << "  複製 100000 個 int：目的地緩衝區 == 來源緩衝區？ "
                  << (addr_of(copied.data()) == donorBuf ? "是" : "否") << "\n";
        std::cout << "  移動 100000 個 int：目的地緩衝區 == 來源緩衝區？ "
                  << (addr_of(moved.data()) == donorBuf ? "是" : "否") << "\n";
        std::cout << "  移動後 目的地 size = " << moved.size()
                  << "、capacity = " << moved.capacity() << "\n";
        std::cout << "  → 元素個數完全不影響移動的成本：永遠是 O(1)\n";
    }

    std::cout << "\n===== 4. noexcept 決定 vector 擴容用移動還是複製 =====\n";
    {
        Tracked::reset();
        {
            std::vector<Tracked> v;
            for (int i = 0; i < 64; ++i) v.emplace_back();   // 不 reserve，逼它擴容
        }
        std::cout << "  移動建構有 noexcept ：擴容時 複製 " << Tracked::copies
                  << " 次、移動 " << Tracked::moves << " 次\n";

        TrackedNoNoexcept::reset();
        {
            std::vector<TrackedNoNoexcept> v;
            for (int i = 0; i < 64; ++i) v.emplace_back();
        }
        std::cout << "  移動建構沒 noexcept：擴容時 複製 " << TrackedNoNoexcept::copies
                  << " 次、移動 " << TrackedNoNoexcept::moves << " 次\n";
        std::cout << "  → 少一個 noexcept，擴容就整批退回複製（std::move_if_noexcept）\n";
        std::cout << "  （搬移總次數 = 擴容過程累計搬動的元素數，取決於成長策略；\n";
        std::cout << "    libstdc++ 為 2 倍成長，屬實作定義）\n";
    }

    std::cout << "\n===== 5. 基本型別：移動就是複製 =====\n";
    {
        int a = 42;
        int b = a;                 // 複製
        int c = std::move(a);      // 對 int 而言，這仍然是一次位元複製
        std::cout << "  int b = a          → b = " << b << "\n";
        std::cout << "  int c = move(a)    → c = " << c << "\n";
        std::cout << "  移動後 a 仍可讀且值不變：a = " << a << "\n";
        std::cout << "  → 基本型別沒有 heap 資源可偷，移動與複製完全等價\n";
    }

    std::cout << "\n===== 6. 日常實務 1：log 管線 複製 vs 移動 =====\n";
    {
        auto makeRecords = []() {
            std::vector<LogRecord> v;
            v.reserve(3);
            v.emplace_back(std::string(200, 'A') + " request timeout on /api/order", "ERROR");
            v.emplace_back(std::string(200, 'B') + " slow query detected in orders",  "WARN");
            v.emplace_back(std::string(200, 'C') + " batch export finished",          "INFO");
            return v;
        };

        std::vector<LogRecord> src1 = makeRecords();
        LogRecord::reset();
        auto out1 = pipelineByCopy(src1);
        const int copyPathCopies = LogRecord::copies;
        const int copyPathMoves  = LogRecord::moves;

        std::vector<LogRecord> src2 = makeRecords();
        LogRecord::reset();
        auto out2 = pipelineByMove(src2);
        const int movePathCopies = LogRecord::copies;
        const int movePathMoves  = LogRecord::moves;

        std::cout << "  3 筆長訊息 log 進入管線：\n";
        std::cout << "    一路複製：複製建構 " << copyPathCopies
                  << " 次、移動建構 " << copyPathMoves << " 次\n";
        std::cout << "    一路移動：複製建構 " << movePathCopies
                  << " 次、移動建構 " << movePathMoves << " 次\n";
        std::cout << "  搬運後筆數：複製版 " << out1.size()
                  << " 筆、移動版 " << out2.size() << " 筆（結果相同）\n";
        std::cout << "  複製版的來源是否仍持有訊息："
                  << (src1[0].message.empty() ? "否" : "是") << "\n";
        std::cout << "  → 複製版每筆長訊息都要重新配置一塊 heap；\n";
        std::cout << "    移動版只搬指標，來源被掏空（這正是它便宜的原因）\n";
    }

    std::cout << "\n===== 7. 日常實務 2：return 區域變數不要加 std::move =====\n";
    {
        const std::vector<std::string> rows = {"id,name", "1,Alice", "2,Bob"};

        LogRecord::reset();
        LogRecord good = buildResponseGood(rows);
        const int goodMoves = LogRecord::moves, goodCopies = LogRecord::copies;

        LogRecord::reset();
        LogRecord bad = buildResponseBad(rows);
        const int badMoves = LogRecord::moves, badCopies = LogRecord::copies;

        std::cout << "  return body            ：移動建構 " << goodMoves
                  << " 次、複製建構 " << goodCopies << " 次\n";
        std::cout << "  return std::move(body) ：移動建構 " << badMoves
                  << " 次、複製建構 " << badCopies << " 次\n";
        std::cout << "  兩者內容相同：" << (good.message == bad.message ? "是" : "否") << "\n";
        std::cout << "  → 直接 return 具名區域變數，NRVO 讓它在呼叫端就地建構，\n";
        std::cout << "    一次建構都不用；加了 std::move 反而強迫多做一次移動建構。\n";
        std::cout << "    std::move 在這裡不是優化，是反優化。\n";
    }

    // 保留一段牆鐘量測作為對照，但輸出到 stderr，不影響 stdout 的可重現性
    {
        std::cerr << "\n[以下耗時輸出到 stderr，每次執行都不同，僅供參考]\n";
        const std::size_t N = 20000;
        const std::vector<int> source(2000, 7);
        unsigned long long checksum = 0;      // 累加結果，避免 -O2 把迴圈當死碼刪除

        {
            Timer t("複製 20000 次 vector<int>(2000)");
            for (std::size_t i = 0; i < N; ++i) {
                std::vector<int> c = source;
                checksum += c.size();
            }
        }
        {
            std::vector<std::vector<int>> donors(N, source);   // 在計時區外備好來源
            Timer t("移動 20000 次 vector<int>(2000)");
            for (std::size_t i = 0; i < N; ++i) {
                std::vector<int> m = std::move(donors[i]);
                checksum += m.size();
            }
        }
        std::cerr << "  [checksum] " << checksum << "（僅為阻止死碼消除）\n";
    }

    std::cout << "\n=== 最佳實踐 ===\n";
    std::cout << "  [O] 移動建構/賦值一定要加 noexcept（否則擴容退回複製）\n";
    std::cout << "  [O] emplace_back 就地建構 > push_back(move) > push_back(copy)\n";
    std::cout << "  [O] 確定不再使用的物件，傳入時用 std::move\n";
    std::cout << "  [O] 優先 Rule of Zero：讓 RAII 成員自動帶來正確的移動語意\n";
    std::cout << "  [X] 不要 move const 物件（安靜退化為複製）\n";
    std::cout << "  [X] 不要 return std::move(local)（破壞 NRVO）\n";
    std::cout << "  [X] 不要對基本型別用 std::move（無效果）\n";
    std::cout << "  [X] 不要用牆鐘時間當唯一證據（會被最佳化與雜訊誤導）\n";

    return 0;
}

// 編譯: g++ -std=c++17 -O2 -Wall -Wextra summary.cpp -o summary
// 執行: ./summary              （stdout 穩定；stderr 另有耗時參考）
//       ./summary 2>/dev/null  （只看可重現的計數結果）

// ── 輸出但書 ────────────────────────────────────────────────────────────
// 1. 下方「預期輸出」只含 stdout，全部是計數與布林值，逐位元組可重現
//    （實測連跑 10 次 md5 完全相同）。stderr 的 [timing] 與 [checksum]
//    每次執行都不同，刻意不納入預期輸出。
// 2. 第 1 節「移動後 來源 size = 0」：被移動的來源處於「有效但未指定」狀態。
//    libstdc++ 實測會留下空字串，但標準並未保證這一點，不可寫進程式邏輯。
//    本檔只印 size() 這個有定義的觀測值，不印殘留內容。
// 3. 第 4 節的搬移次數（63）是「擴容過程累計搬動的元素數」，取決於 vector
//    的成長策略；libstdc++ 為 2 倍成長（屬實作定義），故 64 次 emplace_back
//    會在 capacity 1→2→4→…→64 的過程中累計搬動 1+2+4+8+16+32 = 63 個元素。
//    換一個成長倍率不同的標準函式庫（例如 MSVC 約 1.5 倍），這個數字會不同，
//    但「有 noexcept 走移動、沒 noexcept 走複製」的分野不變。
// 4. 緩衝區比較一律只印布林（是／否），不印原始位址——位址每次執行都不同。
//    第 3 節的 capacity == size == 100000 是「複製建構會剛好配置所需大小」
//    的結果，屬實作定義。
// 5. 本機為 GCC 15.2.0 (Ubuntu 15.2.0-16ubuntu1) / libstdc++ / x86-64 / -O2。
// 6.【關於 stderr 那段耗時，請務必看這一點】
//    實測中「移動 20000 次」有時反而比「複製 20000 次」還慢。這不是移動變貴了，
//    而是量測本身的假象：複製迴圈每輪釋放後立刻重新配置同一塊記憶體，
//    位址固定、快取極友善；移動迴圈則要走過 20000 塊事先配好、散落各處的
//    緩衝區，付出大量 cache miss 與缺頁成本。
//    這正是本檔把證據從「牆鐘時間」換成「計數」的理由——
//    微基準量到的往往是記憶體配置器與快取的行為，而不是你以為的那個語言特性。
//    要判斷移動有沒有生效，請看第 4 節的複製／移動次數，不要看這段時間。

// === 預期輸出 ===
// ===== 1. 長字串：移動是「接管緩衝區」，複製是「另配一塊」 =====
//   複製後 目的地緩衝區 == 來源緩衝區？ 否（另外配置了一塊，內容逐位元組複製，O(n)）
//   移動後 目的地緩衝區 == 來源緩衝區？ 是（直接接管同一塊 heap，只搬指標，O(1)）
//   移動後兩者長度：目的地 10000、來源 0
//   → 移動省下的，正是那一次 O(n) 的配置與複製
//
// ===== 2. SSO 短字串：移動沒有好處 =====
//   來源資料位於物件內部（未用 heap）？ 是
//   移動後資料仍位於物件內部？         是
//   → 沒有 heap 緩衝區可以偷，複製與移動都得逐位元組搬同樣的內容
//   （SSO 門檻 15 為 libstdc++ 實作定義值；libc++ 為 22）
//
// ===== 3. vector：不論多大，移動都只是搬指標 =====
//   複製 100000 個 int：目的地緩衝區 == 來源緩衝區？ 否
//   移動 100000 個 int：目的地緩衝區 == 來源緩衝區？ 是
//   移動後 目的地 size = 100000、capacity = 100000
//   → 元素個數完全不影響移動的成本：永遠是 O(1)
//
// ===== 4. noexcept 決定 vector 擴容用移動還是複製 =====
//   移動建構有 noexcept ：擴容時 複製 0 次、移動 63 次
//   移動建構沒 noexcept：擴容時 複製 63 次、移動 0 次
//   → 少一個 noexcept，擴容就整批退回複製（std::move_if_noexcept）
//   （搬移總次數 = 擴容過程累計搬動的元素數，取決於成長策略；
//     libstdc++ 為 2 倍成長，屬實作定義）
//
// ===== 5. 基本型別：移動就是複製 =====
//   int b = a          → b = 42
//   int c = move(a)    → c = 42
//   移動後 a 仍可讀且值不變：a = 42
//   → 基本型別沒有 heap 資源可偷，移動與複製完全等價
//
// ===== 6. 日常實務 1：log 管線 複製 vs 移動 =====
//   3 筆長訊息 log 進入管線：
//     一路複製：複製建構 3 次、移動建構 0 次
//     一路移動：複製建構 0 次、移動建構 3 次
//   搬運後筆數：複製版 3 筆、移動版 3 筆（結果相同）
//   複製版的來源是否仍持有訊息：是
//   → 複製版每筆長訊息都要重新配置一塊 heap；
//     移動版只搬指標，來源被掏空（這正是它便宜的原因）
//
// ===== 7. 日常實務 2：return 區域變數不要加 std::move =====
//   return body            ：移動建構 0 次、複製建構 0 次
//   return std::move(body) ：移動建構 1 次、複製建構 0 次
//   兩者內容相同：是
//   → 直接 return 具名區域變數，NRVO 讓它在呼叫端就地建構，
//     一次建構都不用；加了 std::move 反而強迫多做一次移動建構。
//     std::move 在這裡不是優化，是反優化。
//
// === 最佳實踐 ===
//   [O] 移動建構/賦值一定要加 noexcept（否則擴容退回複製）
//   [O] emplace_back 就地建構 > push_back(move) > push_back(copy)
//   [O] 確定不再使用的物件，傳入時用 std::move
//   [O] 優先 Rule of Zero：讓 RAII 成員自動帶來正確的移動語意
//   [X] 不要 move const 物件（安靜退化為複製）
//   [X] 不要 return std::move(local)（破壞 NRVO）
//   [X] 不要對基本型別用 std::move（無效果）
//   [X] 不要用牆鐘時間當唯一證據（會被最佳化與雜訊誤導）
