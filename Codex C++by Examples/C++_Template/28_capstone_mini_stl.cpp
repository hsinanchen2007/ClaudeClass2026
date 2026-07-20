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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 27. Remove Element（移除元素）
// 題目：原地移除所有等於 val 的值並回傳新長度；[3,2,2,3]、val=3 得長度 2，前兩格為 2。
// 為何使用本章主題：泛型函式使用 StaticVector 提供的 iterator/value_type 配合 std::remove；
// 這是 mini-STL interoperability 示範，但只計算 logical end，未更新容器 private size_。
// 思路：std::remove 把保留元素移到前段；取得 new_end；計算 begin 到 new_end 的距離並回傳。
// 複雜度：時間 O(N)、額外空間 O(1)，N 是 values 的 logical size。
// 易錯點：回傳 K 後只有前 K 格有效；StaticVector 仍報舊 size，完整容器應提供 erase/resize commit。
// -----------------------------------------------------------------------------
template <typename Container>
std::size_t leetcode_remove_element(Container& values,
                                    const typename Container::value_type& unwanted) {
    auto new_end = std::remove(values.begin(), values.end(), unwanted);
    return static_cast<std::size_t>(std::distance(values.begin(), new_end));
}

// -----------------------------------------------------------------------------
// 【日常實務範例】固定容量近期 Log 緩衝區
// 情境：嵌入式或熱路徑最多保存三筆近期 log，希望避免 vector runtime 擴容。
// 為何使用本章主題：StaticVector<LogEntry,3> 將元素型別與容量放進型別，emplace_back 完美轉送欄位；
// 相較 std::vector 可預知儲存上限，但教學版會預先建構全部三個 LogEntry。
// 設計：原地加入 INFO/WARN；以索引讀近期訊息；pop_back 減 logical size 並重設被移除槽位。
// 成本：push/pop/index O(1)、固定空間 O(C)，C 是 Capacity；物件建立時即建構 C 個元素。
// 上線注意：滿載會丟 length_error，需定義丟棄策略；索引須小於 size，並行讀寫也需同步。
// -----------------------------------------------------------------------------
struct LogEntry {
    std::string level;
    std::string message;

    LogEntry() = default;
    LogEntry(std::string log_level, std::string log_message)
        : level(std::move(log_level)), message(std::move(log_message)) {}
};

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

/*
 * 【教科書補充：logical size 必須由容器本身維護】
 * - remove_element 目前只回傳 logical size，沒有更新 StaticVector::size_；呼叫者後續迭代仍看見舊尾端。
 * - 因此它是 remove algorithm 示範，不是完整 erase；名稱/文件要明示 caller 尚未 commit 新 size。
 * - operator[] 的合法範圍是 index<size()，底層 capacity 尚有槽位不代表其中存在 logical element。
 * - 元素 assignment 拋出時，槽位可能已部分改變；除非 T 提供更強保證，不能宣稱整體 rollback。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '28_capstone_mini_stl.cpp' -o '/tmp/codex_cpp_C_Template_28_capstone_mini_stl' && '/tmp/codex_cpp_C_Template_28_capstone_mini_stl'
//
// === 預期輸出（節錄）===
// mini STL capstone 測試完成
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
