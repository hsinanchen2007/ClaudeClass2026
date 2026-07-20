// =============================================================================
//  第 36 課：list 的特有操作——reverse 1  —  反轉「連線」而不是反轉「資料」
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：#include <list>
//   簽名：  void reverse() noexcept;          ← 成員函式，無回傳值，noexcept
//   複雜度：O(n)（每個節點恰好處理一次）
//   標準版本：C++98 就有；noexcept 標記自 C++11 起
//             （本機實測 noexcept(l.reverse()) == 1）
//
//   ★ 核心：list::reverse **只重接每個節點的 prev/next 指標**，
//     不複製、不移動、不交換任何元素資料。
//     因此 **iterator / reference / pointer 全部保持有效**
//     —— 它們仍指向原本那個元素，只是整條鏈的走訪順序反過來了。
//
//   對照組（三種「反轉」，語意完全不同）：
//     lst.reverse()                    改連線 → 迭代器跟著元素走，順序翻轉
//     std::reverse(lst.begin(), ...)   交換值 → 迭代器位置不動，值被換掉
//     lst.rbegin() / lst.rend()        不改變任何東西，只是反向走訪的視圖，O(1)
//
// 【詳細解釋 Explanation】
//
// 【1. list::reverse 到底做了什麼】
// list 是雙向鏈結串列。反轉整條鏈，只要把**每個節點的 prev 和 next 對調**：
//
//   反轉前：  head ⇄ [10] ⇄ [20] ⇄ [30] ⇄ tail
//   反轉後：  head ⇄ [30] ⇄ [20] ⇄ [10] ⇄ tail
//
//   注意方框裡的元素（10/20/30）**從頭到尾沒有搬動過**，
//   改的只是箭頭的方向。所以：
//     * **工作量**與元素大小無關：不論元素是 int 還是 1KB 的物件，
//       都是 n 次指標對調，不會因為元素變大而多做事。
//       （但**實際耗時**仍會受記憶體局部性影響 —— 大元素的節點在 heap 上
//         相隔較遠，指標追逐的 cache miss 較多。本檔第 6 節實測：
//         同樣 200,000 個節點，int 版 ≈ 407µs、1028 bytes 版 ≈ 6,573µs。
//         做的事一樣多，但走訪的記憶體範圍差了 250 倍。）
//     * 指向元素的指標依然有效（元素還在原地）
//     * 不可能拋例外（沒有配置、沒有呼叫使用者的建構子）→ noexcept
//
// 【2. std::reverse 做的是完全不同的事】
// std::reverse 是泛型演算法，它不知道自己面對的是 list 還是 vector，
// 只能透過迭代器介面做事，於是它的做法是**從兩端往中間交換「值」**：
//     while (first != last && first != --last) std::iter_swap(first++, last);
// 對 list 而言：
//     * 節點的連線完全沒變，改變的是每個節點裡**裝的元素**
//     * 迭代器**位置不變**，但它指的元素被換成別的了
//     * 成本與元素大小**高度相關**（交換 1KB 的物件很貴）
//
// 【3. ★ 兩者的差異是「語意」，不只是「效能」★】
//   這是本課最重要的一點。假設你在別處持有一個指向某元素的指標：
//       auto it = next(lst.begin());   // 指向第 2 個元素，值為 20
//       lst.reverse();                 // → *it 仍然是 20（它變成倒數第 2 個）
//   但若改用 std::reverse：
//       std::reverse(lst.begin(), lst.end());  // → *it 變成 40！
//                                              //   （it 還在第 2 個位置，
//                                              //     但那個位置現在裝的是 40）
//   **兩者結果都「正確」，但如果別的模組持有 iterator，它們的行為天差地遠。**
//   選哪一個要先看語意需求，效能是其次。
//
// 【4. rbegin() / rend()：如果你只是想「反著看」】
//   最常見的情況其實是「我只想反過來走訪一次」，那根本不需要反轉容器：
//       for (auto it = lst.rbegin(); it != lst.rend(); ++it) ...
//   取得反向迭代器是 **O(1)**，而且**完全不修改容器** ——
//   沒有任何寫入、沒有任何失效問題。
//   只有在「需要讓後續所有操作都看到新順序」時，才真的需要 reverse()。
//   實務上很多 reverse() 呼叫其實可以換成 rbegin()/rend()，省下整趟 O(n)。
//
// 【概念補充 Concept Deep Dive】
//
// (A) ★ 本機實測推翻「list::reverse 一定比 std::reverse 快」★
//   直覺上「不搬資料」一定贏，但實測結果取決於**元素大小**
//   （本機 g++ 15.2.0 / -O2 / 200,000 個元素）：
//       元素 = 1028 bytes 的大型物件：
//           list::reverse ≈ 7,000 µs   std::reverse ≈ 22,000 µs → **list 快約 3 倍**
//       元素 = int（4 bytes）：
//           list::reverse ≈ 562 µs     std::reverse ≈ 360 µs    → **std 反而快**
//   為什麼小元素時 std::reverse 會贏？
//     * list::reverse 要走訪**全部 n 個節點**，每個節點對調 prev/next
//       = 每節點寫入 16 bytes 的指標。
//     * std::reverse 只做 **n/2 次**交換，從兩端往中間夾；
//       對 int 而言每次交換只搬 4 bytes，還少走一半的節點。
//   所以：**元素越大，list::reverse 的優勢越明顯；元素小到跟指標差不多時，
//   std::reverse 反而可能更快。**
//   但請記得第 3 點 —— 這兩者語意不同，效能不該是唯一的選擇依據。
//
// (B) 為什麼 list 有自己的 reverse，而 vector 沒有
//   vector 沒有 reverse() 成員，因為對連續容器來說「反轉連線」不存在 ——
//   元素的順序就是記憶體順序，唯一的做法就是交換值，
//   而那正是 std::reverse 已經做得很好的事，不需要重複提供。
//   list 則因為「改連線」這個更好的做法只有它自己知道怎麼做
//   （泛型演算法看不到節點指標），所以必須提供成員版本。
//   這是 STL 的通則：**當容器能用內部知識做得比泛型演算法更好時，
//   它就會提供同名的成員函式**（同理還有 list::sort、list::merge、
//   list::remove、set::find 等）。
//
// (C) forward_list 的情況不同
//   forward_list 只有前向迭代器，而 std::reverse 需要**雙向**迭代器，
//   所以 **std::reverse 根本不能用在 forward_list 上**（編譯會失敗）。
//   forward_list 因此也提供自己的 reverse() 成員 —— 對它而言那是唯一選擇。
//   list 則兩者皆可用，才有了本課的選擇問題。
//
// (D) reverse 之後迭代器的「位置」怎麼變
//   原本指向第 k 個元素的迭代器，reverse 後會指向倒數第 k 個 —— 因為它跟著
//   元素走，而該元素的位置翻轉了。本檔 main() 有實測驗證。
//   一個容易踩的細節：**end() 不是元素**，它是尾後哨兵。
//   reverse 前後 end() 都還是 end()，但 `prev(end())` 指的元素換了。
//
// 【注意事項 Pay Attention】
// 1. list::reverse **不回傳任何東西**（void）。寫 `lst = lst.reverse();`
//    會編譯失敗。它是就地（in-place）修改。
// 2. reverse **不會**讓任何 iterator / reference / pointer 失效。
//    這與 vector 的任何重排操作都不同。
// 3. 空 list 與單元素 list 呼叫 reverse 是完全合法的 no-op，不會有問題。
// 4. 只是想反著讀一次 → 用 rbegin()/rend()，O(1) 且不修改容器，
//    不要為此付一趟 O(n) 的 reverse。
// 5. list::reverse 與 std::reverse **語意不同**（跟著元素 vs 跟著位置），
//    在有外部持有迭代器時不可互換。
// 6. 「list::reverse 一定比較快」是錯的 —— 元素小時 std::reverse 可能更快
//    （本機實測 int 元素：562µs vs 360µs）。
// 7. 效能數字皆為**本機實測，每次執行、每台機器都不同**，只看趨勢。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】list::reverse
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. list::reverse() 是怎麼實作的？複雜度多少？會讓迭代器失效嗎？
//     答：它把每個節點的 prev / next 指標對調，**完全不搬移元素資料**，
//         複雜度 O(n)（每個節點處理一次），而且是 noexcept
//         （沒有配置、沒有呼叫使用者程式碼）。
//         **不會**讓任何 iterator / reference / pointer 失效 ——
//         它們仍指向原本那個元素，只是該元素在鏈中的位置翻轉了。
//     追問：那原本指向第 2 個元素的迭代器，reverse 後指向哪裡？
//         → 指向**倒數第 2 個**，因為它跟著元素走，而該元素的位置翻轉了。
//           值不變、位址不變，只有「在序列中的位置」改變。
//
// 🔥 Q2. list::reverse() 和 std::reverse(l.begin(), l.end()) 差在哪？
//     答：差在**語意**，不只是效能。
//         list::reverse 改的是**連線** → 迭代器跟著元素走：
//             反轉後 *it 的值不變，但它變成倒數第 k 個。
//         std::reverse 改的是**值**（從兩端往中間 iter_swap）→ 迭代器位置不動：
//             反轉後 it 還在第 k 個位置，但 *it 的值被換成別的了。
//         若有其他模組持有迭代器/指標，兩者行為天差地遠，不可互換。
//     追問：那該用哪一個？
//         → 先看語意需求。若無外部迭代器、只是要反轉內容，再看效能：
//           元素大 → list::reverse 明顯快；元素小（如 int）→ 兩者接近，
//           本機實測 std::reverse 甚至更快。
//
// 🔥 Q3. 為什麼 vector 沒有 reverse() 成員，list 卻有？
//     答：因為 list 能用「改連線」這個泛型演算法做不到的方式完成反轉 ——
//         std::reverse 只能透過迭代器介面交換值，看不到節點指標。
//         vector 則不存在「連線」可改，元素順序就是記憶體順序，
//         唯一做法就是交換值，而那正是 std::reverse 已經做得很好的事，
//         沒必要重複提供。
//     追問：STL 裡還有哪些同樣模式的成員函式？
//         → list::sort（歸併 + 指標重接，std::sort 需要 random access
//           所以根本不能用在 list 上）、list::merge、list::remove、
//           以及 set/map::find（利用樹狀結構做 O(log n)，
//           而 std::find 是 O(n) 線性掃描）。
//           通則：**容器能用內部知識做得更好時，就會提供同名成員函式。**
//
// ⚠️ 陷阱 1. 「list::reverse 不搬資料，所以它一定比 std::reverse 快」——對嗎？
//     答：**不一定**。本機 -O2 實測 200,000 個元素：
//         元素 = 1028 bytes 大型物件：list::reverse ≈ 7,000µs、
//             std::reverse ≈ 22,000µs → list 快約 3 倍（符合直覺）
//         元素 = int（4 bytes）      ：list::reverse ≈ 562µs、
//             std::reverse ≈ 360µs   → **std::reverse 反而更快**
//         原因：list::reverse 要走訪**全部 n 個節點**、每個對調 prev/next
//         （每節點寫 16 bytes 指標）；std::reverse 只做 **n/2 次**交換、
//         從兩端往中間夾，對 int 每次只搬 4 bytes，還少走一半節點。
//     為什麼會錯：把「不搬資料」直接等同於「比較快」，卻忽略了
//         「改指標」本身也是寫入，而且 list::reverse 訪問的節點數是
//         std::reverse 的兩倍。元素越小，這個劣勢越明顯。
//
// ⚠️ 陷阱 2. 只是想「從後往前印一次」，寫 lst.reverse() 再走訪 —— 有什麼問題？
//     答：兩個問題。第一，你**修改了容器**：後續所有程式碼看到的順序都變了，
//         若別處假設原順序就會出錯。第二，你付了一趟 **O(n)** 的成本，
//         而 rbegin()/rend() 取得反向迭代器是 **O(1)** 且完全不動容器。
//         正解：for (auto it = lst.rbegin(); it != lst.rend(); ++it) ...
//         若真的只是要印出來，連反轉都不需要。
//     為什麼會錯：把「我想反著看」和「我要把它變成反的」當成同一件事。
//         前者是**唯讀的視圖需求**，後者是**破壞性的狀態變更**。
//         這個區別在唯讀函式（const 成員）裡尤其關鍵 ——
//         reverse() 根本不能在 const list 上呼叫，rbegin() 則可以。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <list>
#include <string>
#include <vector>
#include <algorithm>
#include <chrono>
using namespace std;

template <typename T>
void print_list(const string& label, const list<T>& lst) {
    cout << label << " [" << lst.size() << "]:";
    for (const auto& val : lst) cout << " " << val;
    if (lst.empty()) cout << " (空)";
    cout << endl;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 1】LeetCode 206. Reverse Linked List
//   題目：反轉一條單向鏈結串列，回傳新的頭節點。
//   為什麼用到本主題：這題就是「手寫一遍 list::reverse 的內部原理」。
//         標準庫替你做掉的事，這題要你自己做 —— 三根指標
//         （prev / curr / next）逐節點把箭頭反過來。
//         寫過這題之後，「reverse 只改指標、不搬資料」就不再是背誦，
//         而是你親手實作過的東西。
//   複雜度：時間 O(n)，空間 O(1)。
// -----------------------------------------------------------------------------
struct ListNode {
    int       val;
    ListNode* next;
    explicit ListNode(int v) : val(v), next(nullptr) {}
};

ListNode* reverseList(ListNode* head) {
    ListNode* prev = nullptr;
    ListNode* curr = head;
    while (curr) {
        ListNode* nextTmp = curr->next;   // 先記住下一個，否則改完就找不到了
        curr->next = prev;                // ★ 把箭頭反過來（唯一真正的動作）
        prev = curr;                      // prev 前進
        curr = nextTmp;                   // curr 前進
    }
    return prev;                          // 走完後 prev 是新的頭
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 2】LeetCode 92. Reverse Linked List II
//   題目：只反轉位置 left 到 right 之間的節點（1-indexed），其餘保持原樣。
//   為什麼用到本主題：這題是 206 的進階版，對應到「只反轉 list 的一段」。
//         注意 std::list **沒有**提供「反轉子區間」的成員函式 ——
//         真要做的話得用 splice 把該段切出來、reverse、再 splice 回去
//         （本檔第 7 節示範）。這題則是手寫指標版本。
//   做法：用 dummy 節點簡化邊界，找到 left 的前一個節點，
//         再用「頭插法」把 [left, right] 逐個往前搬。
//   複雜度：時間 O(n)，空間 O(1)。
// -----------------------------------------------------------------------------
ListNode* reverseBetween(ListNode* head, int left, int right) {
    if (!head || left == right) return head;

    ListNode dummy(0);
    dummy.next = head;
    ListNode* prev = &dummy;
    for (int i = 1; i < left; ++i) prev = prev->next;   // prev = left 的前一個

    ListNode* curr = prev->next;                       // 這個節點會一路往後沉
    for (int i = 0; i < right - left; ++i) {
        ListNode* moved = curr->next;                  // 取出 curr 後面那個
        curr->next  = moved->next;                     // 把它從鏈上摘掉
        moved->next = prev->next;                      // 插到 prev 之後（頭插）
        prev->next  = moved;
    }
    return dummy.next;
}

// 教學小工具：把 std::list 轉成手寫鏈結串列（節點存活於 pool）
ListNode* buildList(const vector<int>& vals, vector<ListNode>& pool) {
    pool.clear();
    pool.reserve(vals.size());          // 必要：避免 reallocation 讓指標失效
    for (int v : vals) pool.emplace_back(v);
    for (size_t i = 0; i + 1 < pool.size(); ++i) pool[i].next = &pool[i + 1];
    return pool.empty() ? nullptr : &pool[0];
}

string dumpNodes(const ListNode* head) {
    string s;
    for (const ListNode* p = head; p; p = p->next) {
        if (!s.empty()) s += "->";
        s += to_string(p->val);
    }
    return s.empty() ? "(空)" : s;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】聊天訊息時間軸：儲存用「新→舊」，顯示用「舊→新」
//   情境：聊天室後端收到新訊息時 push_front（最新的在最前面），
//         因為「取最近 N 則」是最高頻的查詢。
//         但畫面上要由上而下依時間**正序**顯示（舊的在上）。
//   為什麼用到本主題：這正是「我想反著看」vs「我要把它變成反的」的實際分野 ——
//     * 只是要正序**顯示一次** → 用 rbegin()/rend()，O(1) 取得、不動資料。
//       這是絕大多數情況的正解。
//     * 使用者按下「改為正序排列」並希望**後續所有操作都用新順序**
//       → 這時才真的呼叫 reverse()，付一趟 O(n)。
//   注意：訊息物件含文字、附件中繼資料，複製昂貴 —— list::reverse 不搬資料，
//         這裡正是它的主場（相對於 std::reverse 會交換整個訊息物件）。
// -----------------------------------------------------------------------------
struct Message {
    string sender;
    string text;
};

ostream& operator<<(ostream& os, const Message& m) {
    return os << m.sender << ": " << m.text;
}

class ChatTimeline {
    list<Message> messages_;   // 最前面 = 最新
public:
    void receive(const string& sender, const string& text) {
        messages_.push_front(Message{sender, text});   // 新訊息在最前，O(1)
    }
    // 唯讀顯示：正序（舊→新）。用反向迭代器，不修改任何東西
    void renderChronological() const {
        int n = 1;
        for (auto it = messages_.rbegin(); it != messages_.rend(); ++it) {
            cout << "    " << n++ << ". " << *it << endl;
        }
    }
    // 取最近 k 則（儲存順序本來就是新→舊，直接取前 k 個）
    void renderRecent(size_t k) const {
        size_t n = 0;
        for (const auto& m : messages_) {
            if (n++ >= k) break;
            cout << "    " << m << endl;
        }
    }
    // 真的要改變儲存順序時才呼叫（破壞性，O(n)）
    void flipStorageOrder() { messages_.reverse(); }

    const Message& front() const { return messages_.front(); }
    size_t size() const { return messages_.size(); }
};

int main() {
    // ===== 1. 基本 reverse =====
    cout << "===== 1. 基本 reverse =====" << endl;
    {
        list<int> lst = {1, 2, 3, 4, 5};
        print_list("反轉前", lst);
        lst.reverse();
        print_list("反轉後", lst);
        cout << "→ reverse() 回傳 void，是就地修改（寫 lst = lst.reverse() 會編譯失敗）" << endl;
    }

    // ===== 2. 邊界情況 =====
    cout << "\n===== 2. 邊界情況（都是合法的 no-op）=====" << endl;
    {
        list<int> empty_lst;
        empty_lst.reverse();
        print_list("空 list   ", empty_lst);

        list<int> single = {42};
        single.reverse();
        print_list("單元素    ", single);

        list<int> two = {10, 20};
        two.reverse();
        print_list("兩元素反轉", two);
    }

    // ===== 3. 迭代器穩定性驗證 =====
    cout << "\n===== 3. 迭代器穩定性：值與位址都不變 =====" << endl;
    {
        list<int> lst = {10, 20, 30, 40, 50};

        auto it_20 = lst.begin();
        advance(it_20, 1);    // 指向 20
        auto it_40 = lst.begin();
        advance(it_40, 3);    // 指向 40

        cout << "reverse 前：" << endl;
        cout << "  *it_20 = " << *it_20 << "  位址: "
             << static_cast<const void*>(&(*it_20)) << endl;
        cout << "  *it_40 = " << *it_40 << "  位址: "
             << static_cast<const void*>(&(*it_40)) << endl;

        lst.reverse();
        print_list("reverse 後 list", lst);

        cout << "reverse 後：" << endl;
        cout << "  *it_20 = " << *it_20 << "  位址: "
             << static_cast<const void*>(&(*it_20)) << endl;
        cout << "  *it_40 = " << *it_40 << "  位址: "
             << static_cast<const void*>(&(*it_40)) << endl;
        cout << "（值和位址都不變 —— 只有「在序列中的位置」改變了）" << endl;

        // 驗證：原本第 2 個的 it_20，現在是倒數第 2 個
        auto check = lst.end();
        advance(check, -2);
        cout << "  倒數第 2 個元素 = " << *check << endl;
        cout << "  it_20 == 倒數第 2 個? " << boolalpha << (check == it_20) << endl;
        cout << "  ★ 迭代器跟著元素走，不是跟著位置走" << endl;
    }

    // ===== 4. list::reverse vs std::reverse 的語意差異 =====
    cout << "\n===== 4. list::reverse vs std::reverse（語意完全不同）=====" << endl;
    {
        list<int> lst = {10, 20, 30, 40, 50};
        auto it_lst = lst.begin();
        advance(it_lst, 1);
        cout << "list::reverse（改連線）:" << endl;
        cout << "  反轉前 *it = " << *it_lst << endl;
        lst.reverse();
        cout << "  反轉後 *it = " << *it_lst << "  ← 值不變，它變成倒數第 2 個" << endl;

        list<int> lst2 = {10, 20, 30, 40, 50};
        auto it_lst2 = lst2.begin();
        advance(it_lst2, 1);
        cout << "\nstd::reverse（交換值）:" << endl;
        cout << "  反轉前 *it = " << *it_lst2 << endl;
        std::reverse(lst2.begin(), lst2.end());
        cout << "  反轉後 *it = " << *it_lst2 << "  ← 位置不變，值被換掉了" << endl;

        print_list("\n  list::reverse 結果", lst);
        print_list("  std::reverse 結果 ", lst2);
        cout << "  ★ 兩者最終容器內容相同，但對「持有迭代器的人」影響完全不同" << endl;
    }

    // ===== 5. rbegin/rend：只想反著看時的正解 =====
    cout << "\n===== 5. rbegin()/rend()：O(1) 取得，不修改容器 =====" << endl;
    {
        const list<int> lst = {1, 2, 3, 4, 5};   // 注意：const，不可能呼叫 reverse()
        cout << "  正向走訪:";
        for (int v : lst) cout << " " << v;
        cout << endl;
        cout << "  反向走訪:";
        for (auto it = lst.rbegin(); it != lst.rend(); ++it) cout << " " << *it;
        cout << endl;
        print_list("  容器本身完全沒變", lst);
        cout << "  ★ const list 不能呼叫 reverse()，但可以用 rbegin()/rend()" << endl;
        cout << "  ★ 只是要反著讀一次，就不該付 O(n) 去真的反轉" << endl;
    }

    // ===== 6. 效能比較：元素大小決定勝負 =====
    cout << "\n===== 6. 效能比較（本機實測，每次執行都不同）=====" << endl;
    {
        struct BigObject {
            char data[1024];
            int  id;
            explicit BigObject(int i = 0) : id(i) { data[0] = 0; }
        };

        const int N = 200000;
        cout << "  元素數量 = " << N << endl;

        // --- 大型物件（1028 bytes）---
        {
            list<BigObject> lst;
            for (int i = 0; i < N; i++) lst.emplace_back(i);

            auto t1 = chrono::high_resolution_clock::now();
            lst.reverse();
            auto t2 = chrono::high_resolution_clock::now();
            auto us1 = chrono::duration_cast<chrono::microseconds>(t2 - t1).count();

            auto t3 = chrono::high_resolution_clock::now();
            std::reverse(lst.begin(), lst.end());
            auto t4 = chrono::high_resolution_clock::now();
            auto us2 = chrono::duration_cast<chrono::microseconds>(t4 - t3).count();

            cout << "  --- 大型物件 (sizeof = " << sizeof(BigObject) << " bytes) ---" << endl;
            cout << "    list::reverse : " << us1 << " us  (只對調指標)" << endl;
            cout << "    std::reverse  : " << us2 << " us  (交換整個物件)" << endl;
            cout << "    → 元素越大，list::reverse 的優勢越明顯" << endl;
            cout << "    驗證 front().id = " << lst.front().id << "（反轉兩次回到原順序）" << endl;
        }

        // --- 小型元素（int）---
        {
            list<int> lst;
            for (int i = 0; i < N; i++) lst.push_back(i);

            auto t1 = chrono::high_resolution_clock::now();
            lst.reverse();
            auto t2 = chrono::high_resolution_clock::now();
            auto us1 = chrono::duration_cast<chrono::microseconds>(t2 - t1).count();

            auto t3 = chrono::high_resolution_clock::now();
            std::reverse(lst.begin(), lst.end());
            auto t4 = chrono::high_resolution_clock::now();
            auto us2 = chrono::duration_cast<chrono::microseconds>(t4 - t3).count();

            cout << "  --- 小型元素 (int, 4 bytes) ---" << endl;
            cout << "    list::reverse : " << us1 << " us  (走訪全部 n 個節點)" << endl;
            cout << "    std::reverse  : " << us2 << " us  (只做 n/2 次交換)" << endl;
            cout << "    ★ 反直覺：元素小的時候 std::reverse 反而可能更快" << endl;
        }
    }

    // ===== 7. 實際應用：迴文檢測 =====
    cout << "\n===== 7. 應用：迴文檢測 =====" << endl;
    {
        auto is_palindrome = [](const list<int>& lst) {
            list<int> rev = lst;   // 複製一份（原本的不能動）
            rev.reverse();
            return lst == rev;
        };

        const list<int> cases[] = {
            {1, 2, 3, 2, 1},
            {1, 2, 3, 4, 5},
            {1},
            {},
        };
        const char* names[] = {"{1,2,3,2,1}", "{1,2,3,4,5}", "{1}       ", "{}        "};
        for (int i = 0; i < 4; ++i) {
            cout << "  " << names[i] << " 迴文? "
                 << (is_palindrome(cases[i]) ? "是" : "否") << endl;
        }
        cout << "  ★ 注意這裡必須先複製 —— reverse() 是破壞性操作" << endl;
    }

    // ===== 8. 組合技：reverse + splice 反轉一個子區間 =====
    cout << "\n===== 8. 組合技：用 splice + reverse 反轉子區間 =====" << endl;
    {
        list<int> lst = {1, 2, 3, 4, 5, 6, 7, 8};
        print_list("原始    ", lst);

        // std::list 沒有「反轉子區間」的成員函式，
        // 要做就是：切出來 → reverse → 接回去（全部 O(1) 的 splice + O(k) 的 reverse）
        auto mid = lst.begin();
        advance(mid, 4);   // 指向 5

        list<int> second_half;
        second_half.splice(second_half.begin(), lst, mid, lst.end());
        print_list("前半段  ", lst);
        print_list("後半段  ", second_half);

        second_half.reverse();
        print_list("反轉後半", second_half);

        lst.splice(lst.end(), second_half);
        print_list("合併結果", lst);
        cout << "  ★ 這就是 LeetCode 92（反轉子區間）在 STL 世界的作法" << endl;
    }

    // ===== 9. LeetCode 206 =====
    cout << "\n===== 9. LeetCode 206. Reverse Linked List =====" << endl;
    {
        vector<ListNode> pool;
        ListNode* head = buildList({1, 2, 3, 4, 5}, pool);
        cout << "  反轉前: " << dumpNodes(head) << endl;
        head = reverseList(head);
        cout << "  反轉後: " << dumpNodes(head) << endl;

        vector<ListNode> pool2;
        ListNode* h2 = buildList({}, pool2);
        cout << "  空串列: " << dumpNodes(reverseList(h2)) << endl;

        vector<ListNode> pool3;
        ListNode* h3 = buildList({1}, pool3);
        cout << "  單節點: " << dumpNodes(reverseList(h3)) << endl;
        cout << "  ★ 這就是 list::reverse 內部在做的事：逐節點把箭頭反過來" << endl;
    }

    // ===== 10. LeetCode 92 =====
    cout << "\n===== 10. LeetCode 92. Reverse Linked List II =====" << endl;
    {
        vector<ListNode> pool;
        ListNode* head = buildList({1, 2, 3, 4, 5}, pool);
        cout << "  原始           : " << dumpNodes(head) << endl;
        head = reverseBetween(head, 2, 4);
        cout << "  反轉位置 [2,4] : " << dumpNodes(head) << endl;

        vector<ListNode> pool2;
        ListNode* h2 = buildList({5}, pool2);
        cout << "  單節點 [1,1]   : " << dumpNodes(reverseBetween(h2, 1, 1)) << endl;

        vector<ListNode> pool3;
        ListNode* h3 = buildList({3, 5}, pool3);
        cout << "  兩節點 [1,2]   : " << dumpNodes(reverseBetween(h3, 1, 2)) << endl;
    }

    // ===== 11. 日常實務：聊天訊息時間軸 =====
    cout << "\n===== 11. 日常實務：聊天訊息時間軸 =====" << endl;
    {
        ChatTimeline chat;
        chat.receive("Alice", "早安！");
        chat.receive("Bob",   "早～今天的會議改到三點");
        chat.receive("Alice", "收到，我改行事曆");
        chat.receive("Carol", "我這邊的報表已經上傳了");
        chat.receive("Bob",   "太好了，謝謝");

        cout << "  儲存順序是「新→舊」（push_front，O(1)）" << endl;
        cout << "  最新一則: " << chat.front() << endl;

        cout << "\n  取最近 2 則（直接取前 2 個，不必反轉）:" << endl;
        chat.renderRecent(2);

        cout << "\n  正序顯示（rbegin/rend，O(1) 取得、完全不修改容器）:" << endl;
        chat.renderChronological();

        cout << "\n  使用者按下「改為正序儲存」→ 這時才真的 reverse()（O(n)）:" << endl;
        chat.flipStorageOrder();
        cout << "  現在最前面的是: " << chat.front() << endl;
        cout << "  ★ 「想反著看」用 rbegin()；「要把它變成反的」才用 reverse()" << endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -O2 -Wall -Wextra 第\ 36\ 課：list\ 的特有操作——reverse1.cpp -o list_reverse1
//   （不加 -O2 也能編譯且零警告，但第 6 節的效能數字會失真）

// ※ 第 6 節的耗時與第 3 節的節點位址（0x...）皆為**本機實測，每次執行都不同**
// ===== 6. 效能比較（本機實測，每次執行都不同）=====

// === 預期輸出 ===
//   （Dell Precision 7550 / Ubuntu 26.04 / g++ 15.2.0 / -O2 編譯）。
//   請只看趨勢，不要當成固定值或效能保證：
//     大型物件（1028 bytes）→ list::reverse 明顯快（本機約 3 倍）
//     小型元素（int）        → std::reverse 反而略快
//   本機重複執行的區間：大型物件 list 6,573~7,450µs / std 21,220~22,896µs；
//   int 元素 list 407~562µs / std 253~360µs。
//
// ===== 1. 基本 reverse =====
// 反轉前 [5]: 1 2 3 4 5
// 反轉後 [5]: 5 4 3 2 1
// → reverse() 回傳 void，是就地修改（寫 lst = lst.reverse() 會編譯失敗）
//
// ===== 2. 邊界情況（都是合法的 no-op）=====
// 空 list    [0]: (空)
// 單元素     [1]: 42
// 兩元素反轉 [2]: 20 10
//
// ===== 3. 迭代器穩定性：值與位址都不變 =====
// reverse 前：
//   *it_20 = 20  位址: 0x5b0b6dced060
//   *it_40 = 40  位址: 0x5b0b6dced0a0
// reverse 後 list [5]: 50 40 30 20 10
// reverse 後：
//   *it_20 = 20  位址: 0x5b0b6dced060
//   *it_40 = 40  位址: 0x5b0b6dced0a0
// （值和位址都不變 —— 只有「在序列中的位置」改變了）
//   倒數第 2 個元素 = 20
//   it_20 == 倒數第 2 個? true
//   ★ 迭代器跟著元素走，不是跟著位置走
//
// ===== 4. list::reverse vs std::reverse（語意完全不同）=====
// list::reverse（改連線）:
//   反轉前 *it = 20
//   反轉後 *it = 20  ← 值不變，它變成倒數第 2 個
//
// std::reverse（交換值）:
//   反轉前 *it = 20
//   反轉後 *it = 40  ← 位置不變，值被換掉了
//
//   list::reverse 結果 [5]: 50 40 30 20 10
//   std::reverse 結果  [5]: 50 40 30 20 10
//   ★ 兩者最終容器內容相同，但對「持有迭代器的人」影響完全不同
//
// ===== 5. rbegin()/rend()：O(1) 取得，不修改容器 =====
//   正向走訪: 1 2 3 4 5
//   反向走訪: 5 4 3 2 1
//   容器本身完全沒變 [5]: 1 2 3 4 5
//   ★ const list 不能呼叫 reverse()，但可以用 rbegin()/rend()
//   ★ 只是要反著讀一次，就不該付 O(n) 去真的反轉
//
//   元素數量 = 200000
//   --- 大型物件 (sizeof = 1028 bytes) ---
//     list::reverse : 6573 us  (只對調指標)
//     std::reverse  : 22530 us  (交換整個物件)
//     → 元素越大，list::reverse 的優勢越明顯
//     驗證 front().id = 0（反轉兩次回到原順序）
//   --- 小型元素 (int, 4 bytes) ---
//     list::reverse : 407 us  (走訪全部 n 個節點)
//     std::reverse  : 253 us  (只做 n/2 次交換)
//     ★ 反直覺：元素小的時候 std::reverse 反而可能更快
//
// ===== 7. 應用：迴文檢測 =====
//   {1,2,3,2,1} 迴文? 是
//   {1,2,3,4,5} 迴文? 否
//   {1}        迴文? 是
//   {}         迴文? 是
//   ★ 注意這裡必須先複製 —— reverse() 是破壞性操作
//
// ===== 8. 組合技：用 splice + reverse 反轉子區間 =====
// 原始     [8]: 1 2 3 4 5 6 7 8
// 前半段   [4]: 1 2 3 4
// 後半段   [4]: 5 6 7 8
// 反轉後半 [4]: 8 7 6 5
// 合併結果 [8]: 1 2 3 4 8 7 6 5
//   ★ 這就是 LeetCode 92（反轉子區間）在 STL 世界的作法
//
// ===== 9. LeetCode 206. Reverse Linked List =====
//   反轉前: 1->2->3->4->5
//   反轉後: 5->4->3->2->1
//   空串列: (空)
//   單節點: 1
//   ★ 這就是 list::reverse 內部在做的事：逐節點把箭頭反過來
//
// ===== 10. LeetCode 92. Reverse Linked List II =====
//   原始           : 1->2->3->4->5
//   反轉位置 [2,4] : 1->4->3->2->5
//   單節點 [1,1]   : 5
//   兩節點 [1,2]   : 5->3
//
// ===== 11. 日常實務：聊天訊息時間軸 =====
//   儲存順序是「新→舊」（push_front，O(1)）
//   最新一則: Bob: 太好了，謝謝
//
//   取最近 2 則（直接取前 2 個，不必反轉）:
//     Bob: 太好了，謝謝
//     Carol: 我這邊的報表已經上傳了
//
//   正序顯示（rbegin/rend，O(1) 取得、完全不修改容器）:
//     1. Alice: 早安！
//     2. Bob: 早～今天的會議改到三點
//     3. Alice: 收到，我改行事曆
//     4. Carol: 我這邊的報表已經上傳了
//     5. Bob: 太好了，謝謝
//
//   使用者按下「改為正序儲存」→ 這時才真的 reverse()（O(n)）:
//   現在最前面的是: Alice: 早安！
//   ★ 「想反著看」用 rbegin()；「要把它變成反的」才用 reverse()
