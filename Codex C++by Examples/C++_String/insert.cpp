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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1309. Decrypt String from Alphabet to Integer Mapping（解密字母映射）
// 題目：1..9 對應 a..i，10#..26# 對應 j..z；例如 "10#11#12" 解成 "jkab"。
// 為何使用本章主題：本教學版由右向左解析，再用 insert(begin,char) 把字元放到答案前端；
//       這能展示 API，但反覆搬移前綴為 O(N^2)，高效提交應 push_back 後 reverse。
// 思路：1. 從 encoded 尾端開始；2. 遇 # 讀前兩位，否則讀一位；3. 映射字母並插到開頭。
// 複雜度：時間 O(N^2)、額外空間 O(N)，N 是 encoded/輸出長度，平方成本來自前端插入。
// 易錯點：原題保證編碼合法；一般 parser 在讀 i-3 前要驗長度與數字範圍，不能只靠 assert。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【日常實務範例】輸出檔名加入版本標記
// 情境：將 `report.csv` 轉成 `report-v2.csv`，無副檔名的 README 則變成 README-v2。
// 為何使用本章主題：rfind 定位最後一點後，insert(position, suffix) 可在副檔名前原地加入內容；
//       相較手動拆成多個 substr，步驟較少。
// 設計：1. 找最後一個點；2. 無點就選字串尾；3. 建立 `-version` 並插入該位置。
// 成本：時間 O(N+V)、額外空間 O(N+V)，N、V 是 filename、version 長度；按值 filename 是工作副本。
// 上線注意：hidden file、尾端點、多重副檔名與非法路徑字元要另訂政策；version 也需清理以防路徑注入。
// -----------------------------------------------------------------------------
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

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'insert.cpp' -o '/tmp/codex_cpp_C_String_insert' && '/tmp/codex_cpp_C_String_insert'
//
// === 預期輸出（節錄）===
// insert: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
