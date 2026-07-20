/*
 * 第 06 章：std::swap 與 ADL swap
 *
 * swap(a,b) 交換兩個物件狀態。對可移動型別，泛型 std::swap 通常做一次 move construction
 * 加兩次 move assignment；自訂資源型別可提供更有效率且 noexcept 的 member swap/free swap。
 * 泛型程式應先 `using std::swap;` 再呼叫未限定的 `swap(a,b)`，讓 ADL 找到型別專屬版本，
 * 找不到時才 fallback 到 std::swap。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

class Buffer {
public:
    Buffer(std::string name, std::vector<int> data)
        : name_(std::move(name)), data_(std::move(data)) {}

    void swap(Buffer& other) noexcept {
        using std::swap;
        swap(name_, other.name_);
        swap(data_, other.data_);
    }

    const std::string& name() const noexcept { return name_; }
    const std::vector<int>& data() const noexcept { return data_; }

private:
    std::string name_;
    std::vector<int> data_;
};

void swap(Buffer& left, Buffer& right) noexcept {
    left.swap(right); // ADL 可找到此 overload
}

// LeetCode 344：Reverse String，原地 O(n) 時間、O(1) 額外空間。
void leetcode_reverse_string(std::vector<char>& text) {
    if (text.empty()) {
        return;
    }
    std::size_t left = 0;
    std::size_t right = text.size() - 1U;
    while (left < right) {
        std::swap(text[left], text[right]);
        ++left;
        --right;
    }
}

// 實務：先在 staging 建好完整設定，最後以 noexcept swap 原子式替換 active 狀態。
void practical_publish_buffer(Buffer& active, Buffer staging) {
    using std::swap;
    swap(active, staging);
    // staging 離開時會銷毀舊 active 資源；commit 點本身不丟例外。
}

int main() {
    int a = 1;
    int b = 2;
    std::swap(a, b);
    assert(a == 2 && b == 1);

    std::vector<char> text{'h', 'e', 'l', 'l', 'o'};
    leetcode_reverse_string(text);
    assert((text == std::vector<char>{'o', 'l', 'l', 'e', 'h'}));

    Buffer active("v1", {1, 2});
    practical_publish_buffer(active, Buffer{"v2", {7, 8, 9}});
    assert(active.name() == "v2");
    assert(active.data().size() == 3U);

    static_assert(noexcept(swap(active, active)));
    std::cout << "swap 測試完成\n";
}

/*
 * 【常見陷阱】
 * - 在泛型程式直接 `std::swap(a,b)` 會略過 ADL 自訂 overload。
 * - 不要把自訂 overload 任意塞進 namespace std；為自己的型別提供 template specialization
 *   有特殊規則，最穩定的慣例是在型別 namespace 提供 free swap。
 * - self-swap 應保持安全；本例各成員 std::swap 支援。
 * - noexcept swap 對容器提供 strong exception guarantee 很重要。
 *
 * 【面試段落】copy-and-swap idiom：以傳值建立安全副本，再 swap；簡潔但可能犧牲重用配置。
 * 【練習】為 Buffer 實作 copy assignment 的 copy-and-swap 版本並測試 self-assignment。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '06_swap.cpp' -o '/tmp/codex_cpp_C_Utility_06_swap' && '/tmp/codex_cpp_C_Utility_06_swap'
//
// === 預期輸出（節錄）===
// swap 測試完成
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
