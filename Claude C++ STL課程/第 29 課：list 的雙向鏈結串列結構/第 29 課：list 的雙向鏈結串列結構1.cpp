// =============================================================================
//  第 29 課：list 的雙向鏈結串列結構 1  —  節點、指標、與 O(1) 插刪的真正代價
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<list>
//   標準版本：std::list 為 C++98；本檔另用到 C++11 的 range-based for
//   迭代器類別：BidirectionalIterator（雙向迭代器，非隨機存取）
//
//   主要操作與複雜度：
//     push_front / push_back / pop_front / pop_back   O(1)
//     insert(pos, x) / erase(pos)                     O(1)  ← 前提：你已經有 pos
//     size()                                          O(1)（C++11 起強制）
//     operator[] / at()                               不存在
//     std::advance(it, n)                             O(n)  ← 這才是真正的成本
//
// 【詳細解釋 Explanation】
//
// 【1. 節點長什麼樣子：為什麼每個元素都要多付兩個指標】
//   list 的每個元素都被包在一個獨立配置的節點裡：
//       ┌──────┬────────┬──────┐
//       │ prev │  data  │ next │
//       └──────┴────────┴──────┘
//   在 64-bit 平台上，prev 與 next 各佔 8 bytes。存一個 int（4 bytes）
//   實際上要花 24 bytes 左右（含對齊填補，實作定義）——也就是說，
//   資料只佔了六分之一，其餘全是結構開銷。
//   這就是為什麼「存大量小型元素」時 list 幾乎永遠是錯的選擇。
//
// 【2. 為什麼插入／刪除是 O(1)，而 vector 是 O(n)】
//   在 vector 中間插入，必須把後面所有元素往後搬一格，成本隨元素數成長。
//   在 list 中間插入，只要改四個指標：
//       new_node->prev = pos->prev;   new_node->next = pos;
//       pos->prev->next = new_node;   pos->prev = new_node;
//   四次指標賦值，與 list 有多長完全無關 → 貨真價實的 O(1)。
//
//   但請注意這句話的完整版：「**已經持有該位置的迭代器**時，插刪是 O(1)」。
//   如果你得先找到那個位置，尋找本身是 O(n)。本檔第 5 段刻意用
//   `while (*pos != 30) ++pos;` 把這個尋找成本顯示出來——很多人只記得
//   「list 插刪 O(1)」，卻忘了前面那個 O(n) 的搜尋。
//
// 【3. 為什麼沒有 operator[]：標準庫不提供「假便宜」的介面】
//   技術上完全做得到 lst[3]（走三步就到），但那是 O(n)。
//   如果標準庫提供了 operator[]，使用者會很自然地寫出
//       for (size_t i = 0; i < lst.size(); ++i) use(lst[i]);
//   這是一個看起來 O(n)、實際 O(n²) 的迴圈——而且完全看不出來。
//   標準庫的原則是：**讓昂貴的操作在語法上就顯得昂貴**。
//   你必須改寫成 std::advance(it, 3)，那個函式名稱就在提醒你「這要走過去」。
//
// 【4. 為什麼 list 的迭代器是「雙向」而不是「隨機存取」】
//   迭代器的分類不是看容器高不高級，而是看「哪些操作能在常數時間內完成」。
//   list 節點只知道自己的前後鄰居，沒有任何辦法在 O(1) 內跳到第 n 個。
//   所以它只能支援 ++／--，不能支援 it + n、it - n、it[n]、it1 < it2。
//   連帶的後果：std::sort 需要隨機存取迭代器，所以**不能**對 list 使用；
//   list 因此自備成員函式 lst.sort()（用歸併排序實作，不需隨機存取）。
//
// 【5. 迭代器穩定性：list 最被低估的優勢】
//   在 list 中插入或刪除，**只有被刪除的那個元素的迭代器會失效**，
//   其他所有迭代器、指標、參考全部保持有效。
//   vector 只要一擴容，全部失效。這個差別在「一邊遍歷一邊改結構」
//   的情境下是決定性的。本檔第 5、6 段就在示範這件事：
//   插入 25 之後，指向 30 的 pos 依然可用。
//
// 【概念補充 Concept Deep Dive】
//   ● 哨兵節點（sentinel node）：libstdc++ 的 std::list 是「環狀」雙向串列，
//     內含一個不存資料的哨兵節點，end() 就是指向它。
//     這個設計讓 insert／erase 不需要為「空串列」「插在頭部」「插在尾部」
//     寫特例分支——所有情況的程式碼路徑完全一樣。這是資料結構教科書裡
//     很經典的技巧，也是為什麼 sizeof(std::list<int>) 在 libstdc++ 是
//     24 bytes（哨兵的兩個指標 + size 計數器，實作定義）。
//   ● C++11 起標準要求 size() 必須是 O(1)，因此實作必須額外維護一個計數器。
//     在 C++11 之前，libstdc++ 的 size() 是 O(n)（現場走一遍數）。
//     這是一個「標準修正把複雜度保證寫進介面」的實例。
//   ● 為什麼 list 遍歷比 vector 慢很多：每個節點是獨立 new 出來的，
//     位址由配置器決定，彼此可能相距甚遠。CPU 的硬體預取器無從預測下一個
//     節點在哪，等於每走一步都可能是一次 cache miss。這叫 pointer chasing，
//     是現代 CPU 上最慢的存取模式之一。本檔第 4 段印出位址就是要讓你看到
//     這個「不連續」。
//
// 【注意事項 Pay Attention】
//   1. 第 4 段印出的是記憶體位址，**每次執行都不同**，且 list 節點位址的
//      間距沒有任何保證。你會看到 vector 的位址等距（差 4 bytes），
//      list 的則不規則——但具體數字不可當成規格。
//   2. 「list 插刪 O(1)」的完整條件是「已持有該位置的迭代器」。
//      若還要先尋找，總成本是 O(n)。
//   3. list 不支援 operator[]、at()、也不能用 std::sort，
//      這些都是編譯期錯誤，不是執行期問題。
//   4. erase 之後，被刪除元素的迭代器就失效了，再解參考是 UB。
//      正確做法是接住 erase 的回傳值（下一個元素的迭代器）。
//   5. sizeof(std::list<int>) 與節點大小都是實作定義。
//   6. 空 list 也會配置哨兵節點，所以不是零記憶體成本。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::list 的雙向鏈結結構
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 std::list 不提供 operator[]？技術上明明做得到。
//     答：做得到，但那是 O(n)。標準庫刻意不提供，是為了避免使用者寫出
//         `for (i...) lst[i]` 這種看起來 O(n)、實際 O(n²) 的迴圈。
//         強迫你改用 std::advance，函式名稱本身就在提醒「這要一步步走」。
//         這是「讓昂貴的操作在語法上顯得昂貴」的 API 設計原則。
//     追問：那要拿第 n 個元素怎麼辦？→ std::advance(it, n) 或 std::next(it, n)，
//         兩者對 list 都是 O(n)，寫法上就看得出成本。
//
// 🔥 Q2. list 的插入刪除是 O(1)，為什麼實務上常常還是 vector 比較快？
//     答：三個原因。(1) O(1) 的前提是你已經持有迭代器，若還要尋找就是 O(n)。
//         (2) 每次插入都要 new 一個節點，動態配置本身就比搬移幾個 int 貴。
//         (3) 節點散布在堆積各處，遍歷是 pointer chasing，幾乎每步都 cache miss；
//         vector 連續存放，硬體預取器全速運作。實務上元素少於數千個時，
//         vector 的 O(n) 搬移常常打敗 list 的 O(1) 插入。
//     追問：那什麼時候 list 真的贏？→ 元素本身很大（搬移昂貴）、
//         需要頻繁 splice 整段搬移、或必須保證迭代器不失效時。
//
// 🔥 Q3. 為什麼不能對 std::list 用 std::sort？
//     答：std::sort 要求 RandomAccessIterator（內部要做 partition 與跳躍取樣），
//         list 只提供 BidirectionalIterator，不滿足需求，會編譯錯誤。
//         list 因此自備成員函式 lst.sort()，它用歸併排序實作——
//         歸併只需要循序走訪與指標接合，不需要隨機存取。
//     追問：lst.sort() 的複雜度？→ O(n log n)，而且是穩定排序；
//         它只重接指標不搬資料，所以元素的位址不變、迭代器仍指向原元素。
//
// ⚠️ 陷阱. 「在 list 中間插入元素，會讓其他迭代器失效嗎？」
//     答：不會。list 的 insert 不影響任何既有迭代器、指標、參考。
//         erase 也只讓「被刪除的那一個」失效，其餘全部有效。
//         這跟 vector 完全相反——vector 一擴容就全滅。
//     為什麼會錯：把 vector 的失效規則直接套到所有容器。
//         失效規則是「每個容器各自規定」的，必須分開記。
//
// ⚠️ 陷阱. 「lst.size() 是 O(n)，因為要走過整條串列。」
//     答：C++11 起標準要求 size() 必須是 O(1)，實作必須額外維護計數器。
//         這個說法在 C++98／C++03 時代對 libstdc++ 是對的，但現在已經過時。
//     為什麼會錯：讀到的是舊資料。複雜度保證會隨標準版本修訂，
//         引用時要標明是哪個標準版本。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <list>
#include <vector>
#include <string>
using namespace std;

// -----------------------------------------------------------------------------
// 【日常實務範例】瀏覽器分頁的「最近使用」清單（LRU 順序維護）
//   情境：使用者切到某個分頁時，那個分頁要被移到清單最前面；
//         關閉分頁則從清單中移除。分頁數量可能上百個。
//   為什麼用 list：切換分頁 = 把節點從中間拔出來接到最前面。
//         用 list 是純指標操作，元素本身完全不搬動；
//         用 vector 則每次都要搬移一大段元素。
//         而且 list 的迭代器不會因為別處的增刪而失效，
//         所以可以把「分頁 → 迭代器」的對照表存起來重複使用，
//         這正是真實 LRU cache 的標準做法（list + unordered_map）。
// -----------------------------------------------------------------------------
class TabHistory {
public:
    // 切換到某個分頁：已存在就搬到最前面，不存在就新增到最前面
    void touch(const string& title) {
        for (auto it = tabs_.begin(); it != tabs_.end(); ++it) {
            if (*it == title) {
                // splice：把節點從原位置搬到最前面，O(1)，不複製也不搬資料
                tabs_.splice(tabs_.begin(), tabs_, it);
                return;
            }
        }
        tabs_.push_front(title);
    }

    void close(const string& title) {
        tabs_.remove(title);          // list 的成員 remove：按值刪除
    }

    void dump() const {
        cout << "  最近使用順序: ";
        for (const auto& t : tabs_) cout << "[" << t << "] ";
        cout << "\n";
    }

private:
    list<string> tabs_;
};

int main() {
    // === 1. 基本建立 ===
    list<int> lst = {10, 20, 30, 40, 50};

    cout << "=== list 基本資訊 ===" << endl;
    cout << "size: " << lst.size() << endl;   // C++11 起保證 O(1)
    // cout << lst[0];   // 編譯錯誤！list 不支援 operator[]
    // cout << lst.at(0); // 編譯錯誤！list 不支援 at()

    // === 2. 遍歷方式 ===
    cout << "\n=== 遍歷 ===" << endl;

    // 方式一：範圍 for（最常用）
    cout << "範圍 for：";
    for (int val : lst) {
        cout << val << " ";
    }
    cout << endl;

    // 方式二：迭代器
    cout << "迭代器：  ";
    for (auto it = lst.begin(); it != lst.end(); ++it) {
        cout << *it << " ";
    }
    cout << endl;

    // 方式三：反向迭代器（雙向迭代器才有；forward_list 就沒有）
    cout << "反向迭代：";
    for (auto rit = lst.rbegin(); rit != lst.rend(); ++rit) {
        cout << *rit << " ";
    }
    cout << endl;

    // === 3. 展示 list 不支援隨機存取 ===
    cout << "\n=== 迭代器類型限制 ===" << endl;
    auto it = lst.begin();

    // 可以用 ++ 和 -- （Bidirectional）
    ++it;  // 移到第 2 個元素
    cout << "++it → " << *it << endl;     // 20
    --it;  // 移回第 1 個元素
    cout << "--it → " << *it << endl;     // 10

    // 不能用 + 或 -（不是 Random Access）
    // auto it2 = it + 3;   // 編譯錯誤！
    // 要到第 4 個元素，必須用 std::advance —— 注意這是 O(n)，不是 O(1)
    advance(it, 3);  // 移動 3 步
    cout << "advance(it, 3) → " << *it << endl;  // 40

    // === 4. 展示元素地址不連續 ===
    // 注意：以下位址每次執行都不同；重點只在「list 不等距、vector 等距」
    cout << "\n=== 記憶體地址（list：不連續） ===" << endl;
    for (auto& val : lst) {
        cout << "值: " << val << "  地址: " << &val << endl;
    }

    // 對比 vector 的連續地址
    cout << "\n=== vector 記憶體地址（連續） ===" << endl;
    vector<int> vec = {10, 20, 30, 40, 50};
    for (auto& val : vec) {
        cout << "值: " << val << "  地址: " << &val << endl;
    }
    // 用「相對距離」呈現連續性 —— 這個是決定性的，不像絕對位址那樣每次都變
    cout << "vector 相鄰元素位址差(以 int 為單位): "
         << (&vec[1] - &vec[0]) << endl;

    // === 5. 展示 O(1) 插入 ===
    cout << "\n=== O(1) 中間插入 ===" << endl;
    // 找到值為 30 的位置 —— 注意：這個「尋找」本身是 O(n)
    auto pos = lst.begin();
    while (*pos != 30) ++pos;

    // 在 30 前面插入 25 —— 這一步才是 O(1)
    lst.insert(pos, 25);

    cout << "在 30 前插入 25：";
    for (int val : lst) cout << val << " ";
    cout << endl;

    // 注意：pos 仍然有效！仍指向 30（list 的插入不使任何既有迭代器失效）
    cout << "插入後 pos 仍指向：" << *pos << endl;  // 30

    // === 6. 展示 O(1) 刪除 ===
    cout << "\n=== O(1) 刪除 ===" << endl;
    // 刪除 pos 指向的元素（30）；erase 後 pos 本身就失效了，不可再解參考
    auto next_pos = lst.erase(pos);  // 回傳下一個元素的迭代器

    cout << "刪除 30 後：";
    for (int val : lst) cout << val << " ";
    cout << endl;

    cout << "erase 回傳的迭代器指向：" << *next_pos << endl;  // 40

    // === 7. front 和 back 操作 ===
    cout << "\n=== front / back ===" << endl;
    cout << "front: " << lst.front() << endl;
    cout << "back:  " << lst.back() << endl;

    // === 8. 日常實務：分頁最近使用順序 ===
    cout << "\n=== 日常實務：瀏覽器分頁 LRU 順序 ===" << endl;
    TabHistory tabs;
    tabs.touch("GitHub");
    tabs.touch("StackOverflow");
    tabs.touch("cppreference");
    tabs.dump();
    cout << "  使用者切回 GitHub：" << endl;
    tabs.touch("GitHub");            // splice 到最前面，O(1)
    tabs.dump();
    cout << "  關閉 StackOverflow：" << endl;
    tabs.close("StackOverflow");
    tabs.dump();

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 29 課：list 的雙向鏈結串列結構1.cpp" -o lesson29

// 注意：第 4 段印出的記憶體位址每次執行都不同（受配置器與 ASLR 影響）。
//       重點不在具體數值，而在「vector 位址等距、list 位址不規則」這個現象。

// === 預期輸出 ===
// === list 基本資訊 ===
// size: 5
//
// === 遍歷 ===
// 範圍 for：10 20 30 40 50
// 迭代器：  10 20 30 40 50
// 反向迭代：50 40 30 20 10
//
// === 迭代器類型限制 ===
// ++it → 20
// --it → 10
// advance(it, 3) → 40
//
// === 記憶體地址（list：不連續） ===
// 值: 10  地址: 0x57f00eba7030
// 值: 20  地址: 0x57f00eba7350
// 值: 30  地址: 0x57f00eba7370
// 值: 40  地址: 0x57f00eba7390
// 值: 50  地址: 0x57f00eba73b0
//
// === vector 記憶體地址（連續） ===
// 值: 10  地址: 0x57f00eba73c0
// 值: 20  地址: 0x57f00eba73c4
// 值: 30  地址: 0x57f00eba73c8
// 值: 40  地址: 0x57f00eba73cc
// 值: 50  地址: 0x57f00eba73d0
// vector 相鄰元素位址差(以 int 為單位): 1
//
// === O(1) 中間插入 ===
// 在 30 前插入 25：10 20 25 30 40 50
// 插入後 pos 仍指向：30
//
// === O(1) 刪除 ===
// 刪除 30 後：10 20 25 40 50
// erase 回傳的迭代器指向：40
//
// === front / back ===
// front: 10
// back:  50
//
// === 日常實務：瀏覽器分頁 LRU 順序 ===
//   最近使用順序: [cppreference] [StackOverflow] [GitHub]
//   使用者切回 GitHub：
//   最近使用順序: [GitHub] [cppreference] [StackOverflow]
//   關閉 StackOverflow：
//   最近使用順序: [GitHub] [cppreference]
