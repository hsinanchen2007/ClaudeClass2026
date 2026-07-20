/*
 * 第 12 章：變數模板（variable template，C++14）
 *
 * 變數模板為每組模板引數產生一個變數。標準庫的 is_integral_v<T> 就是
 * is_integral<T>::value 的便利形式。常搭配 inline constexpr，避免 header 多重定義問題。
 */

/*
 * 【離線教材：模型與取捨】
 * 1. variable template 描述一族變數；pi_v<float> 與 pi_v<double> 是兩個不同 specialization。
 * 2. 語法 `template<class T> inline constexpr T name = expression;` 適合 header-only 常數政策。
 * 3. `_v` 慣例表示布林或數值常數；標準庫 is_integral_v 正是 trait::value 的捷徑。
 * 4. 能直接使用標準常數時優先 std::numbers::pi_v<T>，自訂模板留給領域政策或舊標準相容。
 * 5. `inline` 解決跨 translation unit 的 ODR 定義問題；它不代表編譯器必定 inline 存取。
 * 6. constexpr 值常被 constant-fold 而不配置儲存；取位址或綁 reference 時才需要實體物件。
 * 7. 若實體物件存在，它是 namespace scope 的 static storage duration，不涉及區域變數懸空。
 * 8. Variable template 本身查值是 O(1)；circle_area 也是固定次數乘法，額外空間 O(1)。
 * 9. 每個用到的 T 都要實體化初始化式；巨大的初始化或過多型別會增加 compile time。
 * 10. 常數通常不增加機器碼，但 odr-use 很多大型物件時仍可能增加資料段與 binary size。
 * 11. pi_v<int> 會把圓周率截成 3，所以 circle_area 明確拒絕非浮點型別，而非默默失真。
 * 12. 半徑的領域契約仍由呼叫端負責；負半徑雖可算出正值，物理語意通常應先拒絕。
 * 13. 完美平方採整數二分搜尋，時間 O(log value)、空間 O(1)，不受 sqrt 浮點誤差影響。
 * 14. 比較 `mid > value/mid` 先避開 mid*mid overflow；只有確認安全後才真的相乘。
 * 15. 有號負數先回 false；無號型別不編譯無意義的 value<0 分支，避免診斷雜訊。
 * 16. LeetCode 契約允許 0 與正整數；bool 雖屬 integral，卻不適合作為數值輸入而被拒絕。
 * 17. 感測器 tolerance 是絕對誤差政策，只適用同單位且量級已知的資料，不是通用浮點比較。
 * 18. Float 特化展示「同一演算法、依型別換常數」；variable template 可完整及偏特化。
 * 19. 診斷時若看到某型別初始化失敗，要找首次 odr-use；未被使用的 specialization 不必生成。
 * 20. 面試追問：inline constexpr 各解決什麼？前者管 ODR，後者提供常數表達式與 const。
 * 21. 面試追問：何時改成函式？常數需要 runtime 設定、驗證或依物件狀態計算時。
 */

#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <type_traits>
#include <vector>

template <typename T>
inline constexpr T pi_v = static_cast<T>(3.14159265358979323846L);

template <typename T>
inline constexpr bool is_numeric_v = std::is_arithmetic_v<T> && !std::is_same_v<T, bool>;

template <typename T>
T circle_area(T radius) {
    static_assert(std::is_floating_point_v<T>, "radius 必須是浮點型別");
    return pi_v<T> * radius * radius;
}

// LeetCode 367：Valid Perfect Square，不使用浮點 sqrt 避免精度問題。
template <typename Integer>
bool leetcode_is_perfect_square(Integer value) {
    static_assert(std::is_integral_v<Integer> && !std::is_same_v<Integer, bool>);
    if constexpr (std::is_signed_v<Integer>) {
        if (value < 0) {
            return false;
        }
    }
    Integer left = 0;
    Integer right = value;
    while (left <= right) {
        const Integer mid = left + (right - left) / 2;
        if (mid != 0 && mid > value / mid) {
            right = mid - 1;
        } else {
            const Integer square = mid * mid;
            if (square == value) {
                return true;
            }
            left = mid + 1;
        }
    }
    return false;
}

// 實務：不同感測器精度使用不同 epsilon；特化變數而非複製演算法。
template <typename T>
inline constexpr T sensor_tolerance_v = static_cast<T>(0.001);

template <>
inline constexpr float sensor_tolerance_v<float> = 0.01F;

template <typename T>
bool practical_approximately_equal(T left, T right) {
    static_assert(std::is_floating_point_v<T>, "感測器比較只接受浮點型別");
    return std::abs(left - right) <= sensor_tolerance_v<T>;
}

int main() {
    assert(std::abs(circle_area(2.0) - 12.566370614359172) < 1e-12);
    assert(circle_area(0.0F) == 0.0F);
    static_assert(is_numeric_v<int>);
    static_assert(!is_numeric_v<bool>);
    static_assert(std::is_same_v<decltype(pi_v<float>), const float>);

    assert(leetcode_is_perfect_square(16));
    assert(!leetcode_is_perfect_square(14));
    assert(leetcode_is_perfect_square(0));
    assert(!leetcode_is_perfect_square(-1));
    assert(leetcode_is_perfect_square(2'147'395'600));
    assert(!leetcode_is_perfect_square(std::numeric_limits<unsigned int>::max()));

    assert(practical_approximately_equal(10.0F, 10.005F));
    assert(!practical_approximately_equal(10.0, 10.005));

    std::cout << "變數模板測試完成\n";
}

/*
 * 【陷阱】namespace-scope 變數放 header 時要理解 inline/ODR；C++17 起 inline variable 最直接。
 * 【陷阱】浮點 epsilon 應依量級與領域設計，本例只示範型別化政策，不是通用比較公式。
 * 【面試】variable template 可否特化？可以，完整與偏特化皆可。
 * 【練習】建立 bytes_per_element_v<T>，再估算 vector<T> 的資料區大小。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '12_variable_template.cpp' -o '/tmp/codex_cpp_C_Template_12_variable_template' && '/tmp/codex_cpp_C_Template_12_variable_template'
//
// === 預期輸出（節錄）===
// 變數模板測試完成
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
