// ============================================================================
// 課題 13：Operator overloading（運算子重載）
// ============================================================================
//
// 自訂型別可重載既有 operator，讓值型別以自然語法組合。應保留原運算子直覺：
// `a + b` 不應偷偷修改 a；`==` 應具反身/對稱/傳遞性；`<` 應形成 strict weak order。
// 不能發明新 operator、改變 precedence/arity，也不能重載 `.`, `.*`, `::`, `?:`。
//
// 對稱 binary operator 常寫 non-member（必要時 friend），讓左右 operand 都可轉換。
// `operator+=` 修改自己並回 T&；`operator+` 可複製 left，再呼叫 +=，避免邏輯重複。
//
// 【面試】prefix ++ 回 T&；postfix ++ 用 dummy int 區分，回修改前的 value。
// 【陷阱】比較器若受可變外部 state 影響，放進 set/map 後會破壞容器 invariant。
// ============================================================================

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <ostream>
#include <queue>
#include <string>
#include <utility>
#include <vector>

class Money {
public:
    explicit Money(std::int64_t cents) : cents_(cents) {}

    Money& operator+=(const Money& other)
    {
        cents_ += other.cents_;
        return *this;
    }
    friend Money operator+(Money left, const Money& right)
    {
        left += right;
        return left;
    }
    friend bool operator==(const Money& left, const Money& right)
    {
        return left.cents_ == right.cents_;
    }
    friend std::ostream& operator<<(std::ostream& output, const Money& money)
    {
        return output << money.cents_ << " cents";
    }

private:
    std::int64_t cents_;
};

void basic_example()
{
    const Money subtotal(1'000);
    const Money tax(80);
    const Money total = subtotal + tax;
    assert(total == Money(1'080));
    std::cout << "[基礎] total=" << total << '\n';
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 973. K Closest Points to Origin（最接近原點的 K 個點）
// 題目：回傳距原點最近的 k 個點；例如 (1,3)、(-2,2) 且 k=1 時回 (-2,2)。
// 為何使用本章主題：Point::operator< 以距離平方定義 heap 次序，使 priority_queue top 成為目前最遠候選。
// 思路：每點 push 進 max-heap；大小超過 k 就 pop 最遠點；最後取出剩餘 k 點。
// 複雜度：時間 O(N log K)、額外空間 O(K)，N 為點數、K 為回傳數。
// 易錯點：k 必須不大於 N；不需開根號；同距離順序不限；座標超出題目限制時兩平方相加仍可能溢位。
// -----------------------------------------------------------------------------
class Point {
public:
    Point(int x, int y) : x_(x), y_(y) {}
    long long distance_squared() const
    {
        const long long x = x_;
        const long long y = y_;
        return x * x + y * y;
    }
    bool operator<(const Point& other) const
    {
        return distance_squared() < other.distance_squared();
    }
    int x() const { return x_; }
    int y() const { return y_; }

private:
    int x_;
    int y_;
};

std::vector<Point> k_closest(const std::vector<Point>& points, std::size_t k)
{
    std::priority_queue<Point> candidates;
    for (const Point& point : points) {
        candidates.push(point);
        if (candidates.size() > k) {
            candidates.pop();
        }
    }
    std::vector<Point> result;
    while (!candidates.empty()) {
        result.push_back(candidates.top());
        candidates.pop();
    }
    return result;
}

void leetcode_973_example()
{
    const auto result = k_closest({Point(1, 3), Point(-2, 2)}, 1U);
    assert(result.size() == 1U && result.front().x() == -2 && result.front().y() == 2);
    std::cout << "[LeetCode 973] closest=(-2,2)\n";
}

// -----------------------------------------------------------------------------
// 【日常實務範例】語意版本排序
// 情境：部署工具要把 2.0.0、1.10.2、1.9.9 依 major/minor/patch 排成正確升冪。
// 為何使用本章主題：operator< 將版本的自然次序放進 value type，std::sort 可直接使用且避免字串字典序錯誤。
// 設計：先比較 major；相同再比 minor；最後比較 patch；sort 依 strict weak ordering 排列。
// 成本：每次比較 O(1)，排序 O(N log N)，額外空間依 sort 實作通常 O(log N)。
// 上線注意：欄位不得為負；完整 SemVer 還有 prerelease/build metadata，簡化 comparator 不可直接用於完整規格。
// -----------------------------------------------------------------------------
struct SemanticVersion {
    int major;
    int minor;
    int patch;

    bool operator<(const SemanticVersion& other) const
    {
        if (major != other.major) return major < other.major;
        if (minor != other.minor) return minor < other.minor;
        return patch < other.patch;
    }
};

void practical_example()
{
    std::vector<SemanticVersion> versions{{2, 0, 0}, {1, 10, 2}, {1, 9, 9}};
    std::sort(versions.begin(), versions.end());
    assert(versions.front().major == 1 && versions.front().minor == 9);
    assert(versions.back().major == 2);
    std::cout << "[實務] semantic versions 已依欄位排序\n";
}

int main()
{
    basic_example();
    leetcode_973_example();
    practical_example();
}

// 練習：為 Money 加 -=/-，並思考 overflow 與不同 currency 應如何建模。
// 複雜度：operator 語法不保證便宜；Money arithmetic O(1)，container-like + 可能是 O(N)。
// 生命週期：回傳 value 最安全；回傳 reference 必須指向仍存活 object，prefix/postfix 契約也不同。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '13_OperatorOverloading.cpp' -o '/tmp/codex_cpp_C_OOP_13_OperatorOverloading' && '/tmp/codex_cpp_C_OOP_13_OperatorOverloading'
//
// === 預期輸出（節錄）===
// [實務] semantic versions 已依欄位排序
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
