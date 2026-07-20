// ============================================================================
//  04_template_parameters.cpp  ──  Template 參數的三種類型
// ============================================================================
//
//  【本篇目標】
//    Template 參數可分為 3 種，本篇先講前 2 種，第 3 種 (template template
//    parameter) 在 file 10 詳述：
//
//      ① Type parameter            : template<typename T>
//      ② Non-type parameter (NTTP) : template<int N>, template<size_t N>
//      ③ Template template param   : template<template<typename> class C>
//
// ----------------------------------------------------------------------------
//  【① Type parameter】
//    最常見、前面三篇講的都是這個。語法 template<typename T> 或 class T。
//
// ----------------------------------------------------------------------------
//  【② Non-type Template Parameter (NTTP)】
//    傳「值」(編譯期常數)，而不是「型別」。可用的型別：
//      - 整數型別 (int, std::size_t, char, bool, enum 等)
//      - 指標 / 參考 (有 linkage 的物件)
//      - 列舉 (enum / enum class)
//      - C++17：auto NTTP，編譯器推
//      - C++20：literal class types (e.g. structural type) 也能當 NTTP
//      - C++20：浮點型 (float / double) 也能 (但少用)
//
//    參考：https://en.cppreference.com/cpp/language/template_parameters
//
//    經典用例：std::array<T, N>。N 是編譯期常數，所以陣列大小寫死進型別。
//
//    NTTP 的特色：
//      ◆ 編譯期常數 → 可以做為陣列大小、constexpr 計算
//      ◆ 不同的 N 是不同的型別！Buffer<int,3> 與 Buffer<int,4> 完全不相容
//      ◆ 沒有 runtime cost：全部展開成具體型別
//
//    NTTP 容易踩的坑：
//      ◆ 不能把「runtime 變數」當 NTTP：
//
//            int n = 10;
//            std::array<int, n> arr;     // ❌ n 不是 constexpr
//            constexpr int n2 = 10;
//            std::array<int, n2> arr2;   // ✅
//
// ----------------------------------------------------------------------------
//  【③ Template Template Parameter】
//    template 本身當參數。例如把 std::vector 這種「還沒填型別」的
//    template 整個傳進來：
//
//        template <template<typename> class Container>
//        class Adapter { ... };
//
//        Adapter<std::vector> a;     // 注意：寫 std::vector 而不是 std::vector<int>
//
//    這個會在 file 10 詳細介紹，這裡只先點名。
//
//  【實用 mental model】
//    Type 參數 = 我要操作什麼型別？
//    NTTP     = 我要在編譯期把什麼「值」鎖進型別？(常見：大小、維度、選項)
//    Template template = 我要傳什麼「容器策略」？(很少用)
// ============================================================================

/*
補充筆記：template_parameters
  - template_parameters 涉及模板實例化；請先判斷哪些型別由呼叫端推導，哪些型別由程式指定。
  - 泛型程式要把型別需求寫清楚，例如可比較、可移動、可呼叫或符合某個 concept。
  - 模板技巧的價值在減少重複且保留型別安全，不是讓錯誤訊息變得更長。
  - template_parameters 是 template 主題；template 的重點是讓型別或值在編譯期決定，產生對應的具體程式碼。
  - template 定義通常需要放在 header 或使用點可見的位置，否則編譯器無法實例化需要的版本。
  - 錯誤訊息常出現在實例化深處；閱讀時先找第一個 substitution 或 constraint 不成立的位置。
  - type trait、SFINAE、concepts 都是在表達「這個型別必須具備什麼能力」；C++20 後 concepts 通常更清楚。
  - perfect forwarding 需要 T&& 搭配 std::forward<T>，不要把所有 && 都誤認為 move。
  - template 可提升零成本抽象，但也可能造成編譯時間上升和二進位膨脹；共通實作可用非 template helper 收斂。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】Template 參數的三種類型
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 模板參數有哪三種？各自回答什麼問題？
//     答：① 型別參數 typename T ── 我要操作什麼型別；② 非型別參數 NTTP，例如
//         std::size_t N ── 我要把什麼編譯期常數鎖進型別；③ template template
//         parameter ── 我要傳什麼容器策略。std::array<T, N> 同時用了前兩種。
//     追問：三者都能有預設值嗎？（可以；但函式模板的預設模板引數是 C++11 才允許）
//
// 🔥 Q2. 為什麼 NTTP 不能吃 runtime 變數？
//     答：NTTP 是型別 identity 的一部分，必須在編譯期完全確定，否則編譯器無法替
//         Buffer<int,3> 與 Buffer<int,4> 各生成一份不同型別的程式碼。所以
//         int n = 10; std::array<int,n> 不合法，要寫 constexpr int n = 10;。
//         大小若到 runtime 才知道，就該改用 std::vector。
//
// Q3. 把尺寸寫進型別（Matrix<T,R,C>）有什麼好處與代價？
//     答：好處是維度不合變成編譯錯誤而非 runtime 出包，且全 stack 配置、迴圈次數
//         編譯期已知有機會被 unroll。代價是每一組 (R,C) 都生成一份程式碼，尺寸組合
//         多時 code bloat 明顯；且 data_[Rows*Cols] 在 stack 上，開太大會爆 stack。
// ═══════════════════════════════════════════════════════════════════════════

#include <array>
#include <cstddef>
#include <iostream>
#include <vector>

// ─── 1. Type + NTTP 的最經典範例：固定大小 buffer ───────────────────────────
//   類似 std::array<T,N> 的精簡版。
//   - T 是型別參數
//   - N 是 NTTP（std::size_t）
//   N 不同 → 是不同型別。Buffer<int,3> 與 Buffer<int,4> 不能互相 assign。
template <typename T, std::size_t N>
class Buffer {
public:
    constexpr std::size_t size() const { return N; }

    T&       operator[](std::size_t i)       { return data_[i]; }
    const T& operator[](std::size_t i) const { return data_[i]; }

    T*       begin()       { return data_; }
    T*       end()         { return data_ + N; }
    const T* begin() const { return data_; }
    const T* end()   const { return data_ + N; }

private:
    T data_[N]{};   // 編譯期決定大小，stack 上配置；N 大時要小心 stack overflow
};

// ─── 2. NTTP 在演算法上的好處：迴圈次數已知，編譯器可展開 ──────────────────
//   下面這個 sum 編譯器有機會把 loop 完全 unroll，效能等同手寫直線碼。
//   範例：sum_n<int, 4> 會展開成 a[0]+a[1]+a[2]+a[3]。
template <typename T, std::size_t N>
T sum_n(const T (&a)[N]) {
    T s = T{};
    for (std::size_t i = 0; i < N; ++i) s += a[i];
    return s;
}

// ─── 3. Leetcode 1470 ── Shuffle the Array (用 NTTP 固定大小) ──────────────
//   題目：給陣列 nums，長度 2n，回傳 [x1,y1,x2,y2,...,xn,yn]，
//        其中前半是 x、後半是 y 的洗牌結果。
//   範例：nums=[2,5,1,3,4,7], n=3 → [2,3,5,4,1,7]
//
//   為什麼這題用 NTTP？
//     原題 nums 是 std::vector，runtime 才知道大小。但若你有「固定大小
//     2n」的場景 (例如協議封包 header 永遠是 2*N 個欄位)，就可以把 N 寫
//     成 NTTP，避免 runtime 重新配置。
//
//   時間複雜度：O(n)
//   空間複雜度：O(n)（輸出）
//
//   我們做兩個版本：
//     (a) shuffle_v       —— 一般版 (vector + runtime n)
//     (b) shuffle_fixed   —— 固定大小版 (NTTP)；可放在 stack 上
template <typename T>
std::vector<T> shuffle_v(const std::vector<T>& nums, int n) {
    std::vector<T> out;
    out.reserve(nums.size());
    for (int i = 0; i < n; ++i) {
        out.push_back(nums[i]);
        out.push_back(nums[i + n]);
    }
    return out;
}

template <typename T, std::size_t N>      // 半長度 N → 全長 2N
std::array<T, 2 * N> shuffle_fixed(const std::array<T, 2 * N>& a) {
    std::array<T, 2 * N> out{};
    for (std::size_t i = 0; i < N; ++i) {
        out[2 * i]     = a[i];
        out[2 * i + 1] = a[i + N];
    }
    return out;
}

// ─── 4. 工作實用範例：固定大小矩陣 (rows × cols 都是 NTTP) ────────────────
//   日常工作 / 嵌入式 / 圖學常見：3x3 旋轉矩陣、4x4 變換矩陣等。
//   尺寸寫進型別好處：
//     - 形狀錯誤是「編譯錯誤」而不是 runtime 出包
//     - 完全 stack 配置，無動態記憶體
template <typename T, std::size_t Rows, std::size_t Cols>
class Matrix {
public:
    static constexpr std::size_t rows = Rows;
    static constexpr std::size_t cols = Cols;

    T&       at(std::size_t r, std::size_t c)       { return data_[r * Cols + c]; }
    const T& at(std::size_t r, std::size_t c) const { return data_[r * Cols + c]; }

    void fill(const T& v) {
        for (auto& x : data_) x = v;
    }

private:
    T data_[Rows * Cols]{};
};

// 矩陣加法：尺寸不合 = 編譯錯誤。比 runtime 檢查安全多了。
template <typename T, std::size_t R, std::size_t C>
Matrix<T, R, C> add(const Matrix<T, R, C>& a, const Matrix<T, R, C>& b) {
    Matrix<T, R, C> r;
    for (std::size_t i = 0; i < R; ++i)
        for (std::size_t j = 0; j < C; ++j)
            r.at(i, j) = a.at(i, j) + b.at(i, j);
    return r;
}

// ─── 5. Leetcode 832 ── Flipping an Image (用 NTTP 鎖住影像尺寸) ─────────
//   難度: easy
//   題目：給 n×n 二維 0/1 矩陣，做下列兩步：
//        (1) 每一列左右反轉    (2) 每個元素 0↔1 互換
//   範例：[[1,1,0],[1,0,1],[0,0,0]] → [[1,0,0],[0,1,0],[1,1,1]]
//
//   為什麼用 NTTP？
//     原題尺寸 runtime 才知道，但若你有「固定 N×N 的影像 patch」(常見於
//     CNN feature map 處理 3×3 / 5×5 kernel)，把 N 寫成 NTTP 後維度錯誤
//     是編譯錯誤，效能也更好 (固定 loop count 可被 unroll)。
//
//   時間：O(N²)；空間：O(1) (in-place)。
template <std::size_t N>
void flip_image(int (&img)[N][N]) {
    for (std::size_t i = 0; i < N; ++i) {
        for (std::size_t j = 0; j < N / 2; ++j) {
            std::swap(img[i][j], img[i][N - 1 - j]);   // 左右翻
        }
        for (std::size_t j = 0; j < N; ++j) {
            img[i][j] ^= 1;                              // 0/1 反轉
        }
    }
}

// ─── 6. 工作實用：固定容量 stack (NTTP 鎖大小，全在 stack 配置) ──────────
//   嵌入式 / 演算法函式庫常用：避免 heap、保證最大深度。
//   越界就拋 std::out_of_range，比沉默壞掉好。
#include <stdexcept>
template <typename T, std::size_t N>
class FixedStack {
public:
    void push(const T& v) {
        if (top_ == N) throw std::out_of_range("FixedStack overflow");
        data_[top_++] = v;
    }
    T pop() {
        if (top_ == 0) throw std::out_of_range("FixedStack underflow");
        return data_[--top_];
    }
    std::size_t size() const { return top_; }
    constexpr std::size_t capacity() const { return N; }
    bool empty() const { return top_ == 0; }
private:
    T data_[N]{};
    std::size_t top_ = 0;
};

// ─── main ────────────────────────────────────────────────────────────────
int main() {
    // (1) Buffer<int, 4> vs Buffer<int, 5> 是不同型別
    Buffer<int, 4> b4;
    for (std::size_t i = 0; i < b4.size(); ++i) b4[i] = static_cast<int>(i * i);
    std::cout << "b4: ";
    for (int x : b4) std::cout << x << ' ';
    std::cout << "\n";

    // (2) sum_n
    int arr[] = {1, 2, 3, 4};
    std::cout << "sum_n(arr) = " << sum_n(arr) << "\n";

    // (3) Leetcode 1470 (兩個版本)
    std::vector<int> nums{2, 5, 1, 3, 4, 7};
    auto sv = shuffle_v(nums, 3);
    std::cout << "shuffle_v: ";
    for (int x : sv) std::cout << x << ' ';
    std::cout << "\n";

    std::array<int, 6> a6{2, 5, 1, 3, 4, 7};
    auto sf = shuffle_fixed<int, 3>(a6);
    std::cout << "shuffle_fixed: ";
    for (int x : sf) std::cout << x << ' ';
    std::cout << "\n";

    // (4) Matrix
    Matrix<int, 2, 3> m1, m2;
    for (std::size_t i = 0; i < 2; ++i)
        for (std::size_t j = 0; j < 3; ++j) {
            m1.at(i, j) = static_cast<int>(i + j);
            m2.at(i, j) = static_cast<int>(i * j);
        }
    auto m3 = add(m1, m2);
    std::cout << "m1+m2:\n";
    for (std::size_t i = 0; i < 2; ++i) {
        for (std::size_t j = 0; j < 3; ++j) std::cout << m3.at(i, j) << ' ';
        std::cout << "\n";
    }

    // (5) Leetcode 832 Flipping an Image
    int img[3][3] = {{1, 1, 0}, {1, 0, 1}, {0, 0, 0}};
    flip_image(img);
    std::cout << "flipped:\n";
    for (auto& row : img) {
        for (int v : row) std::cout << v << ' ';
        std::cout << "\n";
    }

    // (6) FixedStack
    FixedStack<int, 4> fs;
    fs.push(10); fs.push(20); fs.push(30);
    std::cout << "FixedStack size = " << fs.size()
              << "/" << fs.capacity() << ", pop = " << fs.pop() << "\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：三種 template 參數 (type / NTTP / template-template) 各自的角色？
    //    A：type parameter 答「我要操作什麼型別」(最常見)；NTTP 答
    //       「我要鎖進型別的編譯期常數值」(陣列大小、選項旗標、維度)；
    //       template-template parameter 答「我要傳什麼容器策略」(把
    //       std::vector、std::deque 整個當參數)。三者可以混用，例如
    //       std::array<T, N> 同時用了前兩種。
    //
    //  Q2：為什麼 NTTP 不能用 runtime 變數？
    //    A：NTTP 是型別系統的一部分，必須在編譯期就完全決定，否則編譯
    //       器無法為 Buffer<int,3> 與 Buffer<int,4> 各自生成不同型別的
    //       程式碼。寫 `int n = 10; std::array<int,n> a;` 會錯，必須改
    //       成 `constexpr int n = 10;`。如果大小到 runtime 才知道，請
    //       改用 std::vector 或自製 runtime-sized 容器。
    //
    //  Q3：C++20 對 NTTP 的限制有什麼放寬？
    //    A：C++20 允許「structural type」(具有 public 成員、constexpr
    //       constructor 的 literal class) 當 NTTP，浮點數 (float / double)
    //       也可以了。最有名的應用是「編譯期字串」：透過 fixed_string
    //       這種 literal class 把字串字面量塞進 template 參數，做出
    //       compile-time format string 檢查 (像 fmt::format 的 fmt"...")。
    //
    return 0;
}

// ============================================================================
//  【小結】
//    1. NTTP 把「編譯期值」鎖進型別：尺寸、選項、維度。
//    2. 不同 NTTP 值 → 不同型別，型別系統可以幫你檢查維度是否匹配。
//    3. 可用型別有限制；最常用就是整數、size_t、enum。C++20 起放寬到
//       literal class types。
//
//  【下一篇】
//    05_template_specialization_full.cpp ── 完全特化。
// ============================================================================
