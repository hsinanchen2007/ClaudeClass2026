/*
 * empty()：以意圖清楚且 O(1) 的方式判斷字串是否沒有元素
 *
 * 【基本模型】empty 只回答 size 是否為零，不掃描內容，也不判斷文字是否有業務意義。
 * 【API】成員函式簽名概念上是 bool empty() const noexcept；const 與非 const 物件都能呼叫。
 * 【選型】已知型別是 string 時用 member empty；泛型 range 可考慮 std::ranges::empty。
 * 【複雜度】對 basic_string 固定為 O(1)，不是從頭尋找第一個非 NUL 字元。
 * 【生命週期】回傳 bool 是獨立值，不借用 buffer；呼叫本身不造成 iterator/reference 失效。
 * 【例外安全】empty 不配置、不修改且 noexcept，適合放在 front/back/pop_back 的 guard。
 * 【易錯】"   " 的 size 不為零，所以不是 empty；空白驗證是另一份領域規則。
 * 【易錯】empty 不代表「未初始化」；一個正常建構的 string 永遠是有效物件。
 */

#include <cassert>
#include <iostream>
#include <string>

namespace {

void basic_demo() {
    const std::string none;
    const std::string spaces = "   ";
    assert(none.empty());
    assert(!spaces.empty());
}

// LeetCode 20（Valid Parentheses）：empty 是 stack underflow 與最終平衡檢查。
bool leetcode_valid_parentheses(const std::string& text) {
    std::string stack;
    for (const char ch : text) {
        if (ch == '(' || ch == '[' || ch == '{') {
            stack.push_back(ch);
            continue;
        }
        if (ch != ')' && ch != ']' && ch != '}') {
            return false;
        }
        if (stack.empty()) {
            return false;
        }
        const char open = stack.back();
        stack.pop_back();
        if ((ch == ')' && open != '(') || (ch == ']' && open != '[') ||
            (ch == '}' && open != '{')) {
            return false;
        }
    }
    return stack.empty();
}

// 實務：必填設定不能只檢查 empty，還要辨認全空白。
bool practical_has_non_space(const std::string& text) {
    for (const char ch : text) {
        if (ch != ' ' && ch != '\t' && ch != '\n' && ch != '\r') {
            return true;
        }
    }
    return false;
}

}  // namespace

int main() {
    basic_demo();
    assert(leetcode_valid_parentheses(""));
    assert(leetcode_valid_parentheses("([]{})"));
    assert(!leetcode_valid_parentheses("([)]"));
    assert(!leetcode_valid_parentheses(")"));
    assert(!leetcode_valid_parentheses("(x)"));
    assert(!practical_has_non_space(""));
    assert(!practical_has_non_space(" \t\n"));
    assert(practical_has_non_space("  value "));
    std::cout << "empty: tests passed\n";
}

/*
 * 【LeetCode 契約】空字串視為合法；任何提早閉合、錯配、殘留開括號或非括號字元皆失敗。
 * 【LeetCode 成本】每個字元至多 push/pop 一次，時間 O(n)，最壞額外空間 O(n)。
 * 【實務契約】practical_has_non_space 將空格、tab、LF、CR 視為空白，不承諾完整 Unicode 分類。
 * 【實務選型】若規格要求 Unicode whitespace，必須先決定編碼與文字函式庫，不能只看 byte。
 * 【面試追問】empty 是否比 size()==0 快？兩者皆 O(1)，empty 的優勢是直接表達意圖。
 * 【陷阱】不要用 `text == ""` 代替必填欄位驗證；它同樣放過只含空白的輸入。
 * 【練習】實作 trim 後再 empty 的版本，並比較是否需要配置新字串。
 */

/*
 * 【面試速查】empty 是 O(1) 且不修改字串，只回答元素數量是否為零。
 * 【前置條件】front/back/pop_back 必須在緊鄰呼叫處用 empty guard，避免空容器 UB。
 * 【泛型程式】不要假設所有 range 都有 size；empty/ranges::empty 更能表達真正需求。
 */
