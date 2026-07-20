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

// LeetCode 876 的容器版：Middle of the Linked List。
// 使用 typename 宣告相依 iterator 型別；forward iterator 也適用，O(n) 時間、O(1) 空間。
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

struct Invoice {
    int cents{};
};

// 實務：型別別名可讓後續宣告易讀；typename 是語法資訊，不會產生執行期成本。
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
