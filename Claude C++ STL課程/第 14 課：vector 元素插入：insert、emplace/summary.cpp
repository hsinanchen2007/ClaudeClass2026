// =============================================================================
//  summary.cpp  —  第 14 課總複習：vector 的元素插入 insert / emplace
// =============================================================================
//
// 【主題資訊 Information】
//   iterator insert(const_iterator pos, const T& value);              // (1) 複製插入
//   iterator insert(const_iterator pos, T&& value);                   // (2) 移動插入 C++11
//   iterator insert(const_iterator pos, size_type n, const T& value); // (3) 插入 n 份
//   template<class InputIt>
//   iterator insert(const_iterator pos, InputIt first, InputIt last); // (4) 插入範圍
//   iterator insert(const_iterator pos, std::initializer_list<T> il); // (5) C++11
//   template<class... Args>
//   iterator emplace(const_iterator pos, Args&&... args);             // (6) 就地建構 C++11
//
//   回傳：指向「第一個被插入元素」的 iterator。
//         若沒有插入任何元素（n == 0 或 first == last），回傳 pos（本機實測確認）。
//   標頭：<vector>
//   複雜度：插入 k 個元素為 O(k + 從 pos 到 end() 的距離)；
//           換句話說，尾端插入攤銷 O(1)，中間／開頭插入 O(n)。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼是「插在 pos 之前」而不是之後】
//   STL 的區間慣例是**半開區間 [first, last)**：begin() 指向第一個元素，
//   end() 指向「最後一個元素的再下一個」。在這套慣例下，「插在 pos 之前」
//   才能讓 insert(end(), x) 有意義 —— 它自然表示「附加到尾端」。
//   若定義成「插在 pos 之後」，就無法表達「插到最前面」（begin() 之前沒有位置可指），
//   得額外發明一個 before_begin()。forward_list 正是因為單向鏈結被迫走上那條路
//   （見第 39 課的 insert_after / before_begin），對照之下更能看出 vector 這個
//   設計的一致性。
//
// 【2. O(n) 從何而來：vector 必須維持「連續」這個承諾】
//   vector 的核心保證是元素在記憶體中**連續**，這讓 operator[] 是一次位址運算、
//   也讓 &v[0] 能直接餵給 C API。代價是：要在中間插入一個元素，
//   就必須把插入點之後的所有元素整體往後搬一格，騰出空位。
//   搬移量 = end() - pos，所以越靠前插入越貴，插在尾端則不用搬（O(1) 攤銷）。
//   這不是實作偷懶，而是「連續」這個資料結構承諾的必然代價。
//
// 【3. insert 與 emplace 的真正差別：初始化方式不同】
//   多數人以為差別只是「少一次複製」，其實更關鍵的是**初始化語意**：
//     - insert(pos, x)   ：x 必須已經是 T（或能**隱式**轉成 T）→ 複製初始化
//     - emplace(pos, a, b)：把 a, b 完美轉發給 T 的建構子 → **直接初始化**
//   直接初始化能呼叫 explicit 建構子，複製初始化不能。本機實測：
//     struct Ex { explicit Ex(int); };
//     v.emplace(v.begin(), 1);  // 編譯成功
//     v.insert(v.begin(), 1);   // 編譯失敗：無法從 int 隱式轉為 Ex
//   這是彈性，也是風險 —— emplace 會放行你「本來不打算允許」的轉換。
//
// 【4. 迴圈裡反覆 insert 是 O(n²) 陷阱】
//   for (auto x : src) v.insert(v.begin(), x);   // 每次都搬整個 vector
//   n 次插入 → 總搬移量 1+2+…+n = O(n²)。
//   正解有三種：(a) 用範圍版 insert 一次插入（只搬一次）；
//   (b) 先 reserve 再 push_back 後 std::reverse；(c) 換用 deque／list。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 插入時若容量不足，實際發生什麼
//   1) 配置一塊更大的新記憶體（成長倍率**是實作定義**：libstdc++ 實測 2×、
//      MSVC 為 1.5×；標準只規定 push_back 為攤銷 O(1)，**沒有**規定倍率）
//   2) 把 [begin, pos) 搬到新空間
//   3) 在新空間就地建構要插入的元素
//   4) 把 [pos, end) 接著搬過去
//   5) 銷毀舊元素、釋放舊記憶體
//   注意第 2、4 步用的是 move 還是 copy，取決於 T 的移動建構子是否為 noexcept
//   （std::move_if_noexcept）：若移動可能拋例外，vector 為了維持**強例外保證**
//   會退化成複製。這是「移動建構子請標 noexcept」這條建議的真正理由。
//
// (B) 自我插入 v.insert(it, v[0]) 為什麼是安全的
//   直覺上這很危險：v[0] 是容器內部的參考，一旦 reallocation 或元素搬移，
//   來源就被摧毀了。但標準要求實作必須正確處理這種 self-referencing 插入，
//   典型做法是先把值複製到區域暫存、或在搬移前先完成新元素的建構。
//   （這是標準對實作者的要求，不是使用者要自己規避的事。）
//
// 【注意事項 Pay Attention】
// 1. **iterator 失效規則（最常考）**：
//    - 若插入導致 reallocation（size + k > capacity）→ **所有** iterator／
//      pointer／reference 全部失效。
//    - 若沒有 reallocation → 只有**插入點之後**（含插入點）的失效，之前的仍有效。
//    實務上不要賭有沒有 reallocation，一律「插入後重新取得 iterator」。
//    insert 的回傳值正是為此設計的。
// 2. insert 傳入的 pos 必須是**這個容器**的有效 iterator；
//    傳別的容器的 iterator 是未定義行為（UB），不保證會有任何特定症狀。
// 3. 插入 0 個元素（n == 0 或空範圍）不是錯誤，回傳 pos、容器不變（本機實測）。
// 4. emplace 不做 narrowing 檢查，參數打錯可能悄悄編過；
//    傳入的若已經是同型別物件，emplace 相對 insert 幾乎沒有好處。
// 5. 成長倍率、capacity 的實際數值都是**實作定義**，不可當成標準保證來背。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】vector::insert / emplace
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. v.insert(pos, x) 之後，先前取得的 iterator 還能用嗎？
//     答：不一定，取決於是否發生 reallocation。若 size + 1 > capacity，
//         會重新配置記憶體，**所有** iterator／pointer／reference 全部失效；
//         若沒有 reallocation，則只有插入點及其之後的失效。
//         安全寫法是改用 insert 的回傳值重新取得 iterator。
//     追問：那 insert 回傳什麼？→ 指向第一個被插入元素的 iterator；
//           若一個都沒插入（n==0 或空範圍），回傳傳進去的 pos。
//
// 🔥 Q2. insert 和 emplace 差在哪？emplace 是不是一定比較快？
//     答：emplace 把參數完美轉發給 T 的建構子做**直接初始化**（就地建構），
//         insert 需要一個已存在的 T 做**複製初始化**。
//         但「一定比較快」是錯的：當你傳進去的本來就是一個同型別物件時，
//         兩者都要做一次 copy/move，效能幾乎沒有差別。
//     追問：那 emplace 有什麼副作用？→ 直接初始化可以呼叫 explicit 建構子、
//           且不做 narrowing 檢查，等於放寬了型別檢查。
//
// 🔥 Q3. 為什麼 vector 中間插入是 O(n)，list 卻是 O(1)？
//     答：vector 保證元素連續，插入必須把 pos 之後的元素整體後移一格，
//         搬移量是 end() - pos。list 是鏈結串列，插入只要改幾個指標，
//         但代價是失去隨機存取、且每個節點多付指標與配置器的空間成本。
//     追問：那 list 就比較好？→ 不一定。找到插入位置本身在 list 是 O(n)，
//           且 vector 連續記憶體對 CPU cache 友善，小資料量常常反而更快。
//
// ⚠️ 陷阱. 這段「把 src 逐一插到最前面」的程式，複雜度是多少？
//         for (int x : src) v.insert(v.begin(), x);
//     答：O(n²)。每次插在 begin() 都要搬移目前全部元素，
//         總搬移量是 1+2+…+n。改用範圍版 v.insert(v.begin(), src.begin(), src.end())
//         只需搬一次，是 O(n + m)。
//     為什麼會錯：多數人記得「insert 是 O(n)」，卻把迴圈裡的 n 當成常數，
//         忘了它每一輪都在成長，於是把 O(n²) 誤算成 O(n)。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <list>
#include <string>
#include <algorithm>
using namespace std;

// ================================================================
// 重點一：insert 基本用法 —— 在指定位置前插入
// ================================================================
// 語法：iterator insert(iterator pos, const T& value)
// 新元素插入在 pos 「之前」
// 回傳值：指向新插入元素的迭代器（C++11）
// 注意：插入後，pos 之後所有元素都要向後位移（O(n)）

void demoInsertSingle() {
    cout << "\n【insert 插入單一元素】" << endl;

    vector<int> v = {1, 2, 3, 4, 5};

    // 在開頭插入（v.begin() 位置之前）
    auto it = v.insert(v.begin(), 0);
    // v: {0, 1, 2, 3, 4, 5}
    cout << "在開頭插入 0，返回的迭代器指向: " << *it << endl;

    // 在第三個位置插入（索引 2 之前）
    it = v.insert(v.begin() + 2, 100);
    // v: {0, 1, 100, 2, 3, 4, 5}
    cout << "在索引 2 插入 100，返回的迭代器指向: " << *it << endl;

    // 在結尾插入（等同於 push_back）
    v.insert(v.end(), 6);
    // v: {0, 1, 100, 2, 3, 4, 5, 6}

    cout << "最終元素: ";
    for (int n : v) cout << n << " ";
    cout << endl;
}

// ================================================================
// 重點二：insert 插入 n 個相同元素
// ================================================================
// 語法：iterator insert(iterator pos, size_type n, const T& value)
// 在 pos 之前插入 n 個 value 的副本

void demoInsertN() {
    cout << "\n【insert 插入多個相同元素】" << endl;

    vector<int> v = {1, 2, 3};

    // 在開頭插入 3 個 0
    v.insert(v.begin(), 3, 0);
    // v: {0, 0, 0, 1, 2, 3}
    cout << "插入 3 個 0: ";
    for (int n : v) cout << n << " ";
    cout << endl;

    // 在結尾插入 2 個 99
    v.insert(v.end(), 2, 99);
    // v: {0, 0, 0, 1, 2, 3, 99, 99}
    cout << "插入 2 個 99: ";
    for (int n : v) cout << n << " ";
    cout << endl;
}

// ================================================================
// 重點三：insert 插入範圍（另一個容器的元素）
// ================================================================
// 語法：iterator insert(iterator pos, InputIt first, InputIt last)
// 將 [first, last) 範圍的元素插入到 pos 之前

void demoInsertRange() {
    cout << "\n【insert 插入範圍元素】" << endl;

    vector<int> v1 = {1, 2, 3};
    vector<int> v2 = {10, 20, 30};

    // 將 v2 的全部元素插入到 v1 的中間（索引 1 之前）
    v1.insert(v1.begin() + 1, v2.begin(), v2.end());
    // v1: {1, 10, 20, 30, 2, 3}
    cout << "插入 v2 後: ";
    for (int n : v1) cout << n << " ";
    cout << endl;

    // 也可以插入初始化列表
    vector<int> v3 = {100, 200, 300};
    v3.insert(v3.begin(), {-3, -2, -1});
    cout << "插入初始化列表: ";
    for (int n : v3) cout << n << " ";
    cout << endl;

    // 從原生陣列插入
    int arr[] = {7, 8, 9};
    v3.insert(v3.end(), arr, arr + 3);
    cout << "插入原生陣列: ";
    for (int n : v3) cout << n << " ";
    cout << endl;
}

// ================================================================
// 重點四：emplace —— 就地構造插入（C++11）
// ================================================================
// 語法：iterator emplace(iterator pos, args...)
// 直接在 pos 前就地構造元素（傳建構函數參數）
// 省去建立臨時物件再複製/移動的開銷

struct Point {
    int x, y;
    Point(int x, int y) : x(x), y(y) {}
    friend ostream& operator<<(ostream& os, const Point& p) {
        return os << "(" << p.x << "," << p.y << ")";
    }
};

void demoEmplace() {
    cout << "\n【emplace 就地構造插入】" << endl;

    vector<Point> v;
    v.reserve(5);

    // push_back：需要構造 Point 再移動
    v.push_back(Point(1, 2));

    // emplace：直接傳建構參數，就地構造
    v.emplace(v.begin(), 0, 0);           // 在最前面插入 (0,0)
    v.emplace(v.end(), 3, 4);             // 在最後插入 (3,4)
    v.emplace(v.begin() + 1, 10, 20);    // 在索引 1 前插入

    cout << "Points: ";
    for (const Point& p : v) cout << p << " ";
    cout << endl;
}

// ================================================================
// 重點五：insert 的效能代價
// ================================================================
// vector 的 insert（非尾端）需要將插入位置後的所有元素向後位移
// 這是 O(n) 的操作！
//
// 若頻繁在中間插入，考慮使用：
//   - std::list（O(1) 任意插入，但無隨機存取）
//   - std::deque（O(1) 頭尾插入）

void demoPerformanceNote() {
    cout << "\n【效能說明】" << endl;

    vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    // 在 v.begin() 插入：需要移動所有 10 個元素
    v.insert(v.begin(), 0);
    cout << "在頭部插入（O(n) 位移）: ";
    for (int n : v) cout << n << " ";
    cout << endl;

    // 在 v.end() 插入（等同 push_back）：O(1)
    v.insert(v.end(), 11);
    cout << "在尾部插入（O(1)）: 最後 = " << v.back() << endl;

    cout << "提示：頻繁在中間插入應改用 std::list" << endl;
}

// ================================================================
// 重點六：insert 回傳值的使用
// ================================================================
// C++11 後，insert 回傳指向新插入元素的迭代器
// 這讓鏈式操作成為可能

void demoReturnValue() {
    cout << "\n【insert 回傳迭代器的使用】" << endl;

    vector<int> v = {10, 30, 50};

    // 取得插入後的迭代器，繼續使用
    auto it = v.insert(v.begin() + 1, 20);  // 插入 20，it 指向 20
    cout << "插入 20 後，it 指向: " << *it << endl;

    // 在剛插入的元素後再插入
    v.insert(it + 1, 25);  // 在 20 後插入 25
    cout << "再插入 25: ";
    for (int n : v) cout << n << " ";
    cout << endl;
}

// ================================================================
// 重點七：插入 0 個元素 與 iterator 失效（實測）
// ================================================================
void demoEdgeAndInvalidation() {
    cout << "\n【邊界：插入 0 個元素】" << endl;
    vector<int> v = {1, 2, 3};
    auto pos = v.begin() + 1;
    auto it = v.insert(pos, 0, 99);          // n == 0：什麼都不插
    cout << "insert n=0 回傳 pos 嗎: " << (it == v.begin() + 1 ? "是" : "否")
         << "，size 仍為 " << v.size() << endl;

    cout << "\n【iterator 失效：靠 capacity 判斷有無 reallocation】" << endl;
    vector<int> w = {1, 2, 3};
    w.reserve(10);                            // 預留足夠空間 → 不會 reallocation
    size_t capBefore = w.capacity();
    w.insert(w.begin(), 0);
    cout << "  reserve(10) 後插入：capacity " << capBefore << " -> " << w.capacity()
         << "（未 reallocation，插入點之前的 iterator 仍有效）" << endl;

    vector<int> u = {1, 2, 3};
    u.shrink_to_fit();
    size_t cap2 = u.capacity();
    u.insert(u.begin(), 0);
    cout << "  容量吃緊時插入：capacity " << cap2 << " -> " << u.capacity()
         << "（發生 reallocation，全部 iterator 失效）" << endl;
    cout << "  註：capacity 數值為本機 libstdc++ 實測，成長倍率非標準規定" << endl;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 1】LeetCode 57. Insert Interval
//   題目：給一組**已排序且互不重疊**的區間，插入一個新區間並合併重疊部分。
//   為什麼用到本主題：這題的本質就是「找到插入位置 → 插入 → 維持不變式」。
//     這裡刻意用 lower_bound 定位 + vector::insert 落點，示範本課的定位/插入組合；
//     合併時用 erase 收掉被吞併的區間。
//   複雜度：定位 O(log n)，插入與合併的搬移是 O(n)。
// -----------------------------------------------------------------------------
vector<vector<int>> insertInterval(vector<vector<int>> intervals, vector<int> newInterval) {
    // 依左端點找到第一個「左端點 >= 新區間左端點」的位置
    auto pos = lower_bound(intervals.begin(), intervals.end(), newInterval,
                           [](const vector<int>& a, const vector<int>& b) {
                               return a[0] < b[0];
                           });
    // insert 回傳指向新插入元素的 iterator —— 用它繼續操作才安全
    auto it = intervals.insert(pos, newInterval);

    // 先和左邊鄰居嘗試合併
    if (it != intervals.begin() && (it - 1)->at(1) >= it->at(0)) {
        --it;
        (*it)[1] = max((*it)[1], (*(it + 1))[1]);
        intervals.erase(it + 1);
    }
    // 再把右邊所有重疊者吃掉
    while (it + 1 != intervals.end() && (*(it + 1))[0] <= (*it)[1]) {
        (*it)[1] = max((*it)[1], (*(it + 1))[1]);
        intervals.erase(it + 1);
    }
    return intervals;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 1】告警規則表：依優先序插入，維持排序不變式
//   情境：監控系統啟動時載入規則，之後隨時可能熱新增一條規則。
//         規則表必須永遠依 priority 由小到大排序，讓比對時可以提早結束。
//   為什麼用到本主題：lower_bound 找位置 + insert 落點，是「維持有序容器」的標準手法，
//         比「push_back 之後整個 re-sort」便宜得多（O(n) vs O(n log n)）。
// -----------------------------------------------------------------------------
struct AlertRule {
    int priority;
    string name;
};

void insertRuleSorted(vector<AlertRule>& rules, int priority, const string& name) {
    auto pos = lower_bound(rules.begin(), rules.end(), priority,
                           [](const AlertRule& r, int p) { return r.priority < p; });
    // emplace：直接把 priority/name 轉發給 AlertRule 的建構子就地建構
    rules.emplace(pos, AlertRule{priority, name});
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】批次併入 log：範圍版 insert vs 逐筆 insert
//   情境：從第二個來源讀進一批 log 行，要併到既有緩衝區的指定位置。
//   為什麼用到本主題：這正是「逐筆插入 O(n²)」與「範圍插入 O(n+m)」的對照，
//         也是本課最容易在 code review 抓到的效能問題。
// -----------------------------------------------------------------------------
vector<string> mergeLogBatch(vector<string> buffer, size_t at, const vector<string>& batch) {
    // 一次插入整段：只搬移一次尾段，而不是每筆都搬
    buffer.insert(buffer.begin() + static_cast<long>(at), batch.begin(), batch.end());
    return buffer;
}

int main() {
    cout << "=============================================" << endl;
    cout << "   第 14 課：vector insert / emplace 總複習" << endl;
    cout << "=============================================" << endl;

    demoInsertSingle();
    demoInsertN();
    demoInsertRange();
    demoEmplace();
    demoPerformanceNote();
    demoReturnValue();
    demoEdgeAndInvalidation();

    cout << "\n=== LeetCode 57. Insert Interval ===" << endl;
    auto r1 = insertInterval({{1, 3}, {6, 9}}, {2, 5});
    cout << "  [[1,3],[6,9]] + [2,5] -> ";
    for (const auto& iv : r1) cout << "[" << iv[0] << "," << iv[1] << "] ";
    cout << endl;
    auto r2 = insertInterval({{1, 2}, {3, 5}, {6, 7}, {8, 10}, {12, 16}}, {4, 8});
    cout << "  [[1,2],[3,5],[6,7],[8,10],[12,16]] + [4,8] -> ";
    for (const auto& iv : r2) cout << "[" << iv[0] << "," << iv[1] << "] ";
    cout << endl;

    cout << "\n=== 日常實務：告警規則依優先序插入 ===" << endl;
    vector<AlertRule> rules;
    insertRuleSorted(rules, 50, "disk_usage_high");
    insertRuleSorted(rules, 10, "service_down");
    insertRuleSorted(rules, 30, "latency_p99");
    insertRuleSorted(rules, 20, "error_rate_spike");
    for (const auto& r : rules) cout << "  P" << r.priority << "  " << r.name << endl;

    cout << "\n=== 日常實務：批次併入 log ===" << endl;
    vector<string> buf = {"09:00 boot", "09:05 ready", "09:30 shutdown"};
    vector<string> batch = {"09:10 request /api", "09:12 request /health"};
    auto merged = mergeLogBatch(buf, 2, batch);
    for (const auto& line : merged) cout << "  " << line << endl;

    cout << "\n==============================================" << endl;
    cout << " 重點：insert 插入在 pos「之前」" << endl;
    cout << " 效能：尾端插入 O(1) 攤銷；中間插入 O(n)" << endl;
    cout << " emplace：就地建構（直接初始化），可呼叫 explicit 建構子" << endl;
    cout << " 失效：有 reallocation 則全失效，否則插入點之後失效" << endl;
    cout << "==============================================" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary

// === 預期輸出 ===
// =============================================
//    第 14 課：vector insert / emplace 總複習
// =============================================
//
// 【insert 插入單一元素】
// 在開頭插入 0，返回的迭代器指向: 0
// 在索引 2 插入 100，返回的迭代器指向: 100
// 最終元素: 0 1 100 2 3 4 5 6
//
// 【insert 插入多個相同元素】
// 插入 3 個 0: 0 0 0 1 2 3
// 插入 2 個 99: 0 0 0 1 2 3 99 99
//
// 【insert 插入範圍元素】
// 插入 v2 後: 1 10 20 30 2 3
// 插入初始化列表: -3 -2 -1 100 200 300
// 插入原生陣列: -3 -2 -1 100 200 300 7 8 9
//
// 【emplace 就地構造插入】
// Points: (0,0) (10,20) (1,2) (3,4)
//
// 【效能說明】
// 在頭部插入（O(n) 位移）: 0 1 2 3 4 5 6 7 8 9 10
// 在尾部插入（O(1)）: 最後 = 11
// 提示：頻繁在中間插入應改用 std::list
//
// 【insert 回傳迭代器的使用】
// 插入 20 後，it 指向: 20
// 再插入 25: 10 20 25 30 50
//
// 【邊界：插入 0 個元素】
// insert n=0 回傳 pos 嗎: 是，size 仍為 3
//
// 【iterator 失效：靠 capacity 判斷有無 reallocation】
//   reserve(10) 後插入：capacity 10 -> 10（未 reallocation，插入點之前的 iterator 仍有效）
//   容量吃緊時插入：capacity 3 -> 6（發生 reallocation，全部 iterator 失效）
//   註：capacity 數值為本機 libstdc++ 實測，成長倍率非標準規定
//
// === LeetCode 57. Insert Interval ===
//   [[1,3],[6,9]] + [2,5] -> [1,5] [6,9]
//   [[1,2],[3,5],[6,7],[8,10],[12,16]] + [4,8] -> [1,2] [3,10] [12,16]
//
// === 日常實務：告警規則依優先序插入 ===
//   P10  service_down
//   P20  error_rate_spike
//   P30  latency_p99
//   P50  disk_usage_high
//
// === 日常實務：批次併入 log ===
//   09:00 boot
//   09:05 ready
//   09:10 request /api
//   09:12 request /health
//   09:30 shutdown
//
// ==============================================
//  重點：insert 插入在 pos「之前」
//  效能：尾端插入 O(1) 攤銷；中間插入 O(n)
//  emplace：就地建構（直接初始化），可呼叫 explicit 建構子
//  失效：有 reallocation 則全失效，否則插入點之後失效
// ==============================================
