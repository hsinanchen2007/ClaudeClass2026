// =============================================================================
//  第四課：迭代器的核心概念 10  —  迭代器與指標：原生指標就是一種迭代器
// =============================================================================
//
// 【主題資訊 Information】
//   簽名：T* vector<T>::data() noexcept;                    // C++11
//         const T* vector<T>::data() const noexcept;
//   標準版本：
//     vector::data()          C++11（在此之前只能寫 &v[0]，且空 vector 時是 UB）
//     std::begin / std::end   C++11（對 C 陣列也適用）
//     contiguous_iterator     C++20（正式把「連續」寫進迭代器分類）
//     std::to_address         C++20（從迭代器安全取得底層指標）
//   複雜度：data() 為 O(1)；指標與 vector 迭代器的所有操作都是 O(1)。
//   標頭檔：<vector>、<iterator>（std::begin/std::end）、<algorithm>
//
// 【詳細解釋 Explanation】
//
// 【1. 迭代器的設計原點就是指標】
//   Alexander Stepanov 設計 STL 時的核心洞見是：
//   「C 語言操作陣列的慣用法（指標 + ++ + 解參考 + 比較）已經是一個
//     可以泛化的介面。」
//   於是他把這組操作抽象成「迭代器」概念，讓鏈結串列、樹、雜湊表
//   都能提供同樣的介面。因此：
//       **原生指標 T* 天生滿足 Random Access Iterator 的所有要求**，
//   不需要任何包裝就能直接餵給 STL 演算法：
//       int arr[] = {5, 3, 1, 4, 2};
//       std::sort(arr, arr + 5);          // arr 會退化成 int*，直接當迭代器用
//       std::find(std::begin(arr), std::end(arr), 3);   // C++11 更清楚的寫法
//
// 【2. 那為什麼 vector::iterator 不直接是 int*？】
//   libstdc++ 把它做成 __gnu_cxx::__normal_iterator<int*, vector<int>> ——
//   一個只包住 int* 的薄包裝（sizeof 相同，都是 8 bytes）。理由有三：
//     (a) 型別安全：讓 vector<int>::iterator 與 vector<double>::iterator、
//         甚至與裸 int* 都是不同型別，避免誤傳。
//     (b) 多載解析：使用者可以為自己的迭代器型別特化行為；
//         若是裸指標則無法區分「這是 vector 的迭代器」還是「隨便一個 int*」。
//     (c) 可替換實作：開啟 _GLIBCXX_DEBUG 時可換成會做邊界檢查的迭代器，
//         使用者程式碼一行都不用改。
//   因為只包一個指標、成員函式全 inline，-O2 下產生的機器碼與裸指標相同。
//
// 【3. &*it 與 it 的差別（很多人搞混）】
//   it     是迭代器物件（可能是包裝、可能是指標）
//   *it    是元素的參考
//   &*it   是元素的**位址**，型別必定是 T*
//   對 vector 而言 v.data() == &*v.begin()（非空時），但兩者語意不同。
//   重要陷阱：**空容器不能寫 &*v.begin()**（解參考 end() 是 UB），
//   但 v.data() 對空 vector 是**合法**的（可能回傳 nullptr 或某個有效指標）。
//   這正是 C++11 引入 data() 的主要理由 —— 取代不安全的 &v[0]。
//
// 【4. 只有「連續」容器才能安全轉指標】
//   vector / array / string 的元素保證連續存放，因此
//       memcpy(dst, v.data(), v.size() * sizeof(T));
//   是合法的，也能直接把 v.data() 傳給 C API。
//   但 deque **不行** —— 它是 Random Access 卻不連續（分段配置），
//   &deq[0] 只是第一個區塊的起點，往後走會越界。
//   list / map 更是完全不連續。
//   C++20 用 contiguous_iterator_tag 把這個區別正式寫進型別系統，
//   讓泛型程式碼可以用 if constexpr 判斷是否能安全取指標。
//
// 【概念補充 Concept Deep Dive】
//   為什麼 vector 的迭代器在 -O2 下和裸指標一樣快？
//   關鍵在於「零成本抽象」的三個條件同時成立：
//     (1) __normal_iterator 只有一個 T* 成員 → 佈局與指標完全相同
//     (2) 所有成員函式（operator*、operator++、operator+）都是 inline 且極短
//     (3) 沒有虛擬函式 → 沒有 vtable 指標、沒有間接跳躍
//   因此編譯器在 inline 展開後，看到的就是純粹的指標運算，
//   後續的迴圈最佳化（向量化、展開、強度削減）全部照常生效。
//   這與 Java 的 Iterator 介面形成鮮明對比：後者每次 next() 都是一次
//   無法內聯的介面呼叫（除非 JIT 成功去虛擬化）。
//   可以用 `g++ -O2 -S` 對照兩種寫法產生的組語來親自驗證這一點。
//
// 【注意事項 Pay Attention】
//   1. 空 vector 不可寫 &*v.begin() 或 &v[0]（UB）；請用 v.data()。
//   2. deque 是 Random Access 但**不連續**，不可把 &deq[0] 當陣列指標。
//   3. data() 取得的指標在容器重新配置（push_back 等）後立即失效。
//   4. C 陣列當作迭代器時請用 std::begin(arr) / std::end(arr)（C++11），
//      比 arr / arr + N 更不易寫錯 —— 但注意陣列一旦退化成指標
//      （例如傳進函式），std::end 就失效了，因為長度資訊已經遺失。
//   5. 對 C 陣列使用 sizeof(arr)/sizeof(arr[0]) 求長度，同樣只在
//      「陣列尚未退化成指標」的作用域內才正確。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】迭代器與指標的關係
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 原生指標算不算迭代器？為什麼 std::sort 可以直接用在 C 陣列上？
//     答：算，而且是最強的 Random Access Iterator —— 指標天生支援
//         *p、++p、--p、p+n、p1-p2、p[n]、p1<p2 全部操作。
//         STL 的迭代器概念本來就是從指標抽象出來的，
//         所以 std::sort(arr, arr+5) 完全合法（arr 退化成 int*）。
//     追問：那 std::begin(arr) 和 arr 有什麼差別？
//           → 對真正的陣列型別兩者等價，但 std::begin/std::end（C++11）
//             更安全、更泛用：同一份泛型程式碼可以同時吃陣列與容器。
//             不過 std::end(arr) 需要編譯期知道長度，
//             陣列一旦退化成指標傳進函式就用不了了。
//
// 🔥 Q2. 為什麼 vector<int>::iterator 不直接定義成 int*？
//     答：型別安全與可替換性。做成包裝類別後，vector<int>::iterator 與裸 int*
//         是不同型別，可避免誤傳、也讓多載解析能區分；
//         而且 debug 模式（_GLIBCXX_DEBUG）可以換成會檢查邊界的實作，
//         使用者程式碼完全不用改。
//         因為它只包一個指標且全部 inline，-O2 下機器碼與裸指標相同。
//     追問：那 deque 的迭代器呢？
//           → deque 是分段連續，迭代器要存 4 個指標
//             （目前位置、本區塊起點、本區塊終點、區塊索引表位置），
//             operator+ 要做除法與取餘。它仍是 Random Access（O(1)），
//             但**不是** contiguous，且複製成本比 vector 迭代器高。
//
// ⚠️ 陷阱. int* p = &v[0]; 對空的 vector 為什麼是未定義行為？v.data() 為什麼可以？
//     答：v[0] 等同於 *(v.begin() + 0)，對空容器就是解參考 end() —— UB。
//         即使你只是想取位址而不讀值，語言規則上 &v[0] 仍然先做了解參考。
//         v.data() 則被標準明確定義為「回傳指向底層陣列的指標」，
//         空容器時回傳值未指定（可能是 nullptr），但**取得它本身是合法的**。
//     為什麼會錯：直覺認為 &x 只是取位址、不會真的讀取記憶體，所以無害。
//         但 UB 的判定是語言層規則而非硬體行為 —— 編譯器可以據此做出
//         「這裡不可能是空容器」的假設並刪掉你後面的檢查。
//         這也是 C++11 補上 data() 的直接動機。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <deque>
#include <string>
#include <iterator>
#include <functional>  // std::greater
#include <algorithm>    // 原檔缺這個標頭，std::find 無法編譯 —— 本次補上

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 217. Contains Duplicate
//   題目：判斷陣列中是否存在重複元素。
//   為什麼用到本主題：這裡刻意寫成**兩個版本**——一個吃 std::vector、
//         一個吃原生 C 陣列（用指標當迭代器）——而**函式本體幾乎一模一樣**。
//         這正是本檔的核心論點：指標就是迭代器，
//         所以同一套 sort + adjacent_find 對兩者都成立，
//         連泛型 template 都不必寫（只是型別不同而已）。
//   複雜度：時間 O(N log N)、空間 O(1)（就地排序）。
// -----------------------------------------------------------------------------

// 版本 A：vector 迭代器
bool containsDuplicateVector(std::vector<int> nums) {
    std::sort(nums.begin(), nums.end());
    return std::adjacent_find(nums.begin(), nums.end()) != nums.end();
}

// 版本 B：原生指標當迭代器（注意 first/last 就是兩個 int*）
bool containsDuplicateRaw(int* first, int* last) {
    std::sort(first, last);                                  // 指標直接當迭代器
    return std::adjacent_find(first, last) != last;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】把 vector 的內容交給 C API（例如 write / send / 影像函式庫）
//   情境：現代 C++ 程式常需要呼叫 C 介面的函式庫（zlib、libpng、POSIX write）,
//         它們的簽名一律是 (const void* buf, size_t len)。
//   為什麼用到本主題：v.data() 正是這個橋樑 —— 它保證回傳指向連續記憶體的指標。
//         這裡也示範為什麼 **deque 不能這樣用**（Random Access 但不連續），
//         以及為什麼要在呼叫前檢查 empty()。
// -----------------------------------------------------------------------------
// 模擬一個 C 風格 API（真實世界可能是 ::write / png_write_row / z_deflate）
std::size_t fake_c_api_checksum(const unsigned char* buf, std::size_t len) {
    std::size_t sum = 0;
    for (std::size_t i = 0; i < len; ++i) sum = (sum * 31 + buf[i]) % 1000003;
    return sum;
}

std::size_t checksumOf(const std::vector<unsigned char>& payload) {
    if (payload.empty()) {
        // data() 對空容器是合法的，但傳 nullptr 給 C API 可能不合法 → 先擋掉
        return 0;
    }
    return fake_c_api_checksum(payload.data(), payload.size());
}

int main() {
    std::vector<int> vec = {10, 20, 30, 40, 50};

    // 取得底層指標
    int* ptr = vec.data();  // C++11
    // 或 int* ptr = &vec[0];

    // 取得迭代器
    auto it = vec.begin();

    std::cout << "=== 相似的操作 ===" << std::endl;
    std::cout << "指標解參考: *ptr = " << *ptr << std::endl;
    std::cout << "迭代器解參考: *it = " << *it << std::endl;

    std::cout << "\n指標算術: *(ptr + 2) = " << *(ptr + 2) << std::endl;
    std::cout << "迭代器算術: *(it + 2) = " << *(it + 2) << std::endl;

    // 迭代器可以轉換成指標（對於連續記憶體的容器）
    std::cout << "\n=== 迭代器轉指標 ===" << std::endl;
    int* from_iterator = &(*it);  // 取得迭代器指向的元素的位址
    std::cout << "*from_iterator = " << *from_iterator << std::endl;

    // 但指標不能直接當迭代器用在演算法中（需要配合 size）
    // 好消息是，原始指標本身就是一種迭代器！
    std::cout << "\n=== 指標作為迭代器 ===" << std::endl;
    int arr[] = {100, 200, 300};

    // 指標可以直接用在 STL 演算法
    auto found = std::find(arr, arr + 3, 200);
    if (found != arr + 3) {
        std::cout << "在陣列中找到 200" << std::endl;
    }

    // C++11 更清楚的寫法：std::begin / std::end
    std::cout << "\n=== std::begin / std::end 對 C 陣列也適用 ===" << std::endl;
    auto found2 = std::find(std::begin(arr), std::end(arr), 300);
    std::cout << "  std::find 找 300: "
              << (found2 != std::end(arr) ? "找到" : "沒找到") << std::endl;
    std::cout << "  陣列長度 std::end - std::begin = "
              << (std::end(arr) - std::begin(arr)) << std::endl;
    std::cout << "  （注意：陣列一旦退化成指標傳進函式，std::end 就用不了了）"
              << std::endl;

    // data() vs &v[0]：空容器時的差別
    std::cout << "\n=== data() vs &v[0]（空容器）===" << std::endl;
    std::vector<int> empty_vec;
    std::cout << "  empty_vec.size()   = " << empty_vec.size() << std::endl;
    std::cout << "  empty_vec.data()   = "
              << (empty_vec.data() == nullptr ? "nullptr" : "非 nullptr")
              << "  ← 取得它本身是合法的" << std::endl;
    std::cout << "  &empty_vec[0]      = 未定義行為，絕對不要寫（本檔刻意不執行）"
              << std::endl;
    std::cout << "  （空容器時 data() 的回傳值由實作決定；"
                 "本機 g++ 15.2 / libstdc++ 回傳 nullptr）" << std::endl;

    // 連續 vs 非連續
    std::cout << "\n=== 連續（vector）vs 非連續（deque）===" << std::endl;
    std::vector<int> v_cont = {1, 2, 3, 4};
    bool contiguous = true;
    for (std::size_t i = 1; i < v_cont.size(); ++i) {
        if (&v_cont[i] != &v_cont[i - 1] + 1) { contiguous = false; break; }
    }
    std::cout << "  vector 元素位址是否連續: " << (contiguous ? "是" : "否")
              << "  → 可以安全傳給 C API" << std::endl;
    std::cout << "  deque 有 operator[] 與 O(1) 隨機存取，但**不保證連續**，"
              << std::endl;
    std::cout << "  所以 deque 沒有提供 data()，也不可把 &deq[0] 當陣列指標。"
              << std::endl;
    std::deque<int> d = {1, 2, 3, 4};
    std::cout << "  （deque 仍可用 std::sort，因為它是 Random Access："
              << "排序後 = ";
    std::sort(d.begin(), d.end(), std::greater<int>());
    for (int n : d) std::cout << n << " ";
    std::cout << ")" << std::endl;

    std::cout << "\n=== LeetCode 217. Contains Duplicate ===" << std::endl;
    std::cout << "  vector 版 [1,2,3,1] → "
              << (containsDuplicateVector({1, 2, 3, 1}) ? "true" : "false") << std::endl;
    std::cout << "  vector 版 [1,2,3,4] → "
              << (containsDuplicateVector({1, 2, 3, 4}) ? "true" : "false") << std::endl;

    int raw1[] = {1, 2, 3, 1};
    int raw2[] = {1, 2, 3, 4};
    std::cout << "  C 陣列版 [1,2,3,1] → "
              << (containsDuplicateRaw(std::begin(raw1), std::end(raw1)) ? "true" : "false")
              << std::endl;
    std::cout << "  C 陣列版 [1,2,3,4] → "
              << (containsDuplicateRaw(std::begin(raw2), std::end(raw2)) ? "true" : "false")
              << std::endl;
    std::cout << "  → 兩個版本的函式本體完全相同，只是迭代器型別不同" << std::endl;

    std::cout << "\n=== 日常實務：把 vector 交給 C API ===" << std::endl;
    std::vector<unsigned char> payload = {'H', 'e', 'l', 'l', 'o', ' ', 'S', 'T', 'L'};
    std::cout << "  payload 長度 = " << payload.size() << std::endl;
    std::cout << "  checksum(data(), size()) = " << checksumOf(payload) << std::endl;

    std::vector<unsigned char> nothing;
    std::cout << "  空 payload 的 checksum   = " << checksumOf(nothing)
              << "  ← 先擋掉 empty，不把 nullptr 丟給 C API" << std::endl;

    return 0;
}

// 注意：空 vector 的 data() 回傳值由實作決定（標準未規定必為 nullptr）。
//       下方輸出中的「nullptr」是本機 g++ 15.2 / libstdc++ / x86-64 的實測結果，
//       其他標準函式庫可能回傳非空的有效指標 —— 兩者都合法，
//       共通點是「取得它是合法的，但不可解參考」。

// 編譯: g++ -std=c++17 -Wall -Wextra 第四課：迭代器（Iterator）的核心概念10.cpp -o demo10

// === 預期輸出 ===
// === 相似的操作 ===
// 指標解參考: *ptr = 10
// 迭代器解參考: *it = 10
//
// 指標算術: *(ptr + 2) = 30
// 迭代器算術: *(it + 2) = 30
//
// === 迭代器轉指標 ===
// *from_iterator = 10
//
// === 指標作為迭代器 ===
// 在陣列中找到 200
//
// === std::begin / std::end 對 C 陣列也適用 ===
//   std::find 找 300: 找到
//   陣列長度 std::end - std::begin = 3
//   （注意：陣列一旦退化成指標傳進函式，std::end 就用不了了）
//
// === data() vs &v[0]（空容器）===
//   empty_vec.size()   = 0
//   empty_vec.data()   = nullptr  ← 取得它本身是合法的
//   &empty_vec[0]      = 未定義行為，絕對不要寫（本檔刻意不執行）
//   （空容器時 data() 的回傳值由實作決定；本機 g++ 15.2 / libstdc++ 回傳 nullptr）
//
// === 連續（vector）vs 非連續（deque）===
//   vector 元素位址是否連續: 是  → 可以安全傳給 C API
//   deque 有 operator[] 與 O(1) 隨機存取，但**不保證連續**，
//   所以 deque 沒有提供 data()，也不可把 &deq[0] 當陣列指標。
//   （deque 仍可用 std::sort，因為它是 Random Access：排序後 = 4 3 2 1 )
//
// === LeetCode 217. Contains Duplicate ===
//   vector 版 [1,2,3,1] → true
//   vector 版 [1,2,3,4] → false
//   C 陣列版 [1,2,3,1] → true
//   C 陣列版 [1,2,3,4] → false
//   → 兩個版本的函式本體完全相同，只是迭代器型別不同
//
// === 日常實務：把 vector 交給 C API ===
//   payload 長度 = 9
//   checksum(data(), size()) = 756062
//   空 payload 的 checksum   = 0  ← 先擋掉 empty，不把 nullptr 丟給 C API
