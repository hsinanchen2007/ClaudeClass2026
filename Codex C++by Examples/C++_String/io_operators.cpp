/*
 * string 的 stream operators：operator<< 與 operator>>
 *
 * 【基本模型】operator<< 做格式化輸出，operator>> 做 formatted extraction，兩者都回傳 stream&。
 * 【API】回傳 stream reference 讓 `out << a << b` 與 `in >> a >> b` 可以由左至右串接。
 * 【輸出】string insertion 依 size 寫出全部字元，內嵌 NUL 不會像 C 字串那樣提早停止。
 * 【輸入】預設 skipws 先略過前導空白，再讀到下一個 locale whitespace，因此只得到 token。
 * 【選型】欄位以空白分隔時用 >>；需要保留空格或讀完整紀錄時使用 getline。
 * 【複雜度】處理 n 個字元至少 O(n)，實際成本另受 stream buffer、locale 與裝置 I/O 影響。
 * 【生命週期】stream 與目的 string 都由 caller 持有；operator 不回傳指向暫存 buffer 的 view。
 * 【失效】成功 extraction 會改寫目的 string，先前指向其內容的 pointer/reference 應視為失效。
 * 【例外安全】預設以 failbit/badbit 回報；若 exceptions mask 啟用，相同錯誤可能改為拋例外。
 */

#include <cassert>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>

namespace {

void basic_demo() {
    std::ostringstream output;
    const std::string name = "Ada Lovelace";
    output << "name=" << name;
    assert(output.str() == "name=Ada Lovelace");

    const std::string bytes{"A\0B", 3U};
    std::ostringstream binary_output;
    binary_output << bytes;
    assert(binary_output.str().size() == 3U);
    assert(binary_output.str()[1] == '\0');

    std::istringstream input("  alpha beta");
    std::string first;
    std::string second;
    assert(static_cast<bool>(input >> first >> second));
    assert(first == "alpha" && second == "beta");
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 58. Length of Last Word（最後一個單字的長度）
// 題目：忽略句首、句中與句尾空格後，回傳最後一個單字長度；"fly me to the moon  " 回 4。
// 為何使用本章主題：operator>> 自動略過 whitespace 並逐 token 抽取，最後一次成功的 word
//       就是答案；這是簡潔教學解，會配置 token，索引倒掃通常常數較低。
// 思路：1. 以 sentence 建 istringstream；2. 持續抽取 word；3. 每次覆寫 last；4. 回 last.size。
// 複雜度：時間 O(N)、額外空間 O(K)，N 是句長，K 是目前/最後單字長度。
// 易錯點：不能用 `while(!eof())`；回 int 前一般程式要驗 size，且 locale whitespace 規則可能影響 token。
// -----------------------------------------------------------------------------
int leetcode_length_of_last_word(const std::string& sentence) {
    std::istringstream input(sentence);
    std::string word;
    std::string last;
    while (input >> word) last = word;
    return static_cast<int>(last.size());
}

// -----------------------------------------------------------------------------
// 【日常實務範例】結構化單行 log 解析
// 情境：輸入格式固定為 `LEVEL CODE MESSAGE...`，前兩欄無空白，message 可含空格且不得為空。
// 為何使用本章主題：operator>> 適合 typed token，getline 適合保留剩餘訊息；先解析 candidate
//       再一次指派，較直接寫 destination 能保證錯誤時舊紀錄不變。
// 設計：1. 抽取 level 與 int code；2. `ws+getline` 讀剩餘內容；3. 拒絕空 message；4. move commit。
// 成本：時間 O(N)、額外空間 O(N)，N 是 line 長度，candidate 暫存完整紀錄。
// 上線注意：std::ws 會吃掉所有前導 whitespace；要限制行長、驗 level/code 範圍，並區分
//       malformed、EOF 與底層 I/O 錯誤。
// -----------------------------------------------------------------------------
struct LogRecord {
    std::string level;
    int code{};
    std::string message;
};

bool practical_parse_log(const std::string& line, LogRecord& record) {
    std::istringstream input(line);
    LogRecord candidate;
    if (!(input >> candidate.level >> candidate.code)) return false;
    std::getline(input >> std::ws, candidate.message);
    if (candidate.message.empty()) return false;
    record = std::move(candidate);
    return true;
}

}  // namespace

int main() {
    basic_demo();
    assert(leetcode_length_of_last_word("fly me to the moon  ") == 4);
    LogRecord record;
    const bool parsed = practical_parse_log("ERROR 503 upstream timeout", record);
    assert(parsed);
    assert(record.level == "ERROR" && record.code == 503 && record.message == "upstream timeout");
    const bool rejected = practical_parse_log("BROKEN", record);
    assert(!rejected);
    assert(record.level == "ERROR" && record.code == 503 && record.message == "upstream timeout");
    const bool empty_message_rejected = !practical_parse_log("INFO 200   ", record);
    assert(empty_message_rejected);
    assert(record.level == "ERROR" && record.code == 503 && record.message == "upstream timeout");
    std::cout << "I/O operators: tests passed\n";
}

/*
 * 【overload 細節】width 可限制下一次 string extraction 的字元數，完成後 width 會重設。
 * 【易錯】operator>> 遇空白停止，所以不能直接讀含空白姓名、訊息或一般檔案路徑。
 * 【易錯】混用 >> 與 getline 時會留下分隔換行；std::ws 會連有意義的前導空白一起吃掉。
 * 【狀態】failbit 後的 extraction 不再工作；復原要先 clear，再移除或略過造成失敗的輸入。
 * 【效能】ostringstream 易組合但可能配置；純數字高吞吐路徑可比較 to_chars。
 * 【LeetCode 契約】輸入長度受題目 int 範圍保證；一般函式應回 size_t 避免窄化。
 * 【LeetCode 成本】逐 token 掃描整句，時間 O(n)，額外空間由目前與最後一個單字決定。
 * 【實務契約】格式固定為 LEVEL CODE MESSAGE；message 去除前導空白後不得為空。
 * 【實務例外安全】先解析 candidate，語法失敗不改 record；成功才一次提交完整欄位。
 * 【面試追問】std::ws 做什麼？依 locale 消耗連續 whitespace，適合在 >> 後接 getline。
 * 【練習】為 LogRecord 實作 operator<<，輸出可再次被 practical_parse_log 讀回。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'io_operators.cpp' -o '/tmp/codex_cpp_C_String_io_operators' && '/tmp/codex_cpp_C_String_io_operators'
//
// === 預期輸出（節錄）===
// I/O operators: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
