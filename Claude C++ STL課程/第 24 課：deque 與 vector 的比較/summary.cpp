// =============================================================================
//  summary.cpp  —  第 24 課總複習：deque 與 vector 的比較（選容器的決策依據）
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<vector>、<deque>、<chrono>
//   標準版本：std::vector / std::deque 為 C++98；<chrono> 為 C++11；
//             結構化綁定與 CTAD 為 C++17（本檔未用到，僅供對照）
//
//   ┌─────────────┬──────────────────────┬──────────────────────────────┐
//   │ 操作        │ std::vector          │ std::deque                   │
//   ├─────────────┼──────────────────────┼──────────────────────────────┤
//   │ operator[]  │ O(1)（一次定址）     │ O(1)（兩次定址，常數較大）   │
//   │ push_back   │ 攤銷 O(1)            │ 攤銷 O(1)                    │
//   │ pop_back    │ O(1)                 │ O(1)                         │
//   │ push_front  │ 不存在（要 insert，  │ 攤銷 O(1)                    │
//   │             │ 那是 O(n)）          │                              │
//   │ pop_front   │ 不存在（erase 是     │ O(1)                         │
//   │             │ O(n)）               │                              │
//   │ 中間 insert │ O(n)                 │ O(n)（可往較近的一端搬）     │
//   │ data()      │ 有（連續記憶體保證） │ 沒有（不保證連續）           │
//   │ 迭代器類別  │ 連續（C++20 起      │ 隨機存取（但非連續）         │
//   │             │ contiguous_iterator）│                              │
//   └─────────────┴──────────────────────┴──────────────────────────────┘
//
// 【詳細解釋 Explanation】
//
// 【1. 兩者的記憶體模型：一整塊 vs 一疊分頁】
//   vector 持有「一塊連續記憶體」，內部只需三個指標：
//       begin（起點）、end（目前最後一個元素之後）、cap（配置區塊之後）
//   所以 sizeof(std::vector<T>) 在 libstdc++ 是 24 bytes（三個 8-byte 指標，
//   實作定義；換平台或換標準庫可能不同）。
//
//   deque 持有「一塊 map（指標陣列）」，map 的每一格指向一塊固定大小的 chunk。
//   元素放在 chunk 裡；chunk 之間彼此不相鄰。deque 的迭代器因此比較胖，
//   要記住四件事：目前元素位址、目前 chunk 的頭、目前 chunk 的尾、
//   以及自己在 map 中的位置（這樣才能跨 chunk 前進／後退）。
//
// 【2. 為什麼 deque 的 push_front 能是 O(1)，vector 卻連這個函式都沒有】
//   vector 的第一個元素就在配置區塊的最前端，前面沒有空間。要在前面插入，
//   唯一辦法是把所有既有元素往後搬一格 → O(n)。標準庫的設計哲學是
//   「不要提供一個看起來便宜、實際上很貴的介面」，所以乾脆不給 push_front。
//   這是很值得學的 API 設計原則：讓昂貴的操作在語法上就顯得昂貴
//   （你必須自己寫 v.insert(v.begin(), x)，一眼就看得出是插入到最前面）。
//
//   deque 的 map 在兩端都刻意留白，往前長只要在 map 前端補上新 chunk 的位址，
//   既有元素完全不動 → 攤銷 O(1)。
//
// 【3. 擴容行為：一次大搬家 vs 逐塊追加】
//   vector 擴容 = 配置新區塊 + 搬移全部元素 + 釋放舊區塊。
//   單次 O(n)，但因容量呈等比成長（libstdc++／libc++ 為 2 倍，MSVC 為 1.5 倍，
//   皆為實作定義），n 次 push_back 總成本 O(n) → 攤銷 O(1)。
//   關鍵副作用：擴容會讓所有既有的 iterator／pointer／reference 全部失效。
//
//   deque 擴容 = 再配一塊 chunk，把位址寫進 map。既有元素不搬。
//   所以在 deque 兩端插入時，指向既有元素的 pointer 與 reference 仍然有效
//   （但 iterator 會失效，因為 map 本身可能重新配置）——這個細微差別
//   是標準明文規定的，很多人記錯。
//
// 【4. 為什麼遍歷 vector 明顯比較快：cache 才是主角】
//   現代 CPU 從記憶體讀資料是以 cache line 為單位（本機為 64 bytes）。
//   vector<int> 連續存放，一次讀進 16 個 int，而且硬體預取器能準確預測
//   下一條 cache line。deque 在同一個 chunk 內連續，跨 chunk 就跳到別處，
//   預取器每次跨界都要重新學習；再加上 operator[] 本身多一層間接定址，
//   遍歷成本自然高出一截。本檔實測就是要讓你看到這個差距的量級。
//
// 【5. data()：能不能傳給 C API，是很多專案的硬性條件】
//   vector 保證元素連續，所以 v.data() 可以直接餵給 read()／memcpy()／
//   OpenGL glBufferData()／各種只吃 T* 的 C 函式庫。
//   deque 沒有 data() 成員函式，因為它根本沒有「一個連續緩衝區」可以指。
//   一旦需求裡有「要傳指標給 C API」，這一項就直接把 deque 判出局，
//   不需要再比效能。
//
// 【概念補充 Concept Deep Dive】
//   ● libstdc++ 的 chunk 大小（_GLIBCXX_DEQUE_BUF_SIZE）預設 512 bytes，
//     每個 chunk 的元素數 = sizeof(T) < 512 ? 512 / sizeof(T) : 1。
//     int 為 4 bytes → 一個 chunk 128 個 int。MSVC 用 16 bytes 為基準，
//     int 只放 4 個 —— 完全不同的數量級，所以「deque 效能」的結論不可跨平台照抄。
//     以上皆為實作定義。
//   ● 空 deque 不是零成本：libstdc++ 會先配置一個 map 與至少一個 chunk，
//     所以 deque 的固定開銷比 vector 大。存放少量元素時 vector 幾乎必勝。
//   ● 為什麼 stack 與 queue 的預設底層容器是 deque？
//     queue 需要 push_back + pop_front，vector 的 pop_front 是 O(n)，直接出局。
//     stack 只需要尾端操作，vector 其實完全夠用，標準仍選 deque 以求一致；
//     追求極致效能時可以明確寫 std::stack<int, std::vector<int>>。
//   ● C++20 起 vector 的迭代器滿足 std::contiguous_iterator 概念，
//     deque 的迭代器只滿足 std::random_access_iterator。這個差別讓某些
//     ranges 演算法能對 vector 走 memcpy 快路徑，對 deque 則不行。
//
// 【注意事項 Pay Attention】
//   1. 本檔輸出含毫秒數，每次執行都不同（受 CPU 頻率、其他行程、記憶體壓力影響）。
//      請看相對倍數，不要把絕對值當規格。
//   2. 教學檔預設未開 -O2，量到的絕對值偏大。
//   3. vector<int> vec(N) 會把元素值初始化為 0，所以 sum 必然是 0，這是規定行為。
//   4. 第 4 段印出的 vector.data() 是記憶體位址，每次執行都不同。
//   5. deque 兩端插入時 pointer/reference 保持有效但 iterator 失效；
//      vector 擴容時三者全部失效。兩者不同，不可混記。
//   6. chunk 大小與成長倍率都是實作定義，換編譯器數字會變。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】deque 與 vector 的取捨
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 什麼時候該用 deque 而不是 vector？請給出判斷順序。
//     答：先看介面需求再看效能。(1) 需要 push_front／pop_front → deque，
//         因為 vector 做這件事是 O(n)，差的是複雜度等級不是常數。
//         (2) 需要把連續緩衝區指標交給 C API → 只能 vector（deque 沒有 data()）。
//         (3) 不能接受「單次擴容搬移全部元素」的尖峰延遲 → deque。
//         (4) 以上皆非、且以順序遍歷為主 → vector，cache locality 勝出。
//     追問：那 stack／queue 為什麼預設用 deque？→ queue 需要 pop_front，
//         vector 的 pop_front 是 O(n) 直接出局；stack 其實可以指定 vector 更快。
//
// 🔥 Q2. vector 擴容時，哪些東西會失效？deque 兩端插入呢？
//     答：vector 擴容後，所有 iterator、pointer、reference 全部失效。
//         deque 在兩端插入時，iterator 會失效，但指向既有元素的
//         pointer 與 reference 仍然有效——因為既有元素沒有被搬動。
//         這是標準明文規定的差異。
//     追問：deque 在「中間」插入呢？→ 那就三者全部失效，因為必須搬動元素。
//
// ⚠️ 陷阱. 「deque 的 operator[] 也是 O(1)，所以遍歷速度應該跟 vector 差不多。」
//     答：複雜度相同不代表速度相同。deque 每次存取多一層 map 間接定址，
//         還要算 chunk 索引與 chunk 內偏移；更致命的是跨 chunk 會打斷
//         硬體預取。本檔實測中 deque 遍歷明顯慢於 vector，量級差距可觀。
//     為什麼會錯：把 O(1) 當成「速度相同」的保證。O 記號刻意丟掉常數，
//         而常數正是這裡的全部勝負所在。
//
// ⚠️ 陷阱. 「deque 不連續，所以不能用 std::sort。」
//     答：可以。std::sort 要求的是「隨機存取迭代器」，deque 的迭代器滿足
//         這個要求，所以 std::sort(d.begin(), d.end()) 完全合法。
//         不能用 std::sort 的是 std::list（雙向迭代器），所以 list 才自備
//         成員函式 sort()。
//     為什麼會錯：把「記憶體連續」跟「隨機存取迭代器」混為一談。
//         連續是更強的條件；std::sort 只需要後者。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <deque>
#include <chrono>
#include <algorithm>
#include <string>
using namespace std;
using namespace chrono;

// -----------------------------------------------------------------------------
// 【日常實務範例 1】網頁伺服器的請求佇列（FIFO）
//   情境：worker thread 從佇列頭端取出待處理請求，前端 accept 後從尾端塞進來。
//   為什麼用 deque：這是純粹的 FIFO。vector 的 erase(begin()) 是 O(n)，
//         每處理一個請求就要搬移整個佇列，QPS 一高就直接被搬移成本吃死。
//   註：實務上會用 std::queue 包起來（其預設底層容器正是 deque），
//       這裡直接用 deque 是為了看清楚底層行為。
// -----------------------------------------------------------------------------
struct Request {
    int id;
    string path;
};

class RequestQueue {
public:
    void enqueue(int id, const string& path) { q_.push_back({id, path}); }  // O(1)

    bool dequeue(Request& out) {
        if (q_.empty()) return false;
        out = q_.front();
        q_.pop_front();                                                     // O(1)
        return true;
    }

    size_t size() const { return q_.size(); }

private:
    deque<Request> q_;
};

// -----------------------------------------------------------------------------
// 【日常實務範例 2】把整批感測器讀數交給 C 函式庫
//   情境：舊有的 C 訊號處理函式庫只接受 (const double* data, size_t n)。
//   為什麼只能用 vector：vector 保證元素連續，v.data() 就是那個 double*。
//         deque 沒有 data()，因為它根本沒有單一連續緩衝區可以指。
//         這種需求會直接決定容器選型，不必比效能。
// -----------------------------------------------------------------------------
// 模擬一個只吃裸指標的 C API
static double c_api_average(const double* data, size_t n) {
    if (n == 0) return 0.0;
    double sum = 0.0;
    for (size_t i = 0; i < n; ++i) sum += data[i];
    return sum / static_cast<double>(n);
}

int main() {
    const int N = 5000000;

    // ============================================================
    // 1. push_back 效能比較
    // ============================================================
    cout << "===== 1. push_back " << N << " 次 =====\n";
    {
        vector<int> vec;
        auto t1 = high_resolution_clock::now();
        for (int i = 0; i < N; i++) vec.push_back(i);
        auto t2 = high_resolution_clock::now();
        cout << "  vector: " << duration_cast<milliseconds>(t2 - t1).count() << " ms\n";
    }
    {
        deque<int> dq;
        auto t1 = high_resolution_clock::now();
        for (int i = 0; i < N; i++) dq.push_back(i);
        auto t2 = high_resolution_clock::now();
        cout << "  deque:  " << duration_cast<milliseconds>(t2 - t1).count() << " ms\n";
    }

    // ============================================================
    // 2. push_front — deque 的獨家優勢
    // ============================================================
    cout << "\n===== 2. push_front " << N << " 次 =====\n";
    {
        deque<int> dq;
        auto t1 = high_resolution_clock::now();
        for (int i = 0; i < N; i++) dq.push_front(i);
        auto t2 = high_resolution_clock::now();
        cout << "  deque:  " << duration_cast<milliseconds>(t2 - t1).count() << " ms\n";
    }
    cout << "  vector: 不測試（insert(begin()) 單次 O(n)，做 N 次是 O(N^2)）\n";

    // ============================================================
    // 3. 順序遍歷效能比較
    // ============================================================
    cout << "\n===== 3. 順序遍歷 " << N << " 個元素 =====\n";
    {
        vector<int> vec(N);   // 值初始化為 0，所以 sum 必為 0
        long long sum = 0;
        auto t1 = high_resolution_clock::now();
        for (int i = 0; i < N; i++) sum += vec[i];
        auto t2 = high_resolution_clock::now();
        cout << "  vector: " << duration_cast<milliseconds>(t2 - t1).count()
             << " ms (sum=" << sum << ")\n";
    }
    {
        deque<int> dq(N);
        long long sum = 0;
        auto t1 = high_resolution_clock::now();
        for (int i = 0; i < N; i++) sum += dq[i];
        auto t2 = high_resolution_clock::now();
        cout << "  deque:  " << duration_cast<milliseconds>(t2 - t1).count()
             << " ms (sum=" << sum << ")\n";
    }

    // ============================================================
    // 4. 關鍵差異示範
    // ============================================================
    cout << "\n===== 4. 關鍵差異 =====\n";

    // vector 有 data() 指標（位址每次執行都不同）
    vector<int> vec = {1, 2, 3};
    int* ptr = vec.data();  // ← 連續記憶體指標
    cout << "  vector.data() 非空指標: " << (ptr != nullptr ? "是" : "否")
         << " → 可傳給 C API\n";
    cout << "  vector 元素連續?  &vec[1]-&vec[0] = " << (&vec[1] - &vec[0]) << "\n";

    // deque 沒有 data()
    deque<int> dq = {1, 2, 3};
    // dq.data();  // ❌ 編譯錯誤！deque 不保證連續記憶體，因此沒有這個成員函式
    cout << "  deque 沒有 data()，因為記憶體不連續\n";

    // 但 deque 的迭代器仍是隨機存取 → std::sort 可用
    deque<int> unsorted = {5, 2, 9, 1, 7};
    sort(unsorted.begin(), unsorted.end());   // 合法！deque 是隨機存取迭代器
    cout << "  deque 可用 std::sort: ";
    for (int v : unsorted) cout << v << " ";
    cout << "\n";

    // ============================================================
    // 5. 日常實務：請求佇列（FIFO）
    // ============================================================
    cout << "\n===== 5. 日常實務：HTTP 請求佇列 =====\n";
    RequestQueue rq;
    rq.enqueue(1001, "/api/users");
    rq.enqueue(1002, "/api/orders");
    rq.enqueue(1003, "/health");
    cout << "  佇列長度: " << rq.size() << "\n";
    Request r;
    while (rq.dequeue(r)) {
        cout << "  處理 #" << r.id << " " << r.path << "\n";
    }

    // ============================================================
    // 6. 日常實務：把 vector 交給只吃裸指標的 C API
    // ============================================================
    cout << "\n===== 6. 日常實務：交給 C API =====\n";
    vector<double> readings = {21.5, 22.0, 21.8, 23.1, 22.6};
    cout << "  感測器平均值 = "
         << c_api_average(readings.data(), readings.size()) << "\n";
    cout << "  （deque 無法這樣做：沒有 data()，元素也不保證連續）\n";

    // ============================================================
    // 重點整理
    // ============================================================
    cout << "\n=== 選擇指引 ===\n";
    cout << "  預設首選：vector\n";
    cout << "  需要 push_front / pop_front → deque\n";
    cout << "  需要傳指標給 C API → vector (data())\n";
    cout << "  大量遍歷重視 cache → vector\n";
    cout << "  stack / queue 底層 → deque（標準庫預設）\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary
// 效能量測請另外加 -O2；未最佳化的絕對數字偏大，只有相對倍數有參考價值。

// 注意：下列毫秒數為本機一次實測，每次執行都不同（受 CPU 頻率、系統負載影響）；
//       請只看相對倍數，不要把絕對值當成規格。

// === 預期輸出 ===
// ===== 1. push_back 5000000 次 =====
//   vector: 49 ms
//   deque:  40 ms
//
// ===== 2. push_front 5000000 次 =====
//   deque:  37 ms
//   vector: 不測試（insert(begin()) 單次 O(n)，做 N 次是 O(N^2)）
//
// ===== 3. 順序遍歷 5000000 個元素 =====
//   vector: 22 ms (sum=0)
//   deque:  331 ms (sum=0)
//
// ===== 4. 關鍵差異 =====
//   vector.data() 非空指標: 是 → 可傳給 C API
//   vector 元素連續?  &vec[1]-&vec[0] = 1
//   deque 沒有 data()，因為記憶體不連續
//   deque 可用 std::sort: 1 2 5 7 9
//
// ===== 5. 日常實務：HTTP 請求佇列 =====
//   佇列長度: 3
//   處理 #1001 /api/users
//   處理 #1002 /api/orders
//   處理 #1003 /health
//
// ===== 6. 日常實務：交給 C API =====
//   感測器平均值 = 22.2
//   （deque 無法這樣做：沒有 data()，元素也不保證連續）
//
// === 選擇指引 ===
//   預設首選：vector
//   需要 push_front / pop_front → deque
//   需要傳指標給 C API → vector (data())
//   大量遍歷重視 cache → vector
//   stack / queue 底層 → deque（標準庫預設）
