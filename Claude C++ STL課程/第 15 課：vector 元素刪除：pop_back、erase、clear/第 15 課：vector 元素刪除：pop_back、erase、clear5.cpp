// =============================================================================
//  第 15 課：vector 元素刪除 5  —  erase(first, last)：範圍刪除
// =============================================================================
//
// 【主題資訊 Information】
//   iterator erase(const_iterator first, const_iterator last);
//   標頭檔：<vector>
//   標準版本：C++98（C++11 起參數改為 const_iterator）。
//   語意：刪除【半開區間 [first, last)】—— 含 first、不含 last。
//   複雜度：O(n)。移動次數 = last 之後的元素個數（與刪除的數量無關！）。
//           另外要對 [first, last) 內每個元素各呼叫一次解構子。
//   回傳：指向「last 原本所指元素」的 iterator（即更新後的 first）。
//   前置條件：[first, last) 必須是 *this 中的合法區間，且 first <= last。
//   特例：first == last 時什麼都不做，是合法的（刪除空區間）。
//
// 【詳細解釋 Explanation】
//
// 【1. 範圍刪除為什麼比「逐一刪除」快得多】
//   這是本檔最重要的一點。刪除 k 個連續元素：
//     逐一 erase k 次 —— 每次都搬移後半段 → O(k × n)
//     erase(範圍) 一次 —— 只搬移一次後半段 → O(n)
//   關鍵在於「搬移次數與刪除數量無關」：
//       [A][B][C][D][E][F][G]
//            └──刪除──┘
//       erase(begin+1, begin+4) 只要把 [E][F][G] 往前搬一次
//   不論你刪 3 個還是 300 個連續元素，後半段都只被搬動一次。
//   所以只要要刪的是【連續區間】，永遠該用範圍版本。
//
// 【2. 半開區間 [first, last) 的一致性】
//   STL 全庫都用半開區間，erase 也不例外。這帶來幾個自然的推論：
//       erase(v.begin(), v.end())      刪光全部（等同 clear()）
//       erase(v.begin(), v.begin())    刪除空區間，什麼都不做
//       erase(it, it + n)              從 it 開始刪 n 個
//   「不含 last」讓「刪 n 個」寫成 it + n 而不是 it + n - 1，
//   天生避免 off-by-one。
//
// 【3. erase(begin, end) 與 clear() 的差別】
//   結果完全相同（size 變 0、capacity 不變、所有元素被解構）。
//   差別只在表達力：clear() 一眼就知道是「清空」，
//   erase(begin, end) 要讀者自己看出那涵蓋了整個容器。
//   實務上：意圖是「清空」就用 clear()，
//   意圖是「刪掉這段剛好是全部」就用 erase。
//
// 【4. 常見用途：截斷（truncate）】
//   「只保留前 N 筆」是範圍刪除最實用的場景：
//       if (v.size() > N) v.erase(v.begin() + N, v.end());
//   注意這裡刪的是尾段，last 就是 end()，所以【一個元素都不必搬移】——
//   只要解構掉尾巴、把 size 設成 N。這是 O(被刪除的數量)，非常便宜。
//   反過來「只保留後 N 筆」要刪頭段，就得搬移整個尾段，較貴：
//       if (v.size() > N) v.erase(v.begin(), v.end() - N);
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼「刪尾段」幾乎免費，而「刪頭段」要付全額
//     erase 的成本 = 解構被刪元素 + 搬移 last 之後的元素。
//     刪尾段時 last == end()，後面沒有元素要搬，成本只剩解構。
//     刪頭段時要把整個尾段往前搬，成本是 O(剩餘元素數)。
//     這個不對稱性直接來自 vector 的連續記憶體佈局，
//     也是「vector 沒有 pop_front」的同一個理由。
//
// (B) 用 resize 做截斷更直接
//     `v.resize(N)` 在 N < size() 時的效果，等同
//     `v.erase(v.begin() + N, v.end())`——都是解構尾巴、size 設為 N。
//     resize 的意圖更明確（「我要它變成 N 個」），
//     而且不必自己算迭代器。
//     注意 resize 在 N > size() 時會補上值初始化的元素，
//     語意比 erase 廣，選用時要確認方向。
//
// (C) 從中間刪除一段時，被搬移的是「last 之後」而非「first 之後」
//     這點常被誤記。搬移的來源是 last 開始的那一段，
//     目的地是 first 開始的位置。所以成本只跟「last 之後還剩幾個」有關，
//     跟你刪了多少個完全無關。本檔的 main 用計數器實測了這件事。
//
// 【注意事項 Pay Attention】
//   1. 區間是半開的：含 first、不含 last。
//   2. first 與 last 必須來自【同一個】容器，且 first <= last。
//      順序顛倒是未定義行為（不會幫你自動交換）。
//   3. 刪除連續區間永遠用範圍版本，別用迴圈逐一刪（O(k×n) vs O(n)）。
//   4. 刪尾段（last == end()）幾乎免費；刪頭段要搬移整個尾段。
//   5. capacity 不變。要釋放記憶體需另外 shrink_to_fit()（非約束性請求）。
//   6. first == last 是合法的空操作，不必特別防範。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】vector::erase(first, last)
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 要刪除 vector 中連續的 100 個元素，用迴圈 erase 100 次
//        和用一次 erase(first, last)，複雜度差多少？
//     答：差 100 倍。逐一 erase 是 O(k × n)——每次都要搬移一次後半段；
//         範圍 erase 是 O(n)——後半段【只搬移一次】。
//         關鍵洞察是「搬移次數只跟 last 之後還剩幾個元素有關，
//         跟你刪了幾個完全無關」。
//     追問：那不連續的元素呢？
//         → 那就不能用範圍版本。要用 erase-remove 慣用法
//           （std::remove_if 把要保留的往前搬，再一次 erase 尾段），
//           那也是 O(n)。見本課第 10 檔。
//
// 🔥 Q2. `v.erase(v.begin() + N, v.end())` 和 `v.resize(N)` 有什麼差別？
//     答：當 N < v.size() 時效果完全相同——都是解構尾段、size 設為 N、
//         capacity 不變。差別在意圖表達與適用範圍：
//         resize(N) 更直接（「我要它變成 N 個」），且 N > size() 時
//         會補上值初始化的元素；erase 只能縮短。
//         實務上做截斷用 resize 比較不容易寫錯迭代器。
//     追問：這個操作的成本是多少？
//         → 只有 O(被刪除的元素數)，因為 last == end()，
//           後面沒有任何元素需要搬移。截斷是 vector 最便宜的刪除。
//
// ⚠️ 陷阱. 「erase(first, last) 的成本跟刪掉幾個元素成正比」——對嗎？
//     答：不對。成本主要由「last 之後還剩幾個元素」決定，
//         因為那才是需要被搬移的部分。刪掉的元素只需各呼叫一次解構子
//         （對 int 這種型別甚至是零成本）。
//         具體例子：一個 10000 元素的 vector，
//           * 從索引 0 刪 5000 個 → 要搬移剩下的 5000 個
//           * 從索引 9995 刪 5 個 → 要搬移 0 個（last 就是 end()）
//         後者刪得少，但更重要的是它幾乎免費。
//     為什麼會錯：直覺把「刪除」想成逐一移除的累加動作，
//         於是推論成本正比於數量。但 vector 的範圍刪除是
//         【一次性的區塊搬移】，數量只影響解構，不影響搬移。
//         本檔的 main 用計數器把這件事實際量出來。
// ═══════════════════════════════════════════════════════════════════════════

#include <vector>
#include <iostream>
#include <string>

// -----------------------------------------------------------------------------
// 計數用的輔助型別：記錄賦值（搬移）被呼叫幾次。
// 用來實測「erase 範圍的成本跟刪幾個無關」。
// -----------------------------------------------------------------------------
struct Counted {
    int value;
    static int assigns;
    Counted(int v = 0) : value(v) {}
    Counted(const Counted&) = default;
    Counted(Counted&&) noexcept = default;
    Counted& operator=(const Counted& o) { value = o.value; ++assigns; return *this; }
    Counted& operator=(Counted&& o) noexcept { value = o.value; ++assigns; return *this; }
};
int Counted::assigns = 0;

// -----------------------------------------------------------------------------
// 【日常實務範例】只保留最近 N 筆日誌（環形緩衝的簡化版）
//   情境：程式在記憶體中保留最近的日誌供除錯用，但不能無限成長。
//         每次寫入後檢查，超過上限就把最舊的砍掉。
//   為什麼用到本主題：這是範圍刪除最典型的用途，而且它示範了
//     「刪頭段 vs 刪尾段」的成本差異——
//     保留「最近」N 筆要刪掉【前面】的舊資料，
//     那正是最貴的方向（整個尾段都要往前搬）。
//   實務取捨：若這個操作很頻繁，用 std::deque（pop_front 是 O(1)）
//     或真正的環形緩衝區會好得多。這裡用 vector 是為了示範語意，
//     並在下方明白指出它的成本。
// -----------------------------------------------------------------------------
class BoundedLog {
public:
    explicit BoundedLog(size_t maxLines) : max_(maxLines) {}

    void append(const std::string& line) {
        lines_.push_back(line);
        if (lines_.size() > max_) {
            // 一次刪掉多餘的前段——【範圍刪除】，不是迴圈逐一刪
            size_t excess = lines_.size() - max_;
            lines_.erase(lines_.begin(),
                         lines_.begin() + static_cast<long>(excess));
        }
    }

    // 截斷成只保留前 n 筆（刪尾段，幾乎免費）
    void truncateTo(size_t n) {
        if (lines_.size() > n) {
            lines_.erase(lines_.begin() + static_cast<long>(n), lines_.end());
        }
    }

    const std::vector<std::string>& lines() const { return lines_; }
    size_t capacity() const { return lines_.capacity(); }

private:
    std::vector<std::string> lines_;
    size_t max_;
};

int main() {
    std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    std::cout << "=== 原始示範：範圍刪除 ===\n";
    std::cout << "初始: ";
    for (int x : v) std::cout << x << " ";
    std::cout << "  (capacity=" << v.capacity() << ")\n";

    // 刪除 [begin+2, begin+5) 範圍，即索引 2, 3, 4
    v.erase(v.begin() + 2, v.begin() + 5);

    std::cout << "刪除範圍後: ";
    for (int x : v) std::cout << x << " ";  // 1 2 6 7 8 9 10
    std::cout << std::endl;
    std::cout << "  （半開區間：含索引 2，不含索引 5）\n";

    // 刪除前三個元素
    v.erase(v.begin(), v.begin() + 3);

    std::cout << "再刪除前三個: ";
    for (int x : v) std::cout << x << " ";  // 7 8 9 10
    std::cout << std::endl;

    // 刪除所有元素（等同 clear）
    v.erase(v.begin(), v.end());
    std::cout << "size: " << v.size() << std::endl;  // 0
    std::cout << "capacity: " << v.capacity() << "（不變）\n";

    std::cout << "\n=== 實測：成本取決於「last 之後剩幾個」，不是刪幾個 ===\n";
    {
        auto measure = [](size_t from, size_t to) {
            std::vector<Counted> c;
            c.reserve(10000);
            for (int i = 0; i < 10000; ++i) c.emplace_back(i);
            Counted::assigns = 0;
            c.erase(c.begin() + static_cast<long>(from),
                    c.begin() + static_cast<long>(to));
            return Counted::assigns;
        };

        std::cout << "10000 個元素的 vector：\n";
        std::cout << "  從索引 0    刪 5000 個 -> 搬移 " << measure(0, 5000) << " 次\n";
        std::cout << "  從索引 4000 刪 5000 個 -> 搬移 " << measure(4000, 9000) << " 次\n";
        std::cout << "  從索引 9995 刪    5 個 -> 搬移 " << measure(9995, 10000) << " 次\n";
        std::cout << "  從索引 0    刪    1 個 -> 搬移 " << measure(0, 1) << " 次\n";
        std::cout << "→ 刪 5 個可能比刪 1 個便宜，關鍵是 last 後面還剩多少。\n";
        std::cout << "  刪尾段（last == end()）搬移 0 次，是最便宜的刪除。\n";
    }

    std::cout << "\n=== 範圍刪除 vs 迴圈逐一刪除 ===\n";
    {
        auto rangeErase = []() {
            std::vector<Counted> c;
            c.reserve(2000);
            for (int i = 0; i < 2000; ++i) c.emplace_back(i);
            Counted::assigns = 0;
            c.erase(c.begin() + 500, c.begin() + 1500);   // 一次刪 1000 個
            return Counted::assigns;
        };
        auto loopErase = []() {
            std::vector<Counted> c;
            c.reserve(2000);
            for (int i = 0; i < 2000; ++i) c.emplace_back(i);
            Counted::assigns = 0;
            for (int i = 0; i < 1000; ++i) c.erase(c.begin() + 500);  // 逐一刪 1000 次
            return Counted::assigns;
        };

        std::cout << "從 2000 個元素中刪除中間連續的 1000 個：\n";
        std::cout << "  一次 erase(範圍) : 搬移 " << rangeErase() << " 次\n";
        std::cout << "  迴圈 erase 1000 次: 搬移 " << loopErase() << " 次\n";
        std::cout << "→ 只要是連續區間，永遠用範圍版本。\n";
    }

    std::cout << "\n=== 半開區間的幾個自然推論 ===\n";
    {
        std::vector<int> a = {1, 2, 3};
        a.erase(a.begin(), a.begin());          // 空區間 → 合法的空操作
        std::cout << "erase(begin, begin) 後 size=" << a.size() << "（什麼都沒發生）\n";

        std::vector<int> b = {1, 2, 3, 4, 5};
        b.erase(b.begin() + 1, b.begin() + 1 + 2);   // 從索引 1 刪 2 個
        std::cout << "從索引 1 刪 2 個 -> ";
        for (int x : b) std::cout << x << " ";
        std::cout << "（寫成 it + n，不必 -1）\n";

        std::vector<int> c = {1, 2, 3};
        c.erase(c.begin(), c.end());
        std::cout << "erase(begin, end) 後 size=" << c.size()
                  << " —— 等同 clear()，但 clear() 意圖更清楚\n";
    }

    std::cout << "\n=== 截斷：erase 尾段 vs resize ===\n";
    {
        std::vector<int> a = {1, 2, 3, 4, 5, 6, 7, 8};
        std::vector<int> b = a;
        a.erase(a.begin() + 3, a.end());
        b.resize(3);
        std::cout << "erase(begin+3, end) -> ";
        for (int x : a) std::cout << x << " ";
        std::cout << "\n";
        std::cout << "resize(3)           -> ";
        for (int x : b) std::cout << x << " ";
        std::cout << "\n";
        std::cout << "結果相同；resize 的意圖更明確，也不必自己算迭代器。\n";
    }

    std::cout << "\n=== 日常實務：只保留最近 N 筆日誌 ===\n";
    {
        BoundedLog log(4);
        const char* events[] = {
            "server started", "config loaded", "listening :8080",
            "client connected", "request /api/v1", "client disconnected",
            "shutdown signal",
        };
        for (const char* e : events) {
            log.append(e);
        }

        std::cout << "寫入 7 筆，上限 4 筆，實際保留 " << log.lines().size() << " 筆：\n";
        for (const auto& l : log.lines()) std::cout << "  " << l << "\n";
        std::cout << "capacity=" << log.capacity() << "（記憶體被重複利用，不縮）\n";
        std::cout << "→ 注意這裡刪的是【頭段】，整個尾段都要往前搬，是最貴的方向。\n";
        std::cout << "  若這個操作很頻繁，該改用 std::deque（pop_front 是 O(1)）\n";
        std::cout << "  或真正的環形緩衝區。\n";

        log.truncateTo(2);
        std::cout << "truncateTo(2)（刪尾段，幾乎免費）-> ";
        for (const auto& l : log.lines()) std::cout << "[" << l << "] ";
        std::cout << "\n";
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第 15 課：vector 元素刪除5.cpp -o demo5

// === 預期輸出 ===
// === 原始示範：範圍刪除 ===
// 初始: 1 2 3 4 5 6 7 8 9 10   (capacity=10)
// 刪除範圍後: 1 2 6 7 8 9 10
//   （半開區間：含索引 2，不含索引 5）
// 再刪除前三個: 7 8 9 10
// size: 0
// capacity: 10（不變）
//
// === 實測：成本取決於「last 之後剩幾個」，不是刪幾個 ===
// 10000 個元素的 vector：
//   從索引 0    刪 5000 個 -> 搬移 5000 次
//   從索引 4000 刪 5000 個 -> 搬移 1000 次
//   從索引 9995 刪    5 個 -> 搬移 0 次
//   從索引 0    刪    1 個 -> 搬移 9999 次
// → 刪 5 個可能比刪 1 個便宜，關鍵是 last 後面還剩多少。
//   刪尾段（last == end()）搬移 0 次，是最便宜的刪除。
//
// === 範圍刪除 vs 迴圈逐一刪除 ===
// 從 2000 個元素中刪除中間連續的 1000 個：
//   一次 erase(範圍) : 搬移 500 次
//   迴圈 erase 1000 次: 搬移 999500 次
// → 只要是連續區間，永遠用範圍版本。
//
// === 半開區間的幾個自然推論 ===
// erase(begin, begin) 後 size=3（什麼都沒發生）
// 從索引 1 刪 2 個 -> 1 4 5 （寫成 it + n，不必 -1）
// erase(begin, end) 後 size=0 —— 等同 clear()，但 clear() 意圖更清楚
//
// === 截斷：erase 尾段 vs resize ===
// erase(begin+3, end) -> 1 2 3
// resize(3)           -> 1 2 3
// 結果相同；resize 的意圖更明確，也不必自己算迭代器。
//
// === 日常實務：只保留最近 N 筆日誌 ===
// 寫入 7 筆，上限 4 筆，實際保留 4 筆：
//   client connected
//   request /api/v1
//   client disconnected
//   shutdown signal
// capacity=8（記憶體被重複利用，不縮）
// → 注意這裡刪的是【頭段】，整個尾段都要往前搬，是最貴的方向。
//   若這個操作很頻繁，該改用 std::deque（pop_front 是 O(1)）
//   或真正的環形緩衝區。
// truncateTo(2)（刪尾段，幾乎免費）-> [client connected] [request /api/v1]
