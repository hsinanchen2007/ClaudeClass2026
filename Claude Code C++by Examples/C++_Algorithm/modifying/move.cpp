// ============================================================
// std::move / std::move_backward   (C++11 起)
// 分類 (Category): Modifying sequence operations (修改型)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/move
//   * https://en.cppreference.com/w/cpp/algorithm/move_backward
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// 「移動 (move)」是 C++11 引入的核心概念。它讓你可以「轉移」
// 一個物件的內部資源 (例如指標、檔案 handle) 到另一個物件,
// 而不必做昂貴的拷貝。
//
// std::move 在 C++ 中其實有「兩個面孔」,容易混淆:
//
//   ┌──────────────────────────┬─────────────────────────────┐
//   │ <utility> 的 std::move    │ 把 lvalue 轉成 xvalue (型別轉換)│
//   │ <algorithm> 的 std::move  │ 把一段範圍的元素「逐一移動」到目的端 │
//   └──────────────────────────┴─────────────────────────────┘
//
// 本檔案介紹的是 <algorithm> 的版本 — 對「整段範圍」做移動的演算法,
// 跟 std::copy 是孿生兄弟,只是把每個元素的「複製」改成「移動」。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、move 與 copy 的差別 (與何時要用 move)                   │
// └────────────────────────────────────────────────────────────┘
//
//   * std::copy : 對每個元素呼叫 copy assignment (拷貝)
//   * std::move : 對每個元素呼叫 move assignment (移動)
//
// 對「基本型別」(int、float、char ...),move 與 copy 完全等價 —
// 因為基本型別沒有「需要轉移的資源」。
//
// 真正受益於 move 的場景:
//   * std::string、std::vector — 內部有指向 heap 的指標,
//     copy 要重新配置記憶體;move 只搬指標即可。
//   * std::unique_ptr           — 不可拷貝,只能移動。
//   * 自訂類別有 move constructor / move assignment 時。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、移動後的來源元素是什麼狀態?                           │
// └────────────────────────────────────────────────────────────┘
//
// 標準說移動後的物件處於「valid but unspecified」狀態 —
// 它仍然可以解構、可以指派新值、可以呼叫不依賴具體內容的成員函式
// (像是 size()、empty(),通常會是「empty」或 0)。
// 但你「不應該」假設它的內容,例如不應該再讀字串內容。
//
// 實務上對 std::string、std::vector 而言,move 後通常變空字串/空容器。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、move_backward 為什麼存在?                             │
// └────────────────────────────────────────────────────────────┘
//
// 與 copy_backward 同理 — 當來源與目的「在同一個容器內重疊」且
// 目的「在來源後方」時,要用 move_backward 從尾巴往前寫,
// 才不會誤覆蓋還沒處理的元素。「前移用 move,後移用 move_backward」。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class InputIt, class OutputIt>
//   OutputIt move(InputIt first, InputIt last, OutputIt d_first);
//
//   template <class BidirIt1, class BidirIt2>
//   BidirIt2 move_backward(BidirIt1 first, BidirIt1 last, BidirIt2 d_last);
//
//   * C++20 起為 constexpr。
//   * C++17 起有執行策略多載。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、複雜度 (Complexity)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   時間: 恰好 N 次 move-assignment — O(n)
//   空間: O(1)
//
// ┌────────────────────────────────────────────────────────────┐
// │ 七、注意事項 (Pitfalls)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   1. 切勿混淆 <utility>::move (型別轉換) 與 <algorithm>::move (範圍移動)。
//   2. 移動後不要再讀來源元素的內容。
//   3. 對 unique_ptr 等不可拷貝型別,move 是「唯一」可行的批次轉移方式。
//   4. 對 trivially copyable 型別,move == copy,無效能差異。
//
// ============================================================

/*
補充筆記：std::move
  - std::move 演算法會把來源元素轉成右值逐一搬到目的地；來源元素仍有效但值未指定。
  - 目的地仍需有足夠空間，move 不會自動 push_back。
  - 移動後的容器通常只適合清空、重新賦值或銷毀，不應再讀取原內容當作有效資料。
  - std::move 會改變目標範圍內容或元素順序；呼叫前要確認輸出範圍空間足夠、iterator 有效。
  - copy/transform/generate/fill 寫入目的地時不會自動擴容，除非你使用 back_inserter 這類 insert iterator。
  - remove/unique 不會真的縮短容器，只把保留元素移到前面並回傳新邏輯結尾；還要搭配 erase 才會改變 size。
  - reverse/rotate/swap_ranges 會改變元素位置；若外部保存索引或 iterator，要重新確認是否仍指向預期元素。
  - move algorithm 會把元素搬到目的地，來源仍有效但值可能改變；後續只能重新指定或安全銷毀。
  - shuffle/sample 需要亂數引擎；不要每次呼叫都用同一個固定種子，除非你刻意要可重現測試結果。
*/
#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <utility>   // std::move (cast)
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    // --- 範例 1: 移動 string vector ---
    std::vector<std::string> src{"alpha", "beta", "gamma"};
    std::vector<std::string> dst(3);
    std::move(src.begin(), src.end(), dst.begin());
    std::cout << "dst: ";
    for (auto& s : dst) std::cout << s << ' ';
    std::cout << '\n';
    std::cout << "src after move (有效但未指定): ";
    for (auto& s : src) std::cout << "['" << s << "'] ";
    std::cout << '\n';

    // --- 範例 2: 移動 unique_ptr (不能拷貝!) ---
    std::vector<std::unique_ptr<int>> ups;
    ups.push_back(std::make_unique<int>(10));
    ups.push_back(std::make_unique<int>(20));
    std::vector<std::unique_ptr<int>> ups2(2);
    std::move(ups.begin(), ups.end(), ups2.begin());
    std::cout << "ups2[0]=" << *ups2[0] << ", ups2[1]=" << *ups2[1] << '\n';
    std::cout << "ups[0] is " << (ups[0] ? "still alive" : "null") << '\n';

    // --- 範例 3: move_backward 在同容器內向後位移 ---
    std::vector<std::string> v{"a", "b", "c", "d", "", ""};
    std::move_backward(v.begin(), v.begin() + 4, v.end());
    std::cout << "after move_backward: ";
    for (auto& s : v) std::cout << '[' << s << "] ";
    std::cout << '\n';

    // === 實務範例 ===
    void practical_move_big_strings();
    void practical_move_unique_ptrs();
    void leetcode_move_concept_array_shift();
    void practical_transfer_pending_jobs();
    practical_move_big_strings();
    practical_move_unique_ptrs();
    leetcode_move_concept_array_shift();
    practical_transfer_pending_jobs();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================
//
// 「批次移動」相比「批次拷貝」是效能優化,LeetCode 比較少強調這個面向。
// 因此這裡給兩個非常接近真實開發的實務範例。

// ----------------------------------------------------------------
// 實務範例 1:把大字串批次 move 進另一個容器,避免拷貝
// ----------------------------------------------------------------
// 場景:從外部讀進一份「大字串」(例如檔案內容、HTTP 回應),
//      要存進管理用的容器中。每個字串可能是幾 MB,拷貝成本太高。
//
// 為什麼用 std::move (範圍版):
//   一次性把整段元素移過去,每個元素只搬內部指標,O(1) per element。
//   原容器可重複使用 (size 還在,內容變空)。
void practical_move_big_strings() {
    std::vector<std::string> bigs;
    bigs.push_back(std::string(1000, 'A'));
    bigs.push_back(std::string(1000, 'B'));
    bigs.push_back(std::string(1000, 'C'));

    std::vector<std::string> dest(3);
    std::move(bigs.begin(), bigs.end(), dest.begin());
    std::cout << "Moved big strings, dest sizes: ";
    for (auto& s : dest) std::cout << s.size() << ' ';
    std::cout << "; src after move sizes: ";
    for (auto& s : bigs) std::cout << s.size() << ' ';
    std::cout << '\n';
}

// ----------------------------------------------------------------
// 實務範例 2:把 unique_ptr 群組所有權轉移
// ----------------------------------------------------------------
// 場景:工廠 (factory) 產生一批 Resource,要交給管理器 (manager) 接管。
//      Resource 用 unique_ptr 表示「獨佔擁有」,不可拷貝 — 只能 move。
//
// 為什麼用 std::move (範圍版):
//   * 不可拷貝的型別,範圍 copy 會編譯錯誤,只剩 move 一條路。
//   * 「擁有權轉移」的語意正好對應 move。
void practical_move_unique_ptrs() {
    std::vector<std::unique_ptr<int>> factory;
    for (int i = 1; i <= 4; ++i) factory.push_back(std::make_unique<int>(i * 11));

    std::vector<std::unique_ptr<int>> manager(factory.size());
    std::move(factory.begin(), factory.end(), manager.begin());

    std::cout << "Manager values: ";
    for (auto& p : manager) std::cout << *p << ' ';
    std::cout << "; factory[0] " << (factory[0] ? "alive" : "null") << '\n';
}

// ----------------------------------------------------------------
// LeetCode 概念題:移動 vector<string> 的前 K 個到後段 (陣列搬移)
// ----------------------------------------------------------------
// 題目:陣列保留尾端 K 個位置,前段資料 (含 string) 整段搬到尾端,前段清空。
//      類似 LC 88 的「就地搬移」概念,但這裡示範 std::move (move algorithm)。
//
// 為什麼用 std::move + std::move_backward:
//   * 若元素是「大字串」或 unique_ptr,std::copy 會做昂貴/不允許的拷貝。
//   * 用 std::move 將元素直接「搬」到新位置,O(1) per element。
//
// 複雜度:時間 O(n) per move;空間 O(1)。
void leetcode_move_concept_array_shift() {
    std::vector<std::string> v{"a", "b", "c", "d", "", "", ""};
    // 把前 4 個 (a..d) 往後搬 3 格 (空出前 3 格)
    std::move_backward(v.begin(), v.begin() + 4, v.end());
    std::cout << "after shift:";
    for (auto& s : v) std::cout << " [" << s << "]";
    std::cout << '\n';
}

// ----------------------------------------------------------------
// 實務範例:把「待處理任務」整批轉交給 worker
// ----------------------------------------------------------------
// 場景:主執行緒積攢一批 Job (含內部資源,昂貴拷貝),
//      要丟給 worker 執行緒接管。用 std::move 一次性轉交所有權。
void practical_transfer_pending_jobs() {
    struct Job { std::string payload; };
    std::vector<Job> pending;
    pending.push_back({std::string(500, 'X')});
    pending.push_back({std::string(500, 'Y')});

    std::vector<Job> worker(pending.size());
    std::move(pending.begin(), pending.end(), worker.begin());
    std::cout << "worker payload sizes:";
    for (auto& j : worker) std::cout << " " << j.payload.size();
    std::cout << " ; pending sizes:";
    for (auto& j : pending) std::cout << " " << j.payload.size();
    std::cout << '\n';
}

// === 預期輸出 (Expected output) ===
// dst: alpha beta gamma
// src after move (有效但未指定): [''] [''] ['']
// ups2[0]=10, ups2[1]=20
// ups[0] is null
// after move_backward: [a] [b] [a] [b] [c] [d]
// Moved big strings, dest sizes: 1000 1000 1000 ; src after move sizes: 0 0 0
// Manager values: 11 22 33 44 ; factory[0] null
// after shift: [] [] [] [a] [b] [c] [d]
// worker payload sizes: 500 500 ; pending sizes: 0 0
