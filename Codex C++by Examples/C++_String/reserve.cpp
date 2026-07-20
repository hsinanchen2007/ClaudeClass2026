/*
 * reserve(n)：要求 capacity 至少為 n，以降低反覆成長的重新配置
 *
 * 【基本模型】reserve 調整儲存容量下限，不改 size、內容，也不建立可存取的新元素。
 * 【API】reserve(n) 沒有回傳值；成功後 capacity() >= n，但實作可以配置得更多。
 * 【overload】舊式無參數 reserve() 在 C++20 已 deprecated；縮容意圖應寫 shrink_to_fit。
 * 【C++20】n 不大於目前 capacity 時不要求縮小；釋放多餘容量只能以 shrink_to_fit 提示。
 * 【選型】已能合理估算大量 append 的最終長度時使用；小字串或未知成長不必預留。
 * 【複雜度】發生重新配置時至多與目前 size 線性相關；沒有重新配置則為常數工作。
 * 【生命週期】capacity 只是 raw storage 能力，合法元素範圍仍然只有 [0,size())。
 * 【失效】若 reserve 造成重新配置，所有舊 iterator/reference/pointer 都失效；否則不失效。
 * 【例外安全】n > max_size 可丟 length_error，配置失敗可丟 bad_alloc，失敗時字串不變。
 * 【易錯】每次 append 前 reserve(size()+1) 可能破壞幾何成長策略，反而退化成 O(n^2)。
 */

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

namespace {

void basic_demo() {
    std::string text = "abc";
    text.reserve(100U);
    assert(text == "abc");
    assert(text.size() == 3U);
    assert(text.capacity() >= 100U);
}

// LeetCode 67（Add Binary）：先預留最大輸入長度 + carry。
std::string leetcode_add_binary(const std::string& a, const std::string& b) {
    std::string reversed;
    const std::size_t max_digits = a.size() > b.size() ? a.size() : b.size();
    reversed.reserve(max_digits + 1U);
    std::size_t i = a.size();
    std::size_t j = b.size();
    int carry = 0;
    while (i > 0U || j > 0U || carry != 0) {
        int total = carry;
        if (i > 0U) total += a[--i] - '0';
        if (j > 0U) total += b[--j] - '0';
        reversed.push_back(static_cast<char>('0' + total % 2));
        carry = total / 2;
    }
    return std::string(reversed.rbegin(), reversed.rend());
}

// 實務：組 CSV 前精確計算可預估部分，避免每欄觸發成長。
std::string practical_join_csv(const std::vector<std::string>& fields) {
    std::size_t total = fields.empty() ? 0U : fields.size() - 1U;
    for (const std::string& field : fields) {
        total += field.size();
    }
    std::string result;
    result.reserve(total);
    for (std::size_t i = 0U; i < fields.size(); ++i) {
        if (i != 0U) result.push_back(',');
        result += fields[i];
    }
    return result;
}

}  // namespace

int main() {
    basic_demo();
    assert(leetcode_add_binary("11", "1") == "100");
    assert(leetcode_add_binary("0", "0") == "0");
    assert(leetcode_add_binary("1111", "1") == "10000");
    assert(practical_join_csv({"id", "name", "state"}) == "id,name,state");
    assert(practical_join_csv({}).empty());
    assert(practical_join_csv({"", ""}) == ",");
    std::cout << "reserve: tests passed\n";
}

/*
 * 【LeetCode 契約】a、b 只含 '0'/'1' 且非空；結果最多 max(a.size,b.size)+1 個字元。
 * 【LeetCode 成本】主迴圈 O(max(n,m))，最後反轉建構再 O(max(n,m))，空間同階。
 * 【實務契約】CSV 範例只負責逗號串接，不處理引號、逗號、換行的 escaping。
 * 【實務容量】先精確加總欄位與分隔符；生產程式還要對 size_t 加法做溢位檢查。
 * 【陷阱】reserve 後寫 text[50] 仍越界，因為 size 沒變；需要元素應呼叫 resize。
 * 【面試追問】reserve 與 resize 差異？前者調容量，後者改元素數量並初始化新增字元。
 * 【面試追問】是否每次都 reserve？只有可估算且追加量夠大時，收益才通常高於額外判斷。
 * 【練習】為 CSV 加上 quote escaping，重新計算預估容量。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'reserve.cpp' -o '/tmp/codex_cpp_C_String_reserve' && '/tmp/codex_cpp_C_String_reserve'
//
// === 預期輸出（節錄）===
// reserve: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
