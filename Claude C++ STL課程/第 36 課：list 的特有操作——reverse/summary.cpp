// =============================================================================
//  第 36 課 總結：list 的特有操作——reverse  —  本課的教科書
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：#include <list>
//   簽名：  void reverse() noexcept;      ← 成員函式、無回傳值、noexcept
//   複雜度：O(n)（每個節點恰好處理一次）
//   標準版本：C++98 就有；noexcept 標記自 C++11 起
//             （本機實測 noexcept(l.reverse()) == 1）
//
//   ★ 核心：list::reverse **只重接每個節點的 prev/next 指標**，
//     不複製、不移動、不交換任何元素資料。
//     因此 **iterator / reference / pointer 全部保持有效** ——
//     它們仍指向原本那個元素，只是整條鏈的走訪順序反過來了。
//
//   ┌─ 三種「反轉」的比較（語意完全不同，不可混用）────────────────────┐
//   │                   改變什麼   迭代器      成本      修改容器?      │
//   │ lst.reverse()     連線       跟著元素走  O(n)      是             │
//   │ std::reverse(...) 元素的值   位置不動    O(n)      是             │
//   │ rbegin()/rend()   什麼都沒改 —          O(1) 取得 **否**          │
//   └───────────────────────────────────────────────────────────────────┘
//
// 【詳細解釋 Explanation】
//
// 【1. list::reverse 做了什麼】
// list 是雙向鏈結串列。反轉整條鏈只需把**每個節點的 prev 與 next 對調**：
//
//   反轉前：  head ⇄ [10] ⇄ [20] ⇄ [30] ⇄ tail
//   反轉後：  head ⇄ [30] ⇄ [20] ⇄ [10] ⇄ tail
//
// 方框裡的元素從頭到尾沒有搬動過，改的只是箭頭方向。因此：
//   * **工作量**與元素大小無關（都是 n 次指標對調）。
//     但**實際耗時**仍受記憶體局部性影響 —— 大元素的節點在 heap 上相隔較遠，
//     指標追逐的 cache miss 較多（本課實測：同樣 200,000 個節點，
//     int 版 ≈ 407µs、1028 bytes 版 ≈ 6,573µs）。
//   * 指向元素的指標依然有效（元素還在原地）。
//   * 不可能拋例外（不配置、不呼叫使用者的建構子）→ noexcept。
//
// 【2. std::reverse 做的是完全不同的事】
// std::reverse 是泛型演算法，看不到節點指標，只能透過迭代器介面
// **從兩端往中間交換「值」**：
//     while (first != last && first != --last) std::iter_swap(first++, last);
// 對 list 而言：連線完全沒變，變的是每個節點裡**裝的元素**；
// 迭代器**位置不變**，但它指的元素被換掉了；成本與元素大小**高度相關**。
//
// 【3. ★ 兩者的差異首先是「語意」，其次才是效能 ★】
//   假設別處持有一個指向某元素的迭代器：
//       auto it = next(lst.begin());          // 第 2 個元素，值 20
//       lst.reverse();                        // → *it 仍是 20（變成倒數第 2 個）
//       std::reverse(lst.begin(), lst.end()); // → *it 變成 40（位置沒動，值被換）
//   兩者最終的容器內容相同，但對「持有迭代器的人」影響天差地遠。
//   **有外部迭代器時，兩者不可互換。**
//
// 【4. rbegin() / rend()：多數情況真正該用的東西】
//   最常見的需求其實是「我只想反過來走訪一次」，那根本不需要反轉容器：
//       for (auto it = lst.rbegin(); it != lst.rend(); ++it) ...
//   取得反向迭代器是 **O(1)**，且**完全不修改容器**。
//   只有「需要讓後續所有操作都看到新順序」時，才真的需要 reverse()。
//   一個明確的訊號：**reverse() 不能在 const list 上呼叫，rbegin() 可以。**
//   如果你的函式是 const 的卻想 reverse，那幾乎肯定該用 rbegin()。
//
// 【5. 為什麼 list 有 reverse 成員而 vector 沒有】
//   vector 不存在「連線」可改 —— 元素順序就是記憶體順序，唯一做法是交換值，
//   而那正是 std::reverse 已經做得很好的事，沒必要重複提供。
//   list 則能用「改連線」這個泛型演算法做不到的方式完成反轉，所以提供成員版本。
//   這是 STL 的通則：**容器能用內部知識做得比泛型演算法更好時，
//   就提供同名的成員函式**。同模式的還有：
//     list::sort   （歸併 + 指標重接；std::sort 需要 random access，
//                    **根本不能用在 list 上**）
//     list::merge / list::remove / list::unique
//     set/map::find（利用樹狀結構 O(log n)，std::find 是 O(n) 線性掃描）
//
// 【概念補充 Concept Deep Dive】
//
// (A) ★ 本機實測推翻「list::reverse 一定比 std::reverse 快」★
//   直覺上「不搬資料」該永遠贏，但實測結果取決於**元素大小**
//   （本機 g++ 15.2.0 / -O2 / 200,000 個元素）：
//       元素 = 1028 bytes 大型物件：
//           list::reverse ≈ 6,573 µs   std::reverse ≈ 22,530 µs → **list 快約 3 倍**
//       元素 = int（4 bytes）：
//           list::reverse ≈   407 µs   std::reverse ≈    253 µs → **std 反而快**
//   為什麼小元素時 std::reverse 會贏？
//     * list::reverse 走訪**全部 n 個節點**，每個對調 prev/next
//       = 每節點寫入 16 bytes 指標。
//     * std::reverse 只做 **n/2 次**交換，從兩端往中間夾；
//       對 int 每次只搬 4 bytes，還少走一半節點。
//   結論：**元素越大，list::reverse 的優勢越明顯；元素小到與指標相當時，
//   std::reverse 反而可能更快。** 但語意差異（第 3 點）才是首要考量。
//
// (B) forward_list 沒有選擇的餘地
//   forward_list 只有**前向**迭代器，而 std::reverse 需要**雙向**迭代器，
//   所以 **std::reverse 根本不能用在 forward_list 上**
//   （本機實測：編譯失敗）。forward_list 因此也提供自己的 reverse() 成員 ——
//   對它而言那是唯一選擇。list 兩者皆可用，才有了本課的選擇問題。
//
// (C) reverse 之後迭代器的「位置」怎麼變
//   原本指向第 k 個元素的迭代器，reverse 後指向**倒數第 k 個** ——
//   因為它跟著元素走，而該元素的位置翻轉了。
//   一個容易踩的細節：**end() 不是元素**，是尾後哨兵。
//   reverse 前後 end() 都還是 end()，但 prev(end()) 指的元素換了。
//
// (D) 沒有「反轉子區間」的成員函式
//   std::list 只能反轉**整條**。要反轉一段（如 LeetCode 92）有兩條路：
//     1. splice 切出來 → reverse → splice 接回去（本課 main() 示範）
//     2. std::reverse(first, last) 直接對子區間交換值
//        —— 但記得它的語意是「值移動、迭代器不動」。
//   兩者複雜度都是 O(k)，選哪個取決於是否有外部迭代器持有那段元素。
//
// 【注意事項 Pay Attention】
// 1. reverse() **回傳 void**。寫 `lst = lst.reverse();` 會編譯失敗，它是就地修改。
// 2. reverse **不會**讓任何 iterator / reference / pointer 失效。
// 3. 空 list 與單元素 list 呼叫 reverse 是完全合法的 no-op。
// 4. 只想反著讀一次 → 用 rbegin()/rend()（O(1)、不修改容器），
//    不要付一趟 O(n) 去真的反轉。
// 5. list::reverse 與 std::reverse **語意不同**，有外部迭代器時不可互換。
// 6. 「list::reverse 一定比較快」是錯的 —— 元素小時 std::reverse 可能更快
//    （本機實測 int：407µs vs 253µs）。
// 7. reverse() 不能在 const list 上呼叫；rbegin() 可以。
// 8. 效能數字皆為**本機實測，每次執行、每台機器都不同**，只看趨勢。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】list::reverse
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. list::reverse() 怎麼實作？複雜度多少？會讓迭代器失效嗎？
//     答：把每個節點的 prev / next 指標對調，**完全不搬移元素資料**；
//         複雜度 O(n)，而且是 noexcept（不配置、不呼叫使用者程式碼）。
//         **不會**讓任何 iterator / reference / pointer 失效 ——
//         它們仍指向原本那個元素，只是該元素在鏈中的位置翻轉了。
//     追問：原本指向第 2 個元素的迭代器，reverse 後指向哪裡？
//         → 指向**倒數第 2 個**。值不變、位址不變，只有「在序列中的位置」改變。
//           因為迭代器跟著**元素**走，不是跟著**位置**走。
//
// 🔥 Q2. list::reverse() 和 std::reverse(l.begin(), l.end()) 差在哪？
//     答：差在**語意**，不只是效能。
//         list::reverse 改**連線** → 迭代器跟著元素走：
//             反轉後 *it 值不變，但它變成倒數第 k 個。
//         std::reverse 改**值**（兩端往中間 iter_swap）→ 迭代器位置不動：
//             反轉後 it 還在第 k 個位置，但 *it 被換成別的值了。
//         最終容器內容相同，但若有模組持有迭代器/指標，行為天差地遠。
//     追問：該用哪一個？
//         → 先看語意需求。無外部迭代器時再看效能：元素大 → list::reverse
//           明顯快（本機 3 倍）；元素小（int）→ 本機實測 std::reverse 反而更快。
//
// 🔥 Q3. 為什麼 vector 沒有 reverse() 成員，list 卻有？
//     答：list 能用「改連線」這個泛型演算法做不到的方式完成反轉
//         （std::reverse 看不到節點指標，只能交換值）。
//         vector 不存在「連線」可改，元素順序就是記憶體順序，唯一做法是交換值，
//         而那正是 std::reverse 已經做得很好的事，沒必要重複提供。
//     追問：STL 裡還有哪些同樣模式？
//         → list::sort（歸併 + 指標重接；std::sort 需要 random access
//           所以**根本不能用在 list 上**）、list::merge / remove / unique，
//           以及 set/map::find（樹狀 O(log n) vs std::find 的 O(n)）。
//           通則：**容器能用內部知識做得更好時，就提供同名成員函式。**
//
// 🔥 Q4. 我只是要把一個 list 從後往前印出來，該怎麼寫？
//     答：用 rbegin() / rend()：
//             for (auto it = lst.rbegin(); it != lst.rend(); ++it) cout << *it;
//         取得反向迭代器是 **O(1)** 且**完全不修改容器**。
//         不要寫 lst.reverse() 再正向走訪 —— 那既修改了容器（後續程式碼看到的
//         順序都變了），又白付一趟 O(n)。
//     追問：怎麼快速判斷自己是不是用錯了？
//         → 看函式能不能是 const。**reverse() 不能在 const list 上呼叫，
//           rbegin() 可以**。如果你在一個「應該是唯讀」的函式裡想 reverse，
//           那幾乎肯定該用 rbegin()。
//
// ⚠️ 陷阱 1. 「list::reverse 不搬資料，所以一定比 std::reverse 快」——對嗎？
//     答：**不一定**。本機 -O2 實測 200,000 個元素：
//         1028 bytes 大型物件：list ≈ 6,573µs、std ≈ 22,530µs → list 快約 3 倍
//         int（4 bytes）      ：list ≈   407µs、std ≈    253µs → **std 反而更快**
//         原因：list::reverse 走訪**全部 n 個節點**、每個對調 prev/next
//         （每節點寫 16 bytes）；std::reverse 只做 **n/2 次**交換、
//         兩端往中間夾，對 int 每次只搬 4 bytes，還少走一半節點。
//     為什麼會錯：把「不搬資料」直接等同於「比較快」，忽略了「改指標」本身
//         也是寫入，而且 list::reverse 訪問的節點數是 std::reverse 的兩倍。
//         元素越小，這個劣勢越明顯。
//
// ⚠️ 陷阱 2. 這段迴文檢測為什麼是錯的？
//         bool isPalindrome(list<int>& lst) {
//             list<int> orig = lst;
//             lst.reverse();
//             return lst == orig;
//         }
//     答：它**把呼叫端的 list 反轉掉了**（參數是非 const 參考）。
//         回傳值雖然正確，但函式產生了呼叫者沒預期的副作用 ——
//         這種「查詢函式卻改了狀態」的 bug 在正式環境極難追查。
//         正解是反轉**複本**、參數用 const 參考：
//             bool isPalindrome(const list<int>& lst) {
//                 list<int> rev = lst; rev.reverse(); return lst == rev;
//             }
//         更好的是完全不複製，直接用正向與反向迭代器對撞比較 —— O(1) 空間。
//     為什麼會錯：reverse() 是**就地、破壞性**的操作，但它讀起來很像
//         「產生一個反轉的結果」。名字是動詞原形（reverse 而非 reversed），
//         這在 STL 裡就是「就地修改」的訊號 ——
//         對照 C++20 的 std::ranges::reverse_view / views::reverse 才是
//         「產生反向視圖」而不修改來源。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <list>
#include <string>
#include <vector>
#include <algorithm>
#include <chrono>
using namespace std;

template <typename T>
void print(const string& label, const list<T>& lst) {
    cout << "  " << label << " [" << lst.size() << "]:";
    for (const auto& v : lst) cout << " " << v;
    if (lst.empty()) cout << " (空)";
    cout << endl;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 1】LeetCode 206. Reverse Linked List
//   題目：反轉一條單向鏈結串列，回傳新的頭節點。
//   為什麼用到本主題：這題就是「手寫一遍 list::reverse 的內部原理」。
//         標準庫替你做掉的事，這題要你自己做 —— 三根指標
//         （prev / curr / next）逐節點把箭頭反過來。
//         寫過之後，「reverse 只改指標、不搬資料」就不再是背誦。
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
        ListNode* nextTmp = curr->next;   // 先記住下一個
        curr->next = prev;                // ★ 把箭頭反過來
        prev = curr;
        curr = nextTmp;
    }
    return prev;                          // 走完後 prev 就是新的頭
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 2】LeetCode 25. Reverse Nodes in k-Group
//   題目：每 k 個節點為一組進行反轉；剩下不足 k 個的部分保持原樣。
//   為什麼用到本主題：這是「分段反轉」的極致版本，對應到 STL 世界就是
//         「用 splice 把每 k 個切出來、reverse、再接回去」（本檔第 7 節示範）。
//         它把本課兩個核心觀念綁在一起：
//           1. 反轉的本質是重接指標
//           2. 反轉「一段」需要先確認該段長度足夠（不足 k 就不動）
//   做法：先數夠 k 個確認可反轉，再對這 k 個做標準的三指標反轉，
//         最後把前後接回去，遞迴/迭代處理下一組。
//   複雜度：時間 O(n)，空間 O(1)。
// -----------------------------------------------------------------------------
ListNode* reverseKGroup(ListNode* head, int k) {
    if (!head || k <= 1) return head;

    ListNode dummy(0);
    dummy.next = head;
    ListNode* groupPrev = &dummy;

    while (true) {
        // 先確認後面還有 k 個節點；不足就結束（剩下的保持原樣）
        ListNode* check = groupPrev;
        for (int i = 0; i < k && check; ++i) check = check->next;
        if (!check) break;

        // 對這一組 k 個節點做標準三指標反轉
        ListNode* groupStart = groupPrev->next;   // 反轉後會變成該組的尾
        ListNode* prev = check->next;             // 接到下一組的頭
        ListNode* curr = groupStart;
        for (int i = 0; i < k; ++i) {
            ListNode* nextTmp = curr->next;
            curr->next = prev;
            prev = curr;
            curr = nextTmp;
        }
        groupPrev->next = prev;                   // 前面接到反轉後的新頭
        groupPrev = groupStart;                   // 舊的頭現在是這組的尾
    }
    return dummy.next;
}

// 教學小工具：建立手寫鏈結串列（節點存活於 pool）
ListNode* buildList(const vector<int>& vals, vector<ListNode>& pool) {
    pool.clear();
    pool.reserve(vals.size());     // 必要：避免 reallocation 讓節點指標失效
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
// 【日常實務範例 1】撤銷歷程（Undo History）：儲存用「新→舊」，匯出用「舊→新」
//   情境：編輯器的 undo stack 最新動作放最前面（push_front，O(1)，
//         因為 undo 永遠從最新的開始）。但要把操作歷程寫進稽核日誌時，
//         必須依時間**正序**輸出。
//   為什麼用到本主題：這是「想反著看」vs「要把它變成反的」的實際分野 ——
//     * 匯出稽核日誌（唯讀、一次性）→ rbegin()/rend()，O(1) 取得、不動資料。
//       而且這個函式可以是 **const**，這是設計正確的訊號。
//     * 真的要改變儲存順序才用 reverse()。
//   注意：動作物件含描述字串與序列化的差異資料，複製昂貴 ——
//         若真要反轉，list::reverse 不搬資料，正是它的主場。
// -----------------------------------------------------------------------------
struct EditAction {
    string kind;      // "insert" / "delete" / "format"
    string detail;
};

ostream& operator<<(ostream& os, const EditAction& a) {
    return os << "[" << a.kind << "] " << a.detail;
}

class UndoHistory {
    list<EditAction> actions_;    // 最前面 = 最新
public:
    void record(const string& kind, const string& detail) {
        actions_.push_front(EditAction{kind, detail});   // O(1)
    }
    // 撤銷：取最新的（就在最前面）
    bool undo(EditAction& out) {
        if (actions_.empty()) return false;
        out = actions_.front();
        actions_.pop_front();
        return true;
    }
    // ★ 稽核匯出：正序。注意這是 const 成員 —— 這裡不可能呼叫 reverse()
    void exportAuditLog() const {
        int n = 1;
        for (auto it = actions_.rbegin(); it != actions_.rend(); ++it) {
            cout << "    " << n++ << ". " << *it << endl;
        }
    }
    size_t size() const { return actions_.size(); }
};

// -----------------------------------------------------------------------------
// 【日常實務範例 2】封包重組：把「後進先出」的分片還原成傳輸順序
//   情境：某些協定的分片會以反向順序抵達（例如從堆疊式緩衝區取出），
//         重組時必須先還原成正確的傳輸順序，**而且之後所有處理都要用新順序**
//         （校驗、解碼、寫檔都依序進行）。
//   為什麼用 reverse() 而不是 rbegin()：
//         這裡不是「看一次」，而是要**永久改變容器的順序**供後續多個階段使用。
//         這正是 reverse() 真正該出場的場合。
//   為什麼用 list 而不是 vector：分片是變長 buffer（複製昂貴），
//         list::reverse 只改指標、完全不搬 payload；
//         若用 vector + std::reverse 會把每個 buffer 物件都交換一遍。
// -----------------------------------------------------------------------------
struct Fragment {
    int    seq;
    string payload;
};

ostream& operator<<(ostream& os, const Fragment& f) {
    return os << "seq" << f.seq << "(" << f.payload.size() << "B)";
}

class PacketReassembler {
    list<Fragment> fragments_;   // 抵達順序（此協定為反向）
public:
    void onFragmentArrived(int seq, size_t bytes) {
        fragments_.push_back(Fragment{seq, string(bytes, 'D')});
    }
    // 還原成傳輸順序：後續所有階段都要用新順序 → 真的需要 reverse()
    void restoreTransmissionOrder() { fragments_.reverse(); }

    bool isOrdered() const {
        int last = -1;
        for (const auto& f : fragments_) {
            if (f.seq < last) return false;
            last = f.seq;
        }
        return true;
    }
    size_t totalBytes() const {
        size_t t = 0;
        for (const auto& f : fragments_) t += f.payload.size();
        return t;
    }
    void dump(const string& label) const { print(label, fragments_); }
};

int main() {
    cout << "========== 一、基本 reverse 與邊界情況 ==========" << endl;
    {
        list<int> lst = {1, 2, 3, 4, 5};
        print("反轉前", lst);
        lst.reverse();
        print("反轉後", lst);

        list<int> empty_lst;
        empty_lst.reverse();
        print("空 list  ", empty_lst);

        list<int> single = {42};
        single.reverse();
        print("單元素   ", single);
        cout << "  ★ reverse() 回傳 void（寫 lst = lst.reverse() 會編譯失敗）" << endl;
        cout << "  ★ 空 / 單元素是合法的 no-op" << endl;
    }

    cout << "\n========== 二、迭代器穩定性：值與位址都不變 ==========" << endl;
    {
        list<int> lst = {10, 20, 30, 40, 50};
        auto it20 = next(lst.begin());
        cout << "  reverse 前 *it20=" << *it20
             << " 位址=" << static_cast<const void*>(&(*it20)) << endl;
        lst.reverse();
        cout << "  reverse 後 *it20=" << *it20
             << " 位址=" << static_cast<const void*>(&(*it20)) << endl;
        print("reverse 後", lst);

        auto check = lst.end();
        advance(check, -2);
        cout << "  它現在是倒數第 2 個嗎? " << boolalpha << (check == it20) << endl;
        cout << "  ★ 迭代器跟著「元素」走，不是跟著「位置」走" << endl;
    }

    cout << "\n========== 三、list::reverse vs std::reverse（語意不同）==========" << endl;
    {
        list<int> lst1 = {10, 20, 30, 40, 50};
        auto it1 = next(lst1.begin());
        cout << "  list::reverse 前 *it=" << *it1 << endl;
        lst1.reverse();
        cout << "  list::reverse 後 *it=" << *it1 << "  ← 值不變，位置變成倒數第 2" << endl;

        list<int> lst2 = {10, 20, 30, 40, 50};
        auto it2 = next(lst2.begin());
        cout << "  std::reverse  前 *it=" << *it2 << endl;
        std::reverse(lst2.begin(), lst2.end());
        cout << "  std::reverse  後 *it=" << *it2 << "  ← 位置不變，值被換掉" << endl;

        print("list::reverse 結果", lst1);
        print("std::reverse  結果", lst2);
        cout << "  ★ 容器內容相同，但對「持有迭代器的人」影響天差地遠" << endl;
    }

    cout << "\n========== 四、rbegin()/rend()：多數情況真正該用的 ==========" << endl;
    {
        const list<int> lst = {1, 2, 3, 4, 5};   // const：不可能呼叫 reverse()
        cout << "  正向:";
        for (int v : lst) cout << " " << v;
        cout << endl;
        cout << "  反向:";
        for (auto it = lst.rbegin(); it != lst.rend(); ++it) cout << " " << *it;
        cout << endl;
        print("容器完全沒變", lst);
        cout << "  ★ O(1) 取得、不修改容器；const list 也能用" << endl;
        cout << "  ★ 判斷訊號：函式能是 const 就該用 rbegin()，不是 reverse()" << endl;
    }

    cout << "\n========== 五、效能：元素大小決定勝負（本機實測，每次都不同）==========" << endl;
    {
        struct BigObject {
            char data[1024];
            int  id;
            explicit BigObject(int i = 0) : id(i) { data[0] = 0; }
        };
        const int N = 200000;
        cout << "  元素數量 = " << N << endl;
        {
            list<BigObject> lst;
            for (int i = 0; i < N; i++) lst.emplace_back(i);
            auto t1 = chrono::high_resolution_clock::now();
            lst.reverse();
            auto t2 = chrono::high_resolution_clock::now();
            auto t3 = chrono::high_resolution_clock::now();
            std::reverse(lst.begin(), lst.end());
            auto t4 = chrono::high_resolution_clock::now();
            cout << "  --- 大型物件 (sizeof=" << sizeof(BigObject) << " bytes) ---" << endl;
            cout << "    list::reverse: "
                 << chrono::duration_cast<chrono::microseconds>(t2 - t1).count()
                 << " us  (只對調指標)" << endl;
            cout << "    std::reverse : "
                 << chrono::duration_cast<chrono::microseconds>(t4 - t3).count()
                 << " us  (交換整個物件)" << endl;
            cout << "    → 元素越大，list::reverse 優勢越明顯" << endl;
        }
        {
            list<int> lst;
            for (int i = 0; i < N; i++) lst.push_back(i);
            auto t1 = chrono::high_resolution_clock::now();
            lst.reverse();
            auto t2 = chrono::high_resolution_clock::now();
            auto t3 = chrono::high_resolution_clock::now();
            std::reverse(lst.begin(), lst.end());
            auto t4 = chrono::high_resolution_clock::now();
            cout << "  --- 小型元素 (int, 4 bytes) ---" << endl;
            cout << "    list::reverse: "
                 << chrono::duration_cast<chrono::microseconds>(t2 - t1).count()
                 << " us  (走訪全部 n 個節點)" << endl;
            cout << "    std::reverse : "
                 << chrono::duration_cast<chrono::microseconds>(t4 - t3).count()
                 << " us  (只做 n/2 次交換)" << endl;
            cout << "    ★ 反直覺：元素小時 std::reverse 反而可能更快" << endl;
        }
    }

    cout << "\n========== 六、迴文檢測（示範正確與錯誤的寫法）==========" << endl;
    {
        // 正確：參數 const 參考，反轉的是複本
        auto isPalindromeOK = [](const list<int>& lst) {
            list<int> rev = lst;
            rev.reverse();
            return lst == rev;
        };
        // 更好：完全不複製，正反迭代器對撞，O(1) 空間
        auto isPalindromeBest = [](const list<int>& lst) {
            auto f = lst.begin();
            auto r = lst.rbegin();
            for (size_t i = 0; i < lst.size() / 2; ++i, ++f, ++r)
                if (*f != *r) return false;
            return true;
        };

        const list<int> cases[] = {{1, 2, 3, 2, 1}, {1, 2, 3, 4, 5}, {1}, {}};
        const char* names[] = {"{1,2,3,2,1}", "{1,2,3,4,5}", "{1}       ", "{}        "};
        for (int i = 0; i < 4; ++i) {
            cout << "  " << names[i]
                 << "  複本法: " << (isPalindromeOK(cases[i])   ? "是" : "否")
                 << "   對撞法: " << (isPalindromeBest(cases[i]) ? "是" : "否") << endl;
        }
        cout << "  ★ 絕不可直接 reverse 傳進來的 list —— 那會改到呼叫端的資料" << endl;
        cout << "  ★ 對撞法連複本都不用，是這題真正的最佳解" << endl;
    }

    cout << "\n========== 七、組合技：splice + reverse 反轉子區間 ==========" << endl;
    {
        list<int> lst = {1, 2, 3, 4, 5, 6, 7, 8};
        print("原始    ", lst);
        auto mid = lst.begin();
        advance(mid, 4);
        list<int> half2;
        half2.splice(half2.begin(), lst, mid, lst.end());   // 切出後半
        half2.reverse();                                    // 反轉
        lst.splice(lst.end(), half2);                       // 接回去
        print("後半反轉", lst);
        cout << "  ★ std::list 沒有「反轉子區間」的成員函式，這是標準作法" << endl;
        cout << "  ★ 這也正是 LeetCode 25（k 個一組反轉）的 STL 版思路" << endl;
    }

    cout << "\n========== 八、LeetCode 206. Reverse Linked List ==========" << endl;
    {
        vector<ListNode> pool;
        ListNode* head = buildList({1, 2, 3, 4, 5}, pool);
        cout << "  反轉前: " << dumpNodes(head) << endl;
        head = reverseList(head);
        cout << "  反轉後: " << dumpNodes(head) << endl;

        vector<ListNode> p2, p3;
        cout << "  空串列: " << dumpNodes(reverseList(buildList({}, p2)))  << endl;
        cout << "  單節點: " << dumpNodes(reverseList(buildList({7}, p3))) << endl;
        cout << "  ★ 這就是 list::reverse 內部在做的事：逐節點把箭頭反過來" << endl;
    }

    cout << "\n========== 九、LeetCode 25. Reverse Nodes in k-Group ==========" << endl;
    {
        vector<ListNode> p1;
        ListNode* h1 = buildList({1, 2, 3, 4, 5}, p1);
        cout << "  [1,2,3,4,5] k=2 → " << dumpNodes(reverseKGroup(h1, 2)) << endl;

        vector<ListNode> p2;
        ListNode* h2 = buildList({1, 2, 3, 4, 5}, p2);
        cout << "  [1,2,3,4,5] k=3 → " << dumpNodes(reverseKGroup(h2, 3)) << endl;

        vector<ListNode> p3;
        ListNode* h3 = buildList({1, 2, 3, 4, 5}, p3);
        cout << "  [1,2,3,4,5] k=1 → " << dumpNodes(reverseKGroup(h3, 1)) << endl;

        vector<ListNode> p4;
        ListNode* h4 = buildList({1, 2, 3, 4, 5, 6}, p4);
        cout << "  [1..6]      k=3 → " << dumpNodes(reverseKGroup(h4, 3)) << endl;
        cout << "  ★ 不足 k 個的尾巴保持原樣（k=3 時最後的 5 沒被反轉）" << endl;
    }

    cout << "\n========== 十、日常實務：編輯器撤銷歷程 ==========" << endl;
    {
        UndoHistory hist;
        hist.record("insert", "在第 12 行插入 'return 0;'");
        hist.record("delete", "刪除第 5 行的除錯輸出");
        hist.record("format", "整份重新縮排");
        hist.record("insert", "加入標頭檔 <list>");

        cout << "  儲存順序是「新→舊」（push_front，O(1)）" << endl;
        cout << "  目前有 " << hist.size() << " 筆動作" << endl;

        cout << "\n  稽核日誌（正序，用 rbegin/rend；這是 const 成員）:" << endl;
        hist.exportAuditLog();

        EditAction undone;
        hist.undo(undone);
        cout << "\n  按下 Ctrl+Z → 撤銷: " << undone << endl;
        cout << "  剩下 " << hist.size() << " 筆" << endl;
        cout << "  ★ exportAuditLog() 能宣告成 const，正是「該用 rbegin」的訊號" << endl;
    }

    cout << "\n========== 十一、日常實務：封包分片重組 ==========" << endl;
    {
        PacketReassembler pkt;
        // 此協定的分片以反向順序抵達
        pkt.onFragmentArrived(4, 512);
        pkt.onFragmentArrived(3, 1024);
        pkt.onFragmentArrived(2, 1024);
        pkt.onFragmentArrived(1, 1024);

        pkt.dump("抵達順序");
        cout << "  順序正確嗎? " << boolalpha << pkt.isOrdered() << endl;

        pkt.restoreTransmissionOrder();   // 真的需要改變順序 → 用 reverse()
        pkt.dump("還原順序");
        cout << "  順序正確嗎? " << pkt.isOrdered() << endl;
        cout << "  總計 " << pkt.totalBytes() << " bytes" << endl;
        cout << "  ★ 後續校驗/解碼/寫檔都要依新順序 → 這才是 reverse() 該出場的場合" << endl;
        cout << "  ★ payload 是變長 buffer，list::reverse 只改指標、完全不搬資料" << endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -O2 -Wall -Wextra summary.cpp -o summary
//   （不加 -O2 也能編譯且零警告，但第五節的效能數字會失真）

// ※ 第五節的耗時與第二節的節點位址（0x...）皆為**本機實測，每次執行都不同**
// ========== 五、效能：元素大小決定勝負（本機實測，每次都不同）==========

// === 預期輸出 ===
//   （Dell Precision 7550 / Ubuntu 26.04 / g++ 15.2.0 / -O2 編譯）。
//   請只看趨勢，不要當成固定值或效能保證：
//     大型物件（1028 bytes）→ list::reverse 明顯快（本機約 3 倍）
//     小型元素（int）        → std::reverse 反而略快
//   本機重複執行的區間：大型物件 list 6,573~7,450µs / std 20,622~22,896µs；
//   int 元素 list 396~562µs / std 223~360µs。
//
// ========== 一、基本 reverse 與邊界情況 ==========
//   反轉前 [5]: 1 2 3 4 5
//   反轉後 [5]: 5 4 3 2 1
//   空 list   [0]: (空)
//   單元素    [1]: 42
//   ★ reverse() 回傳 void（寫 lst = lst.reverse() 會編譯失敗）
//   ★ 空 / 單元素是合法的 no-op
//
// ========== 二、迭代器穩定性：值與位址都不變 ==========
//   reverse 前 *it20=20 位址=0x60b38bd95060
//   reverse 後 *it20=20 位址=0x60b38bd95060
//   reverse 後 [5]: 50 40 30 20 10
//   它現在是倒數第 2 個嗎? true
//   ★ 迭代器跟著「元素」走，不是跟著「位置」走
//
// ========== 三、list::reverse vs std::reverse（語意不同）==========
//   list::reverse 前 *it=20
//   list::reverse 後 *it=20  ← 值不變，位置變成倒數第 2
//   std::reverse  前 *it=20
//   std::reverse  後 *it=40  ← 位置不變，值被換掉
//   list::reverse 結果 [5]: 50 40 30 20 10
//   std::reverse  結果 [5]: 50 40 30 20 10
//   ★ 容器內容相同，但對「持有迭代器的人」影響天差地遠
//
// ========== 四、rbegin()/rend()：多數情況真正該用的 ==========
//   正向: 1 2 3 4 5
//   反向: 5 4 3 2 1
//   容器完全沒變 [5]: 1 2 3 4 5
//   ★ O(1) 取得、不修改容器；const list 也能用
//   ★ 判斷訊號：函式能是 const 就該用 rbegin()，不是 reverse()
//
//   元素數量 = 200000
//   --- 大型物件 (sizeof=1028 bytes) ---
//     list::reverse: 6573 us  (只對調指標)
//     std::reverse : 20622 us  (交換整個物件)
//     → 元素越大，list::reverse 優勢越明顯
//   --- 小型元素 (int, 4 bytes) ---
//     list::reverse: 396 us  (走訪全部 n 個節點)
//     std::reverse : 223 us  (只做 n/2 次交換)
//     ★ 反直覺：元素小時 std::reverse 反而可能更快
//
// ========== 六、迴文檢測（示範正確與錯誤的寫法）==========
//   {1,2,3,2,1}  複本法: 是   對撞法: 是
//   {1,2,3,4,5}  複本法: 否   對撞法: 否
//   {1}         複本法: 是   對撞法: 是
//   {}          複本法: 是   對撞法: 是
//   ★ 絕不可直接 reverse 傳進來的 list —— 那會改到呼叫端的資料
//   ★ 對撞法連複本都不用，是這題真正的最佳解
//
// ========== 七、組合技：splice + reverse 反轉子區間 ==========
//   原始     [8]: 1 2 3 4 5 6 7 8
//   後半反轉 [8]: 1 2 3 4 8 7 6 5
//   ★ std::list 沒有「反轉子區間」的成員函式，這是標準作法
//   ★ 這也正是 LeetCode 25（k 個一組反轉）的 STL 版思路
//
// ========== 八、LeetCode 206. Reverse Linked List ==========
//   反轉前: 1->2->3->4->5
//   反轉後: 5->4->3->2->1
//   空串列: (空)
//   單節點: 7
//   ★ 這就是 list::reverse 內部在做的事：逐節點把箭頭反過來
//
// ========== 九、LeetCode 25. Reverse Nodes in k-Group ==========
//   [1,2,3,4,5] k=2 → 2->1->4->3->5
//   [1,2,3,4,5] k=3 → 3->2->1->4->5
//   [1,2,3,4,5] k=1 → 1->2->3->4->5
//   [1..6]      k=3 → 3->2->1->6->5->4
//   ★ 不足 k 個的尾巴保持原樣（k=3 時最後的 5 沒被反轉）
//
// ========== 十、日常實務：編輯器撤銷歷程 ==========
//   儲存順序是「新→舊」（push_front，O(1)）
//   目前有 4 筆動作
//
//   稽核日誌（正序，用 rbegin/rend；這是 const 成員）:
//     1. [insert] 在第 12 行插入 'return 0;'
//     2. [delete] 刪除第 5 行的除錯輸出
//     3. [format] 整份重新縮排
//     4. [insert] 加入標頭檔 <list>
//
//   按下 Ctrl+Z → 撤銷: [insert] 加入標頭檔 <list>
//   剩下 3 筆
//   ★ exportAuditLog() 能宣告成 const，正是「該用 rbegin」的訊號
//
// ========== 十一、日常實務：封包分片重組 ==========
//   抵達順序 [4]: seq4(512B) seq3(1024B) seq2(1024B) seq1(1024B)
//   順序正確嗎? false
//   還原順序 [4]: seq1(1024B) seq2(1024B) seq3(1024B) seq4(512B)
//   順序正確嗎? true
//   總計 3584 bytes
//   ★ 後續校驗/解碼/寫檔都要依新順序 → 這才是 reverse() 該出場的場合
//   ★ payload 是變長 buffer，list::reverse 只改指標、完全不搬資料
