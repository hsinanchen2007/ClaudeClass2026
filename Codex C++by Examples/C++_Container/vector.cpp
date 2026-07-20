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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 283. Move Zeroes（移動零）
// 題目：原地把所有 0 移到尾端並保留非零相對順序；例如 [0,1,0,3,12] 變 [1,3,12,0,0]。
// 為何使用本章主題：vector 的連續隨機存取適合 read/write 壓縮，能在同一 buffer 內穩定搬移元素。
// 思路：write 指向下一個非零落點；掃描並依序寫入非零值；最後把 [write,end) 填成 0。
// 複雜度：時間 O(N)、額外空間 O(1)，N 為元素數。
// 易錯點：非零相對順序必須保留；write 轉 iterator 差值要可表示；迴圈內不可改變 vector 大小使走訪失效。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【日常實務範例】批次 API 的資料分頁
// 情境：上游提供一串 id，下游 API 每次最多接受 batch_size 筆，需切成擁有資料的多批結果。
// 為何使用本章主題：vector 連續且支援 iterator range 建構；巢狀 vector 讓每批獨立擁有資料，不借用來源。
// 設計：先依筆數預留 batches；每次算 [first,last)；以該範圍建立一批；推進到下一頁。
// 成本：時間 O(N)、結果空間 O(N+B)，N 為 id 數、B 為批次數，另有每批配置成本。
// 上線注意：batch_size 必須大於零且容量公式要防 size_t 溢位；若改 span，來源生命與擴容限制須寫入契約。
// -----------------------------------------------------------------------------
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

/*
 * 【教科書補充：batch 與 vector 失效契約】
 * - batch_size 必須大於零；assert 在 release 消失，正式 API 要避免除零與 size_t 加法溢位。
 * - reallocation 使全部 iterator/reference/pointer（含舊 end）失效；reserve 只有真的重配時才如此。
 * - 未重配的 insert/erase 仍會使操作點及其後方 handle（含 past-the-end）失效。
 * - throwing move-only 元素在重配置失敗時可能無法提供一般期待的 strong guarantee，需查元素型別契約。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'vector.cpp' -o '/tmp/codex_cpp_C_Container_vector' && '/tmp/codex_cpp_C_Container_vector'
//
// === 預期輸出（節錄）===
// vector：基礎、LeetCode、批次分頁測試全部通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
