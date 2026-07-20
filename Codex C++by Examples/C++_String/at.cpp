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

// LeetCode 1678（Goal Parser Interpretation）的簡化解析；at 讓格式錯誤可被捕捉。
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

// 實務：更新固定寬度狀態欄；先做領域驗證，例外只當最後防線。
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
