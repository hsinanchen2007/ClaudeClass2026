/*
 * 第 28 章：Capstone - 迷你 STL 容器
 *
 * 本章整合 type parameter、NTTP、iterator、perfect forwarding、concept 與例外契約，
 * 實作固定容量 StaticVector。它不是 std::vector 替代品：本教學版以 std::array<T,N>
 * 預先建構全部 T，故要求 T default-initializable；真正容器會管理未初始化 storage、
 * placement new、destroy 與完整例外安全，複雜得多。
 */

#include <algorithm>
#include <array>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

template <typename T, std::size_t Capacity>
requires std::default_initializable<T> && std::movable<T>
class StaticVector {
public:
    using value_type = T;
    using iterator = typename std::array<T, Capacity>::iterator;
    using const_iterator = typename std::array<T, Capacity>::const_iterator;

    static constexpr std::size_t capacity() noexcept { return Capacity; }
    std::size_t size() const noexcept { return size_; }
    bool empty() const noexcept { return size_ == 0U; }

    template <typename... Args>
    T& emplace_back(Args&&... args) {
        if (size_ == Capacity) {
            throw std::length_error("StaticVector capacity exceeded");
        }
        data_[size_] = T(std::forward<Args>(args)...);
        return data_[size_++];
    }

    void pop_back() {
        if (empty()) {
            throw std::underflow_error("StaticVector::pop_back");
        }
        --size_;
        data_[size_] = T{}; // 教學版主動釋放該元素持有的資源
    }

    T& operator[](std::size_t index) noexcept { return data_[index]; }
    const T& operator[](std::size_t index) const noexcept { return data_[index]; }

    iterator begin() noexcept { return data_.begin(); }
    iterator end() noexcept { return data_.begin() + static_cast<std::ptrdiff_t>(size_); }
    const_iterator begin() const noexcept { return data_.begin(); }
    const_iterator end() const noexcept {
        return data_.begin() + static_cast<std::ptrdiff_t>(size_);
    }

private:
    std::array<T, Capacity> data_{};
    std::size_t size_{};
};

// LeetCode 27：Remove Element。接受任何有 begin/end 且可指定元素的容器。
template <typename Container>
std::size_t leetcode_remove_element(Container& values,
                                    const typename Container::value_type& unwanted) {
    auto new_end = std::remove(values.begin(), values.end(), unwanted);
    return static_cast<std::size_t>(std::distance(values.begin(), new_end));
}

struct LogEntry {
    std::string level;
    std::string message;

    LogEntry() = default;
    LogEntry(std::string log_level, std::string log_message)
        : level(std::move(log_level)), message(std::move(log_message)) {}
};

// 【實務情境】嵌入式/熱路徑以固定容量保存近期 log，避免 runtime 重新配置。
void practical_bounded_log_test() {
    StaticVector<LogEntry, 3> logs;
    logs.emplace_back("INFO", "started");
    logs.emplace_back("WARN", "slow");
    assert(logs.size() == 2U);
    assert(logs[1].message == "slow");

    logs.pop_back();
    assert(logs.size() == 1U);
}

int main() {
    StaticVector<int, 5> values;
    values.emplace_back(3);
    values.emplace_back(2);
    values.emplace_back(2);
    values.emplace_back(3);

    const std::size_t kept = leetcode_remove_element(values, 3);
    assert(kept == 2U);
    assert(values[0] == 2 && values[1] == 2);

    static_assert(StaticVector<int, 5>::capacity() == 5U);
    practical_bounded_log_test();
    std::cout << "mini STL capstone 測試完成\n";
}

/*
 * 【常見陷阱與重要限制】std::remove 不縮小容器，只把保留元素移到前面並回傳 logical end；
 * 本題回傳新長度。若是 std::vector，通常接 erase(new_end,end)。本 StaticVector 若要真正
 * 更新 size_，應提供 erase API，而不是讓外部偷改 private 狀態。
 * 【Iterator 失效】本容器不配置新記憶體；但被覆寫/移除位置的值語意仍改變。
 * 【例外】emplace_back 先指定成功才遞增 size_，若 T 建構/指定丟例外，size_ 維持原值。
 * 【面試】這版為何不是 production-quality？預先建構 N 個 T、無 allocator、無完整 erase/copy 策略。
 * 【練習】用 raw storage + construct_at/destroy_at 改寫，並寫好 rule of five。
 */
