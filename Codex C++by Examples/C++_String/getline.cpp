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

// LeetCode 1816（Truncate Sentence）：以 stream 逐 word 讀取。
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

// 實務：簡單 key=value 多行設定；忽略空行與 # 註解。此非完整 escaping parser。
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
