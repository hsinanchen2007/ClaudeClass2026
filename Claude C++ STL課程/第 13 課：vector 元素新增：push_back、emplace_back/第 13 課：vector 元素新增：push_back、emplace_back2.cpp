// =============================================================================
//  第 13 課：vector 元素新增：push_back、emplace_back2.cpp
//    —  用會說話的 Tracker 類別，親眼觀察 push_back 何時複製、何時移動
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<vector>
//   void push_back(const T& value);   // lvalue → 呼叫 T 的複製建構子
//   void push_back(T&& value);        // rvalue → 呼叫 T 的移動建構子
//   複雜度：攤銷 O(1)
//
//   本檔用 reserve(5) 先把 capacity 撐開，讓輸出**不含**擴容時的搬移動作，
//   才能乾淨地只觀察「push_back 這一次做了什麼」。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼一定要先 reserve 才能觀察】
// 若不先 reserve，第 2、3 次 push_back 會觸發 reallocation，
// 輸出裡會混進「把既有元素搬到新記憶體」的移動建構訊息，
// 讓人分不清哪一行是 push_back 本身、哪一行是擴容的副作用。
// reserve(5) 保證這三次 push_back 都不會擴容，輸出乾淨。
// 這也是所有「觀察建構行為」的教學程式碼都會先 reserve 的原因。
//
// 【2. 三種呼叫各自的完整動作序列】
//   ① Tracker t1(1); v.push_back(t1);
//        建構 t1  → 複製建構到 vector 內（t1 依然完好）
//        共 2 個物件存在：t1 與 vector 裡那份。
//   ② v.push_back(Tracker(2));
//        建構臨時 Tracker(2) → 移動建構到 vector → **臨時物件立刻解構**
//        注意輸出裡會看到一次「銷毀 Tracker(2)」，那是臨時物件死掉，
//        不是 vector 裡的元素死掉。這是最容易看混的地方。
//   ③ Tracker t3(3); v.push_back(move(t3));
//        建構 t3 → 移動建構到 vector（t3 變成 valid but unspecified）
//        t3 沒有立刻解構，它活到 main 結束。
//
// 【3. 解構順序為什麼看起來很亂】
// 程式結束時的銷毀順序有兩批，而且是交錯的：
//   * 區域變數 t3、t1 依「宣告的相反順序」解構（後宣告的先死）。
//   * vector v 解構時，把裡面 3 個元素依序解構。
// 兩批的先後取決於它們在 main 裡的宣告順序：v 最早宣告 → 最晚解構。
// 所以會先看到 t3、t1 死掉，最後才是 vector 裡的 1、2、3。
// 這裡的 id 會重複出現（例如兩個 Tracker(3)），因為被 move 走的 t3
// 只是資源被偷走，id 這個 int 成員仍然保有原值——移動建構子並沒有
// 把 other.id 清成 0。
//
// 【概念補充 Concept Deep Dive】
// 這個 Tracker 的移動建構子其實**沒有真的移動任何東西**：
//     Tracker(Tracker&& other) noexcept : id(other.id) {}
// 成員只有一個 int，複製與移動的成本完全一樣。
// 它的價值純粹是「印出一行字」，讓你知道編譯器選了哪個重載。
// 真實世界裡值得移動的是持有 heap 指標的型別（string、vector、unique_ptr），
// 移動只是搬指標，複製則要重新配置並複製整塊資料。
//
// 另外注意 move constructor 上的 noexcept——這不是裝飾。
// vector 擴容時用 std::move_if_noexcept 決定策略：move 若非 noexcept，
// 為了維持強例外保證會**退化成 copy**。拿掉這個 noexcept，
// 擴容時的輸出就會從「移動建構」變成「複製建構」。
//
// 【注意事項 Pay Attention】
// 1. 「銷毀 Tracker(2)」出現在 push_back 之後，是臨時物件的死亡，
//    不代表 vector 裡的元素被銷毀。
// 2. 被 move 走的 t3 之後不應再讀其值；本例的 id 湊巧仍是 3，
//    那是因為移動建構子沒清空來源，不是語言保證。
// 3. 沒有 reserve 的話輸出會混入擴容搬移，無法乾淨觀察。
// 4. move constructor 的 noexcept 不能省，否則擴容會退化成複製。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】觀察 push_back 的複製與移動
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. v.push_back(Tracker(2)) 總共產生幾次建構、幾次解構？
//     答：一次普通建構（臨時物件）、一次移動建構（進 vector）、
//         一次解構（臨時物件在完整運算式結束時死亡）。
//         所以輸出是「建構 → 移動建構 → 銷毀」三行。
//     追問：能不能省掉那次移動？
//         → 可以，改用 v.emplace_back(2)，直接在 vector 的記憶體上建構，
//           全程只有一次建構，沒有臨時物件也沒有解構。
//
// 🔥 Q2. 為什麼觀察建構行為的範例都要先呼叫 reserve？
//     答：不 reserve 的話，push_back 會在 capacity 用盡時觸發 reallocation，
//         把既有元素全部搬到新記憶體，輸出裡就會混入額外的移動建構與解構，
//         使人無法分辨哪些是 push_back 本身造成的。
//     追問：擴容時搬移用的是移動還是複製？
//         → 由 std::move_if_noexcept 決定：move constructor 是 noexcept 就用
//           move，否則退化成 copy 以維持強例外保證。
//
// ⚠️ 陷阱 Q3. push_back(t1) 之後，t1 還能不能繼續使用？
//     答：能，而且完全正常。傳的是 lvalue，走的是 const T& 重載，
//         做的是**複製**，t1 絲毫未動。
//         只有 push_back(std::move(t1)) 才會把 t1 的資源搬走。
//     為什麼會錯：很多人看到「元素進了 vector」就直覺認為原物件被「轉移」了。
//         實際上 C++ 預設語意是複製，要移動必須明確寫 std::move
//         或傳入本來就是 rvalue 的臨時物件。
// ═══════════════════════════════════════════════════════════════════════════

#include <vector>
#include <iostream>

class Tracker {
public:
    int id;

    Tracker(int i) : id(i) {
        std::cout << "建構 Tracker(" << id << ")" << std::endl;
    }

    Tracker(const Tracker& other) : id(other.id) {
        std::cout << "複製建構 Tracker(" << id << ")" << std::endl;
    }

    // noexcept 不是裝飾：擴容時 move_if_noexcept 會依此決定用 move 還是 copy
    Tracker(Tracker&& other) noexcept : id(other.id) {
        std::cout << "移動建構 Tracker(" << id << ")" << std::endl;
    }

    ~Tracker() {
        std::cout << "銷毀 Tracker(" << id << ")" << std::endl;
    }
};

int main() {
    std::vector<Tracker> v;
    v.reserve(5);  // 預留空間，避免擴容搬移干擾觀察

    std::cout << "=== 從左值 push_back ===" << std::endl;
    Tracker t1(1);
    v.push_back(t1);  // lvalue → const T& 重載 → 複製；t1 仍然完好

    std::cout << "\n=== 從右值 push_back ===" << std::endl;
    v.push_back(Tracker(2));  // 建構臨時物件 → 移動進 vector → 臨時物件解構

    std::cout << "\n=== 用 std::move push_back ===" << std::endl;
    Tracker t3(3);
    v.push_back(std::move(t3));  // rvalue → T&& 重載 → 移動；t3 之後不應再讀值

    std::cout << "\n=== 程式結束（以下是解構）===" << std::endl;
    // 解構順序：區域變數 t3、t1 先（宣告的相反順序），
    // 最後才是最早宣告的 v，把裡面 3 個元素依序解構。
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 13 課：vector 元素新增：push_back、emplace_back2.cpp" -o observe_copy_move

// === 預期輸出 ===
// === 從左值 push_back ===
// 建構 Tracker(1)
// 複製建構 Tracker(1)
// 
// === 從右值 push_back ===
// 建構 Tracker(2)
// 移動建構 Tracker(2)
// 銷毀 Tracker(2)
// 
// === 用 std::move push_back ===
// 建構 Tracker(3)
// 移動建構 Tracker(3)
// 
// === 程式結束（以下是解構）===
// 銷毀 Tracker(3)
// 銷毀 Tracker(1)
// 銷毀 Tracker(1)
// 銷毀 Tracker(2)
// 銷毀 Tracker(3)
