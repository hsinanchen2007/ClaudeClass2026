// =============================================================================
//  第 10 課：vector 的宣告與初始化方式 6  —  移動建構 std::move
// =============================================================================
//
// 【主題資訊 Information】
//   vector(vector&& other) noexcept;                      // 移動建構（C++11）
//   template<class T> constexpr std::remove_reference_t<T>&& move(T&& t) noexcept;
//
//   標頭檔　：<vector>；std::move 定義在 <utility>
//   標準版本：C++11（C++17 起移動建構明確要求 noexcept）
//   複雜度　：O(1)——與元素個數完全無關
//   移動後　：來源處於「有效但未指定（valid but unspecified）」狀態
//
// 【詳細解釋 Explanation】
//
// 【1. 移動建構做了什麼？——偷走三根指標】
//   回想 vector 內部就是三根指標（start / finish / end_of_storage）。
//   移動建構只做兩件事：
//     ① 把 other 的三根指標「複製」到自己身上（三次指標賦值）
//     ② 把 other 的三根指標設為 nullptr（讓它不會在解構時釋放同一塊記憶體）
//   heap 上的資料一個位元組都沒有搬動，元素也完全沒有被複製或移動。
//   所以不論 vector 裡有 5 個還是 5 千萬個元素，移動成本都一樣是 O(1)。
//
//   ★ 一個容易誤解的點：移動一個 vector<T>，T 的 move constructor
//     一次都不會被呼叫。本檔實測就是這樣——搬的是容器，不是元素。
//
// 【2. std::move 其實不移動任何東西】
//   std::move 只是一個 static_cast，把左值轉成右值參考（rvalue reference），
//   讓多載決議去選中 move constructor 而已。它本身不產生任何執行期程式碼。
//   名字取得很差，Scott Meyers 說它「應該叫 rvalue_cast」。
//   真正做搬移動作的是被選中的 move constructor / move assignment。
//
// 【3. 移動後的來源是什麼狀態？——標準只保證「有效但未指定」】
//   ★ 這是本檔最重要、也最常被講錯的觀念。
//   標準（[lib.types.movedfrom]）只保證移動後的物件處於
//   「valid but unspecified state」——
//     * 有效（valid）：物件仍然完好，可以安全解構，
//       也可以呼叫「不帶前置條件」的操作，例如 clear()、size()、
//       operator=（重新賦值）。
//     * 未指定（unspecified）：具體的值是什麼，標準不保證。
//   所以【不可以】說「移動後 source 一定變成 size 0」。
//   標準沒有這樣要求，那只是實作行為。
//
//   ▸ 本機實作觀察（libstdc++ / g++ 15.2）：移動後 source.size() 與
//     source.capacity() 都是 0——因為它把三根指標都設成 nullptr。
//     這在 libc++、MSVC 上實測也相同。但這是【實作定義】的觀察，
//     不是可以寫進 assert 的保證。
//
//   ▸ 實務守則：移動之後，除非你先重新賦值（source = {...}）
//     或呼叫 clear()，否則就當作「這個變數已經沒有意義」，不要再讀它的內容。
//
// 【4. 為什麼移動建構必須是 noexcept？】
//   因為 vector 自己成長時（見第 11 課）要決定「搬舊元素時用 move 還是 copy」。
//   若元素的 move constructor 可能丟例外，那搬到一半失敗就無法回復原狀
//   （舊元素已被破壞、新空間又不完整），無法提供強例外保證。
//   所以 vector 用 std::move_if_noexcept：
//     * 元素的 move 是 noexcept → 用 move（快）
//     * 否則 → 退回 copy（慢，但失敗時舊資料還在，可以安全回復）
//   結論：自己寫 class 時，move constructor 一定要標 noexcept，
//   否則放進 vector 會在成長時默默退化成複製，效能差很多。
//
// 【概念補充 Concept Deep Dive】
//   ▸ 回傳 vector 需要 std::move 嗎？→ 不需要，而且有害。
//       std::vector<int> f() {
//           std::vector<int> v(1000);
//           return v;              // ✅ 正確：NRVO 直接在呼叫端建構，零成本
//           // return std::move(v); // ❌ 反而阻止 NRVO，多一次移動
//       }
//     C++17 起對「回傳純右值」更是保證複製消除（guaranteed copy elision）。
//     對具名區域變數（NRVO）仍是編譯器最佳化而非保證，但加 std::move
//     只會關掉這個最佳化——這是靜態分析工具會警告的經典反模式。
//
//   ▸ 移動 vs 複製的效能差距：
//     複製 100 萬個 int 要配置 4 MB 並複製 4 MB；移動只是三次指標賦值。
//     這就是為什麼「把大容器塞進另一個容器」時 push_back(std::move(v))
//     幾乎是必寫的。
//
// 【注意事項 Pay Attention】
//   1. ★ 移動後來源是「有效但未指定」，不是「保證為空」。
//      本機 libstdc++ 實測是 size()==0，但那是【實作定義】，別寫進斷言。
//   2. ★ 對 const 物件用 std::move 會「悄悄退化成複製」——
//      const vector&& 無法繫結到 vector&&，只能繫結到 const vector&，
//      於是選中 copy constructor。能編譯、能跑、沒有警告，只是慢。
//      本檔用建構子計次實測證明了這件事（見輸出 [COPY] 標記）。
//   3. 移動後想繼續用該變數，請先重新賦值或 clear()，讓它回到已知狀態。
//   4. 不要對即將回傳的區域變數加 std::move——會阻止 NRVO，弄巧成拙。
//   5. 移動不是「免費」的同義詞：它是 O(1)，但仍有三次指標賦值與
//      一次原物件的重設，在極熱迴圈中仍可能可見（不過幾乎不會是瓶頸）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】移動建構
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. auto b = std::move(a); 之後，a 的內容是什麼？
//     答：標準只保證 a 處於「有效但未指定（valid but unspecified）」狀態——
//         可以安全解構、可以重新賦值，但具體的值標準不保證。
//         本機 libstdc++ 實測 a.size() 與 a.capacity() 都是 0（【實作定義】），
//         libc++/MSVC 實測亦同，但這不能當成可依賴的保證寫進程式。
//     追問：那移動後還能用 a 嗎？
//         → 能，但必須先把它帶回已知狀態（a = {...} 或 a.clear()），
//         否則讀取它的內容是「未指定值」，屬於邏輯錯誤。
//
// 🔥 Q2. 移動一個有 100 萬個元素的 vector，成本是多少？元素的 move ctor 被呼叫幾次？
//     答：O(1)，且元素的 move constructor 一次都不會被呼叫。
//         移動建構只是把來源的三根指標搬過來、再把來源設為 nullptr，
//         heap 上的資料完全沒動。這也是本檔實測中「移動 vector<C> 卻沒印出
//         任何 [MOVE] 標記」的原因。
//     追問：那為什麼 vector 成長（reallocate）時反而會逐一移動元素？
//         → 那是換一塊新記憶體，舊元素必須搬過去，和「移動整個容器」是兩回事。
//
// ⚠️ 陷阱. const std::vector<int> src = {...}; auto dst = std::move(src);
//        這樣有移動到嗎？
//     答：沒有，它悄悄變成了「複製」。std::move(src) 產生的是
//         const vector<int>&&，它無法繫結到需要 vector<int>&& 的移動建構子，
//         於是多載決議退而選中 copy constructor（const vector&）。
//         能編譯、能執行、零警告——只是效能完全沒有改善。
//     為什麼會錯：以為「寫了 std::move 就一定會移動」。
//         std::move 只是型別轉換，真正決定用 move 還是 copy 的是多載決議；
//         const 讓移動建構子失去資格，複製建構子就補上了。
//         推論：想讓物件可被移動，就不要把它宣告成 const。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <utility>  // std::move
#include <vector>

// -----------------------------------------------------------------------------
// 追蹤用的元素型別：印出自己被複製還是被移動
// -----------------------------------------------------------------------------
struct Tracked {
    std::string tag;

    explicit Tracked(std::string t) : tag(std::move(t)) {}
    Tracked(const Tracked& o) : tag(o.tag) {
        std::cout << "    [COPY] " << tag << "\n";
    }
    Tracked(Tracked&& o) noexcept : tag(std::move(o.tag)) {
        std::cout << "    [MOVE]\n";
    }
    Tracked& operator=(const Tracked&) = default;
    Tracked& operator=(Tracked&&) noexcept = default;
};

// -----------------------------------------------------------------------------
// 【日常實務範例】批次處理管線：把讀進來的資料交棒給下一階段
//   情境：日誌處理服務先把一批 log 行讀進記憶體，交給分析階段，
//         再把結果交給輸出階段。每個階段結束後，該階段的資料就不再需要。
//   為什麼用 move：這批資料可能有數十萬行、佔數十 MB。
//     若每次交棒都複製，記憶體用量與耗時都會翻倍；
//     用 std::move 交棒是 O(1)，且來源不再需要，正好符合「移動」的語意。
//   ★ 注意：交棒之後就不再讀取來源變數——這正是移動的正確用法。
// -----------------------------------------------------------------------------
class LogBatch {
public:
    // 以 by-value + move 接收：呼叫端可以選擇複製或移動，成本由呼叫端決定
    explicit LogBatch(std::vector<std::string> lines) : lines_(std::move(lines)) {}

    // 把內部資料「交棒」出去，之後本物件視為已清空
    std::vector<std::string> release() {
        return std::move(lines_);   // 移動出去，O(1)
    }

    std::size_t size() const { return lines_.size(); }

private:
    std::vector<std::string> lines_;
};

// 分析階段：統計含有 ERROR 的行數
int countErrors(const std::vector<std::string>& lines) {
    int n = 0;
    for (const std::string& line : lines) {
        if (line.find("ERROR") != std::string::npos) ++n;
    }
    return n;
}

int main() {
    std::cout << "=== 移動建構：來源被掏空（本機實作觀察）===\n";
    std::vector<int> source = {1, 2, 3, 4, 5};
    std::cout << "  移動前 source size=" << source.size()
              << " capacity=" << source.capacity() << "\n";

    std::vector<int> dest = std::move(source);      // 移動建構，O(1)

    std::cout << "  移動後 dest   size=" << dest.size() << " 內容: ";
    for (int x : dest) std::cout << x << " ";
    std::cout << "\n";
    std::cout << "  移動後 source size=" << source.size()
              << " capacity=" << source.capacity() << "\n";
    std::cout << "  ★ 標準只保證 source 是「有效但未指定」；\n"
              << "    上面看到的 0 是本機 libstdc++ 的【實作定義】行為，\n"
              << "    不可寫成 assert(source.size() == 0) 這種依賴。\n";

    std::cout << "\n=== 移動後可以重新賦值，讓它回到已知狀態 ===\n";
    source = {10, 20};                              // 重新賦值後就完全可用了
    std::cout << "  source = {10, 20} 之後 size=" << source.size() << " 內容: ";
    for (int x : source) std::cout << x << " ";
    std::cout << "\n";

    std::cout << "\n=== 移動整個 vector 時，元素的 move ctor 一次都不會被呼叫 ===\n";
    std::vector<Tracked> a;
    a.reserve(3);
    a.emplace_back("A");
    a.emplace_back("B");
    a.emplace_back("C");
    std::cout << "  準備移動一個含 3 個元素的 vector...\n";
    std::vector<Tracked> b = std::move(a);
    std::cout << "  移動完成，b.size()=" << b.size()
              << "（上方沒有任何 [MOVE]／[COPY] 輸出）\n";
    std::cout << "  → 因為搬的是三根指標，不是元素本身。\n";

    std::cout << "\n=== ⚠️ 陷阱：對 const 物件 std::move 會悄悄變成複製 ===\n";
    const std::vector<Tracked> constSrc = [] {
        std::vector<Tracked> t;
        t.reserve(2);
        t.emplace_back("X");
        t.emplace_back("Y");
        return t;
    }();
    std::cout << "  對 const vector 執行 std::move...\n";
    std::vector<Tracked> copied = std::move(constSrc);   // 實際上是複製！
    std::cout << "  結果 size=" << copied.size()
              << "，而且來源 constSrc 仍有 " << constSrc.size() << " 個元素\n";
    std::cout << "  → 上方印出的 [COPY] 證明它走的是複製建構子。\n"
              << "    const vector&& 無法繫結到 move ctor，只能退回 copy ctor。\n";

    std::cout << "\n=== 日常實務：批次資料交棒（避免整批複製）===\n";
    std::vector<std::string> rawLines = {
        "2026-07-19 10:00:01 INFO  服務啟動",
        "2026-07-19 10:00:05 ERROR 資料庫連線逾時",
        "2026-07-19 10:00:07 WARN  重試第 1 次",
        "2026-07-19 10:00:09 ERROR 資料庫連線逾時",
        "2026-07-19 10:00:12 INFO  連線恢復"
    };
    std::cout << "  讀入 " << rawLines.size() << " 行原始 log\n";

    LogBatch batch(std::move(rawLines));            // ① 交棒給 LogBatch，O(1)
    std::cout << "  交棒給 LogBatch 後，batch 持有 " << batch.size() << " 行\n";

    std::vector<std::string> forAnalysis = batch.release();  // ② 再交棒給分析階段
    std::cout << "  交棒給分析階段後，batch 剩下 " << batch.size() << " 行\n";
    std::cout << "  分析結果：發現 " << countErrors(forAnalysis) << " 筆 ERROR\n";
    std::cout << "  → 全程只有指標搬移，沒有複製任何一行字串。\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 10 課：vector 的宣告與初始化方式6.cpp" -o lesson10_6

// === 預期輸出 ===
// === 移動建構：來源被掏空（本機實作觀察）===
//   移動前 source size=5 capacity=5
//   移動後 dest   size=5 內容: 1 2 3 4 5
//   移動後 source size=0 capacity=0
//   ★ 標準只保證 source 是「有效但未指定」；
//     上面看到的 0 是本機 libstdc++ 的【實作定義】行為，
//     不可寫成 assert(source.size() == 0) 這種依賴。
//
// === 移動後可以重新賦值，讓它回到已知狀態 ===
//   source = {10, 20} 之後 size=2 內容: 10 20
//
// === 移動整個 vector 時，元素的 move ctor 一次都不會被呼叫 ===
//   準備移動一個含 3 個元素的 vector...
//   移動完成，b.size()=3（上方沒有任何 [MOVE]／[COPY] 輸出）
//   → 因為搬的是三根指標，不是元素本身。
//
// === ⚠️ 陷阱：對 const 物件 std::move 會悄悄變成複製 ===
//   對 const vector 執行 std::move...
//     [COPY] X
//     [COPY] Y
//   結果 size=2，而且來源 constSrc 仍有 2 個元素
//   → 上方印出的 [COPY] 證明它走的是複製建構子。
//     const vector&& 無法繫結到 move ctor，只能退回 copy ctor。
//
// === 日常實務：批次資料交棒（避免整批複製）===
//   讀入 5 行原始 log
//   交棒給 LogBatch 後，batch 持有 5 行
//   交棒給分析階段後，batch 剩下 0 行
//   分析結果：發現 2 筆 ERROR
//   → 全程只有指標搬移，沒有複製任何一行字串。
