/*
 * std::getline：從 stream 讀到 delimiter，結果放入 std::string
 *
 * 預設 delimiter 是 '\n'，delimiter 被取走但不放入結果。可指定例如 ','。getline
 * 成功/失敗反映在 stream 狀態，正確迴圈是 `while (getline(in,line))`。Windows CRLF
 * 經文字模式通常會轉換；跨來源仍可能留下 '\r'，需明確處理。
 * `operator>>` 後緊接 getline 時，前一個換行可能仍在 stream，造成第一次讀到空行。
 */

#include <cassert>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

void basic_demo() {
    std::istringstream input("first line\nsecond line\n");
    std::string line;
    const bool read_first = static_cast<bool>(std::getline(input, line));
    assert(read_first && line == "first line");
    const bool read_second = static_cast<bool>(std::getline(input, line));
    assert(read_second && line == "second line");
    const bool read_third = static_cast<bool>(std::getline(input, line));
    assert(!read_third);

    std::istringstream csv("a,b,c");
    const bool read_field = static_cast<bool>(std::getline(csv, line, ','));
    assert(read_field && line == "a");
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1816. Truncate Sentence（截斷句子）
// 題目：sentence 由單一空格分隔單字，回傳前 k 個單字；"Hello how are you Contestant"、k=4
//       得 "Hello how are you"。
// 為何使用本章主題：題解用 istringstream 的 operator>> 逐 token，而非 getline；放在本檔是
//       對照完整行輸入與 token 輸入的差異，真正 getline 迴圈在下方設定解析器。
// 思路：1. 以句子建立輸入 stream；2. 最多抽取 k 個 word；3. 非首字加空格；4. 串接結果。
// 複雜度：時間 O(N)、額外空間 O(N)，N 是被讀取與輸出的句子 bytes 上限。
// 易錯點：原題保證 k 合法且單空格；一般 API 應處理 k<=0，且 `>>` 會合併任意連續空白。
// -----------------------------------------------------------------------------
std::string leetcode_truncate_sentence(const std::string& sentence, const int k) {
    std::istringstream input(sentence);
    std::string result;
    std::string word;
    for (int count = 0; count < k && input >> word; ++count) {
        if (!result.empty()) result.push_back(' ');
        result += word;
    }
    return result;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】多行 key=value 設定讀取
// 情境：從記憶體內容逐行讀設定，略過空行與行首 `#`，並保留第一個等號兩側字串。
// 為何使用本章主題：std::getline 保留行內空白與等號，且以讀取結果驅動迴圈；相較 `>>`，
//       不會把一行設定拆成多個 whitespace token。
// 設計：1. getline 逐行；2. 去除可能殘留的 CR；3. 略過空白/註解行；4. 找第一個等號後加入結果。
// 成本：時間 O(T)、額外空間 O(T)，T 是輸入總 bytes，結果擁有 key/value 副本。
// 上線注意：此格式沒有 trim、escaping、重複 key 或行長上限；讀取結束若非 clean EOF 也應回報 I/O 錯誤。
// -----------------------------------------------------------------------------
std::vector<std::pair<std::string, std::string>> practical_parse_config(
    const std::string& content) {
    std::istringstream input(content);
    std::vector<std::pair<std::string, std::string>> entries;
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line.front() == '#') continue;
        const std::size_t equal = line.find('=');
        if (equal != std::string::npos && equal != 0U) {
            entries.emplace_back(line.substr(0U, equal), line.substr(equal + 1U));
        }
    }
    return entries;
}

}  // namespace

int main() {
    basic_demo();
    assert(leetcode_truncate_sentence("Hello how are you Contestant", 4) ==
           "Hello how are you");
    const auto config = practical_parse_config("# demo\r\ntimeout=30\r\nmode=fast\n");
    assert(config.size() == 2U);
    assert(config[0].first == "timeout" && config[0].second == "30");
    assert(config[1].first == "mode" && config[1].second == "fast");
    std::cout << "getline: tests passed\n";
}

/*
 * 【混用修法】`in >> number; getline(in,line);` 前可先 getline 吃掉剩餘行，或用
 * `in.ignore(numeric_limits<streamsize>::max(),'\n')`；不能盲目 ignore 一個字元。
 * 【陷阱】`while (!input.eof())` 會在失敗後多處理一次舊 line；以 getline 本身當條件。
 * 【CSV 限制】指定 ',' 的 getline 不理解 quoted comma，例如 "a,b"；正式用 CSV parser。
 * 【面試】為何不要 `while (!in.eof())`？EOF 只在讀取失敗後設定，會多處理一次舊資料。
 * 【練習】讓 config parser trim key/value，並拒絕重複 key。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'getline.cpp' -o '/tmp/codex_cpp_C_String_getline' && '/tmp/codex_cpp_C_String_getline'
//
// === 預期輸出（節錄）===
// getline: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
