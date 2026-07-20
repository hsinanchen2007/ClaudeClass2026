// =============================================================================
//  第 40 課 總結  —  四大序列容器的選擇：大 O 之外還要看什麼
// =============================================================================
//
// 【主題資訊 Information】
//   四個候選容器與各自的標頭檔：
//     <vector>        連續記憶體、動態陣列
//     <deque>         分段連續（本機 libstdc++ 每段 512 bytes）
//     <list>          雙向鏈結串列（節點 24 bytes/int，本機實測）
//     <forward_list>  單向鏈結串列（節點 16 bytes/int，本機實測）C++11 新增
//   複雜度速查：
//                    隨機存取  頭端插刪  尾端插刪  中間插刪  迭代器穩定
//     vector          O(1)      O(N)      O(1)攤銷  O(N)      ✗
//     deque           O(1)      O(1)      O(1)      O(N)      ✗（參考仍有效）
//     list            ✗         O(1)      O(1)      O(1)*     ✓
//     forward_list    ✗         O(1)      ✗         O(1)*     ✓
//     * 前提是「已經握有該位置的迭代器」，否則走過去本身就是 O(N)
//   標準版本：vector/deque/list 自 C++98；forward_list 是 C++11。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼「vector 是預設答案」不是偷懶】
//   大 O 相同時，決勝的是常數項與記憶體階層，而 vector 在這兩者上都壓倒性領先：
//     - **連續記憶體**：一次 cache miss 帶回 64 bytes = 16 個 int，
//       接下來 15 次存取全部命中。list 每跳一個節點就可能是一次 miss。
//     - **硬體預取**：CPU 能預測連續存取並提前抓資料；
//       list 的 pointer chasing 必須先讀到 next 才知道下一個位址，預取器無能為力。
//     - **記憶體用量**：vector<int> 每元素 4 bytes；list<int> 每節點 24 bytes（6 倍）。
//     - **配置次數**：vector 攤銷後 O(log N) 次配置；list 每個元素一次 malloc。
//   Bjarne Stroustrup 多次公開展示過一個反直覺的實測：
//   即使是「在排序序列中間插入」這種理論上對 list 有利的工作負載，
//   vector 在數萬元素以內仍然勝出 —— 因為「找到插入點」那一步
//   在 list 上是 cache 極不友善的走訪，而 vector 的 memmove 快得驚人。
//
// 【2. 那 list 什麼時候真的贏】
//   兩個條件必須**同時**成立：
//     (a) 你已經握有插入/刪除位置的迭代器（不需要走過去找）
//     (b) 元素本身很大、或搬移成本很高（例如含有需要深複製的資源）
//   典型場景：LRU cache（拿 hash map 直接定位到節點）、
//   編輯器的段落串列（書籤即迭代器）、任務排程器的可插拔佇列。
//   還有一個 list 獨有、無可取代的能力：**splice** ——
//   O(1) 把節點從一條串列轉移到另一條，不複製、不移動、迭代器仍有效。
//
// 【3. forward_list 的定位：省記憶體，不是求快】
//   它比 list 每節點省 8 bytes（33%），但**速度通常沒有優勢**，
//   甚至因為缺少 prev 而在某些操作上更麻煩
//   （想刪除 it 指向的元素，必須先握有它的前一個迭代器）。
//   選它的理由是記憶體：節點數以百萬計、或嵌入式環境。
//   最正當的用途是雜湊表的分離鏈結桶（bucket）——
//   libstdc++ 的 std::unordered_map 內部正是這樣做的。
//
// 【4. deque 常常是被遺忘的正解】
//   需要「兩端都能 O(1) 進出」時，很多人直覺選 list，
//   但 deque 幾乎總是更好：它同樣 O(1)，卻保有分段連續帶來的 cache 優勢，
//   而且支援 operator[] 與 std::sort。
//   deque 唯一的重要限制是**不連續** —— 不能把 &deq[0] 當陣列指標傳給 C API。
//
// 【概念補充 Concept Deep Dive】
//   計時（wall-clock）作為證據其實很脆弱：CPU 頻率調節、其他行程干擾、
//   分支預測器暖機、記憶體配置器狀態，都會讓同一份程式的結果在同一台機器上
//   相差數倍。因此下方的計時區塊**每次執行的數字都會不同**。
//   更可靠的證據是「計數確定性的操作次數」。
//   本檔在計時之後另加了一個 §6：直接計算「元素被移動的次數」，
//   證明 vector 中間插入是 O(N²)、list 是 O(N) —— 這個數字每次跑都一模一樣，
//   而且能直接對應到理論複雜度。
//   結論：**要證明複雜度就數操作次數；要證明實際效能才用計時，且要多跑幾次取中位數。**
//
// 【注意事項 Pay Attention】
//   1. 節點大小（24 / 16 bytes）與 deque 區塊大小（512 bytes）都是**實作定義**，
//      本檔數值為本機 g++ 15.2 / libstdc++ / x86-64 的實測結果。
//   2. list 的 O(1) 插入前提是「已握有迭代器」；要先走過去就是 O(N)。
//   3. deque 是 Random Access 但**不連續**，沒有 data()，不可當陣列指標用。
//   4. forward_list 沒有 size()、push_back()、rbegin()。
//   5. 計時結果每次執行都不同，不能當作可重現的測試斷言。
//   6. 「省記憶體」與「跑得快」是兩個不同目標，forward_list 只保證前者。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】序列容器的選擇
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 需要頻繁在中間插入，是不是就該用 list？
//     答：不一定，而且實務上常常是「不該」。list 的 O(1) 插入有個前提：
//         **你已經握有那個位置的迭代器**。如果得先走過去找，那一步就是 O(N)，
//         而且是 cache 極不友善的 pointer chasing。
//         vector 的插入雖是 O(N)，但做的是連續記憶體的 memmove ——
//         硬體跑得極快。數萬元素以內、元素又是小型 POD 時，vector 經常勝出。
//         真正該選 list 的條件是「已有迭代器」＋「元素搬移成本高」。
//     追問：那 list 有沒有什麼是 vector 完全做不到的？
//           → 有兩件。(1) splice：O(1) 把節點轉移到另一條串列，
//             不複製不移動且迭代器仍有效。(2) 迭代器穩定性：
//             插入刪除不會使其他迭代器失效，所以可以長期持有元素位置。
//             LRU cache 同時需要這兩者，這就是它必用 list 的原因。
//
// 🔥 Q2. list 和 forward_list 該怎麼選？
//     答：先問「需不需要往回走」。需要就用 list，不需要才考慮 forward_list。
//         forward_list 每節點省 8 bytes（24→16，本機實測），
//         但**速度通常沒有優勢**，而且缺少 prev 讓某些操作更麻煩
//         （要刪除 it 指的元素得先握有它的前一個）。
//         選它的唯一理由是記憶體 —— 節點數以百萬計或嵌入式環境。
//     追問：實務上哪裡真的會用到 forward_list？
//           → 雜湊表的分離鏈結桶。每個桶只需單向走訪、
//             插入一律在頭端、而桶的數量可能上百萬個 ——
//             省下的 prev 指標就是好幾 MB。libstdc++ 的 unordered_map
//             內部正是用單向串列實作的。
//
// ⚠️ 陷阱. 「我跑了 benchmark，list 比 vector 快 3 倍，所以這個場景該用 list」
//          —— 這個推論可能哪裡有問題？
//     答：計時作為證據非常脆弱。常見的量測錯誤包括：
//         (1) 只跑一次就下結論（CPU 頻率調節、其他行程干擾可造成數倍差異）
//         (2) 兩邊的工作量其實不對等（例如 list 版預先握有迭代器，
//             vector 版每次都重新計算插入點）
//         (3) 沒有考慮元素型別（int 與含 std::string 的結構結論完全不同）
//         (4) 測試規模不具代表性（N=100 與 N=1,000,000 的勝負常常相反）
//         更可靠的做法是**計算確定性的操作次數**（元素移動次數、比較次數），
//         那是每次執行都一樣、且能直接對應理論複雜度的證據。
//     為什麼會錯：把「這次跑出來的數字」當成「這件事的性質」。
//         效能是量測出來的沒錯，但單次量測不是量測 —— 它只是一個樣本。
//         本檔的 §6 就示範了如何用可重現的計數取代脆弱的計時。
// ═══════════════════════════════════════════════════════════════════════════

// ============================================================
// 第 40 課 總結：list 與 forward_list 的選擇時機
// 編譯：g++ -std=c++17 -O2 -o summary summary.cpp
// ============================================================
// 【四大序列容器選擇指南】
//   ┌──────────────┬─────────┬─────────┬─────────┬─────────────┐
//   │              │ vector  │ deque   │ list    │ forward_list│
//   ├──────────────┼─────────┼─────────┼─────────┼─────────────┤
//   │ 隨機存取     │ O(1) ★ │ O(1)   │ ❌      │ ❌          │
//   │ 頭端插刪     │ O(n)   │ O(1) ★ │ O(1)    │ O(1)        │
//   │ 尾端插刪     │ O(1) ★ │ O(1)   │ O(1)    │ ❌          │
//   │ 中間插刪     │ O(n)   │ O(n)   │ O(1) ★ │ O(1) ★     │
//   │ 迭代器穩定   │ ❌     │ ❌     │ ✅ ★   │ ✅ ★       │
//   │ 記憶體效率   │ ★★★  │ ★★   │ ★      │ ★★         │
//   │ cache 效率   │ ★★★  │ ★★   │ ★      │ ★           │
//   │ splice       │ ❌     │ ❌     │ O(1) ★ │ O(1) ★     │
//   └──────────────┴─────────┴─────────┴─────────┴─────────────┘
//
// 【選擇建議】
//   預設首選：vector（最好的 cache 效率）
//   需要雙端操作：deque
//   需要頻繁中間插刪 + 迭代器穩定：list
//   記憶體極度敏感 + 只需前向：forward_list
//   需要 splice：list / forward_list
// ============================================================

#include <iostream>
#include <vector>
#include <deque>
#include <list>
#include <forward_list>
#include <chrono>
#include <random>
#include <algorithm>
#include <numeric>
#include <iterator>
using namespace std;

// 注意：原本這裡寫成 `auto measure_us(auto f)`。
//       「函式參數用 auto」是 C++20 的縮寫函式樣板（abbreviated function template），
//       但本檔宣告使用 C++17 —— g++ 只是以擴充功能放行並發出
//       -Wc++20-extensions 警告，並非合法的 C++17。
//       改寫成明確的函式樣板後，在 C++17 下完全合規且行為不變。
template <typename Func>
long long measure_us(Func f) {
    auto t = chrono::high_resolution_clock::now();
    f();
    return chrono::duration_cast<chrono::microseconds>(
        chrono::high_resolution_clock::now() - t).count();
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 705. Design HashSet
//   題目：不使用內建雜湊表，自行實作 add / remove / contains。
//   為什麼用到本主題：這題的核心決策就是「每個桶要用什麼容器」——
//         正是本課「容器選擇」的實戰。逐一檢視候選：
//           vector<int>        每桶一個 vector = 每桶至少 24 bytes 的物件開銷，
//                              桶數上千時光是空桶就吃掉大量記憶體
//           list<int>          可行，但每節點多一個用不到的 prev 指標
//           forward_list<int>  ★ 最佳：只需單向走訪、插入固定在頭端、
//                              節點最小（16 vs 24 bytes，本機實測）
//         這正是 libstdc++ 的 std::unordered_map 內部採用單向串列的理由。
//   複雜度：平均 O(1)（假設雜湊均勻）；最壞 O(N)（全部擠在同一桶）。
// -----------------------------------------------------------------------------
class MyHashSet {
    static const int BUCKETS = 769;          // 質數可減少雜湊聚集
    vector<forward_list<int>> table_;

    int bucketOf(int key) const { return key % BUCKETS; }

public:
    MyHashSet() : table_(BUCKETS) {}

    void add(int key) {
        if (contains(key)) return;
        table_[static_cast<size_t>(bucketOf(key))].push_front(key);   // O(1)
    }

    void remove(int key) {
        auto& chain = table_[static_cast<size_t>(bucketOf(key))];
        // 單向串列刪除：必須握有「前一個」，所以從 before_begin 起走
        auto prev_it = chain.before_begin();
        for (auto it = chain.begin(); it != chain.end(); ++it) {
            if (*it == key) { chain.erase_after(prev_it); return; }
            prev_it = it;
        }
    }

    bool contains(int key) const {
        const auto& chain = table_[static_cast<size_t>(bucketOf(key))];
        for (int v : chain) if (v == key) return true;
        return false;
    }

    // 教學用：回報記憶體特性
    size_t nonEmptyBuckets() const {
        size_t n = 0;
        for (const auto& c : table_) if (!c.empty()) ++n;
        return n;
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】用「操作次數」而非「計時」證明複雜度
//   情境：團隊爭論「中間插入到底該用 vector 還是 list」，
//         但每個人跑 benchmark 得到的數字都不一樣（機器忙碌、頻率調節……）。
//   為什麼用到本主題：計時是脆弱的證據；**計算元素被移動的次數**
//         才是每次執行都一模一樣、且能直接對應理論複雜度的證據。
//         下面用一個會自報搬移次數的元素型別，
//         直接量出 vector 的 O(N²) 與 list 的 O(N)。
// -----------------------------------------------------------------------------
long long g_moves = 0;      // 全域搬移計數器

struct Tracked {
    int v = 0;
    Tracked() = default;
    explicit Tracked(int x) : v(x) {}
    Tracked(const Tracked& o) : v(o.v) { ++g_moves; }
    Tracked(Tracked&& o) noexcept : v(o.v) { ++g_moves; }
    Tracked& operator=(const Tracked& o) { v = o.v; ++g_moves; return *this; }
    Tracked& operator=(Tracked&& o) noexcept { v = o.v; ++g_moves; return *this; }
};

// 在正中間插入 n 次，回傳元素總搬移次數
long long vectorMiddleInsertMoves(int n) {
    g_moves = 0;
    vector<Tracked> v;
    v.reserve(static_cast<size_t>(n) + 1);   // 先配足容量，排除「重新配置」的干擾
    for (int i = 0; i < n; ++i) {
        v.insert(v.begin() + static_cast<ptrdiff_t>(v.size() / 2), Tracked(i));
    }
    return g_moves;
}

long long listMiddleInsertMoves(int n) {
    g_moves = 0;
    list<Tracked> l;
    auto mid = l.end();                       // 事先握有插入位置的迭代器
    for (int i = 0; i < n; ++i) {
        l.insert(mid, Tracked(i));            // 純粹接指標，不搬移既有元素
    }
    return g_moves;
}

int main() {
    const int N = 100000;

    // 1. 頭端插入
    cout << "===== 1. " << N << " 次 push_front =====\n";
    {
        auto t_dq = measure_us([&]{ deque<int> d; for(int i=0;i<N;i++) d.push_front(i); });
        auto t_lst = measure_us([&]{ list<int> l; for(int i=0;i<N;i++) l.push_front(i); });
        auto t_fl = measure_us([&]{ forward_list<int> f; for(int i=0;i<N;i++) f.push_front(i); });
        cout << "  deque:        " << t_dq << " us\n";
        cout << "  list:         " << t_lst << " us\n";
        cout << "  forward_list: " << t_fl << " us\n";
    }

    // 2. 中間插入（已有迭代器）
    cout << "\n===== 2. 中間 insert 10000 次 =====\n";
    {
        const int INS = 10000;
        auto t_vec = measure_us([&]{
            vector<int> v(N);
            for(int i=0;i<INS;i++) v.insert(v.begin()+v.size()/2, i);
        });
        auto t_lst = measure_us([&]{
            list<int> l(N);
            auto mid=l.begin(); advance(mid, N/2);
            for(int i=0;i<INS;i++) l.insert(mid, i);
        });
        cout << "  vector: " << t_vec << " us\n";
        cout << "  list:   " << t_lst << " us\n";
    }

    // 3. 遍歷效能
    cout << "\n===== 3. 遍歷 " << N << " 個元素 =====\n";
    {
        vector<int> vec(N); iota(vec.begin(), vec.end(), 0);
        list<int> lst(vec.begin(), vec.end());
        forward_list<int> fl(vec.begin(), vec.end());
        volatile long long sum = 0;

        auto t_vec = measure_us([&]{ sum=0; for(int v:vec) sum+=v; });
        auto t_lst = measure_us([&]{ sum=0; for(int v:lst) sum+=v; });
        auto t_fl  = measure_us([&]{ sum=0; for(int v:fl)  sum+=v; });
        cout << "  vector:       " << t_vec << " us\n";
        cout << "  list:         " << t_lst << " us\n";
        cout << "  forward_list: " << t_fl  << " us\n";
        cout << "  → vector 最快（連續記憶體 + CPU 預取）\n";
    }

    // 4. 排序效能
    cout << "\n===== 4. 排序 " << N << " 個元素 =====\n";
    {
        mt19937 gen(42);
        vector<int> data(N);
        for(auto& v:data) v = gen() % 1000000;
        vector<int> vec = data;
        list<int> lst(data.begin(), data.end());
        forward_list<int> fl(data.begin(), data.end());

        auto t_vec = measure_us([&]{ sort(vec.begin(), vec.end()); });
        auto t_lst = measure_us([&]{ lst.sort(); });
        auto t_fl  = measure_us([&]{ fl.sort(); });
        cout << "  vector std::sort:   " << t_vec << " us\n";
        cout << "  list::sort:         " << t_lst << " us\n";
        cout << "  forward_list::sort: " << t_fl  << " us\n";
    }

    // 5. 記憶體用量
    cout << "\n===== 5. 記憶體估算（" << N << " 個 int）=====\n";
    cout << "  vector:       " << (N*sizeof(int))/1024 << " KB\n";
    cout << "  list:         ~" << (N*24)/1024 << " KB（每節點 24B）\n";
    cout << "  forward_list: ~" << (N*16)/1024 << " KB（每節點 16B）\n";
    cout << "  forward_list 比 list 省 " << (N*8)/1024 << " KB\n";

    // 6. 用「操作次數」取代計時 —— 可重現的複雜度證據
    cout << "\n===== 6. 可重現的證據：元素搬移次數（不是計時）=====\n";
    {
        cout << "  在正中間插入 n 次，統計「元素被複製/移動」的總次數：\n";
        cout << "    n        vector 搬移次數    list 搬移次數\n";
        for (int n : {500, 1000, 2000, 4000}) {
            long long vm = vectorMiddleInsertMoves(n);
            long long lm = listMiddleInsertMoves(n);
            cout << "    " << n;
            for (int pad = 0; pad < 9 - static_cast<int>(to_string(n).size()); ++pad)
                cout << ' ';
            cout << vm;
            for (int pad = 0; pad < 19 - static_cast<int>(to_string(vm).size()); ++pad)
                cout << ' ';
            cout << lm << "\n";
        }
        cout << "  → vector：n 翻倍，搬移次數約變 4 倍 = O(n²)\n";
        cout << "  → list  ：n 翻倍，搬移次數剛好 2 倍 = O(n)（每個元素只搬進去一次）\n";
        cout << "  這組數字每次執行都完全相同，比計時可靠得多。\n";
    }

    // 7. LeetCode 705
    cout << "\n===== LeetCode 705. Design HashSet =====\n";
    {
        MyHashSet hs;
        hs.add(1);
        hs.add(2);
        cout << "  add(1), add(2)\n";
        cout << "  contains(1) = " << (hs.contains(1) ? "true" : "false") << "\n";
        cout << "  contains(3) = " << (hs.contains(3) ? "true" : "false") << "\n";
        hs.add(2);
        cout << "  add(2) 重複 → contains(2) = " << (hs.contains(2) ? "true" : "false") << "\n";
        hs.remove(2);
        cout << "  remove(2)   → contains(2) = " << (hs.contains(2) ? "true" : "false") << "\n";

        // 加入一批會落在同一桶的 key，觀察鏈結長度
        for (int k = 0; k < 5; ++k) hs.add(769 * k + 100);   // 全部 % 769 == 100
        cout << "  再加入 5 個雜湊到同一桶的 key\n";
        cout << "  非空桶數 = " << hs.nonEmptyBuckets() << "\n";
        cout << "  → 桶用 forward_list：只需單向走訪、插入固定在頭端，\n";
        cout << "    每節點比 list 省 8 bytes（16 vs 24，本機實測）\n";
    }

    // 選擇指南
    cout << "\n===== 選擇指南 =====\n";
    cout << "  預設首選 → vector\n";
    cout << "  需要 push_front → deque\n";
    cout << "  需要頻繁中間插刪 + 迭代器穩定 → list\n";
    cout << "  需要 splice → list\n";
    cout << "  記憶體極度敏感 + 只需前向 → forward_list\n";
    cout << "  需要 data() 指標 → vector\n";

    return 0;
}

// 注意（非決定性輸出）：
//   §1~§4 印出的是 wall-clock 微秒數，**每次執行都會不同** ——
//   受 CPU 頻率調節、其他行程、記憶體配置器狀態影響，
//   同一台機器上跑兩次相差數倍是常態。
//   下方預期輸出中的那些微秒數字僅為本機 g++ 15.2 -O2 的某一次取樣，
//   **不可當作測試斷言**；要看的是「哪個明顯較快」這個相對關係。
//   §6 的搬移次數則完全確定、每次執行完全相同 —— 那才是複雜度的可靠證據。
//   §5 的記憶體估算使用實作定義的節點大小
//   （list 24 bytes / forward_list 16 bytes，本機 x86-64 實測）。

// 編譯: g++ -std=c++17 -O2 -Wall -Wextra summary.cpp -o summary

// === 預期輸出 ===
// ===== 1. 100000 次 push_front =====
//   deque:        223 us
//   list:         2822 us
//   forward_list: 2076 us
//
// ===== 2. 中間 insert 10000 次 =====
//   vector: 29436 us
//   list:   2678 us
//
// ===== 3. 遍歷 100000 個元素 =====
//   vector:       112 us
//   list:         165 us
//   forward_list: 190 us
//   → vector 最快（連續記憶體 + CPU 預取）
//
// ===== 4. 排序 100000 個元素 =====
//   vector std::sort:   4061 us
//   list::sort:         9731 us
//   forward_list::sort: 12018 us
//
// ===== 5. 記憶體估算（100000 個 int）=====
//   vector:       390 KB
//   list:         ~2343 KB（每節點 24B）
//   forward_list: ~1562 KB（每節點 16B）
//   forward_list 比 list 省 781 KB
//
// ===== 6. 可重現的證據：元素搬移次數（不是計時）=====
//   在正中間插入 n 次，統計「元素被複製/移動」的總次數：
//     n        vector 搬移次數    list 搬移次數
//     500      63000              500
//     1000     251000             1000
//     2000     1002000            2000
//     4000     4004000            4000
//   → vector：n 翻倍，搬移次數約變 4 倍 = O(n²)
//   → list  ：n 翻倍，搬移次數剛好 2 倍 = O(n)（每個元素只搬進去一次）
//   這組數字每次執行都完全相同，比計時可靠得多。
//
// ===== LeetCode 705. Design HashSet =====
//   add(1), add(2)
//   contains(1) = true
//   contains(3) = false
//   add(2) 重複 → contains(2) = true
//   remove(2)   → contains(2) = false
//   再加入 5 個雜湊到同一桶的 key
//   非空桶數 = 2
//   → 桶用 forward_list：只需單向走訪、插入固定在頭端，
//     每節點比 list 省 8 bytes（16 vs 24，本機實測）
//
// ===== 選擇指南 =====
//   預設首選 → vector
//   需要 push_front → deque
//   需要頻繁中間插刪 + 迭代器穩定 → list
//   需要 splice → list
//   記憶體極度敏感 + 只需前向 → forward_list
//   需要 data() 指標 → vector
