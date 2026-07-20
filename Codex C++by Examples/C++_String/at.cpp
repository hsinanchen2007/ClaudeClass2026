/*
 * std::string::at：帶邊界檢查的元素存取
 *
 * 【基本模型】at(index) 將索引檢查與元素存取合成一個操作，不會擴張字串。
 * 【API】非 const 字串回傳 char&，const 字串回傳 const char&，因此可讀也可就地改字元。
 * 【邊界】index >= size() 會丟 std::out_of_range；at(size()) 也不是尾端元素。
 * 【選型】索引來自外部輸入或不易證明範圍時用 at；已由迴圈證明合法時可用 operator[]。
 * 【複雜度】成功與失敗檢查都是 O(1)，差異是 at 多做一次範圍判斷。
 * 【生命週期】回傳 reference 不擁有字元，字串銷毀或後續操作使其失效後便不能使用。
 * 【失效】只改既有字元不改 size/capacity；會重新配置的字串操作則可能使舊 reference 失效。
 * 【例外安全】越界時不會寫入任何字元；呼叫端可把 out_of_range 轉成領域錯誤。
 * 【易錯】負的 signed index 轉成 size_type 後會成為巨大正數，應先在 signed 領域驗證。
 */

#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void basic_demo() {
    std::string text = "cat";
    text.at(0U) = 'C';
    assert(text == "Cat");

    const std::string read_only = text;
    const char& last = read_only.at(read_only.size() - 1U);
    assert(last == 't');

    bool caught = false;
    try {
        // 刻意越界以驗證 at 的例外契約；這不是產品路徑的未檢查索引。
        // cppcheck-suppress containerOutOfBounds
        static_cast<void>(text.at(text.size()));
    } catch (const std::out_of_range&) {
        caught = true;
    }
    assert(caught);
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1678. Goal Parser Interpretation（Goal 指令解析）
// 題目：command 只由 "G"、"()"、"(al)" 串成，分別轉為 "G"、"o"、"al"；
//       例如 "G()(al)" 轉成 "Goal"。
// 為何使用本章主題：at() 在依 token 前進時提供邊界檢查；本版還額外拒絕原題契約外格式，
//       讓錯誤輸入成為 invalid_argument，而不是未檢查索引。
// 思路：1. 從索引 0 掃描；2. 依序辨認 G、()、(al)；3. 附加翻譯並跳過完整 token。
// 複雜度：時間 O(N)、額外空間 O(N)，N 是 command 長度，答案最長與輸入同階。
// 易錯點：呼叫 at(i+1) 前仍須先做剩餘長度檢查；at 的例外不能取代 token 文法驗證。
// -----------------------------------------------------------------------------
std::string leetcode_interpret_goal(const std::string& command) {
    std::string answer;
    for (std::size_t i = 0U; i < command.size();) {
        if (command.at(i) == 'G') {
            answer += 'G';
            ++i;
        } else if (command.at(i) == '(' && command.size() - i >= 2U &&
                   command.at(i + 1U) == ')') {
            answer += 'o';
            i += 2U;
        } else if (command.size() - i >= 4U && command.compare(i, 4U, "(al)") == 0) {
            answer += "al";
            i += 4U;
        } else {
            throw std::invalid_argument("invalid Goal command");
        }
    }
    return answer;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】固定寬度功能旗標更新
// 情境：字串每一格代表一個功能開關，只允許把既有索引設成字元 '0' 或 '1'。
// 為何使用本章主題：at(index) 讓實際寫入仍帶標準邊界檢查；前置 if 則把可預期的外部錯誤
//       轉成 bool，而不是依例外控制一般流程。
// 設計：1. 同時驗證索引與值域；2. 任一不合法便回 false；3. 合法時以 at 原地寫入。
// 成本：時間 O(1)、額外空間 O(1)，不改字串長度也不配置。
// 上線注意：失敗時 flags 必須保持不變；若索引來自 signed 值，要在轉 size_t 前先拒絕負數。
// -----------------------------------------------------------------------------
bool practical_set_flag(std::string& flags, const std::size_t index, const char value) {
    if (index >= flags.size() || (value != '0' && value != '1')) {
        return false;
    }
    flags.at(index) = value;
    return true;
}

}  // namespace

int main() {
    basic_demo();
    try {
        const std::string first = leetcode_interpret_goal("G()(al)");
        const std::string second = leetcode_interpret_goal("G()()()()(al)");
        assert(first == "Goal");
        assert(second == "Gooooal");
    } catch (const std::invalid_argument&) {
        // 以上都是題目保證的合法測資；若抵達這裡，parser 實作有 bug。
        assert(false && "valid Goal command was rejected");
    }

    bool malformed_rejected = false;
    try {
        static_cast<void>(leetcode_interpret_goal("x)"));
    } catch (const std::invalid_argument&) {
        malformed_rejected = true;
    }
    assert(malformed_rejected);

    std::string flags = "0000";
    assert(practical_set_flag(flags, 2U, '1') && flags == "0010");
    assert(!practical_set_flag(flags, 9U, '1'));
    assert(!practical_set_flag(flags, 1U, 'X') && flags == "0010");
    std::cout << "at: tests passed\n";
}

/*
 * 【LeetCode 契約】輸入只由 G、()、(al) 組成；本實作遇其他 token 明確丟 invalid_argument。
 * 【LeetCode 成本】每個 token 只前進一次，時間 O(n)，輸出字串需要 O(n) 額外空間。
 * 【實務契約】practical_set_flag 只接受既有位置與 '0'/'1'，失敗時 flags 保持不變。
 * 【陷阱】catch 例外後繼續使用同一錯誤索引，並沒有修好資料來源或控制流程。
 * 【面試追問】at 與 [] 的複雜度是否不同？兩者都 O(1)，at 額外提供可診斷邊界檢查。
 * 【面試追問】為何不能保存 at 回傳值？保存 reference 可以，但必須同步管理字串生命週期。
 * 【練習】讓 leetcode_interpret_goal 回傳 optional<string>，把格式錯誤變成一般解析失敗。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'at.cpp' -o '/tmp/codex_cpp_C_String_at' && '/tmp/codex_cpp_C_String_at'
//
// === 預期輸出（節錄）===
// at: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
