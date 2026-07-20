// =============================================================================
//  第 15 課：vector 元素刪除 7  —  觀察 erase 時的建構／移動／銷毀
// =============================================================================
//
// 【主題資訊 Information】
//   本檔用一個會印出每次特殊成員函式呼叫的型別（Item），
//   把 erase 與 clear 內部真正發生的事情攤開來看。
//   涉及的成員（皆 <vector>）：
//     void     reserve(size_type);        預先配置，避免擴容干擾觀察
//     T&       emplace_back(Args&&...);   C++11，就地建構
//     iterator erase(const_iterator);     刪除，後方元素往前搬
//     void     clear() noexcept;          全部解構，capacity 不變
//   關鍵事實：erase 中間元素時，【不是】把被刪的那個元素解構後留洞，
//     而是「後面的元素依序移動賦值往前補，最後解構掉尾巴那一格」。
//
// 【詳細解釋 Explanation】
//
// 【1. erase(begin()+1) 的實際執行順序】
//   容器內容是 [A][B][C][D]，要刪掉索引 1 的 B。
//   直覺會以為是「先解構 B，再把 C、D 搬過來」。實際順序是：
//       ① B = std::move(C)   ← 移動賦值。B 原本的資源在這一步被覆蓋
//       ② C = std::move(D)   ← 移動賦值
//       ③ 解構最後一格（此時它是「已被移走的 D」，內容為空）
//       ④ size 減一
//   注意兩件事：
//     * B 的解構子【沒有】被單獨呼叫。它是被「移動賦值」覆蓋掉的，
//       資源的釋放發生在 operator=(T&&) 內部。
//     * 最後被解構的那一格，其內容是「已被移走的 D」——
//       對 std::string 這類型別，它的 name 已經是空字串。
//   本檔的輸出會清楚顯示這個順序。
//
// 【2. 為什麼要用「移動賦值」而不是「解構 + 移動建構」】
//   目標位置上已經有一個活著的物件（B），所以要用【賦值】而不是建構。
//   若改成「先解構 B、再在原地移動建構」，中間會出現一段
//   「該位置沒有有效物件」的空窗；一旦移動建構拋出例外，
//   容器就處於無法描述的狀態。用賦值則自始至終每一格都有有效物件。
//   代價是要求元素【可移動賦值】（或至少可複製賦值）。
//
// 【3. reserve(5) 的作用：排除擴容的干擾】
//   本檔一開始就 reserve(5)，這不是最佳化，是【實驗控制】。
//   若不 reserve，四次 emplace_back 會觸發多次擴容，
//   每次擴容都要把既有元素移動到新記憶體，輸出裡就會混入
//   大量與 erase 無關的「移動」訊息，看不出重點。
//   寫這類觀察型程式時，先固定容量是必要的手法。
//
// 【4. clear() 的輸出順序】
//   libstdc++ 的 clear 是從前往後逐一解構（_Destroy(begin, end)）。
//   所以你會看到 A、C、D 依序被銷毀。
//   標準【沒有規定】解構順序，只規定「所有元素都被解構」，
//   所以這個由前往後的順序是實作定義，不該寫進正式的邏輯依賴。
//   （對比：陣列與區域變數的解構順序是標準規定的「反向」。）
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼移動建構子要標 noexcept
//     本檔的 Item 把移動建構子標成 noexcept。這不只是好習慣：
//     vector 擴容時要決定「搬移舊元素用移動還是複製」，
//     判斷依據正是 std::move_if_noexcept——
//     移動建構子若不是 noexcept，vector 會退回用【複製】，
//     以維持強例外保證（搬到一半失敗時原資料仍完好）。
//     所以忘了標 noexcept，效能會默默掉一大截。
//
// (B) 「已被移走」的物件處於什麼狀態
//     標準只保證它處於「有效但未指定（valid but unspecified）」狀態：
//     你可以安全地解構它、對它賦新值，但不該假設它的內容。
//     實務上 libstdc++ 的 std::string 被移走後會變成空字串，
//     所以本檔的輸出會看到 `銷毀 `（名字是空的）。
//     這是實作行為，不是標準保證——不要寫依賴「移走後一定是空」的程式碼。
//
// (C) 觀察型別的一個陷阱：拷貝建構子不能忘
//     本檔的 Item 同時定義了複製建構、移動建構、移動賦值。
//     若只定義移動而不定義複製，這個型別就變成 move-only，
//     很多 vector 操作會編譯失敗或行為不同，觀察到的就不是一般情況了。
//     另外要注意：一旦自訂了任何一個特殊成員函式，
//     其餘的生成規則就會改變（rule of five）。
//
// 【注意事項 Pay Attention】
//   1. erase 中間元素時，被刪元素的解構子【不會】被單獨呼叫——
//      它是被移動賦值覆蓋掉的。真正被解構的是尾端那一格。
//   2. 最後被解構的那一格內容是「已被移走的物件」，
//      其狀態是有效但未指定（libstdc++ 的 string 會是空字串，但別依賴）。
//   3. clear() 的解構順序（本機是由前往後）是實作定義，標準未規定。
//   4. 移動建構子沒標 noexcept 的話，vector 擴容會改用複製。
//   5. 觀察這類行為前務必先 reserve，否則擴容的搬移會淹沒輸出。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】erase 內部的元素生命週期
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. vector 有 [A][B][C][D]，呼叫 erase(begin()+1) 刪掉 B。
//        B 的解構子會在什麼時候被呼叫？
//     答：不會被單獨呼叫。實際順序是
//         ① B = std::move(C)（移動賦值，B 原本的資源在賦值運算子內被釋放）
//         ② C = std::move(D)
//         ③ 解構【最後一格】（此時內容是已被移走的 D）
//         ④ size 減一
//         所以呼叫次數是「N 次移動賦值 + 1 次解構」，
//         而那唯一一次解構針對的是尾端，不是被刪的位置。
//     追問：為什麼用移動賦值而不是先解構再移動建構？
//         → 賦值讓每一格自始至終都有有效物件。
//           先解構會出現「該位置沒有有效物件」的空窗，
//           若接著的移動建構拋出例外，容器就無法回復。
//
// 🔥 Q2. 為什麼觀察這類行為之前一定要先 reserve？
//     答：因為不 reserve 的話，push_back/emplace_back 會觸發擴容，
//         每次擴容都要把所有既有元素移動（或複製）到新記憶體，
//         輸出裡會混入大量與 erase 無關的搬移訊息。
//         先 reserve 固定容量，是這類實驗必要的控制手段。
//     追問：擴容時是用移動還是複製？
//         → 看移動建構子有沒有標 noexcept。標了就用移動；
//           沒標的話 vector 會用 std::move_if_noexcept 選擇【複製】，
//           以維持強例外保證。
//
// ⚠️ 陷阱. 「輸出顯示最後被銷毀的那個物件名字是空的，
//         代表 vector 在解構前把它清空了」——這個解讀錯在哪？
//     答：不是 vector 清空它的。那一格的內容是「已被移動走的 D」——
//         在步驟 ② `C = std::move(D)` 時，D 的字串資源被搬給了 C，
//         D 自己就進入「有效但未指定」的狀態。
//         libstdc++ 的 std::string 被移走後【剛好】是空字串，
//         所以印出來是空的。
//         這是實作行為，不是標準保證：標準只說移走後的物件
//         「可以安全解構或重新賦值」，沒有規定它的內容。
//     為什麼會錯：把一次觀察到的實作行為當成語言規則。
//         寫程式時若依賴「移走後一定是空」，換一個標準函式庫實作
//         （或換一個自訂型別）就會壞掉。
//         正確的心態是：移走後的物件，除了解構與重新賦值，什麼都別做。
// ═══════════════════════════════════════════════════════════════════════════

#include <vector>
#include <iostream>
#include <string>

struct Item {
    std::string name;

    Item(const std::string& n) : name(n) {
        std::cout << "建構 " << name << std::endl;
    }

    Item(const Item& other) : name(other.name) {
        std::cout << "複製 " << name << std::endl;
    }

    Item(Item&& other) noexcept : name(std::move(other.name)) {
        std::cout << "移動 " << name << std::endl;
    }

    Item& operator=(Item&& other) noexcept {
        name = std::move(other.name);
        std::cout << "移動賦值 " << name << std::endl;
        return *this;
    }

    ~Item() {
        std::cout << "銷毀 " << (name.empty() ? "(已被移走，內容為空)" : name) << std::endl;
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】為什麼「元素持有資源」時要在意這些細節
//   情境：一個連線池，每個元素持有一個真實資源（這裡用檔案描述子編號模擬）。
//         資源必須在物件解構時歸還，不能重複歸還、也不能漏掉。
//   為什麼用到本主題：只有搞清楚 erase 內部是「移動賦值 + 解構尾端」，
//     才能正確寫出這類型別的移動賦值運算子——
//     它必須先歸還自己原有的資源，再接手來源的資源，
//     並把來源標記為「已無資源」，否則就會出現
//     重複歸還（double free）或資源洩漏。
//   下方 Resource 的 operator=(Resource&&) 就是正確的三步驟寫法。
// -----------------------------------------------------------------------------
struct Resource {
    int  handle;                 // -1 表示「沒有持有資源」
    static int liveCount;        // 目前實際持有資源的物件數

    explicit Resource(int h) : handle(h) { ++liveCount; }

    Resource(Resource&& o) noexcept : handle(o.handle) {
        o.handle = -1;           // 來源交出資源
    }

    Resource& operator=(Resource&& o) noexcept {
        if (this != &o) {
            if (handle != -1) --liveCount;   // ① 先歸還自己原有的
            handle = o.handle;               // ② 接手來源的
            o.handle = -1;                   // ③ 來源標記為已無資源
        }
        return *this;
    }

    Resource(const Resource&) = delete;              // 資源不可複製
    Resource& operator=(const Resource&) = delete;

    ~Resource() {
        if (handle != -1) --liveCount;       // 只有真的持有時才歸還
    }
};
int Resource::liveCount = 0;

int main() {
    std::vector<Item> v;
    v.reserve(5);        // 先固定容量：排除擴容搬移對觀察的干擾

    std::cout << "=== 建立四個元素（已 reserve，不會擴容）===" << std::endl;
    v.emplace_back("A");
    v.emplace_back("B");
    v.emplace_back("C");
    v.emplace_back("D");

    std::cout << "\n=== erase B ===" << std::endl;
    v.erase(v.begin() + 1);
    // 觀察：C 和 D 會往前移動賦值，然後最後一個位置的元素被銷毀

    std::cout << "\n--- 上面發生的事情，逐步拆解 ---" << std::endl;
    std::cout << "① B = std::move(C)  ← B 原本的資源在賦值運算子內被覆蓋" << std::endl;
    std::cout << "② C = std::move(D)" << std::endl;
    std::cout << "③ 解構最後一格（內容是「已被移走的 D」，所以名字是空的）" << std::endl;
    std::cout << "④ size 減一" << std::endl;
    std::cout << "注意：B 的解構子【沒有】被單獨呼叫——它是被賦值覆蓋掉的。" << std::endl;

    std::cout << "\n=== 目前內容 ===" << std::endl;
    for (const auto& item : v) {
        std::cout << item.name << " ";
    }
    std::cout << std::endl;
    std::cout << "size=" << v.size() << " capacity=" << v.capacity()
              << "（capacity 不變）" << std::endl;

    std::cout << "\n=== clear ===" << std::endl;
    v.clear();
    std::cout << "（本機 libstdc++ 是由前往後逐一解構；" << std::endl;
    std::cout << "  標準只規定「全部解構」，沒有規定順序）" << std::endl;
    std::cout << "clear 後 size=" << v.size() << " capacity=" << v.capacity() << std::endl;

    std::cout << "\n=== 日常實務：持有資源的元素被 erase 時 ===" << std::endl;
    {
        std::vector<Resource> pool;
        pool.reserve(4);
        pool.emplace_back(100);
        pool.emplace_back(101);
        pool.emplace_back(102);
        pool.emplace_back(103);
        std::cout << "建立 4 個資源，目前持有數: " << Resource::liveCount << std::endl;

        pool.erase(pool.begin() + 1);       // 刪掉 handle=101
        std::cout << "erase 一個之後，持有數: " << Resource::liveCount
                  << "（正確：只少一個，沒有重複歸還也沒有洩漏）" << std::endl;
        std::cout << "剩下的 handle: ";
        for (const auto& r : pool) std::cout << r.handle << " ";
        std::cout << std::endl;

        pool.clear();
        std::cout << "clear 之後，持有數: " << Resource::liveCount << std::endl;
        std::cout << "→ 關鍵在 operator=(Resource&&) 的三步驟：" << std::endl;
        std::cout << "  ① 先歸還自己原有的資源 ② 接手來源的 ③ 把來源標記為已無資源" << std::endl;
        std::cout << "  少了 ① 會洩漏，少了 ③ 會在來源解構時重複歸還。" << std::endl;
    }

    std::cout << "\n=== 程式結束（v 已空，沒有元素需要解構）===" << std::endl;
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第 15 課：vector 元素刪除7.cpp -o demo7
//
// 可以看到 erase 的運作方式：
//   1. C 和 D 往前移動賦值
//   2. 最後一個位置（已被移走的 D）被銷毀（name 已空）

// === 預期輸出 ===
// === 建立四個元素（已 reserve，不會擴容）===
// 建構 A
// 建構 B
// 建構 C
// 建構 D
//
// === erase B ===
// 移動賦值 C
// 移動賦值 D
// 銷毀 (已被移走，內容為空)
//
// --- 上面發生的事情，逐步拆解 ---
// ① B = std::move(C)  ← B 原本的資源在賦值運算子內被覆蓋
// ② C = std::move(D)
// ③ 解構最後一格（內容是「已被移走的 D」，所以名字是空的）
// ④ size 減一
// 注意：B 的解構子【沒有】被單獨呼叫——它是被賦值覆蓋掉的。
//
// === 目前內容 ===
// A C D
// size=3 capacity=5（capacity 不變）
//
// === clear ===
// 銷毀 A
// 銷毀 C
// 銷毀 D
// （本機 libstdc++ 是由前往後逐一解構；
//   標準只規定「全部解構」，沒有規定順序）
// clear 後 size=0 capacity=5
//
// === 日常實務：持有資源的元素被 erase 時 ===
// 建立 4 個資源，目前持有數: 4
// erase 一個之後，持有數: 3（正確：只少一個，沒有重複歸還也沒有洩漏）
// 剩下的 handle: 100 102 103
// clear 之後，持有數: 0
// → 關鍵在 operator=(Resource&&) 的三步驟：
//   ① 先歸還自己原有的資源 ② 接手來源的 ③ 把來源標記為已無資源
//   少了 ① 會洩漏，少了 ③ 會在來源解構時重複歸還。
//
// === 程式結束（v 已空，沒有元素需要解構）===
