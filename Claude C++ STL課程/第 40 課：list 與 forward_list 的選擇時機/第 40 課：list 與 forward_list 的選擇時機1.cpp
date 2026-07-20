// =============================================================================
//  第 40 課  —  容器選型的實測：計時能告訴你什麼、不能告訴你什麼
// =============================================================================
//
// 【主題資訊 Information】
//   本檔是第 40 課的「實測版」：對 vector / deque / list / forward_list
//   跑五組工作負載並計時。
//   標頭檔：<vector> <deque> <list> <forward_list> <chrono> <random> <numeric>
//   標準版本：forward_list 為 C++11；<chrono> 為 C++11；
//             std::iota 在 <numeric>（C++98）。
//   量測工具：std::chrono::high_resolution_clock
//             （在 libstdc++ 上等同 steady_clock；GCC 文件與實作皆如此，
//               但標準並未保證，MSVC 曾有不同對應）。
//   關鍵前提：**所有計時數字每次執行都不同**，詳見〈概念補充〉。
//
// 【詳細解釋 Explanation】
//
// 【1. 這五組測試各自在問什麼問題】
//     測試 1 push_front       → 「只在頭端進出」時誰快？（deque vs list vs forward_list）
//     測試 2 中間 insert      → 「已握有迭代器」時 list 的 O(1) 能不能兌現？
//     測試 3 走訪             → cache 效應有多大？（這是 vector 最大的優勢來源）
//     測試 4 排序             → std::sort（vector）vs 成員 sort（list/forward_list）
//     測試 5 記憶體估算       → 節點開銷（非計時，是確定性的算術）
//   看懂「一個 benchmark 在問什麼問題」比看懂數字更重要 ——
//   換一個問法（例如測試 2 改成「每次都要先走到中間」）結論就會反過來。
//
// 【2. 為什麼測試 2 對 list 特別有利（這是刻意的）】
//   程式碼裡 list 版本是這樣寫的：
//       auto mid = l.begin(); advance(mid, N/2);      // 走一次，O(N)
//       for (...) l.insert(mid, i);                    // 之後每次 O(1)
//   而 vector 版本每次都重新計算 v.begin() + v.size()/2 並做 memmove。
//   這是刻意設計的「對 list 最有利」情境，用來展示**迭代器穩定性的價值**：
//   mid 在插入後依然有效，所以只需走一次。
//   若改成「每次都要重新找插入點」，list 就得每次 O(N) 走訪（且 cache 不友善），
//   vector 反而經常勝出。**benchmark 的設定本身就是結論的一部分。**
//
// 【3. 為什麼測試 4 中 list::sort 反而比 std::sort 慢】
//   直覺上 list::sort 只重接指標、不搬移元素，應該比較快。
//   實際上通常慢 2~4 倍，原因是：
//     - std::sort 對連續的 int 陣列極度 cache 友善，且能被向量化
//     - list::sort 的歸併排序每一步都在 pointer chasing，幾乎每次都 cache miss
//     - int 的「搬移成本」本來就趨近於零，所以「不搬移」這個優勢根本沒兌現
//   反過來說，若元素是「移動成本很高」的型別（例如內含多個 std::string），
//   list::sort 的優勢才會顯現。這再次說明：**元素型別會改變結論。**
//
// 【4. 記憶體估算為什麼可以直接用算的】
//   測試 5 沒有計時，而是用節點大小直接乘：
//       list<int>         24 bytes/節點（prev 8 + next 8 + int 4 + padding 4）
//       forward_list<int> 16 bytes/節點（next 8 + int 4 + padding 4）
//   這些是實作定義的值（本機 g++ 15.2 / libstdc++ / x86-64 實測），
//   但一旦確定就是確定的 —— 不像計時會浮動。
//   這也是為什麼「記憶體」比「速度」更適合當作選 forward_list 的理由。
//
// 【概念補充 Concept Deep Dive】
//   為什麼同一支 benchmark 跑兩次可以差好幾倍？主要來源有五個：
//     (1) **CPU 頻率調節**：現代 CPU 會依溫度與負載動態調頻，
//         第一次跑可能還在低頻，跑一陣子才升上來（turbo boost）。
//     (2) **其他行程干擾**：作業系統排程、背景服務都在搶 CPU 與 cache。
//     (3) **記憶體配置器狀態**：第一次 malloc 要跟 OS 要頁面（page fault），
//         之後就從已有的堆積切 —— 差距很大。
//     (4) **分支預測器與 cache 暖機**：同一段程式跑第二次通常明顯較快。
//     (5) **記憶體對齊與 ASLR**：每次執行的位址不同，可能造成 cache 衝突差異。
//   實務上要得到可信的效能結論，至少要：跑多次取**中位數**（不是平均，
//   平均容易被離群值拉走）、先做暖機、固定 CPU 親和性、關閉頻率調節。
//   而若你要證明的是「複雜度」而非「絕對速度」，
//   更好的做法是計算確定性的操作次數 —— 本課 summary.cpp 的 §6 即為示範。
//
// 【注意事項 Pay Attention】
//   1. **所有計時數字每次執行都不同**，下方預期輸出僅為某一次取樣。
//   2. 節點大小（24 / 16 bytes）為實作定義，本機 x86-64 實測值。
//   3. 測試 2 刻意讓 list 預先握有迭代器 —— 這是對 list 最有利的設定。
//   4. 原始碼中的 `volatile long long sum` 是為了防止編譯器把整個
//      走訪迴圈最佳化掉（結果沒被使用 → 可能被完全刪除）。
//      這是寫 benchmark 時必要的防護，但 volatile 也會抑制部分最佳化，
//      更嚴謹的做法是用編譯器屏障（asm volatile("" ::: "memory")）。
//   5. 本檔已把亂數種子固定（見下方說明），讓每次執行的**資料**相同；
//      但這只消除了資料變異，**計時本身仍然會浮動**。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】容器效能實測
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 list::sort 通常比對 vector 用 std::sort 慢，即使前者不搬移元素？
//     答：因為對 int 而言「搬移成本」本來就趨近於零，那個優勢沒有兌現；
//         而 std::sort 對連續陣列極度 cache 友善、還能被向量化，
//         list::sort 的歸併排序則每一步都在 pointer chasing，幾乎次次 cache miss。
//         本機實測 list::sort 約慢 2~3 倍。
//     追問：什麼情況下 list::sort 才會贏？
//           → 元素移動成本很高時（例如內含多個 std::string 或需要深複製的資源）。
//             此時「只重接指標、完全不搬移元素」的優勢才大於 cache 的劣勢。
//             這說明元素型別會直接改變選型結論。
//
// 🔥 Q2. 一個 benchmark 顯示 list 在中間插入比 vector 快 10 倍，
//        這個結論可以直接套用到你的專案嗎？
//     答：要先看它怎麼測的。若 list 版預先握有插入點的迭代器（只走一次），
//         而 vector 版每次都重算位置並 memmove，那是對 list 最有利的設定。
//         實際專案裡若每次都得重新找插入點，list 就得每次 O(N) 走訪
//         且 cache 極不友善，結論常常反過來。
//     追問：那要怎麼測才算公平？
//           → 讓兩邊做「同樣的工作」：都預先握有位置，或都重新尋找。
//             另外要跑多次取中位數、先暖機、並用與正式環境相同的元素型別
//             與資料規模 —— N=100 和 N=1,000,000 的勝負常常相反。
//
// ⚠️ 陷阱. 測試 3 的走訪迴圈若把 `volatile long long sum` 改成普通的
//          `long long sum`，量出來的時間可能變成 0 微秒，為什麼？
//     答：因為 sum 的結果之後沒有被使用，編譯器在 -O2 下可以判定
//         「這個迴圈沒有任何可觀察的副作用」而**整段刪除**（dead code elimination）。
//         於是你量到的是「什麼都沒做」的時間。
//         volatile 告訴編譯器「這個變數的每次存取都有意義，不准省略」，
//         迴圈才會真的被執行。
//     為什麼會錯：以為「我寫了迴圈，它就一定會執行」。
//         在最佳化開啟時，編譯器唯一的義務是保持**可觀察行為**一致 ——
//         沒有輸出、沒有副作用的計算隨時可能整段消失。
//         這是寫 micro-benchmark 最經典的坑；除了 volatile，
//         更精確的做法是用編譯器屏障 asm volatile("" ::: "memory")
//         或 Google Benchmark 的 DoNotOptimize()。
// ═══════════════════════════════════════════════════════════════════════════

// lesson40_selection.cpp
// 編譯：g++ -std=c++17 -O2 -Wall -Wextra -o lesson40 lesson40_selection.cpp

#include <iostream>
#include <vector>
#include <deque>
#include <list>
#include <forward_list>
#include <chrono>
#include <random>
#include <algorithm>
#include <numeric>
#include <string>
#include <iterator>
using namespace std;

template <typename Func>
long long measure_us(Func f) {
    auto start = chrono::high_resolution_clock::now();
    f();
    auto end = chrono::high_resolution_clock::now();
    return chrono::duration_cast<chrono::microseconds>(end - start).count();
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】不強加。
//   理由：本檔是一支效能量測程式，不解任何演算法問題。
//         LeetCode 的評分只看「是否通過 + 大致的時間/空間等級」，
//         不會要求你比較 vector 與 list 的 cache 行為 ——
//         事實上 LeetCode 幾乎所有題目的正解都是直接用 vector。
//         硬掛一題只會模糊「這是一份量測方法論教材」這個定位。
//         同一課的 summary.cpp 已用 LeetCode 705. Design HashSet
//         示範「容器選型如何影響解法」，此處不重複。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】容器選型決策表：把「該選哪個」變成可回答的問題
//   情境：Code review 時最常見的爭論是「這裡該用 vector 還是 list」，
//         而爭論往往停留在「我覺得」的層次。
//   為什麼用到本主題：與其憑感覺，不如把選型拆成幾個**可以明確回答**的問題。
//         下面把本課的全部結論編碼成一個決策函式 ——
//         這種「把經驗變成可檢查的規則」的做法，在真實團隊裡比 benchmark 更有價值，
//         因為它可以寫進文件、放進 review checklist。
// -----------------------------------------------------------------------------
struct Requirement {
    bool needs_random_access = false;   // 需要 v[i] 或 std::sort？
    bool needs_push_front    = false;   // 需要在頭端插入？
    bool needs_push_back     = false;   // 需要在尾端插入？
    bool needs_stable_iters  = false;   // 需要長期持有元素位置？
    bool needs_splice        = false;   // 需要 O(1) 轉移節點到另一個容器？
    bool needs_c_api_pointer = false;   // 需要 data() 傳給 C API？
    bool memory_critical     = false;   // 記憶體極度敏感？
};

const char* recommendContainer(const Requirement& r) {
    // 順序很重要：從「硬性限制」排到「偏好」
    if (r.needs_c_api_pointer) return "vector（唯一保證連續記憶體、提供 data()）";
    if (r.needs_random_access) {
        return r.needs_push_front
             ? "deque（隨機存取 + 兩端 O(1)，但不連續）"
             : "vector（隨機存取 + 最佳 cache 效率）";
    }
    if (r.needs_splice)       return "list（splice 是 O(1)，其他容器做不到）";
    if (r.needs_stable_iters) {
        return r.needs_push_back
             ? "list（迭代器穩定 + 兩端皆可 O(1)）"
             : "forward_list（迭代器穩定，且每節點省 8 bytes）";
    }
    if (r.memory_critical && !r.needs_push_back)
        return "forward_list（節點最小，但沒有 push_back / size()）";
    if (r.needs_push_front)   return "deque（頭尾皆 O(1)，且保有部分 cache 優勢）";
    return "vector（沒有特殊需求時的預設答案）";
}

int main() {
    const int N = 100000;
    // 注意：原本這裡是 random_device rd; mt19937 gen(rd());
    //       那會讓每次執行的**測試資料**都不同，使結果多了一項不必要的變異來源。
    //       教學用的 benchmark 應該固定種子，讓「資料」是常數，
    //       這樣浮動就只剩下計時本身（見檔頭〈概念補充〉列出的五個來源）。
    mt19937 gen(42);

    // ===== 測試 1：頭端插入 =====
    cout << "===== 測試 1：" << N << " 次 push_front =====" << endl;
    {
        auto t_deque = measure_us([&]() {
            deque<int> dq;
            for (int i = 0; i < N; i++) dq.push_front(i);
        });
        auto t_list = measure_us([&]() {
            list<int> lst;
            for (int i = 0; i < N; i++) lst.push_front(i);
        });
        auto t_flist = measure_us([&]() {
            forward_list<int> flst;
            for (int i = 0; i < N; i++) flst.push_front(i);
        });

        cout << "  deque:        " << t_deque << " us" << endl;
        cout << "  list:         " << t_list << " us" << endl;
        cout << "  forward_list: " << t_flist << " us" << endl;
    }

    // ===== 測試 2：中間插入（已持有迭代器）=====
    cout << "\n===== 測試 2：中間插入（已持有迭代器）=====" << endl;
    {
        // 預先建立容器
        list<int> lst;
        for (int i = 0; i < N; i++) lst.push_back(i);
        auto lst_mid = lst.begin();
        advance(lst_mid, N / 2);

        vector<int> vec(N);
        iota(vec.begin(), vec.end(), 0);

        const int INSERT_COUNT = 10000;

        auto t_vec = measure_us([&]() {
            auto v = vec;
            for (int i = 0; i < INSERT_COUNT; i++) {
                v.insert(v.begin() + v.size() / 2, i);
            }
        });
        auto t_list = measure_us([&]() {
            auto l = lst;
            auto mid = l.begin();
            advance(mid, N / 2);
            for (int i = 0; i < INSERT_COUNT; i++) {
                l.insert(mid, i);  // mid 不會失效！
            }
        });

        cout << "  vector（中間insert " << INSERT_COUNT << "次）: " << t_vec << " us" << endl;
        cout << "  list  （中間insert " << INSERT_COUNT << "次）: " << t_list << " us" << endl;
        if (t_list < t_vec) {
            cout << "  → list 更快（迭代器穩定 + O(1) 插入）" << endl;
        } else {
            cout << "  → vector 更快（快取效率）" << endl;
        }
    }

    // ===== 測試 3：遍歷效能 =====
    cout << "\n===== 測試 3：遍歷 " << N << " 個元素 =====" << endl;
    {
        vector<int> vec(N);
        iota(vec.begin(), vec.end(), 0);

        list<int> lst(vec.begin(), vec.end());
        forward_list<int> flst(vec.begin(), vec.end());

        volatile long long sum = 0;

        auto t_vec = measure_us([&]() {
            sum = 0;
            for (int val : vec) sum += val;
        });
        auto t_list = measure_us([&]() {
            sum = 0;
            for (int val : lst) sum += val;
        });
        auto t_flist = measure_us([&]() {
            sum = 0;
            for (int val : flst) sum += val;
        });

        cout << "  vector:       " << t_vec << " us" << endl;
        cout << "  list:         " << t_list << " us" << endl;
        cout << "  forward_list: " << t_flist << " us" << endl;
        cout << "  → vector 通常最快（連續記憶體 + CPU 預取）" << endl;
    }

    // ===== 測試 4：排序 =====
    cout << "\n===== 測試 4：排序 " << N << " 個元素 =====" << endl;
    {
        vector<int> data(N);
        for (int i = 0; i < N; i++) data[i] = gen() % 1000000;

        vector<int> vec = data;
        list<int> lst(data.begin(), data.end());
        forward_list<int> flst(data.begin(), data.end());

        auto t_vec = measure_us([&]() {
            sort(vec.begin(), vec.end());
        });
        auto t_list = measure_us([&]() {
            lst.sort();
        });
        auto t_flist = measure_us([&]() {
            flst.sort();
        });

        cout << "  vector std::sort:   " << t_vec << " us" << endl;
        cout << "  list::sort:         " << t_list << " us" << endl;
        cout << "  forward_list::sort: " << t_flist << " us" << endl;
        cout << "  → vector 通常快 2~4 倍（快取效率）" << endl;
    }

    // ===== 測試 5：記憶體用量 =====
    cout << "\n===== 測試 5：記憶體估算（" << N << " 個 int）=====" << endl;
    {
        cout << "  vector:       " << (N * sizeof(int)) / 1024 << " KB" << endl;
        cout << "  list:         ~" << (N * 24) / 1024 << " KB（每節點 24B）" << endl;
        cout << "  forward_list: ~" << (N * 16) / 1024 << " KB（每節點 16B）" << endl;
        cout << "  forward_list 比 list 省 " << (N * 8) / 1024 << " KB" << endl;
    }

    // ===== 日常實務：容器選型決策表 =====
    cout << "\n===== 日常實務：容器選型決策表 =====" << endl;
    {
        struct Case { const char* name; Requirement req; };
        vector<Case> cases = {
            {"影像緩衝區（要傳給 C 的編碼函式庫）",
             []{ Requirement r; r.needs_c_api_pointer = true; return r; }()},

            {"排行榜（要排序 + 依名次取值）",
             []{ Requirement r; r.needs_random_access = true; return r; }()},

            {"滑動視窗（兩端進出 + 要看第 k 個）",
             []{ Requirement r; r.needs_random_access = true;
                 r.needs_push_front = true; return r; }()},

            {"LRU 快取（要 O(1) 把節點搬到頭端）",
             []{ Requirement r; r.needs_splice = true;
                 r.needs_stable_iters = true; return r; }()},

            {"編輯器書籤（長期持有段落位置，兩端都要插）",
             []{ Requirement r; r.needs_stable_iters = true;
                 r.needs_push_back = true; return r; }()},

            {"雜湊表的桶（百萬個桶，只需單向走訪）",
             []{ Requirement r; r.memory_critical = true; return r; }()},

            {"一般的資料清單（沒有特殊需求）",
             Requirement{}},
        };

        for (const Case& c : cases) {
            cout << "  " << c.name << "\n    → " << recommendContainer(c.req) << endl;
        }
        cout << "  → 把選型拆成幾個可明確回答的問題，" << endl;
        cout << "    比在 code review 上憑感覺爭論有效得多" << endl;
    }

    return 0;
}

// 注意（非決定性輸出）：
//   測試 1~4 印出的是 wall-clock 微秒數，**每次執行都會不同**。
//   造成浮動的原因至少有五個（CPU 頻率調節、其他行程干擾、
//   記憶體配置器狀態、分支預測器與 cache 暖機、ASLR 造成的對齊差異），
//   詳見檔頭〈概念補充〉。同一台機器上跑兩次相差數倍是常態。
//   下方預期輸出中的微秒數字僅為本機 g++ 15.2 -O2 的某一次取樣，
//   **不可當作測試斷言**；要看的是「哪個明顯較快」這個相對關係。
//   （測試 2 的結尾會依當次計時結果印出不同的結論句，也屬非決定性輸出。）
//   測試 5 的記憶體估算則是確定性的算術，使用實作定義的節點大小
//   （list 24 bytes / forward_list 16 bytes，本機 x86-64 實測）。
//   本檔已固定亂數種子為 42，因此**測試資料**每次相同，
//   但這只消除了資料變異，計時本身仍會浮動。

// 編譯: g++ -std=c++17 -O2 -Wall -Wextra 第 40 課：list 與 forward_list 的選擇時機1.cpp -o lesson40

// === 預期輸出 ===
// ===== 測試 1：100000 次 push_front =====
//   deque:        219 us
//   list:         2908 us
//   forward_list: 2155 us
//
// ===== 測試 2：中間插入（已持有迭代器）=====
//   vector（中間insert 10000次）: 27553 us
//   list  （中間insert 10000次）: 3421 us
//   → list 更快（迭代器穩定 + O(1) 插入）
//
// ===== 測試 3：遍歷 100000 個元素 =====
//   vector:       113 us
//   list:         143 us
//   forward_list: 156 us
//   → vector 通常最快（連續記憶體 + CPU 預取）
//
// ===== 測試 4：排序 100000 個元素 =====
//   vector std::sort:   4120 us
//   list::sort:         9831 us
//   forward_list::sort: 11661 us
//   → vector 通常快 2~4 倍（快取效率）
//
// ===== 測試 5：記憶體估算（100000 個 int）=====
//   vector:       390 KB
//   list:         ~2343 KB（每節點 24B）
//   forward_list: ~1562 KB（每節點 16B）
//   forward_list 比 list 省 781 KB
//
// ===== 日常實務：容器選型決策表 =====
//   影像緩衝區（要傳給 C 的編碼函式庫）
//     → vector（唯一保證連續記憶體、提供 data()）
//   排行榜（要排序 + 依名次取值）
//     → vector（隨機存取 + 最佳 cache 效率）
//   滑動視窗（兩端進出 + 要看第 k 個）
//     → deque（隨機存取 + 兩端 O(1)，但不連續）
//   LRU 快取（要 O(1) 把節點搬到頭端）
//     → list（splice 是 O(1)，其他容器做不到）
//   編輯器書籤（長期持有段落位置，兩端都要插）
//     → list（迭代器穩定 + 兩端皆可 O(1)）
//   雜湊表的桶（百萬個桶，只需單向走訪）
//     → forward_list（節點最小，但沒有 push_back / size()）
//   一般的資料清單（沒有特殊需求）
//     → vector（沒有特殊需求時的預設答案）
//   → 把選型拆成幾個可明確回答的問題，
//     比在 code review 上憑感覺爭論有效得多
