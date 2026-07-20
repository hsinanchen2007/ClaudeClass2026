/*
 * clear()：移除全部元素，但不承諾釋放容量
 *
 * 呼叫後 size()==0、empty()==true。標準的複雜度至多線性；對 char 的實作常很便宜。
 * capacity 可以保留以利重用，因此 clear 不是「釋放記憶體」API。先前 iterator、
 * reference 與 pointer 不應繼續使用。
 */

#include <cassert>
#include <iostream>
#include <string>

namespace {

void basic_demo() {
    std::string buffer = "temporary";
    const std::size_t old_capacity = buffer.capacity();
    buffer.clear();
    assert(buffer.empty());
    assert(buffer.capacity() >= buffer.size());
    static_cast<void>(old_capacity);  // 是否保留精確容量不是本例正確性條件。
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1047. Remove All Adjacent Duplicates In String（移除相鄰重複字元）
// 題目：反覆刪除相鄰且相同的一對字元，直到不能再刪；例如 "abbaca" 最後得到 "ca"。
// 為何使用本章主題：單次求解不需要 clear，因為 stack 從空字串建立；本例用來對照
//       「建立新工作區」與下方多筆處理時用 clear 重用工作區的差別。
// 思路：1. 字串當 stack；2. 新字元等於 stack 尾端就 pop；3. 否則 push；4. stack 即答案。
// 複雜度：時間 O(N)、額外空間 O(N)，N 是 input 長度。
// 易錯點：讀 stack.back() 前必須先判空；clear 不會安全抹除敏感 bytes，也不是此演算法的刪對操作。
// -----------------------------------------------------------------------------
std::string leetcode_remove_adjacent_duplicates(const std::string& input) {
    std::string stack;
    for (const char ch : input) {
        if (!stack.empty() && stack.back() == ch) {
            stack.pop_back();
        } else {
            stack.push_back(ch);
        }
    }
    return stack;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】逗號欄位工作區重用
// 情境：逐 byte 解析一列逗號資料，每完成一欄便輸出方括號格式並重用同一 field buffer。
// 為何使用本章主題：clear() 讓 field 的 size 歸零且通常可重用容量，相較每欄重新建字串，
//       能降低長列資料中的配置抖動。
// 設計：1. 一般字元累積到 field；2. 遇逗號就提交 `[field]`；3. clear field；4. 迴圈後提交末欄。
// 成本：時間 O(N)，答案與工作空間合計 O(N)；clear 本身不承諾釋放容量。
// 上線注意：本例沒有 CSV quoting/escaping；超大單欄會讓工作區長期保留大 capacity，需配合上限
//       或在批次邊界決定是否縮容。
// -----------------------------------------------------------------------------
std::string practical_normalize_lines(const std::string& input) {
    std::string result;
    std::string field;
    for (const char ch : input) {
        if (ch == ',') {
            result += '[' + field + ']';
            field.clear();
        } else {
            field.push_back(ch);
        }
    }
    result += '[' + field + ']';
    return result;
}

}  // namespace

int main() {
    basic_demo();
    assert(leetcode_remove_adjacent_duplicates("abbaca") == "ca");
    assert(practical_normalize_lines("red,green,blue") == "[red][green][blue]");
    std::cout << "clear: tests passed\n";
}

/*
 * 【面試】clear、erase、assign("") 的語意差異？都可變空，但 clear 最直接；erase 能刪區間。
 * 【陷阱】clear 不保證抹除敏感資料的底層 bytes，不能當安全清除密碼機制。
 * 【練習】讓 practical_normalize_lines 支援空欄位，測試 "a,,b"。
 */

/*
 * 【考前速查】
 * - clear 後 empty 為 true、size 為 0；capacity 是否改變不可當契約。
 * - 想重用 builder 時 clear 合適；想降低長期記憶體才考慮 shrink_to_fit。
 * - clear 使元素不存在，任何舊 reference/iterator 都不可再用。
 * - `s = {}`、`s = ""` 也能變空，但 clear 最直接表達移除全部元素。
 * 【面試題】敏感字串如何可靠抹除？標準 string 不提供安全擦除保證。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'clear.cpp' -o '/tmp/codex_cpp_C_String_clear' && '/tmp/codex_cpp_C_String_clear'
//
// === 預期輸出（節錄）===
// clear: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
