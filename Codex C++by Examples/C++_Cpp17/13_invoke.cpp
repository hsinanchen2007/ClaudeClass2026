/*
 * C++17 教科書：std::invoke
 *
 * std::invoke 統一呼叫 ordinary function、function object、lambda、pointer-to-member function
 * 與 pointer-to-data-member。generic library 不必自己寫一堆 overload 判斷 callable 種類。
 * 對 member pointer，object 可用 object/reference、pointer 或 reference_wrapper 形式傳入。
 *
 * 【相關 traits】std::is_invocable、is_invocable_r、invoke_result 可做 compile-time 檢查。
 * 【回傳】std::invoke 本身依 callable 回傳型別與 noexcept；不要保存可能指向 temporary 的 ref。
 * 【成本】通常可 inline，抽象本身不要求 type erasure；std::function 才有 type-erasure 成本。
 * 【陷阱】overloaded member function pointer 需 static_cast 指定簽名，否則取址有歧義。
 * 【面試題】invoke 與直接 `f(args...)` 差別主要在哪？member pointers 的統一處理。
 */

#include <cassert>
#include <functional>
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace basic {
struct Calculator {
    int base;
    int add(int value) const { return base + value; }
};

void demo() {
    Calculator calculator{10};
    assert(std::invoke(&Calculator::add, calculator, 5) == 15);
    std::invoke(&Calculator::base, calculator) = 20;
    assert(calculator.base == 20);
    static_assert(std::is_invocable_r<int, decltype(&Calculator::add), Calculator&, int>::value,
                  "member function 應可由 Calculator& 與 int 呼叫");
}
}  // namespace basic

namespace leetcode {
// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 94. Binary Tree Inorder Traversal（二元樹中序走訪）
// 題目：依左子樹、根、右子樹順序回傳節點值；樹 [1,null,2,3] 回傳 [1,3,2]。
// 為何使用本章主題：泛型 inorder 以 std::invoke 呼叫任意 visitor，lambda、函式物件或相容 callable 共用同一走訪器。
// 思路：1. nullptr 直接返回；2. 遞迴走左側；3. invoke 根節點 visitor，再遞迴右側。
// 複雜度：N 為節點數、H 為樹高；時間 O(N)、遞迴堆疊 O(H)，結果另占 O(N)。
// 易錯點：樹必須無環且節點活過走訪；偏斜樹可能耗盡 stack，visitor 的例外會直接向外傳播。
// -----------------------------------------------------------------------------
struct TreeNode {
    int value;
    TreeNode* left;
    TreeNode* right;
};

template <class Visitor>
void inorder(const TreeNode* node, Visitor&& visitor) {
    if (node == nullptr) return;
    inorder(node->left, visitor);
    std::invoke(visitor, node->value);
    inorder(node->right, visitor);
}

std::vector<int> leetcode_inorder_traversal(const TreeNode* root) {
    std::vector<int> values;
    inorder(root, [&values](int value) { values.push_back(value); });
    return values;
}

void leetcode_test() {
    TreeNode third{3, nullptr, nullptr};
    TreeNode second{2, &third, nullptr};
    TreeNode first{1, nullptr, &second};
    assert(leetcode_inorder_traversal(&first) == std::vector<int>({1, 3, 2}));
}
}  // namespace leetcode

namespace practical {
// -----------------------------------------------------------------------------
// 【日常實務範例】通用工作 callable runner
// 情境：執行器要以同一入口呼叫 lambda、一般 callable 或 Job member pointer，並保留回傳 reference 與 noexcept。
// 為何使用本章主題：std::invoke 統一 member pointer 與一般呼叫；decltype(auto) 和 perfect forwarding 保留底層呼叫契約。
// 設計：1. 接收 Callable 與參數 forwarding references；2. 以 is_nothrow_invocable 推導 noexcept；3. invoke 並原樣回傳。
// 成本：抽象本身通常 O(1) 且可 inline；總成本等同 callable，無 std::function type-erasure 配置。
// 上線注意：forwarded reference 與回傳 reference 都受原物件生命週期約束；overloaded member pointer 要先指定簽章。
// -----------------------------------------------------------------------------
struct Job {
    int value;
    int scale(int factor) const { return value * factor; }
};

template <class Callable, class... Args>
decltype(auto) practical_run(Callable&& callable, Args&&... args)
    noexcept(std::is_nothrow_invocable<Callable&&, Args&&...>::value) {
    return std::invoke(std::forward<Callable>(callable), std::forward<Args>(args)...);
}

void practical_test() {
    const Job job{7};
    assert(practical_run(&Job::scale, job, 3) == 21);
    assert(practical_run([](int left, int right) { return left + right; }, 2, 4) == 6);

    struct NonCopyable {
        int value;
        NonCopyable(const NonCopyable&) = delete;
        NonCopyable& operator=(const NonCopyable&) = delete;
    } object{7};
    const auto identity = [](NonCopyable& item) noexcept -> NonCopyable& { return item; };
    static_assert(std::is_same<decltype(practical_run(identity, object)),
                               NonCopyable&>::value,
                  "practical_run 必須保留 reference return");
    static_assert(noexcept(practical_run(identity, object)),
                  "practical_run 必須傳遞 callable 的 noexcept");
    practical_run(identity, object).value = 9;
    assert(object.value == 9);
}
}  // namespace practical

int main() {
    basic::demo();
    leetcode::leetcode_test();
    practical::practical_test();
    std::cout << "std::invoke：member pointer、inorder visitor、job runner 測試通過\n";
}

// 【延伸練習】加入 overloaded member function，使用 static_cast 消除取址歧義並驗證 traits。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '13_invoke.cpp' -o '/tmp/codex_cpp_C_Cpp17_13_invoke' && '/tmp/codex_cpp_C_Cpp17_13_invoke'
//
// === 預期輸出（節錄）===
// std::invoke：member pointer、inorder visitor、job runner 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
