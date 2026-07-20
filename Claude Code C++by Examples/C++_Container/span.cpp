// ============================================================================
//  span.cpp — std::span 完整學習筆記 (C++20)
// ----------------------------------------------------------------------------
//  編譯: g++ -std=c++20 -Wall -Wextra span.cpp -o span && ./span
//  參考 (cppreference): https://en.cppreference.com/w/cpp/container/span
//  參考 (cplusplus.com): 註 — span 屬 C++20,cplusplus.com 尚未涵蓋,以 cppreference 為主
// ----------------------------------------------------------------------------
//
//  ▌ 概述
//  std::span<T, Extent>(C++20) 是「對連續記憶體的非擁有 view」 (non-owning view)。
//  它「不擁有」資料 — 只是一個 (pointer, size) 的輕量包裝。
//
//  span 不是真正的 container — 它沒有自己管理的記憶體。
//  常用來「以一致介面接受任何連續儲存的序列」(C array、std::array、std::vector)。
//
//  ▌ 底層
//  概念上等同於:
//      template <typename T, size_t Extent> struct span {
//          T*     ptr;
//          size_t size;       // 若 Extent 是 dynamic_extent
//      };
//
//  其中 Extent 可以是:
//      • static extent  : 編譯期固定大小,如 std::span<int, 5>
//      • dynamic extent : 執行期決定,如 std::span<int> = std::span<int, std::dynamic_extent>
//
//  ▌ 所屬類別
//  Views (C++20)
//
//  ▌ 時間複雜度
//      operator[] / front / back / size / data    O(1)
//      first / last / subspan                     O(1)
//      建構                                       O(1)
//
//  ▌ 與其他 container 的比較
//      span vs vector       : span 不擁有資料,vector 擁有並負責生命週期
//      span vs array_view   : array_view 是 span 的前身名稱 (各家擴充版本)
//      span vs C pointer+len: span 多了型別安全 + STL 介面 + 邊界檢查 (at)
//      span vs string_view  : string_view 專門針對 char、唯讀;span 通用且可寫
//
//  ▌ 適用情境
//      ✅ 函式參數想接受任意連續序列 (C array / vector / array)
//      ✅ 不想拷貝資料,只想「借用」一段
//      ✅ 想要邊界安全的 (ptr, len) pair
//      ❌ 想擁有資料生命週期 → 用 vector / array
//      ❌ 想接受非連續 container (list / map) → span 不行
//
//  ▌ ★ 生命週期警告
//      span 不延長底層資料的生命週期!原始資料先死,span 變懸吊指標。
//      千萬不要回傳指向 local 變數的 span。
//
// ============================================================================

/*
補充筆記：std::span
  - span 是 non-owning view，保存指標與長度，不負責延長資料生命週期。
  - span 很適合函式參數接受 array/vector/C array 的連續資料。
  - 不要從暫時容器建立 span 後保存，來源容器死亡後 span 立即 dangling。
  - std::span 的核心是資料如何儲存、查找與保持順序；選容器前先想插入刪除位置、查找方式和 iterator 失效規則。
  - vector 連續記憶體適合索引和快取區域性，list/deque/set/map 則針對不同插入刪除或查找需求。
  - 關聯容器 set/map 依比較器排序，unordered 容器依 hash 分桶；兩者不是誰永遠比較快，而是前提與需求不同。
  - operator[] 在 map/unordered_map 查不到 key 會插入預設值；純查詢應使用 find、contains 或 at。
  - 容器元素型別若昂貴，優先理解 emplace、move 和 reference/iterator 有效性，不要盲目複製。
  - 所有容器都要考慮空容器邊界；front/back/top 在空容器上呼叫通常是未定義行為或前置條件違反。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::span (C++20)
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::span 是什麼？解決什麼問題？
//     答：std::span<T> 是對連續記憶體的「非擁有 (non-owning) view」，內部就是 (pointer, size)，
//         不管生命週期、不做配置，建構與存取都是 O(1)。
//         用途是統一函式介面：一個吃 std::span<const int> 的函式可同時接受
//         std::vector<int>、std::array<int,N>、C 陣列，而不需要寫成 template 或傳 (ptr, len) 兩個參數。
//     追問：span 要 pass by value 還是 reference？（by value，它本身就很輕）
//
// 🔥 Q2. static extent 和 dynamic extent 差在哪？
//     答：std::span<int, 5> 是 static extent——大小是編譯期常數，尺寸不需存在物件裡，
//         且能在編譯期檢查不匹配。std::span<int>（即 Extent = std::dynamic_extent）大小在執行期決定，
//         物件內額外存一個 size，彈性較大但少了編譯期保證。
//
// ⚠️ 陷阱. span 會不會延長被指向物件的壽命？
//     答：不會。span 不擁有也不管理生命週期，若底層 vector 擴容、被清空或解構，
//         span 就懸空 (dangling)，再存取就是 UB。尤其不要回傳指向局部容器的 span。
//     為什麼會錯：把它當成「輕量版 vector」，
//         實際上它與 std::string_view 同屬一類（view），危險模型也完全相同。
// ═══════════════════════════════════════════════════════════════════════════

#include <span>
#include <vector>
#include <array>
#include <iostream>
#include <string>
#include <numeric>     // std::accumulate
#include <algorithm>   // std::max (for LeetCode 範例)

// ★ 經典用法:函式以 std::span 為參數,可同時接受
//    C array / std::array / std::vector / 子區段
template <typename T>
double average(std::span<const T> data) {
    if (data.empty()) return 0;
    T sum{};
    for (auto x : data) sum += x;
    return static_cast<double>(sum) / data.size();
}

// 也可以是非 const,允許函式修改原資料
void doubleAll(std::span<int> data) {
    for (auto& x : data) x *= 2;
}

template <typename T, std::size_t E>
void print(std::span<T, E> sp, const std::string& label = "") {
    if (!label.empty()) std::cout << label << ": ";
    std::cout << "[ ";
    for (auto& e : sp) std::cout << e << ' ';
    std::cout << "] (size=" << sp.size() << ")\n";
}

int main() {
    // ========================================================================
    //  1. 建構子
    // ========================================================================

    // (1) 從 C array
    int c_arr[] = {1, 2, 3, 4, 5};
    std::span<int> sp1(c_arr);                         // 自動推導 size = 5
    std::span<int, 5> sp1s(c_arr);                      // static extent

    // (2) 從 std::array
    std::array<int, 4> arr = {10, 20, 30, 40};
    std::span<int> sp2(arr);
    std::span<int, 4> sp2s(arr);

    // (3) 從 std::vector
    std::vector<int> vec = {100, 200, 300};
    std::span<int> sp3(vec);

    // (4) 從 (pointer, size)
    std::span<int> sp4(vec.data(), vec.size());

    // (5) 從 (pointer, pointer)  — 只在 C++20+
    std::span<int> sp5(vec.data(), vec.data() + 2);

    // (6) 預設建構 (空 span)
    std::span<int> sp6;
    std::cout << "sp6.empty() = " << std::boolalpha << sp6.empty() << '\n';

    print(sp1, "sp1 (from C arr)    ");
    print(sp2, "sp2 (from array)    ");
    print(sp3, "sp3 (from vector)   ");
    print(sp5, "sp5 (ptr, ptr)      ");

    // ========================================================================
    //  2. 元素存取
    // ========================================================================
    //  operator[] / front / back / data
    //  ★ 沒有 at() — span 認為邊界檢查應該由使用者負責 (或自己檢查 size)
    std::cout << "\nsp1[2] = " << sp1[2] << '\n';
    std::cout << "front  = " << sp1.front() << '\n';
    std::cout << "back   = " << sp1.back()  << '\n';
    std::cout << "data() points to " << *sp1.data() << '\n';

    // ========================================================================
    //  3. Iterators
    // ========================================================================
    //  random access iterator,支援正向 + 反向
    std::cout << "\n[正向] ";
    for (auto it = sp1.begin(); it != sp1.end(); ++it) std::cout << *it << ' ';
    std::cout << "\n[反向] ";
    for (auto it = sp1.rbegin(); it != sp1.rend(); ++it) std::cout << *it << ' ';
    std::cout << '\n';

    // ========================================================================
    //  4. 觀察 (Observers)
    // ========================================================================
    //  size() / size_bytes() / empty()
    std::cout << "\nsize       = " << sp1.size() << '\n';
    std::cout << "size_bytes = " << sp1.size_bytes() << '\n';   // size * sizeof(T)
    std::cout << "empty      = " << sp1.empty() << '\n';

    // ========================================================================
    //  5. 子區段 (Subviews)
    // ========================================================================

    // ──── first<N>() / first(n) ────  取前 N 個元素
    auto f3 = sp1.first(3);
    print(f3, "first(3)            ");
    auto fs3 = sp1.first<3>();          // template 版本,回傳 static extent
    print(fs3, "first<3>()          ");

    // ──── last<N>() / last(n) ────  取後 N 個元素
    auto l2 = sp1.last(2);
    print(l2, "last(2)             ");

    // ──── subspan<Offset, Count>() / subspan(offset, count) ────
    // 取 [offset, offset+count) 的子區段
    auto sub = sp1.subspan(1, 3);       // 從 index 1 開始取 3 個 → [2,3,4]
    print(sub, "subspan(1,3)        ");

    auto sub_to_end = sp1.subspan(2);   // count 省略 → 取到尾
    print(sub_to_end, "subspan(2)          ");

    // ========================================================================
    //  6. 函式參數應用
    // ========================================================================
    int  c2[] = {1, 2, 3, 4, 5};
    std::array<int, 4>  ar = {10, 20, 30, 40};
    std::vector<int>    vc = {100, 200, 300};
    std::cout << "\navg(C array)  = " << average<int>(c2) << '\n';
    std::cout << "avg(array)    = " << average<int>(ar) << '\n';
    std::cout << "avg(vector)   = " << average<int>(vc) << '\n';

    // 取部分
    std::cout << "avg(first 3 of vector) = "
              << average<int>(std::span<const int>(vc).first(3)) << '\n';

    // 修改原資料
    std::vector<int> w = {1, 2, 3};
    doubleAll(w);
    std::cout << "doubled: ";
    for (auto x : w) std::cout << x << ' ';
    std::cout << '\n';

    // ========================================================================
    //  7. static vs dynamic extent
    // ========================================================================
    std::span<int, 5>            sp_static(c_arr);   // 大小編譯期已知
    std::span<int>               sp_dynamic(c_arr); // 大小執行期儲存

    std::cout << "\nstatic extent  = "
              << decltype(sp_static)::extent << '\n';
    std::cout << "dynamic extent = "
              << decltype(sp_dynamic)::extent
              << " (== std::dynamic_extent="
              << std::dynamic_extent << ")\n";

    // 從 dynamic 轉 static 必須明確 (size 不對是 UB)
    // std::span<int, 5> back_to_static = sp_dynamic;   // 錯,需要明確轉換
    // std::span<int, 5> ok = sp_dynamic.first<5>();    // 這樣可以 (compile-time 5)

    // ========================================================================
    //  8. 與 const 結合
    // ========================================================================
    //  span<const T>  : 唯讀視圖 (常用於 input parameter)
    //  const span<T>  : 視圖本身 const,但仍可改元素 (跟 const ptr 同理)
    std::vector<int> vv{1,2,3};
    std::span<const int> ro(vv);
    // ro[0] = 99;        // 編譯錯誤
    const std::span<int> co(vv);
    co[0] = 99;           // OK,co 本身 const 但元素可改
    std::cout << "const span: ";
    for (auto x : vv) std::cout << x << ' ';
    std::cout << '\n';

    // ========================================================================
    //  9. as_bytes / as_writable_bytes (低階 byte view)
    // ========================================================================
    //  把 span<T> 看成一個 span<const std::byte> / span<std::byte>
    //  常用於序列化 / 寫入 binary file。
    int xs[] = {0x01020304, 0x05060708};
    std::span<int> sxs(xs);
    auto bytes = std::as_bytes(sxs);                  // span<const std::byte>
    std::cout << "\nbyte view size = " << bytes.size() << '\n';

    // ========================================================================
    //  10. 常見陷阱 (Pitfalls) ★必看
    // ========================================================================
    //
    //  (1) span 不擁有資料
    //      底層 vector / array 死掉 → span 變懸吊。
    //      千萬不要:
    //          std::span<int> bad() {
    //              std::vector<int> v{1,2,3};
    //              return std::span<int>(v);    // ❌ v 馬上 destruct
    //          }
    //
    //  (2) 沒有 at() — operator[] 越界是 UB
    //      自己用 if (i < sp.size()) 檢查。
    //
    //  (3) 不能接受非連續 container
    //      list / map / forward_list 都不行 (沒有連續儲存)。
    //
    //  (4) static extent 與 dynamic extent 不能隱式互轉
    //      span<int, 5> → span<int> 可以 (退化);反向需要明確且大小要對。
    //
    //  (5) span<const T> 無法寫入,但仍可被 reassign 指向新區段
    //      const span 才是 view 自身不可變。
    //
    //  (6) span 的 resize / push_back 不存在
    //      它是「視圖」,不能變更大小。
    //
    //  (7) 與 string 互通要用 std::string_view,不要用 span<char>
    //      span<char> 不知道是不是 null-terminated。

    // ========================================================================
    //  11. 最佳實踐
    // ========================================================================
    //
    //  • 函式參數用 std::span<const T>(輸入) / std::span<T>(可改) 取代:
    //      void f(const T* arr, size_t n)        // ❌ 老 C 風格
    //      void f(const std::vector<T>& v)        // ❌ 限定要 vector
    //      void f(std::span<const T> data)        // ✅ 通吃
    //
    //  • 不需要 own 資料時,優先 span 而非 const vector&,可避免不必要拷貝
    //  • 想讀部分區段 → first/last/subspan
    //  • 操作 binary 資料 → std::as_bytes / std::as_writable_bytes
    //  • 永遠注意「底層資料的生命週期」是否還在

    // ========================================================================
    //  12. LeetCode 實戰範例 ★ 真實場景常見題型
    // ========================================================================
    // span 不是「演算法容器」, 它是「函式介面」的好朋友 — 把演算法寫成接受
    // span 的版本,vector / array / C-array 都能直接傳進來,不需要 template。

    // ──── 經典題: 用 span 寫一個「通用版本」的 max ────
    // 同一個函式可同時接受 vector / array / C-array,因為它們的記憶體都是連續的。
    {
        auto max_in = [](std::span<const int> data) -> int {
            int m = data[0];
            for (int x : data) m = std::max(m, x);
            return m;
        };
        std::vector<int> v{3, 1, 4, 1, 5, 9, 2, 6};
        std::array<int, 5> a{10, 20, 30, 40, 50};
        int c[]{7, 8, 9};

        std::cout << "\n[span max]\n";
        std::cout << "  vector:  " << max_in(v) << '\n';   // 9
        std::cout << "  array:   " << max_in(a) << '\n';   // 50
        std::cout << "  C-array: " << max_in(c) << '\n';   // 9
    }

    // ──── 切片操作: 把 vector 拆成兩半分別處理 ────
    // 在分治演算法 (merge sort、quick select) 中,常需要把一段資料拆給遞迴函式。
    // span 是「零拷貝」的 view,subspan 完美對應這種需求。
    {
        auto sum = [](std::span<const int> data) {
            long long s = 0;
            for (int x : data) s += x;
            return s;
        };
        std::vector<int> nums{1, 2, 3, 4, 5, 6, 7, 8};
        std::span<const int> all(nums);
        auto left  = all.subspan(0, nums.size() / 2);
        auto right = all.subspan(nums.size() / 2);
        std::cout << "[span subspan] left sum = " << sum(left)
                  << ", right sum = " << sum(right) << '\n';
        // 預期輸出: left sum = 10, right sum = 26
    }

    // ──── LC 480 風格: 滑動視窗以 span 表示 ────
    // 用 span 表示視窗 [i, i+k),avoid 拷貝。
    {
        std::vector<int> nums{4, 1, 7, 2, 8, 3, 5};
        int k = 3;
        std::cout << "[span sliding window k=3] window maxes: ";
        for (size_t i = 0; i + k <= nums.size(); ++i) {
            std::span<const int> win(nums.data() + i, k);
            int mx = win[0];
            for (int x : win) mx = std::max(mx, x);
            std::cout << mx << ' ';
        }
        std::cout << '\n';
        // 預期輸出: 7 7 8 8 8
    }

    // ──── LC 88: Merge Sorted Array — 用 span 表達「子區間 view」 ────
    // span 在 LC 中最自然的用途是「把一塊連續記憶體傳給函式但不指定容器型別」。
    // 這裡示範:把 nums1 的前 m 段 + nums2 兩段都當成 span 傳給合併函式,
    // 直接寫到 nums1 的完整空間,完全零拷貝、無 vector 重新配置。
    {
        auto merge_into = [](std::span<int> dst,           // 完整 nums1 (含尾巴 0)
                             std::span<const int> a,        // 前 m 段
                             std::span<const int> b) {      // nums2 全部
            int i = (int)a.size() - 1, j = (int)b.size() - 1, k = (int)dst.size() - 1;
            while (j >= 0) {
                if (i >= 0 && a[i] > b[j]) dst[k--] = a[i--];
                else                       dst[k--] = b[j--];
            }
        };
        std::vector<int> nums1{1, 2, 3, 0, 0, 0};
        std::vector<int> nums2{2, 5, 6};
        int m = 3;
        // 注意:必須先把 a 的 view 取出,因為等等寫入 nums1 後前段內容會被覆蓋
        std::vector<int> snapshot(nums1.begin(), nums1.begin() + m);
        merge_into(std::span<int>(nums1), std::span<const int>(snapshot),
                   std::span<const int>(nums2));
        std::cout << "[LC88 Merge via span] = [ ";
        for (int x : nums1) std::cout << x << ' ';
        std::cout << "]\n";
        // 預期輸出: [ 1 2 2 3 5 6 ]
    }

    // ========================================================================
    //  12. 實戰範例:影像處理 — 行/列 view (Row View) 不拷貝 buffer
    // ========================================================================
    // 真實場景:影像處理 / DSP 演算法常把整張 image (連續記憶體) 拆成多列分別運算。
    // 用 span<uint8_t> 表示每一列,完全沒有拷貝;傳入濾波函式時也不必綁定容器型別。
    //   • span(ptr, n) 建構 O(1)
    //   • subspan 切片 O(1)
    //   • 可直接 indexing / iterate,寫法與 vector 幾乎一樣
    {
        constexpr int W = 4, H = 3;
        std::array<int, W * H> img{
            10, 20, 30, 40,    // row 0
            50, 60, 70, 80,    // row 1
            15, 25, 35, 45     // row 2
        };
        // 取得第 r 列的 view
        auto row_view = [&](int r) {
            return std::span<int>(img.data() + r * W, W);
        };
        // 對每列做簡單統計
        for (int r = 0; r < H; ++r) {
            auto row = row_view(r);
            int sum = 0, mx = row[0];
            for (int v : row) { sum += v; mx = std::max(mx, v); }
            std::cout << "[Image Row " << r << "] sum=" << sum << ",max=" << mx << '\n';
        }
        // 預期輸出: row 0 sum=100,max=40 / row 1 sum=260,max=80 / row 2 sum=120,max=45
    }

    std::cout << "\n=== span demo 結束 ===\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：std::span 是「擁有」還是「不擁有」記憶體?
    //    A：不擁有 (non-owning view)。它只是 (pointer, size) 兩個欄位的薄包裝,
    //       生命週期完全依附於底層資料。傳遞 / 拷貝 span 都是 O(1) 兩個 word,
    //       銷毀時也不會釋放任何記憶體。誤用 dangling span 會 UB。
    //
    //  Q2：span<int> 與 span<int, 5> 有何差別?
    //    A：前者是 dynamic extent,size 在 runtime 決定;後者是 static extent,
    //       size 編譯期就固定為 5,可被視為一種強型別「固定大小視窗」,給編譯器
    //       更多優化空間,並可在介面上明確要求剛好 5 個元素的陣列。
    //
    //  Q3：何時用 span 取代 const std::vector<T>& 或 (T*, size_t)?
    //    A：寫泛型函式接受「任何連續記憶體序列」(C array、std::array、std::vector
    //       甚至 string_view 風格的子陣列) 時,span 是最佳選擇。比 vector& 更彈性
    //       (不限定容器型別)、比 (T*, size_t) 更安全 (帶 bounds-checked 介面)。
    //
    return 0;
}

/*
============================================================================
  附錄:std::span 完整 member function 一覽 (C++20)
============================================================================
  Constructors / destructor / operator=
      span()  (Extent==0 或 dynamic 才能預設建構)
      span(It first, size_type count)
      span(It first, It last)
      span(T(&arr)[N])
      span(std::array<T, N>&) / (const std::array<T, N>&)
      span(R&& range)              (C++20 ranges 的 contiguous_range)
      span(const span&)            (拷貝)
      span(std::initializer_list<T>)  (C++26 預定)

  Element access:    operator[], front, back, data
  Iterators:         begin/end/rbegin/rend (cbegin等 C++23)
  Observers:         size, size_bytes, empty
  Subviews:          first, last, subspan

  Free functions:
      std::as_bytes(span)
      std::as_writable_bytes(span)

  ★ 沒有:at, push_back, insert, erase, resize, swap (因為 view 不擁有資料)
============================================================================
*/
