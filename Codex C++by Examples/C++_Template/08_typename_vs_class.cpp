/*
 * 第 08 章：typename 與 class
 *
 * 在 template parameter list 中，typename T 與 class T 意義相同；現代程式碼常用
 * typename，因為 T 不一定是 class（也可能是 int）。但在「相依名稱」前，typename
 * 有另一個不可替代的用途：告訴 parser，Container::value_type 是型別而非靜態成員。
 *
 * 規則口訣：名稱依賴模板參數，而且編譯器無法預先確定它是型別時，加 typename。
 */

#include <cassert>
#include <cstddef>
#include <iostream>
#include <list>
#include <string>
#include <utility>
#include <vector>

template <typename Container>
typename Container::value_type first_or(const Container& values,
                                        typename Container::value_type fallback) {
    return values.empty() ? std::move(fallback) : values.front();
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 876. Middle of the Linked List（鏈結串列的中間節點）
// 題目：回傳串列中點，偶數長度取第二個中點；[1,2,3,4,5,6] 得 4。
// 為何使用本章主題：本例改接通用 Container，以 typename 指明相依的 value_type/const_iterator；
// 它回傳中間值而非原題 ListNode*，是為展示相依型別語法的教學改寫。
// 思路：slow/fast 從 begin 出發；fast 每輪走兩步、slow 走一步；fast 到 end 時 slow 位於中點。
// 複雜度：時間 O(N)、額外空間 O(1)，N 是元素數；回傳 value_type 時另有一次元素複製。
// 易錯點：values 必須非空且至少有 forward iterator 能力；release 建置移除 assert 後空輸入會解參考 end。
// -----------------------------------------------------------------------------
template <class Container>
typename Container::value_type leetcode_middle_value(const Container& values) {
    assert(!values.empty());
    typename Container::const_iterator slow = values.begin();
    typename Container::const_iterator fast = values.begin();
    while (fast != values.end()) {
        ++fast;
        if (fast == values.end()) {
            break;
        }
        ++fast;
        ++slow;
    }
    return *slow;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】發票金額批次加總
// 情境：不同序列容器保存 Invoice，財務摘要要加總所有 cents 而不綁定 vector。
// 為何使用本章主題：typename Container::value_type 讓模板把相依名稱解析成 Item 型別；
// 相較硬寫 Invoice，函式可接受具有相同元素欄位的容器，但語法不增加 runtime 成本。
// 設計：以 Item alias 取得元素型別；逐筆讀取 cents；累加後回傳整數總額。
// 成本：時間 O(N)、額外空間 O(1)，N 是 invoices 數量。
// 上線注意：int 累加可能溢位，應使用較寬金額型別；模板未檢查 Item 一定有 cents 成員。
// -----------------------------------------------------------------------------
struct Invoice {
    int cents{};
};

template <typename Container>
int practical_total_cents(const Container& invoices) {
    using Item = typename Container::value_type;
    static_assert(sizeof(Item) >= sizeof(int));
    int total = 0;
    for (const Item& invoice : invoices) {
        total += invoice.cents;
    }
    return total;
}

int main() {
    assert(first_or(std::vector<int>{}, 99) == 99);
    assert(first_or(std::vector<std::string>{"A", "B"}, std::string{"N/A"}) == "A");

    const std::list<int> odd{1, 2, 3, 4, 5};
    const std::list<int> even{1, 2, 3, 4, 5, 6};
    assert(leetcode_middle_value(odd) == 3);
    assert(leetcode_middle_value(even) == 4); // 題目定義偶數長度取第二個中點

    const std::vector<Invoice> invoices{{1200}, {350}, {450}};
    assert(practical_total_cents(invoices) == 2000);

    std::cout << "typename 相依名稱測試完成\n";
}

/*
 * 【容易混淆】
 * - `template <typename T>` 的 typename 可換成 class。
 * - `typename T::value_type` 的 typename 不能任意換成 class。
 * - 呼叫相依的成員模板時可能要寫 `object.template get<int>()`；這是相似的解析提示。
 * 【面試】為何編譯器不能自行猜？模板定義時尚不知道 T，T::x 可能是值也可能是型別。
 * 【練習】寫一個回傳 Container::const_reference 的 last()，並處理空容器契約。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '08_typename_vs_class.cpp' -o '/tmp/codex_cpp_C_Template_08_typename_vs_class' && '/tmp/codex_cpp_C_Template_08_typename_vs_class'
//
// === 預期輸出（節錄）===
// typename 相依名稱測試完成
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
