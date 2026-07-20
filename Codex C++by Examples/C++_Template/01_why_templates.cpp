/*
 * 第 01 章：為什麼需要模板
 *
 * 【先理解問題】
 * 若沒有模板，max_int、max_double、max_string 的演算法完全相同，卻要複製三份。
 * 複製貼上會造成兩種成本：修 bug 時容易漏改，新增型別時又要增加函式。
 * 模板把「不變的演算法」與「會變的型別」分離；編譯器會在使用點為實際型別
 * 產生實體（instantiation），所以仍有靜態型別檢查，通常也沒有虛擬呼叫成本。
 *
 * 【何時不要用模板】
 * 1. 執行期才知道型別：考慮 virtual、std::variant 或 type erasure。
 * 2. 不同型別的流程其實不同：硬塞進同一模板會堆滿 if constexpr。
 * 3. 公開 ABI 必須長期穩定：模板實作通常放 header，改動會要求使用者重編。
 *
 * 【複雜度】better_of 只比較一次，時間 O(1)、額外空間 O(1)。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

template <typename T>
const T& better_of(const T& left, const T& right) {
    // 回傳 const reference 可避免複製大型物件；呼叫者必須確保引數生命週期仍存在。
    return (left < right) ? right : left;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 217. Contains Duplicate（存在重複元素）
// 題目：判斷陣列中是否有任一值至少出現兩次；[1,2,3,1] 為 true，[1,2,3,4] 為 false。
// 為何使用本章主題：函式模板把原題的 vector<int> 泛化為 vector<T>，同一套雜湊流程可處理
// int 與 string；這是教學擴充，前提是 T 可供 unordered_set 雜湊與相等比較。
// 思路：依輸入大小 reserve seen；逐項插入；第一次插入失敗立即回 true，走完則回 false。
// 複雜度：平均時間 O(N)、額外空間 O(N)，N 是 values 長度；嚴重碰撞時時間可退化。
// 易錯點：空輸入應回 false；模板簽名未明寫 Hash/Equal constraint，不合格 T 會在實體化時報錯。
// -----------------------------------------------------------------------------
template <typename T>
bool leetcode_contains_duplicate(const std::vector<T>& values) {
    std::unordered_set<T> seen;
    seen.reserve(values.size());
    for (const T& value : values) {
        if (!seen.insert(value).second) {
            return true;
        }
    }
    return false;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】挑選較緊急的工作報價
// 情境：排程器收到兩筆含 sku 與 priority 的 Quote，要借用並回傳優先級較高的一筆。
// 為何使用本章主題：better_of<T> 只依賴 operator<，Quote 提供比較後即可重用同一模板；
// 相較為每個領域型別複製 max 函式，修正比較流程只需維護一處。
// 設計：Quote 將 `<` 定義為 priority 比較；practical_select_urgent 把兩筆交給 better_of；同值保留左側。
// 成本：時間與額外空間 O(1)，回傳 const reference 不複製 sku 字串。
// 上線注意：回傳參考不得活過兩個輸入；還需定義 priority 相同時的 tie-break 與空候選集合行為。
// -----------------------------------------------------------------------------
struct Quote {
    std::string sku;
    int priority{};

    friend bool operator<(const Quote& lhs, const Quote& rhs) {
        return lhs.priority < rhs.priority;
    }
};

const Quote& practical_select_urgent(const Quote& a, const Quote& b) {
    return better_of(a, b);
}

int main() {
    // 基礎：T 由引數推導，分別實體化為 int 與 std::string。
    const int low = 7;
    const int high = 11;
    assert(better_of(low, high) == 11);

    const std::string short_name = "Ada";
    const std::string long_name = "Grace";
    assert(better_of(short_name, long_name) == "Grace"); // 字典序比較

    // LeetCode 式測試：平均 O(n) 時間、O(n) 空間。
    assert(leetcode_contains_duplicate(std::vector<int>{1, 2, 3, 1}));
    assert(!leetcode_contains_duplicate(std::vector<int>{1, 2, 3, 4}));
    assert(leetcode_contains_duplicate(std::vector<std::string>{"api", "db", "api"}));

    // 實務案例。
    const Quote normal{"JOB-17", 2};
    const Quote urgent{"JOB-99", 9};
    assert(practical_select_urgent(normal, urgent).sku == "JOB-99");

    std::cout << "模板可重用演算法，且仍在編譯期檢查型別。\n";
}

/*
 * 【常見陷阱】
 * - better_of(1, 2.0) 無法推導單一 T；可明寫 better_of<double>(1, 2.0)，
 *   或另設兩個型別參數並決定回傳型別。
 * - better_of(Quote{...}, Quote{...}) 回傳 reference 會指向暫時物件；不要保存該 reference。
 * - 模板不是巨集：巨集只做文字替換，沒有作用域與型別安全。
 *
 * 【面試快問】模板的成本是什麼？
 * 答：可能增加編譯時間、錯誤訊息與 code bloat；好處是零成本抽象與靜態檢查。
 *
 * 【練習】為 contains_duplicate 加上自訂 Hash 與 Equal 模板參數。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '01_why_templates.cpp' -o '/tmp/codex_cpp_C_Template_01_why_templates' && '/tmp/codex_cpp_C_Template_01_why_templates'
//
// === 預期輸出（節錄）===
// 模板可重用演算法，且仍在編譯期檢查型別。
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
