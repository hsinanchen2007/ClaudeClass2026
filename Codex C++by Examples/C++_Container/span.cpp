// ============================================================================
// span：不擁有資料的連續序列 view
// ============================================================================
// std::span<T> 只保存 pointer+length（static extent 可能只需 pointer），不配置、不複製
// 元素。它可統一接收 C array、std::array、vector 的連續資料，適合函式參數。
// span<T> 可修改元素；span<const T> 只讀。subspan/first/last 建立 O(1) 子 view。
//
// 【最重要：生命週期】span 不延長底層生命。來源被銷毀、vector 擴容、erase 影響區段
// 後，span 可能懸空。不可回傳指向函式區域陣列的 span，也不可保存由 temporary
// container 建出的 view。span 不表示 null-terminated，不能直接當 C string。

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <numeric>
#include <optional>
#include <span>
#include <stdexcept>
#include <vector>

namespace {

void expect(bool condition, const char* message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

}  // namespace

// accumulate 的初始值決定累加型別。若寫 0，整段會以 int 相加，尚未回傳前就可能
// signed overflow。使用 int64_t 初始值，讓一般 int 序列的總和可超出 int 範圍。
std::int64_t sum(std::span<const int> values)
{
    return std::accumulate(values.begin(), values.end(), std::int64_t{0});
}

void scale(std::span<int> values, int factor)
{
    // 先完整驗證，再修改，避免 signed overflow，也提供 strong guarantee。
    for (const int value : values) {
        const auto scaled = static_cast<std::int64_t>(value) * factor;
        if (scaled < std::numeric_limits<int>::min() ||
            scaled > std::numeric_limits<int>::max()) {
            throw std::overflow_error("scaled value does not fit int");
        }
    }
    for (int& value : values) {
        value *= factor;
    }
}

void basic_demo()
{
    int c_array[]{1, 2, 3};
    std::array<int, 3> fixed{4, 5, 6};
    std::vector<int> dynamic{7, 8, 9};
    expect(sum(c_array) == 6, "C array 總和錯誤");
    expect(sum(fixed) == 15, "array 總和錯誤");
    expect(sum(dynamic) == 24, "vector 總和錯誤");
    expect(sum(std::span<const int>{}) == 0, "空 span 的總和應為零");

    const std::array large_values{std::numeric_limits<int>::max(),
                                  std::numeric_limits<int>::max()};
    const auto expected_large_sum =
        static_cast<std::int64_t>(std::numeric_limits<int>::max()) * 2;
    expect(sum(large_values) == expected_large_sum,
           "累加型別不可在 int 上限溢位");

    scale(std::span<int>{dynamic}.subspan(1), 10);
    expect(dynamic == std::vector<int>{7, 80, 90}, "subspan 修改範圍錯誤");

    std::array<int, 2> overflow_input{1, std::numeric_limits<int>::max()};
    bool overflow_rejected = false;
    try {
        scale(overflow_input, 2);
    } catch (const std::overflow_error&) {
        overflow_rejected = true;
    }
    expect(overflow_rejected && overflow_input == std::array<int, 2>{1, std::numeric_limits<int>::max()},
           "scale 溢位時不得留下半套修改");
}

// ----------------------------------------------------------------------------
// LeetCode 53：Maximum Subarray
// ----------------------------------------------------------------------------
// Kadane：ending_here 是必須以目前元素結尾的最佳和，best 是全域最佳。
// 空輸入沒有合法子陣列，因此回傳 nullopt；非空結果用 int64_t，避免 int 加法溢位。
// O(n) 時間、O(1) 空間；span 讓演算法不限定呼叫端容器型別。
std::optional<std::int64_t> max_subarray(std::span<const int> numbers)
{
    if (numbers.empty()) {
        return std::nullopt;
    }

    std::int64_t ending_here = numbers.front();
    std::int64_t best = ending_here;
    for (const int value : numbers.subspan(1)) {
        const auto wide_value = static_cast<std::int64_t>(value);
        ending_here = std::max(wide_value, ending_here + wide_value);
        best = std::max(best, ending_here);
    }
    return best;
}

void leetcode_demo()
{
    const std::array numbers{-2, 1, -3, 4, -1, 2, 1, -5, 4};
    const auto answer = max_subarray(numbers);
    expect(answer.has_value() && *answer == 6, "Maximum Subarray 答案錯誤");

    expect(!max_subarray(std::span<const int>{}).has_value(),
           "空輸入應回傳 nullopt");

    const std::array large_values{std::numeric_limits<int>::max(),
                                  std::numeric_limits<int>::max()};
    const auto large_answer = max_subarray(large_values);
    const auto expected =
        static_cast<std::int64_t>(std::numeric_limits<int>::max()) * 2;
    expect(large_answer.has_value() && *large_answer == expected,
           "Kadane 的中間加總不可在 int 上限溢位");
}

// ----------------------------------------------------------------------------
// 實務：解析固定格式 network packet
// ----------------------------------------------------------------------------
// 格式：[version][type][payload...][checksum]。最短長度為 3；長度不足或 checksum 錯誤
// 時回傳 nullopt。view 不複製 packet；使用 PacketView 期間，來源 bytes 必須仍有效。
struct PacketView {
    std::uint8_t version;
    std::uint8_t type;
    std::span<const std::uint8_t> payload;
};

std::optional<PacketView> parse_packet(std::span<const std::uint8_t> bytes)
{
    if (bytes.size() < 3U) {
        return std::nullopt;
    }

    std::uint32_t expected{};
    for (const std::uint8_t byte : bytes.first(bytes.size() - 1U)) {
        // 每一步取模，無論封包多長都不會讓 checksum 累加器溢位。
        expected = (expected + static_cast<std::uint32_t>(byte)) % 256U;
    }
    if (expected != static_cast<std::uint32_t>(bytes.back())) {
        return std::nullopt;
    }

    const auto payload = bytes.subspan(2U, bytes.size() - 3U);
    // C++20 span 沒有 at()；runtime 長度檢查已建立 [] 的前置條件。
    return PacketView{bytes[0], bytes[1], payload};
}

void practical_demo()
{
    const std::array<std::uint8_t, 5> bytes{1U, 7U, 10U, 20U, 38U};
    const auto packet = parse_packet(bytes);
    expect(packet.has_value(), "合法封包應解析成功");
    expect(packet->version == 1U && packet->type == 7U, "封包標頭錯誤");
    expect(packet->payload.size() == 2U && packet->payload[1] == 20U,
           "封包 payload 錯誤");

    const std::array<std::uint8_t, 2> too_short{1U, 7U};
    expect(!parse_packet(too_short).has_value(), "短封包必須被拒絕");
    expect(!parse_packet(std::span<const std::uint8_t>{}).has_value(),
           "空封包必須被拒絕");

    const std::array<std::uint8_t, 5> bad_checksum{1U, 7U, 10U, 20U, 39U};
    expect(!parse_packet(bad_checksum).has_value(), "錯誤 checksum 必須被拒絕");

    const std::array<std::uint8_t, 3> header_only{1U, 7U, 8U};
    const auto minimal_packet = parse_packet(header_only);
    expect(minimal_packet.has_value() && minimal_packet->payload.empty(),
           "最短合法封包應得到空 payload");
}

int main()
{
    basic_demo();
    leetcode_demo();
    practical_demo();
    std::cout << "span：零拷貝 view、Kadane 與 packet 解析測試通過\n";
}

// 【陷阱】vector.push_back 觸發擴容後，既有 span 全部懸空。
// 【陷阱】C++20 span 沒有 at() 邊界檢查；在使用 operator[] 前驗證 size。
// 【面試】span 與 vector const& 差異：span 不擁有且可表示子範圍/多種連續來源。
// 【練習】把 optional 擴充成含錯誤碼的結果型別，區分長度與 checksum 錯誤。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'span.cpp' -o '/tmp/codex_cpp_C_Container_span' && '/tmp/codex_cpp_C_Container_span'
//
// === 預期輸出（節錄）===
// span：零拷貝 view、Kadane 與 packet 解析測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
