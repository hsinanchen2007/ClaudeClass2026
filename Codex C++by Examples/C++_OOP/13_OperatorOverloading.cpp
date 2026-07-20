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

// LeetCode 973：K Closest Points to Origin。
// Point::operator< 依 squared distance 排序；priority_queue top 是目前最遠者，
// 超過 k 就 pop，最後留下 k 個最近點。
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

// 實務案例：SemanticVersion 的 operator< 讓 std::sort 直接依版本欄位排序。
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
