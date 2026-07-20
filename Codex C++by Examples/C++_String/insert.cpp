/*
 * std::string::insert：在任意位置加入字元序列
 *
 * 支援 index 版與 iterator 版，可插入 string、substring、C 字串、count 個 char 或範圍。
 * 在中間插入需搬移後段，複雜度與插入量及受影響字元數相關。pos > size() 會丟
 * out_of_range。標準允許 basic_string 修改讓既有 iterator/pointer/reference 失效；
 * 不要以「看起來沒有重配」推論舊 handle 還有效，修改後一律重新取得。
 */

#include <cassert>
#include <iostream>
#include <string>

namespace {

void basic_demo() {
    std::string text = "HelloWorld";
    text.insert(5U, " ");
    assert(text == "Hello World");
    text.insert(text.begin(), '[');
    text.insert(text.end(), ']');
    assert(text == "[Hello World]");
}

// LeetCode 1309（Decrypt String from Alphabet to Integer Mapping）：倒序解析並插在前端。
// 前端反覆 insert 是 O(n^2)，此寫法用來教 API；高效版應 push_back 後 reverse。
std::string leetcode_decrypt_mapping(const std::string& encoded) {
    std::string answer;
    std::size_t i = encoded.size();
    while (i > 0U) {
        if (encoded[i - 1U] == '#') {
            assert(i >= 3U);
            const int value = (encoded[i - 3U] - '0') * 10 + (encoded[i - 2U] - '0');
            answer.insert(answer.begin(), static_cast<char>('a' + value - 1));
            i -= 3U;
        } else {
            const int value = encoded[i - 1U] - '0';
            answer.insert(answer.begin(), static_cast<char>('a' + value - 1));
            --i;
        }
    }
    return answer;
}

// 實務：在檔名前插入版本標記，保留最後一個副檔名。
std::string practical_add_version_suffix(std::string filename, const std::string& version) {
    const std::size_t dot = filename.rfind('.');
    const std::size_t position = dot == std::string::npos ? filename.size() : dot;
    filename.insert(position, "-" + version);
    return filename;
}

}  // namespace

int main() {
    basic_demo();
    assert(leetcode_decrypt_mapping("10#11#12") == "jkab");
    assert(leetcode_decrypt_mapping("1326#") == "acz");
    assert(practical_add_version_suffix("report.csv", "v2") == "report-v2.csv");
    assert(practical_add_version_suffix("README", "v2") == "README-v2");
    std::cout << "insert: tests passed\n";
}

/*
 * 【面試】vector/string 中間 insert 為何線性？後方元素必須右移，且可能整體重配。
 * 【陷阱】使用 iterator insert 後又使用舊 iterator，可能已失效。
 * 【練習】把 leetcode_decrypt_mapping 改成 O(n)：倒序 push_back，最後 reverse。
 */

/*
 * 【面試速查】
 * - index overload 的 pos 可等於 size，代表尾插；大於 size 丟 out_of_range。
 * - iterator overload 回指向第一個新元素的 iterator（依 overload 查規格）。
 * - 中段 insert 必須搬移尾端，且可能重配，通常不是 O(1)。
 * - 多次前端 insert 容易 O(n^2)；反向建立再 reverse 通常更好。
 * - 自我來源與目的重疊不要用失效中的 iterator/pointer，先釐清 overload 契約。
 */
