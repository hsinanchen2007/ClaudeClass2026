// ============================================================================
// vector：連續、可變長度的預設序列容器
// C++20 / 離線教材章節
// ============================================================================
// 【先建立模型】
// std::vector<T> 管理一段連續記憶體，size() 是已建構元素數，capacity() 是目前
// 不重新配置可容納的元素數。兩者不可混為一談。連續性讓 vector 可交給 C API
// （data()）、CPU cache 友善，並支援 O(1) 隨機存取，因此沒有特殊需求時先選它。
//
// 【複雜度】
// - operator[] / at / front / back：O(1)
// - push_back / emplace_back：攤銷 O(1)，觸發擴容的單次操作為 O(n)
// - 中間 insert / erase：O(n)，因後方元素需搬移
// - reserve(n)：若 n > capacity，配置並搬移 O(size)
//
// 【生命週期與 iterator 失效】
// 擴容會讓所有 iterator、reference、pointer 失效；未擴容的 insert/erase 仍會讓
// 操作點及其後方失效。跨修改若要記住元素，優先保存穩定 domain id 或 index，
// 修改後重新取得 iterator；不要把 data() 指標長期保存。
//
// 【API 選擇】
// [] 不檢查邊界；at() 越界丟 std::out_of_range。reserve 改容量但不新增元素；
// resize 改 size 並建構/銷毀元素。emplace_back 直接以參數建構尾端元素。

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

void basic_demo()
{
    std::vector<int> values;
    values.reserve(4);  // 先知道大約數量時可減少重新配置。
    for (int value : {10, 20, 30, 40}) {
        values.push_back(value);
    }

    assert(values.size() == 4U);
    assert(values.capacity() >= values.size());
    assert(values.at(2) == 30);
    assert(std::accumulate(values.begin(), values.end(), 0) == 100);

    // C++20 std::erase_if：一次壓縮並 erase，比迴圈逐筆 erase 更適合 vector。
    const auto removed = std::erase_if(values, [](int value) {
        return value % 20 == 0;
    });
    assert(removed == 2U);
    assert((values == std::vector<int>{10, 30}));
}

// ----------------------------------------------------------------------------
// LeetCode 283：Move Zeroes
// ----------------------------------------------------------------------------
// write 指向下一個非零值的落點。每個元素至多讀寫一次，時間 O(n)、額外空間 O(1)。
// 這種「read/write 雙索引」也是資料清洗、filter-in-place 的常見模式。
void move_zeroes(std::vector<int>& numbers)
{
    std::size_t write = 0;
    for (const int value : numbers) {
        if (value != 0) {
            numbers.at(write) = value;
            ++write;
        }
    }
    std::fill(numbers.begin() + static_cast<std::ptrdiff_t>(write),
              numbers.end(), 0);
}

void leetcode_demo()
{
    std::vector<int> numbers{0, 1, 0, 3, 12};
    move_zeroes(numbers);
    assert((numbers == std::vector<int>{1, 3, 12, 0, 0}));
}

// ----------------------------------------------------------------------------
// 實務：批次 API 的資料分頁
// ----------------------------------------------------------------------------
// 服務一次最多送 batch_size 筆 id。回傳 vector<vector<int>> 是擁有資料的結果；
// 若改用 span view，呼叫端必須保證原 vector 在所有 view 使用期間仍存在且不擴容。
std::vector<std::vector<int>> make_batches(const std::vector<int>& ids,
                                           std::size_t batch_size)
{
    assert(batch_size > 0U);
    std::vector<std::vector<int>> batches;
    batches.reserve((ids.size() + batch_size - 1U) / batch_size);

    for (std::size_t first = 0; first < ids.size(); first += batch_size) {
        const std::size_t last = std::min(first + batch_size, ids.size());
        batches.emplace_back(ids.begin() + static_cast<std::ptrdiff_t>(first),
                             ids.begin() + static_cast<std::ptrdiff_t>(last));
    }
    return batches;
}

void practical_demo()
{
    const auto batches = make_batches({101, 102, 103, 104, 105}, 2U);
    assert(batches.size() == 3U);
    assert((batches.at(0) == std::vector<int>{101, 102}));
    assert((batches.at(2) == std::vector<int>{105}));
}

int main()
{
    basic_demo();
    leetcode_demo();
    practical_demo();
    std::cout << "vector：基礎、LeetCode、批次分頁測試全部通過\n";
}

// 【常見陷阱】
// 1. reserve 後仍不能用 values[i] 寫入尚未建構的元素；要 resize 或 push_back。
// 2. range-for 中 push_back 可能使 range 內部 iterator 失效，形成 UB。
// 3. vector<bool> 是位元壓縮特化，operator[] 不回傳真正 bool&；需要穩定位址時避開。
// 【面試】為何攤銷 O(1) 不代表每次 O(1)？請用幾何擴容解釋總搬移量。
// 【練習】讓 make_batches 回傳 span，並列出呼叫端不可進行的 vector 操作。
