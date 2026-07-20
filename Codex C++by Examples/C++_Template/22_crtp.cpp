/*
 * 第 22 章：CRTP（Curiously Recurring Template Pattern）
 *
 * Derived 繼承 Base<Derived>，Base 再以 static_cast 呼叫 Derived。
 * 這是「靜態多型」：實際函式在編譯期決定，通常可 inline，不需要 vtable。
 * 代價是每個 Derived 會有不同 Base 實體，不能直接放進同一 Base 容器；若需執行期
 * 異質集合，仍應考慮 virtual 或 type erasure。
 */

#include <array>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <string>
#include <utility>

template <typename Derived>
class Printable {
public:
    std::string text() const {
        return derived().text_impl();
    }

private:
    const Derived& derived() const {
        return static_cast<const Derived&>(*this);
    }
};

struct Point : Printable<Point> {
    int x{};
    int y{};

    std::string text_impl() const {
        return "(" + std::to_string(x) + "," + std::to_string(y) + ")";
    }
};

// LeetCode 303：Range Sum Query。CRTP base 提供一致的 query 驗證外殼，
// Derived 提供資料結構特有的 query_impl。
template <typename Derived>
class RangeQuery {
public:
    int query(std::size_t left, std::size_t right) const {
        assert(left <= right);
        return static_cast<const Derived&>(*this).query_impl(left, right);
    }
};

template <std::size_t N>
class PrefixSum final : public RangeQuery<PrefixSum<N>> {
public:
    explicit constexpr PrefixSum(const std::array<int, N>& values) {
        for (std::size_t i = 0; i < N; ++i) {
            prefix_[i + 1U] = prefix_[i] + values[i];
        }
    }

    constexpr int query_impl(std::size_t left, std::size_t right) const {
        return prefix_[right + 1U] - prefix_[left];
    }

private:
    std::array<int, N + 1U> prefix_{};
};

int leetcode_range_sum(const PrefixSum<6>& sums, std::size_t left, std::size_t right) {
    return sums.query(left, right);
}

template <typename Derived>
class Service {
public:
    std::string execute(std::string input) {
        ++calls_;
        return static_cast<Derived&>(*this).execute_impl(std::move(input));
    }
    std::size_t calls() const noexcept { return calls_; }

private:
    std::size_t calls_{};
};

class UppercaseService : public Service<UppercaseService> {
public:
    std::string execute_impl(std::string input) {
        for (char& ch : input) {
            if (ch >= 'a' && ch <= 'z') {
                ch = static_cast<char>(ch - 'a' + 'A');
            }
        }
        return input;
    }
};

// 【實務情境】服務共用呼叫計數，但每種服務以 CRTP 提供不同核心處理。
void practical_service_test() {
    UppercaseService service;
    assert(service.execute("gpu") == "GPU");
    assert(service.execute("cuda") == "CUDA");
    assert(service.calls() == 2U); // 共通計數邏輯只寫在 base
}

int main() {
    const Point point{.x = 3, .y = 4};
    assert(point.text() == "(3,4)");

    const PrefixSum<6> sums(std::array{-2, 0, 3, -5, 2, -1});
    assert(leetcode_range_sum(sums, 0U, 2U) == 1);
    assert(leetcode_range_sum(sums, 2U, 5U) == -1);

    practical_service_test();
    std::cout << "CRTP 測試完成\n";
}

/*
 * 【陷阱】static_cast<Derived&>(*this) 假設真的是 Derived；不要讓外部直接建構 Base<Wrong>。
 * 【陷阱】Base 的 destructor 若經 Base pointer delete 仍需 virtual；CRTP 不自動解決所有權。
 * 【面試】CRTP 與 virtual 的選擇：封閉型別集合/效能敏感用 CRTP；開放外掛/異質集合用 virtual。
 * 【練習】加一個 CRTP Comparable，自動由 derived().key() 實作 operator== 與 operator<。
 */
