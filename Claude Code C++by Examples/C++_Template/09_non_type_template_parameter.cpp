// ============================================================================
//  09_non_type_template_parameter.cpp  ──  Non-type Template Parameter (NTTP)
// ============================================================================
//
//  【本篇目標】
//    File 04 已經初步介紹過 NTTP，本篇深入：
//      - 哪些型別可以當 NTTP？(C++17、C++20 放寬了限制)
//      - NTTP 與「value_type」的差異：NTTP 是型別系統的一部分
//      - 編譯期計算的好處
//      - C++17 新功能：auto NTTP
//      - 實戰：固定大小資料結構、編譯期 transpose 矩陣
//
// ----------------------------------------------------------------------------
//  【可以當 NTTP 的型別 (依 C++ 版本)】
//
//    C++98/03：
//      - 整數型別 (int, char, bool, short, long, std::size_t, ...)
//      - 列舉 (enum / enum class)
//      - 指標 / 參考 (對外可見有 linkage 的物件)
//
//    C++11：
//      - 跟 C++98/03 一樣，但 enum class 普及
//      - constexpr 表達式可以做 NTTP 引數
//
//    C++17：
//      - auto NTTP：template <auto V> 讓編譯器推 V 的型別
//      - std::nullptr_t 可以當 NTTP
//
//    C++20：
//      - Literal class types (有 constexpr constructor、structural type)
//      - 浮點型 (float / double)
//      - 字串 (透過 fixed_string trick) ← 需要自己定義 literal class
//
//  參考：https://en.cppreference.com/cpp/language/template_parameters
//
// ----------------------------------------------------------------------------
//  【NTTP 是型別系統的一部分】
//    這點極重要：Buffer<int, 4> 與 Buffer<int, 5> 是「不同型別」。
//    這帶來：
//      ◆ 維度不對直接編譯錯誤 (而非 runtime 錯誤)
//      ◆ 兩個不同 N 的物件無法互相 assign / compare
//      ◆ 函式可以對 N 做 SFINAE / concept 限制
//
//  【NTTP 的編譯期優勢】
//    - 迴圈次數已知 → 編譯器有機會 unroll
//    - 陣列大小已知 → 純 stack 配置，無動態記憶體
//    - constexpr 函式可以對 NTTP 做編譯期計算
//
// ----------------------------------------------------------------------------
//  【auto NTTP (C++17)】
//
//        template <auto V>
//        struct Constant {
//            static constexpr decltype(V) value = V;
//        };
//
//        Constant<42>      a;   // V 推為 int
//        Constant<'a'>     b;   // V 推為 char
//        Constant<3.14>    c;   // C++20 起合法
//
//    讓你寫一份模板處理多種 NTTP 型別。
// ============================================================================

/*
補充筆記：non_type_template_parameter
  - non_type_template_parameter 涉及模板實例化；請先判斷哪些型別由呼叫端推導，哪些型別由程式指定。
  - 泛型程式要把型別需求寫清楚，例如可比較、可移動、可呼叫或符合某個 concept。
  - 模板技巧的價值在減少重複且保留型別安全，不是讓錯誤訊息變得更長。
  - non_type_template_parameter 是 template 主題；template 的重點是讓型別或值在編譯期決定，產生對應的具體程式碼。
  - template 定義通常需要放在 header 或使用點可見的位置，否則編譯器無法實例化需要的版本。
  - 錯誤訊息常出現在實例化深處；閱讀時先找第一個 substitution 或 constraint 不成立的位置。
  - type trait、SFINAE、concepts 都是在表達「這個型別必須具備什麼能力」；C++20 後 concepts 通常更清楚。
  - perfect forwarding 需要 T&& 搭配 std::forward<T>，不要把所有 && 都誤認為 move。
  - template 可提升零成本抽象，但也可能造成編譯時間上升和二進位膨脹；共通實作可用非 template helper 收斂。
*/
#include <array>
#include <cstddef>
#include <iostream>

// ─── 1. 編譯期 power：N 是 NTTP，編譯器可以完全展開 ─────────────────────
//   pow_n<3>(2) 在編譯期就被展開成 2*2*2 = 8。
template <std::size_t N>
constexpr long long pow_n(long long base) {
    long long r = 1;
    for (std::size_t i = 0; i < N; ++i) r *= base;
    return r;
}

// ─── 2. C++17 auto NTTP ──────────────────────────────────────────────────
//   一個通用「常數型別」的盒子，無視 V 是 int、char、enum 還是其他。
template <auto V>
struct Constant {
    static constexpr decltype(V) value = V;
};

// ─── 3. AlignedBuffer：用 NTTP 控制對齊 ───────────────────────────────────
//   alignas 接受編譯期常數。
//   應用：硬體要求對齊存取 (e.g. SIMD vector 必須 16 / 32 byte 對齊)。
//   alignas 的 alignment 必須是 2 的次方 (這裡為了簡潔不檢查)。
template <std::size_t Align, std::size_t N>
struct AlignedBuffer {
    alignas(Align) unsigned char data[N];
    constexpr std::size_t size()      const { return N; }
    constexpr std::size_t alignment() const { return Align; }
};

// ─── 4. Leetcode 867 ── Transpose Matrix (固定維度版) ─────────────────────
//   題目：給一個 m×n 矩陣，回傳轉置 (n×m)。
//   範例：[[1,2,3],[4,5,6]] → [[1,4],[2,5],[3,6]]
//
//   為什麼用 NTTP？
//     原題 vector<vector<int>> 是動態的。如果你寫純粹「靜態維度」的版本：
//        Matrix<int, R, C>::transpose() 回傳 Matrix<int, C, R>
//     維度錯誤直接編譯錯誤；無動態配置；可能被編譯器完全展開。
//     工作上：影像處理固定卷積 kernel、3D 變換矩陣 (3x3 / 4x4) 都是這個模式。
//
//   時間：O(R·C)
//   空間：O(R·C)（輸出）
//
//   邊界：R 或 C 為 0 時也能 compile，因為迴圈本身就跳過了。
template <typename T, std::size_t R, std::size_t C>
struct Matrix {
    T data[R][C]{};

    Matrix<T, C, R> transpose() const {
        Matrix<T, C, R> out;
        for (std::size_t i = 0; i < R; ++i)
            for (std::size_t j = 0; j < C; ++j)
                out.data[j][i] = data[i][j];
        return out;
    }
};

// ─── 5. 工作實用範例：編譯期固定環形緩衝 (RingBuffer) ─────────────────────
//   嵌入式 / 高性能場景常見：固定大小、無動態配置、index 用 mod 計算。
//   N 為 NTTP，編譯期決定大小。
template <typename T, std::size_t N>
class RingBuffer {
public:
    static_assert(N > 0, "RingBuffer size must be > 0");

    void push(const T& v) {
        data_[head_] = v;
        head_ = (head_ + 1) % N;
        if (size_ < N) ++size_;
        else           tail_ = (tail_ + 1) % N;   // 滿了 → 覆蓋最舊
    }

    bool pop(T& out) {
        if (size_ == 0) return false;
        out = data_[tail_];
        tail_ = (tail_ + 1) % N;
        --size_;
        return true;
    }

    std::size_t size()     const { return size_; }
    constexpr std::size_t capacity() const { return N; }

private:
    T data_[N]{};
    std::size_t head_{0};
    std::size_t tail_{0};
    std::size_t size_{0};
};

// ─── 6. Leetcode 1572 ── Matrix Diagonal Sum (固定維度 NTTP) ────────────
//   難度: easy
//   題目：給 n×n 矩陣，求「兩條對角線」的總和 (中心格只算一次)。
//   範例：[[1,2,3],[4,5,6],[7,8,9]] → 主對角 1+5+9=15、副對角 3+5+7=15，
//        中心 5 重複扣一次 → 15+15-5 = 25
//
//   為什麼用 NTTP？
//     工作中常需要對「固定 NxN 影像 patch / kernel」做運算。把 N 寫成 NTTP
//     讓編譯期固定尺寸、loop unroll 可能性更高。
//
//   時間：O(N)；空間：O(1)。
template <typename T, std::size_t N>
T diagonal_sum(const T (&m)[N][N]) {
    T s = T{};
    for (std::size_t i = 0; i < N; ++i) {
        s += m[i][i];                  // 主對角
        s += m[i][N - 1 - i];          // 副對角
    }
    if (N % 2 == 1) s -= m[N / 2][N / 2];   // 中心扣一次
    return s;
}

// ─── 7. 工作實用：FixedBitSet<N>，N 為 NTTP 鎖住位元數 ──────────────────
//   類似 std::bitset 的精簡版。常用於旗標管理 / 狀態壓縮。
//   N 不同 → 是不同型別，無法互相 assign。
template <std::size_t N>
class FixedBitSet {
public:
    static_assert(N > 0, "FixedBitSet N must be > 0");
    void set(std::size_t i)   { if (i < N) bits_[i / 8] |=  (1u << (i % 8)); }
    void clear(std::size_t i) { if (i < N) bits_[i / 8] &= ~(1u << (i % 8)); }
    bool test(std::size_t i)  const {
        return i < N && (bits_[i / 8] & (1u << (i % 8))) != 0;
    }
    constexpr std::size_t size() const { return N; }
private:
    unsigned char bits_[(N + 7) / 8]{};
};

// ─── main ────────────────────────────────────────────────────────────────
int main() {
    // (1) 編譯期 pow
    constexpr auto p3 = pow_n<3>(2);    // 8
    constexpr auto p10 = pow_n<10>(2);  // 1024
    std::cout << "2^3  = " << p3  << "\n";
    std::cout << "2^10 = " << p10 << "\n";

    // (2) auto NTTP
    std::cout << "Constant<42>::value      = " << Constant<42>::value      << "\n";
    std::cout << "Constant<'a'>::value     = " << Constant<'a'>::value     << "\n";

    // (3) AlignedBuffer
    AlignedBuffer<16, 64> buf;
    std::cout << "AlignedBuffer alignment = " << buf.alignment()
              << ", size = " << buf.size() << "\n";

    // (4) Leetcode 867
    Matrix<int, 2, 3> m;
    int v = 1;
    for (std::size_t i = 0; i < 2; ++i)
        for (std::size_t j = 0; j < 3; ++j) m.data[i][j] = v++;
    auto t = m.transpose();         // Matrix<int, 3, 2>
    std::cout << "transpose:\n";
    for (std::size_t i = 0; i < 3; ++i) {
        for (std::size_t j = 0; j < 2; ++j) std::cout << t.data[i][j] << ' ';
        std::cout << "\n";
    }

    // (5) RingBuffer
    RingBuffer<int, 3> rb;
    rb.push(1); rb.push(2); rb.push(3);
    rb.push(4);             // 覆蓋掉 1
    int x;
    while (rb.pop(x)) std::cout << x << ' ';
    std::cout << "\n";       // expect 2 3 4

    // (6) Leetcode 1572 Matrix Diagonal Sum
    int mm[3][3] = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
    std::cout << "diagonal_sum(3x3) = " << diagonal_sum(mm) << " (expect 25)\n";

    // (7) FixedBitSet
    FixedBitSet<16> bs;
    bs.set(0); bs.set(3); bs.set(7); bs.set(15);
    std::cout << "FixedBitSet bits: ";
    for (std::size_t i = 0; i < bs.size(); ++i)
        std::cout << (bs.test(i) ? '1' : '0');
    std::cout << "\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：哪些型別「不能」當 NTTP？
    //    A：C++17 規則：不能用 std::string、std::vector 等動態型別、
    //       浮點數 (float/double)、非 literal 的 class type。原因是 NTTP
    //       要在編譯期完全確定且能比較，動態記憶體管理跟非 trivial 的
    //       建構解構都做不到。C++20 放寬：浮點數 OK、structural class
    //       type (具備 constexpr ctor、所有成員 public 且為 structural)
    //       也 OK，於是出現「fixed_string 字串字面量當 NTTP」這種神技。
    //
    //  Q2：為什麼 Buffer<int,3> 與 Buffer<int,4> 是「不同型別」？這有什麼好處？
    //    A：因為 NTTP 跟 type parameter 一樣是型別 identity 的一部分，
    //       N 不同就是不同實例化結果。好處是維度錯誤變成「編譯錯誤」而
    //       非 runtime 例外，例如要把 Vec3 加到 Vec4 上會直接拒絕；矩
    //       陣維度不合也是 compile-time 擋掉。代價是有時 N 變化太多會
    //       導致 code bloat (每個 N 都產生一份程式碼)。
    //
    //  Q3：C++17 的 `template <auto V>` 能解決什麼問題？
    //    A：在它之前，要寫一個能接 int / char / enum 的常數包裝必須拆成
    //       多份 (template<int>、template<char>...) 或寫 macro。`auto V`
    //       讓編譯器自己推 V 的型別，一份模板搞定。常見用法是
    //       `Constant<42>`、`std::integral_constant<auto>` 風格的 trait，
    //       還有寫 generic compile-time switch (例如 `Lookup<EnumX::Foo>`)
    //       不必先重複寫一遍 enum type。
    //
    return 0;
}

// ============================================================================
//  【小結】
//    1. NTTP 把「值」鎖進型別 → 編譯期決定，零 runtime cost。
//    2. 不同 N 是不同型別，型別系統幫你檢查維度。
//    3. C++17 起有 auto NTTP；C++20 起允許 literal class type、float。
//    4. 適合場景：固定大小資料結構、固定維度矩陣、對齊控制、編譯期常數庫。
//
//  【下一篇】
//    10_template_template_parameter.cpp ── Template template parameter。
// ============================================================================
