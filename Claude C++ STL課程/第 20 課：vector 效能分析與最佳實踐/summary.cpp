/*
 * ================================================================
 * 【第 20 課：vector 效能分析與最佳實踐】總複習 summary.cpp
 * ================================================================
 * 編譯方式：g++ -std=c++17 -Wall -Wextra -o summary summary.cpp
 * 只看 stdout：./summary 2>/dev/null
 *
 * 本課重點：
 * 1. 效能測量：用 chrono 計時（以及為什麼「計數」通常比「計時」更好）
 * 2. reserve() 的重要性 —— 避免多次重分配
 * 3. emplace_back vs push_back 效能比較
 * 4. 避免不必要的複製：const 參考、移動語意
 * 5. 迭代遍歷的效能：range-for vs index vs iterator
 * 6. 縮減記憶體：shrink_to_fit()、swap trick
 * 7. 選擇正確的容器
 * ================================================================
 */

// =============================================================================
//  第 20 課 總複習  —  vector 效能分析與最佳實踐
// =============================================================================
//
// 【主題資訊 Information】
//   本課涉及的核心 API：
//     void      reserve(size_type n);          // 預留容量，不改變 size
//     size_type capacity() const noexcept;     // 目前配置了多少格
//     void      shrink_to_fit();               // C++11，「請求」歸還多餘記憶體
//     template <class... Args> reference emplace_back(Args&&...);  // C++11/17
//     void      push_back(const T&);  void push_back(T&&);
//
//   標頭檔：<vector>、<algorithm>、<numeric>、<chrono>
//   本檔標準：C++17
//
// 【詳細解釋 Explanation】
//
// 【1. 效能問題的三個層級（由大到小）】
//   談 vector 效能時，先分清楚你在處理哪一層，才不會在錯的地方花力氣：
//     ① 演算法／資料結構層（影響最大，動輒數十倍）
//        —— 你是不是該用 vector？在頭部插入頻繁就該用 deque；
//           要頻繁查找 key 就該用 unordered_map。選錯容器，怎麼調都沒用。
//     ② 記憶體配置層（影響大，數倍）
//        —— reserve 了沒？有沒有不必要的複製？這是本課的主戰場。
//     ③ 指令層（影響小，通常個位數 %）
//        —— range-for vs index vs iterator，這層多半由編譯器擺平。
//   最常見的錯誤是「跳過①②直接優化③」，花了很多力氣卻沒什麼效果。
//
// 【2. reserve：本課最重要的一件事】
//   沒有 reserve 的 push_back 迴圈會觸發約 log₂(n) 次重新配置，
//   每次都要「配置新記憶體 + 搬移全部既有元素 + 釋放舊記憶體」。
//   實測（本檔第 1 節，本機 g++ 15.2）：填入 100 萬個 int，
//     無 reserve → 配置 21 次、累計向配置器要了 2,097,151 格（= 2²¹ - 1）
//     有 reserve → 配置  1 次、累計 1,000,000 格（剛好，零浪費）
//   多要的那 1,097,151 格全是「配置新的、搬過去、丟掉舊的」所浪費的。
//   複雜度都是 O(n)，但常數差很多，且無 reserve 版還會造成記憶體碎片。
//
// 【3. emplace_back vs push_back】
//   emplace_back 把參數轉發給元素的建構子，就地建構，省掉一次臨時物件的
//   建構與移動。但它**只有在「參數是建構原料」時才有優勢**；
//   若你已經有一個現成物件，兩者都要複製／移動一次，效能相同。
//   而且 emplace_back 會啟用 explicit 建構子、可能選到你沒預期的重載
//   （見同課 5.cpp 的 vv.emplace_back(5, 0) 陷阱）。
//
// 【4. 遍歷方式：這層其實差不多】
//   range-for / index / iterator / accumulate 在 -O2 下通常被編譯器
//   優化成幾乎相同的機器碼。**真正影響大的是迴圈變數的型別**：
//   for (std::string s : v) 每次複製一個字串，那才是數量級的差別。
//   所以選遍歷方式應該以「可讀性」為準，而不是效能。
//
// 【5. 記憶體歸還：clear 不還、shrink_to_fit 才還】
//   clear() 只把 size 歸零、銷毀元素，**capacity 完全不變**。
//   要真正歸還記憶體有兩種做法：
//     * shrink_to_fit()（C++11）——標準說這是「非強制的請求」
//     * swap trick：vector<int>(v).swap(v);——建一個剛好大小的副本再交換，
//       這是 C++11 之前唯一的做法，現在仍可用且行為明確
//
// 【概念補充 Concept Deep Dive】
//   ● 為什麼本檔把耗時印到 stderr
//     耗時受 CPU 頻率調節、快取狀態、其他行程干擾影響，每次執行都不同，
//     不能當作「預期輸出」。本檔的做法是：
//       耗時 → stderr（給你臨場觀察用）
//       確定性的結構性數據（capacity 序列、size、配置次數）→ stdout
//     這樣預期輸出才可重現、可驗證。
//
//   ● 為什麼「計數」比「計時」更適合教學
//     計時測的是「這台機器此刻跑多久」，混雜了大量與主題無關的變因；
//     計數（配置次數、元素搬移次數）直接反映演算法行為，
//     不隨機器與最佳化等級改變，而且可以和理論值對照驗證。
//     同課 15.cpp 就用搬移次數證明了 O(n) vs O(n²)：4095 次 vs 7,998,000 次，
//     剛好等於 2¹²-1 與 3999×4000/2 的閉式解。
//
//   ● -O0 與 -O2 下的結論可能相反
//     這是效能測量最重要的一課。未開最佳化時，函式呼叫、迭代器包裝、
//     邊界檢查全都是實打實的成本；開了 -O2 之後大多會被 inline 掉。
//     因此「在 -O0 測出 A 比 B 快 2 倍」很可能在 -O2 下變成完全一樣。
//     **任何效能宣稱都必須註明最佳化等級**，否則沒有意義。
//
//   ● 成長倍率是實作定義
//     本機 g++ 15.2 / libstdc++ 為 2 倍（1→2→4→8…），MSVC 的 STL 是 1.5 倍。
//     標準只要求 push_back 攤銷 O(1)，沒有規定倍率。
//
// 【注意事項 Pay Attention】
//   1. reserve 只改 capacity，**不改 size**；reserve 之後用 v[i] 存取是 UB。
//   2. clear() 不會歸還記憶體；shrink_to_fit() 標準上只是「請求」。
//   3. 任何重新配置都會使既有 iterator / pointer / reference 失效。
//   4. capacity 成長倍率、shrink_to_fit 是否真的照做，都是**實作定義**。
//   5. 效能數據必須註明編譯器、最佳化等級與機器，否則無法解讀。
//   6. 本檔耗時輸出在 stderr，每次執行都不同，屬正常現象。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】vector 效能與最佳實踐
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. reserve() 和 resize() 差在哪？什麼時候用哪個？
//     答：reserve(n) 只把 capacity 拉到至少 n，**size 不變**，
//         不建構任何元素；之後要用 push_back / emplace_back 填入。
//         resize(n) 會真的把 size 改成 n，並**建構**（或銷毀）元素，
//         之後才可以用 v[i] 存取。
//         「我要陸續 push_back 很多個」→ reserve；
//         「我要一塊 n 格、可以直接用索引寫入的空間」→ resize。
//     追問：reserve 之後直接寫 v[0] = 1 會怎樣？→ 是 UB。
//         那塊記憶體雖已配置，但上面還沒有元素存在。實務上常常「看起來能跑」，
//         但 size() 仍是 0，後續所有走訪、複製、序列化都會漏掉這些資料。
//
// 🔥 Q2. clear() 之後記憶體被釋放了嗎？怎麼真的把記憶體還給系統？
//     答：沒有。clear() 只銷毀元素、把 size 歸零，capacity 完全不變。
//         要歸還記憶體有兩招：shrink_to_fit()（C++11，標準說是「請求」，
//         不保證一定照做），或 swap trick：vector<int>(v).swap(v);
//         —— 建一個剛好大小的副本再交換，行為明確。
//     追問：為什麼標準不保證 shrink_to_fit 一定生效？→ 因為它可能需要
//         重新配置並搬移元素，而某些實作或某些 allocator 未必想付這個代價。
//         標準保留了彈性，只把它定義成「非約束性的請求」。
//
// ⚠️ 陷阱. 「我測出 index 迴圈比 range-for 快 2 倍，所以以後都寫 index」
//     答：先問一句——**你是在 -O0 還是 -O2 測的？** 未開最佳化時
//         iterator 的 operator++ / operator* 都是真正的函式呼叫，
//         當然比裸指標算術慢；開了 -O2 之後全部被 inline，
//         兩者通常產生幾乎相同的機器碼，差距消失。
//     為什麼會錯：拿 debug build 的數據去指導 release build 的寫法。
//         更根本的錯誤是把力氣花在「指令層」（影響個位數 %），
//         卻忽略了「有沒有 reserve」「迴圈變數是不是不小心複製了整個物件」
//         這些數量級的問題。優化的順序永遠是：先選對資料結構，
//         再消除不必要的配置與複製，最後才輪到指令層——
//         而且每一步都要用 profiler 量過，不要憑感覺。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <chrono>
#include <string>
#include <algorithm>
#include <numeric>
using namespace std;

// 簡單計時工具（RAII 風格）
// 【修正】結果改印到 std::cerr：耗時每次執行都不同，
//         不應混進要當作教材驗證基準的 stdout。
struct Timer {
    string label;
    chrono::high_resolution_clock::time_point start;

    explicit Timer(const string& l)
        : label(l), start(chrono::high_resolution_clock::now()) {}

    ~Timer() {
        auto end = chrono::high_resolution_clock::now();
        auto us = chrono::duration_cast<chrono::microseconds>(end - start).count();
        cerr << "  [計時] " << label << ": " << us << " μs" << endl;
    }
};

// -----------------------------------------------------------------------------
// 可重現的證據：計數 allocator。
// 「配置了幾次、累計要了多少格」比耗時穩定得多，也更能說明問題本質。
// -----------------------------------------------------------------------------
struct AllocStats {
    static long allocs;
    static long elems;
    static void reset() { allocs = 0; elems = 0; }
};
long AllocStats::allocs = 0;
long AllocStats::elems = 0;

template <typename T>
struct CountingAllocator {
    using value_type = T;
    CountingAllocator() noexcept = default;
    template <typename U> CountingAllocator(const CountingAllocator<U>&) noexcept {}

    T* allocate(size_t n) {
        ++AllocStats::allocs;
        AllocStats::elems += static_cast<long>(n);
        return static_cast<T*>(::operator new(n * sizeof(T)));
    }
    void deallocate(T* p, size_t) noexcept { ::operator delete(p); }

    template <typename U> bool operator==(const CountingAllocator<U>&) const noexcept { return true; }
    template <typename U> bool operator!=(const CountingAllocator<U>&) const noexcept { return false; }
};

// ================================================================
// 重點一：reserve() 的重要性
// ================================================================
// vector 在 push_back 超過 capacity 時會：
//   1. 分配新的（通常 2 倍）記憶體
//   2. 將所有元素移動到新位置
//   3. 釋放舊記憶體
// 若事先知道大概元素數量，reserve() 可避免重複重分配

void demoReserve() {
    cout << "\n【reserve() 效能比較】" << endl;

    const int N = 1'000'000;

    {
        Timer t("不用 reserve");
        vector<int> v;
        for (int i = 0; i < N; ++i) v.push_back(i);
    }

    {
        Timer t("reserve(N) 後 push_back");
        vector<int> v;
        v.reserve(N);  // 一次預留所有空間
        for (int i = 0; i < N; ++i) v.push_back(i);
    }

    {
        Timer t("直接用 resize + 索引賦值");
        vector<int> v(N);  // 預先建構 N 個預設值
        for (int i = 0; i < N; ++i) v[i] = i;
    }

    // 觀察 capacity 增長規律
    cout << "\n  capacity 增長規律（前 5 次重分配）:" << endl;
    vector<int> v;
    size_t lastCap = 0;
    int count = 0;
    for (int i = 0; i < 100; ++i) {
        v.push_back(i);
        if (v.capacity() != lastCap) {
            cout << "  size=" << v.size() << " -> capacity=" << v.capacity() << endl;
            lastCap = v.capacity();
            if (++count >= 5) break;
        }
    }
    cout << "  （本機 g++ 15.2 / libstdc++ 為 2 倍成長；MSVC 是 1.5 倍。"
            "實作定義，非標準保證）" << endl;

    // ★ 可重現的證據：用計數 allocator 直接數「配置了幾次」
    cout << "\n  【可重現證據】填入 " << N << " 個 int 的配置次數：" << endl;
    using CVec = vector<int, CountingAllocator<int>>;

    AllocStats::reset();
    { CVec c; for (int i = 0; i < N; ++i) c.push_back(i); }
    cout << "    不用 reserve  → 配置 " << AllocStats::allocs
         << " 次，累計要求 " << AllocStats::elems << " 格" << endl;

    AllocStats::reset();
    { CVec c; c.reserve(N); for (int i = 0; i < N; ++i) c.push_back(i); }
    cout << "    先 reserve(N) → 配置 " << AllocStats::allocs
         << " 次，累計要求 " << AllocStats::elems << " 格" << endl;
    cout << "    → 多出來的格數全是「配置新的、搬過去、丟掉舊的」所浪費的。" << endl;
    cout << "    （這組數字每次執行完全相同，不像上面的耗時會浮動）" << endl;
}

// ================================================================
// 重點二：push_back vs emplace_back 效能
// ================================================================
// 對於複雜物件，emplace_back 就地構造，省去一次移動/複製
// 對於內建類型（int、double），幾乎無差別

struct HeavyObject {
    string name;
    int value;
    vector<int> data;

    // 建構函數
    HeavyObject(const string& n, int v) : name(n), value(v), data(100, v) {}
};

void demoPushVsEmplace() {
    cout << "\n【push_back vs emplace_back 效能】" << endl;

    const int N = 10000;

    {
        Timer t("push_back（臨時物件 + 移動）");
        vector<HeavyObject> v;
        v.reserve(N);
        for (int i = 0; i < N; ++i) {
            v.push_back(HeavyObject("item", i));  // 構造臨時物件，再移動
        }
    }

    {
        Timer t("emplace_back（就地構造）");
        vector<HeavyObject> v;
        v.reserve(N);
        for (int i = 0; i < N; ++i) {
            v.emplace_back("item", i);  // 直接就地構造，無臨時物件
        }
    }
}

// ================================================================
// 重點三：遍歷方式效能比較
// ================================================================
// 對於 vector，各種遍歷方式效能相近（都是順序存取）
// 但語意清晰的方式是首選

void demoTraversal() {
    cout << "\n【遍歷方式比較】" << endl;

    const int N = 10'000'000;
    vector<int> v(N);
    iota(v.begin(), v.end(), 0);

    long long sum = 0;

    {
        Timer t("range-based for");
        sum = 0;
        for (int x : v) sum += x;
    }
    cout << "  sum = " << sum << endl;

    {
        Timer t("index-based for");
        sum = 0;
        for (size_t i = 0; i < v.size(); ++i) sum += v[i];
    }
    cout << "  sum = " << sum << endl;

    {
        Timer t("iterator-based for");
        sum = 0;
        for (auto it = v.begin(); it != v.end(); ++it) sum += *it;
    }
    cout << "  sum = " << sum << endl;

    {
        Timer t("std::accumulate");
        sum = accumulate(v.begin(), v.end(), 0LL);
    }
    cout << "  sum = " << sum << endl;
}

// ================================================================
// 重點四：縮減記憶體 —— shrink_to_fit() 與 swap trick
// ================================================================
// clear() 只清空元素，不釋放記憶體（capacity 不變）
// shrink_to_fit() 要求釋放多餘記憶體（非強制，但通常有效）
// swap trick（C++11 前）：強制釋放記憶體

void demoShrink() {
    cout << "\n【縮減記憶體】" << endl;

    vector<int> v;
    v.reserve(1000);
    for (int i = 0; i < 100; ++i) v.push_back(i);

    cout << "初始 size=" << v.size() << " capacity=" << v.capacity() << endl;

    // clear() 只刪元素，不釋放記憶體
    v.clear();
    cout << "clear() 後 size=" << v.size() << " capacity=" << v.capacity() << endl;

    // shrink_to_fit() 釋放多餘記憶體
    v.shrink_to_fit();
    cout << "shrink_to_fit() 後 size=" << v.size() << " capacity=" << v.capacity() << endl;

    // swap trick（強制釋放，C++11 前的做法）
    vector<int> v2;
    v2.reserve(1000);
    for (int i = 0; i < 100; ++i) v2.push_back(i);
    cout << "\nswap 前 size=" << v2.size() << " capacity=" << v2.capacity() << endl;

    { vector<int>(v2).swap(v2); }  // 建立一個剛好大小的副本，再 swap
    cout << "swap trick 後 size=" << v2.size() << " capacity=" << v2.capacity() << endl;
}

// ================================================================
// 重點五：避免不必要的複製
// ================================================================
// 傳遞 vector 時：
//   - 唯讀：const vector<T>&（參考，零複製）
//   - 需要修改：vector<T>&（參考）
//   - 取得所有權：vector<T>&&（移動）或 vector<T>（值傳遞+移動）

void readVector(const vector<int>& v) {    // 唯讀，零複製
    cout << "  讀取 vector，size=" << v.size() << endl;
}

void modifyVector(vector<int>& v) {        // 修改，零複製
    for (int& n : v) n *= 2;
}

vector<int> createVector(int n) {          // RVO/NRVO 通常零複製返回
    vector<int> result(n);
    iota(result.begin(), result.end(), 1);
    return result;  // 編譯器通常會做 RVO（Return Value Optimization）
}

void demoCopyAvoidance() {
    cout << "\n【避免不必要的複製】" << endl;

    vector<int> v = createVector(5);
    readVector(v);                          // 傳 const 參考，零複製
    modifyVector(v);                        // 傳參考，零複製

    cout << "  修改後: ";
    for (int n : v) cout << n << " ";
    cout << endl;

    // 移動 vector（O(1)，轉移所有權）
    vector<int> v2 = move(v);              // v 的資料轉給 v2，v 變空
    cout << "  move 後 v.size()=" << v.size()
         << " v2.size()=" << v2.size() << endl;
}

// ================================================================
// 重點六：最佳實踐總結
// ================================================================
//
// ✓ DO（應該做的）：
//   1. 事先知道元素數量時，用 reserve()
//   2. 傳遞 vector 時優先用 const 參考（唯讀）或參考（修改）
//   3. 就地構造複雜物件時用 emplace_back
//   4. 返回大型 vector 時讓 RVO/NRVO 優化（直接 return）
//   5. 不再需要 vector 時，用 shrink_to_fit() 釋放記憶體
//
// ✗ DON'T（不應該做的）：
//   1. 不要在 vector 中間頻繁插入（改用 list）
//   2. 不要傳 vector by value（除非需要複製）
//   3. 不要忽略重分配後的迭代器失效問題
//   4. 不要在大型 vector 前面插入（O(n) 位移）

// ================================================================
// 【LeetCode 實戰範例】LeetCode 118. Pascal's Triangle
// ================================================================
//   題目：給定 numRows，回傳巴斯卡三角形的前 numRows 列。
//   為什麼用到本課主題：這題是「事先知道總量 → 先 reserve」的教科書案例。
//     外層 vector 的列數已知（numRows），每一列的長度也已知（i+1），
//     兩層都能 reserve，全程零重新配置。
//     這正是本課「批量插入前先 reserve」最直接的應用。
// ================================================================
vector<vector<int>> generate(int numRows) {
    vector<vector<int>> triangle;
    triangle.reserve(static_cast<size_t>(numRows));      // 外層：列數已知

    for (int i = 0; i < numRows; ++i) {
        vector<int> row;
        row.reserve(static_cast<size_t>(i + 1));         // 內層：長度也已知
        for (int j = 0; j <= i; ++j) {
            if (j == 0 || j == i) {
                row.push_back(1);                        // 頭尾都是 1
            } else {
                const vector<int>& prev = triangle[static_cast<size_t>(i - 1)];
                row.push_back(prev[static_cast<size_t>(j - 1)] +
                              prev[static_cast<size_t>(j)]);
            }
        }
        triangle.push_back(std::move(row));              // 移動，不複製
    }
    return triangle;                                     // 直接 return，讓 NRVO 生效
}

// ================================================================
// 【日常實務範例】批次匯入 CSV 到記憶體資料表
// ================================================================
//   情境：服務啟動時要把一份 CSV 全部載入記憶體。
//         這是本課所有最佳實踐的綜合演練：
//           ① 先數行數 → reserve，避免匯入過程反覆擴容
//           ② 解析出來的 row 用 std::move 放進容器，不複製字串
//           ③ 查詢函式一律用 const& 收參數，零複製
//           ④ 匯入完成後 shrink_to_fit，把估算多的容量還回去
// ================================================================
struct DataTable {
    vector<vector<string>> rows;

    void importFrom(const vector<string>& csvLines) {
        rows.clear();
        rows.reserve(csvLines.size());          // ① 一次配置到位

        for (const string& line : csvLines) {   // ③ const&，零複製
            vector<string> fields;
            fields.reserve(4);                  // 已知大約 4 欄
            size_t start = 0;
            while (true) {
                size_t comma = line.find(',', start);
                if (comma == string::npos) {
                    fields.push_back(line.substr(start));
                    break;
                }
                fields.push_back(line.substr(start, comma - start));
                start = comma + 1;
            }
            rows.push_back(std::move(fields));  // ② 移動，不複製整列字串
        }
    }

    // ③ 查詢一律 const&：不論資料表多大，呼叫成本都是 O(1) 的參考傳遞
    size_t countWhere(const string& column0Value) const {
        size_t n = 0;
        for (const vector<string>& r : rows) {
            if (!r.empty() && r[0] == column0Value) ++n;
        }
        return n;
    }
};

int main() {
    cout << "=============================================" << endl;
    cout << "   第 20 課：vector 效能分析與最佳實踐" << endl;
    cout << "=============================================" << endl;
    cout << "（耗時資料印在 stderr；只看 stdout 請用 ./summary 2>/dev/null）" << endl;

    demoReserve();
    demoPushVsEmplace();
    demoTraversal();
    demoShrink();
    demoCopyAvoidance();

    // ============================================================
    // LeetCode 118. Pascal's Triangle
    // ============================================================
    cout << "\n【LeetCode 118. Pascal's Triangle】" << endl;
    auto triangle = generate(6);
    for (const auto& row : triangle) {
        cout << "  ";
        for (int x : row) cout << x << " ";
        cout << endl;
    }
    cout << "  共 " << triangle.size() << " 列；外層與內層都事先 reserve，"
            "全程零重新配置" << endl;

    // ============================================================
    // 日常實務：批次匯入 CSV
    // ============================================================
    cout << "\n【日常實務：批次匯入 CSV 到記憶體資料表】" << endl;
    vector<string> csv = {
        "ERROR,db,connection timeout,504",
        "INFO,web,request served,200",
        "ERROR,db,deadlock detected,500",
        "WARN,cache,eviction rate high,0",
        "ERROR,web,upstream refused,502"
    };

    DataTable table;
    table.importFrom(csv);
    cout << "  匯入 " << table.rows.size() << " 列，"
         << "capacity = " << table.rows.capacity()
         << "（等於列數 → 只配置一次）" << endl;
    cout << "  第 1 列欄位數：" << table.rows[0].size() << endl;
    cout << "  ERROR 等級筆數：" << table.countWhere("ERROR") << endl;
    cout << "  INFO  等級筆數：" << table.countWhere("INFO") << endl;
    cout << "  → reserve + move + const& 三件事一起做，"
            "整個匯入過程沒有任何多餘的配置或字串複製" << endl;

    cout << "\n==============================================" << endl;
    cout << " 最重要的最佳實踐：" << endl;
    cout << " 1. 批量插入前 reserve()" << endl;
    cout << " 2. 傳遞時用 const ref" << endl;
    cout << " 3. 就地構造用 emplace_back" << endl;
    cout << " 4. 注意迭代器/指標失效" << endl;
    cout << "==============================================" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary
// 只看 stdout: ./summary 2>/dev/null
//
// ⚠️ 但書：
//   1. 所有耗時資料印在 stderr，每次執行都不同（受 CPU 頻率、快取、
//      其他行程影響），故不納入下方預期輸出。
//   2. capacity 成長倍率（2 倍）、shrink_to_fit 是否真的縮小，
//      都是本機 g++ 15.2 / libstdc++ 的實作行為，非標準保證。
//   3. demoCopyAvoidance 中 move 後 v.size()==0 是 libstdc++ 實測結果；
//      標準只保證 moved-from 為「合法但未指定」。

// === 預期輸出 ===
// =============================================
//    第 20 課：vector 效能分析與最佳實踐
// =============================================
// （耗時資料印在 stderr；只看 stdout 請用 ./summary 2>/dev/null）
//
// 【reserve() 效能比較】
//
//   capacity 增長規律（前 5 次重分配）:
//   size=1 -> capacity=1
//   size=2 -> capacity=2
//   size=3 -> capacity=4
//   size=5 -> capacity=8
//   size=9 -> capacity=16
//   （本機 g++ 15.2 / libstdc++ 為 2 倍成長；MSVC 是 1.5 倍。實作定義，非標準保證）
//
//   【可重現證據】填入 1000000 個 int 的配置次數：
//     不用 reserve  → 配置 21 次，累計要求 2097151 格
//     先 reserve(N) → 配置 1 次，累計要求 1000000 格
//     → 多出來的格數全是「配置新的、搬過去、丟掉舊的」所浪費的。
//     （這組數字每次執行完全相同，不像上面的耗時會浮動）
//
// 【push_back vs emplace_back 效能】
//
// 【遍歷方式比較】
//   sum = 49999995000000
//   sum = 49999995000000
//   sum = 49999995000000
//   sum = 49999995000000
//
// 【縮減記憶體】
// 初始 size=100 capacity=1000
// clear() 後 size=0 capacity=1000
// shrink_to_fit() 後 size=0 capacity=0
//
// swap 前 size=100 capacity=1000
// swap trick 後 size=100 capacity=100
//
// 【避免不必要的複製】
//   讀取 vector，size=5
//   修改後: 2 4 6 8 10
//   move 後 v.size()=0 v2.size()=5
//
// 【LeetCode 118. Pascal's Triangle】
//   1
//   1 1
//   1 2 1
//   1 3 3 1
//   1 4 6 4 1
//   1 5 10 10 5 1
//   共 6 列；外層與內層都事先 reserve，全程零重新配置
//
// 【日常實務：批次匯入 CSV 到記憶體資料表】
//   匯入 5 列，capacity = 5（等於列數 → 只配置一次）
//   第 1 列欄位數：4
//   ERROR 等級筆數：3
//   INFO  等級筆數：1
//   → reserve + move + const& 三件事一起做，整個匯入過程沒有任何多餘的配置或字串複製
//
// ==============================================
//  最重要的最佳實踐：
//  1. 批量插入前 reserve()
//  2. 傳遞時用 const ref
//  3. 就地構造用 emplace_back
//  4. 注意迭代器/指標失效
// ==============================================
