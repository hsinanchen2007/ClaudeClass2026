// =============================================================================
//  第六課 5 — std::forward_list：只付一個指標代價的單向鏈結串列
// =============================================================================
//
// 【主題資訊 Information】
//   template<class T, class Allocator = std::allocator<T>> class forward_list;
//                                                       // <forward_list>,C++11 起
//
//   常用介面（注意「所有變動操作都叫 *_after」）：
//     void      push_front(const T& v);                 // O(1)
//     iterator  insert_after(const_iterator pos, const T& v);   // O(1)
//     iterator  erase_after(const_iterator pos);                // O(1)
//     iterator  emplace_after(const_iterator pos, Args&&...);   // O(1)
//     iterator  before_begin() noexcept;                // O(1),不可解參考
//     void      splice_after(const_iterator pos, forward_list& other);  // O(1)
//     size_type remove(const T& v);                     // O(n)
//     void      sort();                                 // O(n log n),不失效
//     bool      empty() const noexcept;                 // O(1)
//     // 注意：沒有 size()、沒有 push_back()、沒有 back()、沒有反向迭代器
//
//   複雜度：前端插入／刪除 O(1);走訪 O(n);取得長度 O(n)（要靠 std::distance）
//   標頭檔：#include <forward_list>（算長度另需 #include <iterator>）
//
// 【詳細解釋 Explanation】
//
// 【1. forward_list 的設計目標：對抗「手寫 C 單向串列」】
// STL 已經有 std::list（雙向鏈結串列）了,為什麼 C++11 還要再加一個
// forward_list?答案不在功能,而在**成本**。
//
// C++ 標準委員會給 forward_list 訂的目標非常明確:它必須做到
// 「相對於手寫的 C 單向鏈結串列,零額外開銷」。也就是說,如果你在 C 裡面寫:
//
//     struct Node { int value; struct Node* next; };
//     struct Node* head;                 // 整個串列就是這一個指標
//
// 那 C++ 的 forward_list 就不准比它胖。本機實測（GCC 15.2.0 / x86-64）:
//
//     sizeof(std::forward_list<int>) == 8      ← 就是一個指標
//     sizeof(std::list<int>)         == 24     ← 三倍
//     sizeof(std::vector<int>)       == 24
//
// forward_list 的**整個物件只有一個指標**,指向第一個節點。這個「8 bytes」
// 不是巧合,而是設計上的硬性約束 —— 而接下來你會看到,forward_list 那些
// 「缺東缺西」的怪介面,全部都是為了守住這個 8 bytes 才長成那樣的。
//
// 【2. 為什麼沒有 size()：因為它要花掉一個 word】
// 初學者最常抱怨的就是 forward_list 沒有 size()。這不是遺漏,是刻意的取捨。
//
// 要讓 size() 是 O(1),容器就得**額外存一個計數欄位**,並在每次 insert/erase
// 時維護它。那會讓 sizeof 從 8 變成 16（多一個 size_t）,直接違反「不比手寫
// C 串列胖」的設計目標。委員會的判斷是:會選 forward_list 的人,正是那些
// 錙銖必較到連 8 bytes 都在乎的人;真的需要 size() 的人應該去用 list 或 vector。
//
// 所以想知道長度,得自己付 O(n) 的代價:
//     auto n = std::distance(flst.begin(), flst.end());   // O(n),要走完整串
//
// 對照:std::list 的 size() 自 C++11 起被標準**強制要求**為 O(1),
// 所以 list 一定會存那個計數欄位（這正是它 24 bytes 的成因之一）。
//
// 【3. 為什麼沒有 push_back()：因為它要花掉一個尾指標】
// 同樣的邏輯。要讓 push_back() 是 O(1),容器必須額外存一個「指向最後一個
// 節點」的 tail 指標,sizeof 又會從 8 變 16。所以 forward_list 只提供
// push_front()（O(1),只要改 head 指標）。
//
// 如果硬要在尾端加東西,你得自己走到尾巴,那是 O(n):
//     auto it = flst.before_begin();
//     while (std::next(it) != flst.end()) ++it;   // 自己走到最後一個元素
//     flst.insert_after(it, value);              // O(n) 走訪 + O(1) 插入
//
// 這也解釋了為什麼**沒有 back()、沒有 pop_back()、沒有 rbegin()/rend()**:
// 單向節點只有 next 指標,無法往回走,任何「從尾端」的操作都做不到 O(1),
// 甚至反向迭代根本無法實作。
//
// 【4. 為什麼所有變動都是 *_after：單向節點回不了頭】
// 這是 forward_list 最違反直覺、也最必須理解的一點。
//
// 一般容器是 insert(pos, v)「插在 pos 之前」、erase(pos)「刪掉 pos」。
// 但在單向串列裡,這兩件事**做不到 O(1)**。原因很單純:
//
//     [A] → [B] → [C]
//            ↑
//           pos
//
// 要在 B 之前插入新節點,你必須修改 **A 的 next 指標**。但你手上只有指向 B
// 的迭代器,而 B 這個節點裡面**只有 next,沒有 prev** —— 你根本無法從 B 找到 A。
// 唯一的辦法是從 head 重新走一遍找出 B 的前驅,那是 O(n)。
//
// 委員會的選擇是:**不提供做不到 O(1) 的介面**,而不是提供一個偷偷 O(n) 的
// 假 O(1)。所以 forward_list 把整組介面改成「以前驅位置定址」:
//
//     insert_after(pos, v)   在 pos 的**後面**插入
//     erase_after(pos)       刪掉 pos 的**下一個**元素（不是 pos 自己！）
//     emplace_after(pos, …)  在 pos 的後面就地建構
//     splice_after(pos, …)   接到 pos 的後面
//
// 這樣每個操作都只要改「pos 這個節點的 next」,而 pos 你手上就有 —— 真正的 O(1)。
// 對照 std::list:它的節點有 prev 和 next 兩個指標（這是它 24 bytes 的另一個
// 成因）,所以 list 可以提供正常的 insert(pos)/erase(pos)。
// 「介面長相」的差異,根源是「節點結構」的差異。
//
// 【5. before_begin()：為了讓「插在頭部」也能走同一套 _after 介面】
// 既然所有插入都需要「前一個位置」,那要插在**第一個元素之前**怎麼辦?
// 第一個元素的前面沒有元素,拿不到迭代器。
//
// forward_list 的解法是提供 before_begin():一個**虛擬的、指向 begin() 前一格**
// 的迭代器。它讓「插在頭部」也能用統一的 insert_after 表達:
//
//     flst.insert_after(flst.before_begin(), 5);   // 等價於 push_front(5)
//     flst.erase_after(flst.before_begin());       // 刪掉第一個元素
//
// 關鍵限制:**before_begin() 回傳的迭代器不可以解參考**。
// 寫 *flst.before_begin() 是 undefined behavior,因為它並不指向任何真實元素;
// 它指向的是容器內部那個 head 指標所在的「掛勾」位置。它只能拿來當
// insert_after / erase_after / splice_after 的參數,或用 ++ 前進到 begin()。
//
// 【6. 迭代器穩定性：節點式容器的共同優勢】
// forward_list 是節點式（node-based）容器,每個元素住在自己獨立配置的節點裡,
// 位置一輩子不會變。因此:
//   * insert_after 之後,**所有既有的迭代器、指標、參考全部保持有效**。
//   * erase_after 之後,只有「被刪掉的那個元素」的迭代器失效,其餘全部有效。
// 這和 vector 形成強烈對比 —— vector 一旦重新配置（reallocation）,
// 所有迭代器與指標會全部失效。
//
// 也因為節點不搬家,forward_list 的 sort()、merge()、splice_after() 都是
// **只改指標、不搬移元素**的操作,不會呼叫任何元素的複製或搬移建構子。
// splice_after 把另一串接過來甚至是 O(1)。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 記憶體佈局：容器本體與節點是分開的兩塊
//     libstdc++ 的結構簡化後是這樣:
//
//         struct _Fwd_list_node_base { _Fwd_list_node_base* _M_next; };
//         template<class T> struct _Fwd_list_node : _Fwd_list_node_base {
//             T _M_storage;                    // 實際元素
//         };
//         template<class T, class A> class forward_list {
//             _Fwd_list_node_base _M_head;     // 唯一的資料成員 → sizeof == 8
//         };
//
//     注意容器本體存的是一個 **_Fwd_list_node_base**（只含一個 next 指標）,
//     而不是 _Fwd_list_node。這正是 before_begin() 的實作祕密:它回傳的迭代器
//     就指向這個 _M_head 掛勾。因為 _M_head 的型別是 base（沒有 T 成員）,
//     所以解參考它會去讀取一塊根本不存在的元素 —— 這就是為什麼標準規定
//     before_begin() 不可解參考,而且違反是 UB 而不是丟例外。
//
// (B) 每個元素的真實成本
//     一個 forward_list<int> 節點包含 next 指標(8) + int(4) + padding(4) = 16 bytes,
//     用來裝 4 bytes 的資料。加上 heap 配置器本身的 metadata 開銷,
//     實際佔用還更多（確切數字屬**實作定義**）。
//     對照 std::vector<int>:每個元素攤銷後就是 4 bytes,且連續排列。
//     所以「forward_list 比較省」指的是**容器物件本身**只有 8 bytes,
//     **不是**指存資料比較省 —— 論每元素成本,鏈結串列一向是最貴的。
//
// (C) 為什麼實務上 vector 常常還是比較快
//     鏈結串列的節點散落在 heap 各處,走訪時每跳一個節點就是一次可能的
//     cache miss,而且 CPU 的硬體預取器（prefetcher）對這種指標追逐
//     （pointer chasing）幾乎無能為力。vector 的元素連續排列,一次 cache line
//     可以載入 16 個 int,預取器也能完美運作。
//     現代 CPU 上,即使「vector 中間插入要搬移元素」聽起來很貴,
//     在中小資料量下往往仍勝過鏈結串列 —— 因為記憶體存取才是瓶頸,不是搬移次數。
//     選 forward_list 的正當理由通常是:**需要 O(1) splice**、
//     **需要迭代器/指標永久穩定**、或**元素巨大到搬移成本極高**。
//
// (D) 編譯器做了什麼
//     forward_list 的所有操作都是薄薄的指標運算,加上 -O2 後幾乎完全 inline,
//     產生的機器碼和手寫 C 的 `node->next = newnode;` 幾乎一模一樣。
//     這就是「零開銷抽象」在鏈結串列上的體現:抽象存在於原始碼,不存在於機器碼。
//
// 【注意事項 Pay Attention】
//  1. erase_after(it) 刪掉的是 **it 的下一個元素**,不是 it 自己。
//     這是最常見的 off-by-one 錯誤來源,寫的時候務必在腦中畫出節點圖。
//  2. *before_begin() 是 undefined behavior。它不指向任何元素,行為不保證、
//     也不可預測,絕不能解參考。它只能當 *_after 系列的參數或拿來 ++。
//  3. 對 empty() 為 true 的串列呼叫 front() 是 undefined behavior。
//     判空請用 empty(),它是 O(1);不要用 std::distance(...) == 0（那是 O(n)）。
//  4. 沒有 size() 是設計,不是缺陷。需要頻繁查長度就代表你選錯容器了 ——
//     請改用 std::list（size() 保證 O(1)）或 std::vector。
//  5. std::distance 對 forward_list 是 O(n),因為它的迭代器是 forward iterator,
//     只能一步一步走。在迴圈條件裡呼叫它會讓整體變成 O(n²)。
//  6. sizeof(std::forward_list<int>) == 8、sizeof(std::list<int>) == 24 是本機
//     （GCC 15.2.0 / libstdc++ / x86-64）實測值,屬**實作定義**;
//     標準只保證介面與複雜度,不保證確切大小。
//  7. remove_if / remove 會刪除**所有**符合條件的元素,並回傳刪除個數
//     （回傳值自 C++20 起才由標準規定;C++11~17 為 void,本檔不依賴回傳值）。
//  8. forward_list 沒有反向迭代器（rbegin/rend）。需要反向走訪請改用 list。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::forward_list
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::forward_list 為什麼沒有 size()?
//     答：因為要讓 size() 是 O(1),容器就得多存一個計數欄位並在每次增刪時
//         維護它,sizeof 會從 8 變 16。forward_list 的設計目標是「相對於
//         手寫 C 單向串列零額外開銷」,本機實測 sizeof(forward_list<int>) == 8,
//         就只有一個 head 指標。為了守住這 8 bytes,size() 被刻意拿掉。
//         需要長度就自己付 O(n):std::distance(begin(), end())。
//     追問：那 std::list 有 size() 嗎?複雜度多少?
//         → 有,而且自 C++11 起標準**強制要求** list::size() 為 O(1),
//           所以 list 一定會存那個計數欄位 —— 這正是 list 24 bytes 的成因之一。
//
// 🔥 Q2. 為什麼 forward_list 的插入刪除都叫 insert_after / erase_after,
//        而不是像其他容器一樣的 insert / erase?
//     答：因為單向串列的節點只有 next 指標,沒有 prev。要在某個節點「之前」
//         插入,必須修改它**前驅節點**的 next,但從該節點無法回頭找到前驅,
//         只能從頭走 O(n)。委員會選擇不提供做不到 O(1) 的介面,
//         於是把整組介面改成「以前驅位置定址」—— 這樣只要改 pos 自己的 next,
//         就是真正的 O(1)。
//     追問：那要插在第一個元素前面怎麼辦?
//         → 用 before_begin(),它是一個指向 begin() 前一格的虛擬迭代器,
//           讓「插在頭部」也能走同一套 _after 介面。但它**不可解參考**,
//           因為它指的是容器內部的 head 掛勾,不是真實元素。
//
// ⚠️ 陷阱. 我用 std::find 找到值為 30 的位置,然後呼叫 erase_after(it) 想刪掉它。
//        為什麼刪錯了?
//     答：erase_after(it) 刪掉的是 **it 的下一個元素**,不是 it 指向的元素。
//         find 回傳的是「30 自己的位置」,所以你刪掉的是 30 後面那個元素,30 還在。
//         正解是找出 30 的**前驅**:從 before_begin() 起手,檢查
//         *std::next(it) == 30 再 erase_after(it);或直接用 flst.remove(30)。
//     為什麼會錯：腦中沿用了 vector/list 的 erase(pos) 語意 ——「參數指誰就刪誰」。
//         但 forward_list 整組 _after 介面的參數意義是「**前驅**位置」,
//         不是「目標」位置。名字裡的 after 就是在提醒這件事。
//
// ⚠️ 陷阱. forward_list 比 list 省一半以上記憶體,那是不是能用 list 的場合
//        都該優先改用 forward_list?
//     答：不是。省的是**容器物件本身**（8 vs 24 bytes）,不是每個元素的成本。
//         論每元素成本,forward_list<int> 一個節點約 16 bytes 才裝 4 bytes 資料,
//         比 vector 貴得多。而且沒有 size()、沒有 push_back、沒有反向迭代、
//         也無法從中間節點往回走。只有在「確實只需單向走訪」且
//         「容器數量非常多（例如每個 hash bucket 一條串列）」時,那 16 bytes 的
//         差距才真的有意義。
//     為什麼會錯：把「sizeof 比較小」直接讀成「比較省記憶體」。
//         sizeof 量的是容器的**管理結構**,資料量的成本在節點,兩者是不同的東西。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <forward_list>
#include <iterator>     // std::distance, std::next
#include <list>
#include <string>
#include <vector>

// -----------------------------------------------------------------------------
// 【日常實務範例】過濾應用程式 log:丟掉 DEBUG 等級的行
//   情境：從服務收到一大批 log 行,要在送進分析管線前先濾掉 DEBUG 級別的雜訊。
//     資料只會從頭到尾掃一次、之後也只需單向走訪,不需要隨機存取或反向走訪。
//   為什麼用到本主題：
//     (1) remove_if 在 forward_list 上只是**改指標**,不搬移任何元素 ——
//         而 vector 的 erase 要把後面所有元素往前搬。當元素是長字串時,
//         「不搬移」這件事很有價值。
//     (2) 沒有 push_back,所以示範用一個 tail 游標配合 insert_after
//         以 O(1) 逐行建串,維持原始順序（若用 push_front 會得到反序）。
// -----------------------------------------------------------------------------
std::forward_list<std::string> buildLogChain(const std::vector<std::string>& lines) {
    std::forward_list<std::string> chain;
    // forward_list 沒有 push_back,用一個「永遠指向最後一個元素」的游標,
    // 每次 insert_after 之後把游標推進 —— 整體仍是 O(n),且保持原始順序。
    auto tail = chain.before_begin();
    for (const std::string& line : lines) {
        tail = chain.insert_after(tail, line);   // 回傳新元素的位置,直接當下一個 tail
    }
    return chain;
}

std::size_t dropDebugLines(std::forward_list<std::string>& chain) {
    std::size_t before = static_cast<std::size_t>(
        std::distance(chain.begin(), chain.end()));   // O(n):沒有 size() 可用

    // remove_if:刪掉所有開頭是 "[DEBUG]" 的行。只改指標,不搬移字串。
    chain.remove_if([](const std::string& s) {
        return s.rfind("[DEBUG]", 0) == 0;            // rfind(...,0)==0 即「以此開頭」
    });

    std::size_t after = static_cast<std::size_t>(
        std::distance(chain.begin(), chain.end()));
    return before - after;                            // 回傳被刪掉幾行
}

// -----------------------------------------------------------------------------
// 輔助：安全地刪除「值等於 target」的第一個元素。
//   重點示範:因為 erase_after 需要的是**前驅**位置,所以必須從 before_begin()
//   起手,檢查「下一個」是不是目標,而不能拿 std::find 的結果直接去 erase_after。
// -----------------------------------------------------------------------------
bool eraseFirstEqual(std::forward_list<int>& fl, int target) {
    for (auto prev = fl.before_begin(); std::next(prev) != fl.end(); ++prev) {
        if (*std::next(prev) == target) {
            fl.erase_after(prev);      // prev 是前驅 → 刪掉的正好是 target
            return true;
        }
    }
    return false;
}

int main() {
    // ── 原始課堂示範:forward_list 的基本操作 ──────────────────────────────
    std::cout << "=== std::forward_list ===" << std::endl;

    std::forward_list<int> flst = {20, 30, 40};

    // 只能在前端插入
    flst.push_front(10);

    std::cout << "元素: ";
    for (int n : flst) std::cout << n << " ";
    std::cout << std::endl;

    // 在某個位置「之後」插入
    auto it = flst.begin();  // 指向 10
    flst.insert_after(it, 15);  // 在 10 之後插入 15

    std::cout << "insert_after 後: ";
    for (int n : flst) std::cout << n << " ";
    std::cout << std::endl;

    // 注意：forward_list 沒有 size() 成員函數！
    // 需要用 std::distance 計算
    auto count = std::distance(flst.begin(), flst.end());
    std::cout << "元素個數: " << count << std::endl;

    // ── 為什麼沒有 size()：sizeof 的實證 ──────────────────────────────────
    std::cout << "\n=== 為什麼沒有 size(): 用 sizeof 看設計取捨 ===" << std::endl;
    std::cout << "sizeof(std::forward_list<int>) = "
              << sizeof(std::forward_list<int>) << "  (只有一個 head 指標)" << std::endl;
    std::cout << "sizeof(std::list<int>)         = "
              << sizeof(std::list<int>) << "  (prev/next 結構 + O(1) 的 size 計數)" << std::endl;
    std::cout << "sizeof(std::vector<int>)       = "
              << sizeof(std::vector<int>) << "  (begin/end/capacity 三個指標)" << std::endl;
    std::cout << "→ 多存一個 size 欄位就會讓 8 變 16,違反「不比手寫 C 串列胖」的設計目標"
              << std::endl;
    std::cout << "  (以上為本機 GCC 15.2.0 / x86-64 實測值,屬實作定義)" << std::endl;

    // ── before_begin()：讓「插在頭部」也走 _after 介面 ────────────────────
    std::cout << "\n=== before_begin(): 頭部操作的統一入口 ===" << std::endl;
    std::forward_list<int> bb = {2, 3, 4};
    bb.insert_after(bb.before_begin(), 1);       // 等價於 push_front(1)
    std::cout << "insert_after(before_begin(), 1) 後: ";
    for (int n : bb) std::cout << n << " ";
    std::cout << "  ← 等價於 push_front" << std::endl;

    bb.erase_after(bb.before_begin());           // 刪掉第一個元素
    std::cout << "erase_after(before_begin())  後: ";
    for (int n : bb) std::cout << n << " ";
    std::cout << "  ← 等價於 pop_front" << std::endl;
    std::cout << "注意: *before_begin() 是 UB —— 它不指向任何元素,故不示範解參考"
              << std::endl;

    // ── erase_after 的 off-by-one 陷阱 ────────────────────────────────────
    std::cout << "\n=== erase_after 刪的是「下一個」,不是自己 ===" << std::endl;
    std::forward_list<int> demo = {10, 20, 30, 40};
    auto p = demo.begin();                       // p 指向 10
    demo.erase_after(p);                         // 刪掉的是 20,不是 10！
    std::cout << "p 指向 10,呼叫 erase_after(p) 後: ";
    for (int n : demo) std::cout << n << " ";
    std::cout << "  ← 消失的是 20" << std::endl;

    std::forward_list<int> demo2 = {10, 20, 30, 40};
    std::cout << "要刪掉「值等於 30」的元素,必須從 before_begin 找前驅: ";
    eraseFirstEqual(demo2, 30);
    for (int n : demo2) std::cout << n << " ";
    std::cout << std::endl;

    // ── splice_after：節點式容器的殺手級功能 ──────────────────────────────
    std::cout << "\n=== splice_after: 只改指標,不複製元素 ===" << std::endl;
    std::forward_list<int> a = {1, 2};
    std::forward_list<int> b = {100, 200, 300};
    a.splice_after(a.begin(), b);                // 把整個 b 接到 a 的第一個元素之後
    std::cout << "a = ";
    for (int n : a) std::cout << n << " ";
    std::cout << std::endl;
    std::cout << "b 變空了嗎? " << std::boolalpha << b.empty()
              << "  (節點被移交,沒有任何元素被複製)" << std::endl;

    // ── 日常實務：過濾 log ────────────────────────────────────────────────
    std::cout << "\n=== 日常實務: 過濾 DEBUG log ===" << std::endl;
    std::vector<std::string> raw = {
        "[INFO ] service started on port 8080",
        "[DEBUG] cache lookup key=user:1024 miss",
        "[WARN ] upstream latency 812ms",
        "[DEBUG] retry backoff = 200ms",
        "[ERROR] upstream connection refused",
        "[DEBUG] gc pause 3ms",
        "[INFO ] shutting down"
    };

    std::forward_list<std::string> chain = buildLogChain(raw);
    std::cout << "原始行數: "
              << std::distance(chain.begin(), chain.end())
              << "  (用 std::distance,O(n))" << std::endl;

    std::size_t dropped = dropDebugLines(chain);
    std::cout << "丟棄 DEBUG 行數: " << dropped << std::endl;
    std::cout << "保留下來的 log:" << std::endl;
    for (const std::string& line : chain) {
        std::cout << "  " << line << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第六課：容器（Container）的概念與分類5.cpp" -o forward_list_demo

//   (以上為本機 GCC 15.2.0 / x86-64 實測值,屬實作定義)

// === 預期輸出 ===
// === std::forward_list ===
// 元素: 10 20 30 40
// insert_after 後: 10 15 20 30 40
// 元素個數: 5
//
// === 為什麼沒有 size(): 用 sizeof 看設計取捨 ===
// sizeof(std::forward_list<int>) = 8  (只有一個 head 指標)
// sizeof(std::list<int>)         = 24  (prev/next 結構 + O(1) 的 size 計數)
// sizeof(std::vector<int>)       = 24  (begin/end/capacity 三個指標)
// → 多存一個 size 欄位就會讓 8 變 16,違反「不比手寫 C 串列胖」的設計目標
//
// === before_begin(): 頭部操作的統一入口 ===
// insert_after(before_begin(), 1) 後: 1 2 3 4   ← 等價於 push_front
// erase_after(before_begin())  後: 2 3 4   ← 等價於 pop_front
// 注意: *before_begin() 是 UB —— 它不指向任何元素,故不示範解參考
//
// === erase_after 刪的是「下一個」,不是自己 ===
// p 指向 10,呼叫 erase_after(p) 後: 10 30 40   ← 消失的是 20
// 要刪掉「值等於 30」的元素,必須從 before_begin 找前驅: 10 20 40
//
// === splice_after: 只改指標,不複製元素 ===
// a = 1 100 200 300 2
// b 變空了嗎? true  (節點被移交,沒有任何元素被複製)
//
// === 日常實務: 過濾 DEBUG log ===
// 原始行數: 7  (用 std::distance,O(n))
// 丟棄 DEBUG 行數: 3
// 保留下來的 log:
//   [INFO ] service started on port 8080
//   [WARN ] upstream latency 812ms
//   [ERROR] upstream connection refused
//   [INFO ] shutting down
