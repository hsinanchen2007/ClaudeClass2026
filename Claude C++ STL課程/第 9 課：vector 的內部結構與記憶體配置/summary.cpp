// =============================================================================
//  summary.cpp  —  vector 的內部結構、容量成長與擴容代價（第 9 課總複習）
// =============================================================================
//
// 【主題資訊 Information】
//   內部結構 : 三個指標 _begin / _end / _cap（實作定義的典型佈局）
//              本機 sizeof(std::vector<int>) = 24 bytes = 3 × 8
//   核心介面 : size() / capacity() / reserve() / resize() / data() / shrink_to_fit()
//   標頭檔   : <vector>
//   複雜度   : operator[] O(1)；push_back 攤銷 O(1)（單次最壞 O(n)）；
//              reserve O(n)（需搬移既有元素）
//   標準保證 : 元素連續儲存 —— 對 0 <= n < size()，&v[n] == data() + n
//   實作定義 : 成長倍率（libstdc++／libc++ 為 2 倍，MSVC 約 1.5 倍）
//
// 【詳細解釋 Explanation】
//
// 【1. 三個指標決定了 vector 的一切行為】
//   vector 本身只是一個小小的三指標結構，資料在堆積上：
//       _begin ──► 第一個元素
//       _end   ──► 最後一個元素的下一個位置
//       _cap   ──► 已配置空間的尾端
//   於是：
//       size()     = _end - _begin      實際有幾個元素
//       capacity() = _cap - _begin      不重新配置最多能放幾個
//       empty()    = (_begin == _end)
//   不變量 size() <= capacity() 永遠成立。理解這三個指標，
//   push_back、reserve、resize、迭代器失效全都能自己推導出來。
//
// 【2. size 與 capacity 為什麼一定要分開】
//   若每次 push_back 都精確配置 size+1，加入 n 個元素要複製 n(n-1)/2 次，
//   即 O(n²)。分開之後採「容量不夠就成倍要」，總搬移量小於 2n，
//   於是 push_back 成為攤銷 O(1)。這是用空間換時間的經典設計。
//
// 【3. 擴容的四個步驟，以及 noexcept 為何決定第二步】
//   擴容時 vector 會：(1) 配置更大區塊 →(2) 搬移舊元素 →(3) 銷毀舊元素
//   →(4) 釋放舊區塊。
//   第 (2) 步用移動還是複製，取決於元素的移動建構子有沒有 noexcept：
//   因為 push_back 承諾「強例外保證」（失敗時容器維持原狀），
//   而移動到一半失敗是無法回滾的（舊元素已被掏空），複製則可以。
//   所以標準庫用 std::move_if_noexcept 判斷——沒標 noexcept 且型別可複製時，
//   擴容退化成複製。對持有堆積資源的型別，這是 O(1) 變 O(size)。
//
// 【4. reserve vs resize：最容易混淆的一組】
//   reserve(n)：只改 capacity，size 不變。之後 v[0] 仍是越界（UB）。
//   resize(n) ：改 size（會實際建構或銷毀元素），必要時也擴容。
//   規則：「只是想避免擴容」用 reserve；「要能直接索引」用 resize。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼連續是 vector 最大的優勢
//     除了 O(1) 隨機存取，更關鍵的是 cache locality。CPU 一次載入
//     一整條 cache line（本機 64 bytes = 16 個 int），連續走訪幾乎全命中；
//     list 節點散落堆積各處，幾乎每步都 miss，而一次 miss 可達上百 cycle。
//     這就是「理論上 list 插入 O(1)，實測卻常輸給 vector」的原因。
//
// (B) capacity 只增不減
//     clear()、erase()、pop_back() 都不會歸還記憶體。要真正釋放得用
//     shrink_to_fit()（C++11，且是非約束性請求，實作可以不理會），
//     或經典的 swap trick：std::vector<int>(v).swap(v)。
//
// (C) std::vector<bool> 是例外
//     它是特化版本，內部以 bit 打包，「沒有」連續的 bool 陣列，
//     也不提供 data()。需要真正的位元組陣列請改用 std::vector<char>。
//
// (D) 為什麼本檔印「位移」而不是「位址」
//     原始位址受 ASLR 影響，每次執行都不同（本機實測確認）。
//     位移是確定性的，可寫進預期輸出，而且更直接呈現
//     「每個 int 相差 4 bytes」這個要教的重點。原始位址改送 stderr。
//
// 【注意事項 Pay Attention】
// 1. 任何重新配置（push_back 擴容、reserve、resize、insert、shrink_to_fit）
//    都會讓既有的迭代器／指標／參考全部失效，之後再使用是 UB。
//    危險之處在於「不一定會壞」——容量還夠時不會重新配置，
//    於是這類 bug 常在資料量變大後才在正式環境爆發。
// 2. reserve 只改 capacity 不改 size；reserve 後直接索引是越界（UB），
//    而且通常不會崩潰（記憶體確實已配置），屬於最難查的那種錯誤。
// 3. 成長倍率是實作定義，不可寫死進程式邏輯。
// 4. 移動建構子請標 noexcept，否則擴容會退化成複製。
// 5. std::for_each 等演算法收的是 functor 副本；本檔 Tracker 的示範
//    請注意分辨「新元素建構」與「舊元素搬遷」兩種輸出。
// 6. 移動後的來源物件處於「有效但未指定」狀態：可解構、可重新賦值，
//    但不可假設其內容。本檔 Tracker 的移動建構子刻意未清空 other.id，
//    所以解構時仍印得出原值——實務上建議明確置空。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】vector 的內部結構與記憶體配置
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 請說明 vector 的內部結構，並用它推導 size() 與 capacity()。
//     答：內部是三個指標 _begin／_end／_cap。size() = _end - _begin，
//         capacity() = _cap - _begin，恆有 size() <= capacity()。
//         vector 物件本身很小（本機 sizeof(vector<int>) = 24 bytes，
//         與元素數量無關），元素資料放在堆積上。
//     追問：那 sizeof(vector<int>) 會隨元素變多而變大嗎？→ 不會，
//         永遠是那三個指標的大小；變大的是它指向的堆積區塊。
//
// 🔥 Q2. push_back 的複雜度是什麼？請說精確。
//     答：攤銷 O(1)。多數呼叫只是寫入一個元素；容量滿時要配置新空間並
//         搬移全部 n 個元素，那一次是 O(n)。因為採倍率成長，
//         n 次 push_back 的總搬移量小於 2n，平均仍是常數。
//     追問：什麼場景不能接受「攤銷」？→ 即時系統（音訊回呼、遊戲每幀）。
//         平均值再漂亮，某一次突然 O(n) 就會掉幀，這種場合必須先 reserve。
//
// 🔥 Q3. 為什麼自訂型別的移動建構子一定要標 noexcept？
//     答：vector::push_back 要提供強例外保證。移動到一半丟例外無法回滾，
//         複製則可以。所以標準庫用 std::move_if_noexcept 判斷：
//         沒有 noexcept 且型別可複製時，擴容改用複製。
//         對持有堆積資源的型別，等於 O(1) 變 O(size)，且毫無警告。
//     追問：那不可複製的型別呢？→ 沒有退路，仍用移動，
//         但強例外保證降級為基本保證。
//
// ⚠️ 陷阱 1. std::vector<int> v; v.reserve(10); v[0] = 42;  錯在哪？
//     答：reserve 只配置容量，size() 仍是 0，v[0] 是越界存取，屬 UB。
//         要能直接索引必須用 resize(10)。
//     為什麼會錯：因為記憶體確實已經配置好，寫進去通常不崩潰，
//         印出來就是 42，看起來完全正常。但 size() 仍是 0，
//         range-for、size()、迭代器全都看不到它，下一次 push_back
//         還會直接覆蓋它。這是典型「不崩潰的 UB」，比 segfault 難查得多。
//
// ⚠️ 陷阱 2. int* p = v.data(); v.push_back(x); 之後再用 p——為什麼危險？
//     答：push_back 若觸發擴容，舊區塊已被釋放，p 成為懸空指標，
//         再使用是 UB。
//     為什麼會錯：它「不一定會壞」。容量還夠時不會重新配置，p 依然有效，
//         所以開發與測試階段可能完全正常，上線後資料量一大才開始隨機出錯。
//         安全性要靠推理（這裡會不會擴容？）判斷，不能靠測試沒出事來背書。
// ═══════════════════════════════════════════════════════════════════════════

/*
 * ================================================================
 * 【第9課：vector 的內部結構與記憶體配置】總複習 summary.cpp
 * ================================================================
 * 編譯方式：g++ -std=c++17 -o summary summary.cpp
 * 本課重點：
 * 1. vector 的三個關鍵內部指標：_begin、_end、_cap
 * 2. 連續記憶體的保證與 data() 原始指標存取
 * 3. size 與 capacity 的概念與差異
 * 4. 擴容策略：MSVC 約 1.5 倍，GCC/Clang 約 2 倍
 * 5. 擴容代價：配置新空間、搬移/複製元素、釋放舊空間
 * 6. noexcept 移動建構子對擴容行為的影響
 * ================================================================
 */

#include <iostream>
#include <vector>
#include <string>

// ===== 重點一：vector 的三個內部指標 =====
// vector 內部維護三個指標，是理解其所有行為的基礎：
//
//   T* _begin;   // 指向第一個元素
//   T* _end;     // 指向最後一個元素的下一個位置
//   T* _cap;     // 指向配置空間的尾端
//
// 記憶體示意圖：
//   已配置的記憶體區塊
//   ┌───┬───┬───┬───┬───┬───┬───┬───┐
//   │ A │ B │ C │ D │ E │   │   │   │
//   └───┴───┴───┴───┴───┴───┴───┴───┘
//   ↑                   ↑           ↑
//   _begin             _end        _cap
//
//   size()     = _end - _begin = 5  （實際存放的元素數量）
//   capacity() = _cap - _begin = 8  （已配置的空間可容納的元素數量）
//
// 不變量：size() <= capacity()  永遠成立

void demo_internal_structure() {
    std::cout << "\n===== 重點一：三個內部指標 =====\n";

    std::vector<int> v = {10, 20, 30, 40, 50};
    std::cout << "size:     " << v.size()     << std::endl;  // 5
    std::cout << "capacity: " << v.capacity() << std::endl;  // >= 5
    std::cout << "empty:    " << v.empty()    << std::endl;  // 0 (false)
}

// ===== 重點二：連續記憶體的保證 =====
// C++ 標準保證 vector 元素儲存在連續的記憶體空間中。
// 可以用 v.data() 或 &v[0] 取得指向內部陣列的原始指標。
// 這使得 vector 可以直接和 C 風格 API 互動。

void demo_contiguous_memory() {
    std::cout << "\n===== 重點二：連續記憶體 =====\n";

    std::vector<int> v = {10, 20, 30, 40, 50};

    // 印出每個元素「相對 data() 的 byte 位移」，而不是原始位址。
    // 原始位址受 ASLR 影響，每次執行都不同（本機實測確認），
    // 寫進預期輸出沒有意義；位移則是確定性的證據，也更直接看出
    // 「每個 int 相差 sizeof(int) bytes」這件事。
    const int* base = v.data();
    std::cout << "各元素相對 data() 的位移：\n";
    for (size_t i = 0; i < v.size(); ++i) {
        std::cout << "  v[" << i << "] 位移: "
                  << reinterpret_cast<const char*>(&v[i]) -
                     reinterpret_cast<const char*>(base)
                  << " bytes\n";
    }

    // 標準保證 &v[n] == v.data() + n —— 直接驗證它，印布林而非位址
    bool contiguous = true;
    for (size_t i = 0; i < v.size(); ++i) {
        if (&v[i] != base + i) contiguous = false;
    }
    std::cout << "所有 &v[n] == data() + n ? " << std::boolalpha
              << contiguous << std::noboolalpha << std::endl;

    // 原始位址僅供人眼觀察 → 送 stderr，讓 stdout 保持逐位元組穩定
    std::cerr << "[stderr] v.data() = " << static_cast<const void*>(base)
              << "（每次執行都不同）" << std::endl;

    // 透過 data() 取得原始指標存取元素
    int* ptr = v.data();  // 等同於 &v[0]
    std::cout << "ptr[2] = " << ptr[2] << std::endl;  // 30

    // 修改也會反映到 vector 中
    ptr[0] = 100;
    std::cout << "修改後 v[0] = " << v[0] << std::endl;  // 100
}

// ===== 重點三：size 與 capacity 的差異及擴容策略 =====
// 為什麼要分開 size 和 capacity？
// 若每次 push_back 都重新配置記憶體，加入 1000 個元素需要
// 0+1+2+...+999 = 499,500 次複製操作，非常低效。
//
// 解決方案：「預留空間」策略。空間不足時，一次配置更多空間：
//   - MSVC：通常約 1.5 倍成長
//   - GCC/Clang：通常約 2 倍成長
//
// 實際觀察 capacity 的變化：
//   GCC 的輸出範例：
//   size: 1, capacity: 1
//   size: 2, capacity: 2
//   size: 3, capacity: 4
//   size: 5, capacity: 8
//   size: 9, capacity: 16
//   size: 17, capacity: 32
//   ...

void demo_capacity_growth() {
    std::cout << "\n===== 重點三：capacity 擴容觀察 =====\n";

    std::vector<int> v;
    size_t prev_cap = 0;

    for (int i = 0; i < 100; ++i) {
        v.push_back(i);
        if (v.capacity() != prev_cap) {
            std::cout << "  size: " << v.size()
                      << ", capacity: " << v.capacity() << std::endl;
            prev_cap = v.capacity();
        }
    }
}

// ===== 重點四：擴容代價 — 用 Tracker 類別觀察 =====
// 當 capacity 不足時，vector 會：
//   1. 配置新的更大的記憶體區塊
//   2. 將所有舊元素移動或複製到新空間
//      （若移動建構子有 noexcept，優先用移動；否則用複製）
//   3. 銷毀舊空間中的元素
//   4. 釋放舊的記憶體區塊
//
// noexcept 的重要性：
//   若移動建構子沒有 noexcept，vector 在擴容時會改用複製（更安全但更慢）。
//   原因：若移動過程拋出例外，vector 需要能回滾操作，
//         但移動語意下原物件已被改變，無法恢復；複製則可以安全復原。

class Tracker {
public:
    int id;
    Tracker(int i) : id(i) {
        std::cout << "  建構 Tracker(" << id << ")" << std::endl;
    }
    Tracker(const Tracker& other) : id(other.id) {
        std::cout << "  複製建構 Tracker(" << id << ")" << std::endl;
    }
    Tracker(Tracker&& other) noexcept : id(other.id) {
        // noexcept 讓 vector 擴容時優先使用移動而非複製
        std::cout << "  移動建構 Tracker(" << id << ")" << std::endl;
    }
    ~Tracker() {
        std::cout << "  銷毀 Tracker(" << id << ")" << std::endl;
    }
};

void demo_reallocation_cost() {
    std::cout << "\n===== 重點四：擴容代價觀察 =====\n";

    std::vector<Tracker> v;
    // 不預留空間，讓擴容自然發生，觀察複製/移動

    std::cout << "--- 加入第 1 個（capacity 從 0 變 1）---\n";
    v.emplace_back(1);

    std::cout << "\n--- 加入第 2 個（capacity 從 1 擴容）---\n";
    v.emplace_back(2);

    std::cout << "\n--- 加入第 3 個（capacity 可能再擴容）---\n";
    v.emplace_back(3);

    std::cout << "\n--- 程式結束，所有元素被銷毀 ---\n";
}

// ===== 重點五：push_back 加入多種型別的元素 =====
// 示範 v.push_back(lvalue)  → 複製
// 示範 v.push_back(rvalue)  → 移動
// 示範 v.push_back(std::move(obj)) → 強制移動

void demo_push_back_lvalue_rvalue() {
    std::cout << "\n===== 重點五：push_back 左值 vs 右值 =====\n";

    std::vector<Tracker> v;
    v.reserve(5);  // 預留空間，避免擴容干擾觀察

    std::cout << "-- 從左值 push_back（複製）--\n";
    Tracker t1(1);
    v.push_back(t1);  // 複製，t1 仍然可用

    std::cout << "\n-- 從臨時物件 push_back（移動）--\n";
    v.push_back(Tracker(2));  // 建構臨時物件 + 移動建構 + 銷毀臨時

    std::cout << "\n-- std::move push_back（強制移動）--\n";
    Tracker t3(3);
    v.push_back(std::move(t3));  // 移動，t3 之後處於有效但未指定狀態

    std::cout << "\n-- 程式結束 --\n";
}

// ===== 重點六：摘要表 =====
// | 概念       | 說明                                          |
// |------------|-----------------------------------------------|
// | 連續記憶體 | vector 元素保證存放在連續空間，可用指標算術    |
// | size       | 目前實際存放的元素數量（_end - _begin）        |
// | capacity   | 目前配置的空間可容納的元素數量（_cap - _begin）|
// | 擴容策略   | 空間不足時，配置約 1.5x 或 2x 的新空間        |
// | 擴容代價   | 需要配置新空間、搬移所有元素、釋放舊空間       |
// | noexcept   | 移動建構子加 noexcept，擴容時優先用移動        |

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 88. Merge Sorted Array
//   題目：nums1 長度為 m+n，前 m 個是有效元素、後 n 個是預留的 0；
//         把已排序的 nums2（n 個）就地合併進 nums1，結果保持排序。
//   為什麼用到本主題：這題是「vector 就是一塊連續緩衝區」的最佳體現。
//     題目刻意先把 nums1 開好 m+n 的空間（相當於已經 reserve/resize 過），
//     解法從「後往前」填，正是因為連續記憶體支援 O(1) 索引、
//     且從尾端寫不會覆蓋還沒讀到的資料。若是 list 這種非連續容器，
//     這個解法根本寫不出來。
//   複雜度：時間 O(m+n)，額外空間 O(1)——不配置任何新記憶體，
//     所以全程不會發生擴容，既有指標也不會失效。
// -----------------------------------------------------------------------------
void merge(std::vector<int>& nums1, int m, const std::vector<int>& nums2, int n) {
    int i = m - 1;        // nums1 有效區的最後一個
    int j = n - 1;        // nums2 的最後一個
    int k = m + n - 1;    // 寫入位置：整塊緩衝區的最後一格

    // 從大到小往回填。因為 k 永遠 >= i，寫入不會踩到還沒讀的資料
    while (j >= 0) {
        if (i >= 0 && nums1[i] > nums2[j]) {
            nums1[k--] = nums1[i--];
        } else {
            nums1[k--] = nums2[j--];
        }
    }
    // i < 0 時剩下的 nums1 前段本來就已在正確位置，不需搬動
}

// -----------------------------------------------------------------------------
// 【日常實務範例】固定容量的環形取樣緩衝（避免執行期擴容）
//   場景：感測器／監控每秒取樣數千次，只保留「最近 N 筆」做移動平均。
//         這類程式常跑在即時或近即時路徑上。
//   為什麼要在這裡談 vector 的容量：即時路徑最怕的就是「某一次 push_back
//     剛好觸發擴容」——那一次是 O(n) 的配置加搬移，會造成明顯的延遲尖峰。
//     解法是啟動時就 resize 到固定大小，之後只覆寫、永不改變容量，
//     於是完全沒有配置行為，延遲也就穩定。
//   注意：這裡用 resize 而非 reserve，因為我們要「直接用索引寫入」。
// -----------------------------------------------------------------------------
class RingSampler {
private:
    std::vector<double> buf_;   // 啟動時配置一次，之後容量永不改變
    size_t next_ = 0;           // 下一個寫入位置
    size_t count_ = 0;          // 已累積筆數（未滿時小於容量）
public:
    explicit RingSampler(size_t capacity) : buf_(capacity, 0.0) {}

    void add(double sample) {
        buf_[next_] = sample;                 // 只覆寫，不 push_back → 不會擴容
        next_ = (next_ + 1) % buf_.size();
        if (count_ < buf_.size()) ++count_;
    }

    double average() const {
        if (count_ == 0) return 0.0;
        double sum = 0.0;
        for (size_t i = 0; i < count_; ++i) sum += buf_[i];
        return sum / static_cast<double>(count_);
    }

    size_t capacity() const { return buf_.capacity(); }
    size_t count() const { return count_; }
};

int main() {
    std::cout << "====================================================\n";
    std::cout << " 第9課：vector 的內部結構與記憶體配置 — 總複習\n";
    std::cout << "====================================================\n";

    demo_internal_structure();
    demo_contiguous_memory();
    demo_capacity_growth();
    demo_reallocation_cost();
    demo_push_back_lvalue_rvalue();

    // ---- LeetCode 88 ----
    std::cout << "\n===== LeetCode 88. Merge Sorted Array =====\n";
    std::vector<int> nums1 = {1, 2, 3, 0, 0, 0};
    std::vector<int> nums2 = {2, 5, 6};
    merge(nums1, 3, nums2, 3);
    std::cout << "  [1,2,3,0,0,0] + [2,5,6] = ";
    for (int n : nums1) std::cout << n << " ";
    std::cout << std::endl;

    std::vector<int> n1b = {1};
    std::vector<int> n2b = {};
    merge(n1b, 1, n2b, 0);
    std::cout << "  [1] + [] = ";
    for (int n : n1b) std::cout << n << " ";
    std::cout << std::endl;

    std::vector<int> n1c = {0};
    std::vector<int> n2c = {1};
    merge(n1c, 0, n2c, 1);
    std::cout << "  [0](m=0) + [1] = ";
    for (int n : n1c) std::cout << n << " ";
    std::cout << std::endl;

    // ---- 日常實務：固定容量環形緩衝 ----
    std::cout << "\n===== 日常實務：固定容量取樣緩衝 =====\n";
    RingSampler sampler(4);
    std::cout << "  初始容量: " << sampler.capacity() << std::endl;
    const double readings[] = {20.0, 22.0, 24.0, 26.0, 28.0, 30.0};
    for (double r : readings) {
        sampler.add(r);
        std::cout << "  加入 " << r << " -> 筆數 " << sampler.count()
                  << ", 平均 " << sampler.average()
                  << ", 容量 " << sampler.capacity() << std::endl;
    }
    std::cout << "  容量全程未變 -> 執行期完全沒有記憶體配置，延遲穩定\n";

    std::cout << "\n====================================================\n";
    std::cout << " 複習完畢！\n";
    std::cout << "====================================================\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary

// 【但書】
//   1. v.data() 的原始位址刻意輸出到 stderr（受 ASLR 影響、每次執行都不同），
//      下方預期輸出只涵蓋 stdout，是逐位元組穩定的。
//   2. capacity 成長序列 1,2,4,8,... 是 libstdc++ 的 2 倍策略，屬實作定義；
//      MSVC 約 1.5 倍，序列會不同。
//   3. 「重點五」結尾的解構順序包含 t1／t3 兩個區域 Tracker 與容器內元素，
//      它們的銷毀時機由各自的生命週期決定，不是隨機的。

// === 預期輸出 ===
// ====================================================
//  第9課：vector 的內部結構與記憶體配置 — 總複習
// ====================================================
//
// ===== 重點一：三個內部指標 =====
// size:     5
// capacity: 5
// empty:    0
//
// ===== 重點二：連續記憶體 =====
// 各元素相對 data() 的位移：
//   v[0] 位移: 0 bytes
//   v[1] 位移: 4 bytes
//   v[2] 位移: 8 bytes
//   v[3] 位移: 12 bytes
//   v[4] 位移: 16 bytes
// 所有 &v[n] == data() + n ? true
// ptr[2] = 30
// 修改後 v[0] = 100
//
// ===== 重點三：capacity 擴容觀察 =====
//   size: 1, capacity: 1
//   size: 2, capacity: 2
//   size: 3, capacity: 4
//   size: 5, capacity: 8
//   size: 9, capacity: 16
//   size: 17, capacity: 32
//   size: 33, capacity: 64
//   size: 65, capacity: 128
//
// ===== 重點四：擴容代價觀察 =====
// --- 加入第 1 個（capacity 從 0 變 1）---
//   建構 Tracker(1)
//
// --- 加入第 2 個（capacity 從 1 擴容）---
//   建構 Tracker(2)
//   移動建構 Tracker(1)
//   銷毀 Tracker(1)
//
// --- 加入第 3 個（capacity 可能再擴容）---
//   建構 Tracker(3)
//   移動建構 Tracker(1)
//   銷毀 Tracker(1)
//   移動建構 Tracker(2)
//   銷毀 Tracker(2)
//
// --- 程式結束，所有元素被銷毀 ---
//   銷毀 Tracker(1)
//   銷毀 Tracker(2)
//   銷毀 Tracker(3)
//
// ===== 重點五：push_back 左值 vs 右值 =====
// -- 從左值 push_back（複製）--
//   建構 Tracker(1)
//   複製建構 Tracker(1)
//
// -- 從臨時物件 push_back（移動）--
//   建構 Tracker(2)
//   移動建構 Tracker(2)
//   銷毀 Tracker(2)
//
// -- std::move push_back（強制移動）--
//   建構 Tracker(3)
//   移動建構 Tracker(3)
//
// -- 程式結束 --
//   銷毀 Tracker(3)
//   銷毀 Tracker(1)
//   銷毀 Tracker(1)
//   銷毀 Tracker(2)
//   銷毀 Tracker(3)
//
// ===== LeetCode 88. Merge Sorted Array =====
//   [1,2,3,0,0,0] + [2,5,6] = 1 2 2 3 5 6 
//   [1] + [] = 1 
//   [0](m=0) + [1] = 1 
//
// ===== 日常實務：固定容量取樣緩衝 =====
//   初始容量: 4
//   加入 20 -> 筆數 1, 平均 20, 容量 4
//   加入 22 -> 筆數 2, 平均 21, 容量 4
//   加入 24 -> 筆數 3, 平均 22, 容量 4
//   加入 26 -> 筆數 4, 平均 23, 容量 4
//   加入 28 -> 筆數 4, 平均 25, 容量 4
//   加入 30 -> 筆數 4, 平均 27, 容量 4
//   容量全程未變 -> 執行期完全沒有記憶體配置，延遲穩定
//
// ====================================================
//  複習完畢！
// ====================================================
