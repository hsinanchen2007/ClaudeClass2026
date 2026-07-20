/*
 * std::binary_search：在已排序範圍判斷「是否存在」
 * ==================================================
 *
 * 【先建立正確模型】
 * binary_search(first, last, value) 搜尋半開區間 [first, last)，回傳 bool。
 * 它不告訴你元素在哪裡；需要位置時應選 lower_bound。
 *
 * 前置條件：範圍必須依同一個比較規則排序。若未排序，結果不是可靠答案。
 * 預設比較器是 std::less<>，自訂型別通常傳入 comp(element, value)。
 * 「相等」不是呼叫 operator==，而是 !comp(a,b) && !comp(b,a)。
 *
 * 複雜度：比較次數 O(log N)。但 forward/list iterator 的移動可能是 O(N)；
 * vector、array、deque 的 random-access iterator 才能同時得到快速跳躍。
 * iterator 只在演算法執行期間被讀取，演算法不保存 iterator，也不改容器。
 *
 * 常見錯誤：
 * 1. 先搜尋、忘了排序；2. 排序用 descending，搜尋卻忘記同傳 greater<>；
 * 3. 需要索引卻用 binary_search；4. 浮點 NaN 破壞嚴格弱序。
 */

#include <algorithm>
#include <cassert>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

// LeetCode 704：Binary Search。
// 題目要索引，因此用 lower_bound 找候選，再驗證是否真的相等。
int leetcode_search_index(const std::vector<int>& nums, int target) {
    const auto it = std::lower_bound(nums.begin(), nums.end(), target);
    if (it == nums.end() || *it != target) {
        return -1;
    }
    return static_cast<int>(std::distance(nums.begin(), it));
}

struct Product {
    int sku;
    std::string name;
};

struct ProductSkuLess {
    bool operator()(const Product& product, int key) const {
        return product.sku < key;
    }
    bool operator()(int key, const Product& product) const {
        return key < product.sku;
    }
};

// 實務：後端收到 SKU，先快速判斷商品主檔是否存在。
// vector 必須按 sku 排序；透明 lambda 允許 Product 與 int 比較，不必造假 Product。
bool practical_has_sku(const std::vector<Product>& catalog, int sku) {
    return std::binary_search(catalog.begin(), catalog.end(), sku,
                              ProductSkuLess{});
}

int main() {
    // 基礎用法：升冪資料。
    const std::vector<int> values{1, 3, 3, 7, 9, 12};
    assert(std::binary_search(values.begin(), values.end(), 7));
    assert(!std::binary_search(values.begin(), values.end(), 8));

    // 降冪範圍必須沿用 std::greater<>。
    const std::vector<int> descending{12, 9, 7, 3, 1};
    assert(std::binary_search(descending.begin(), descending.end(), 9,
                              std::greater<>{}));

    // LeetCode 704 測試。
    assert(leetcode_search_index({-1, 0, 3, 5, 9, 12}, 9) == 4);
    assert(leetcode_search_index({-1, 0, 3, 5, 9, 12}, 2) == -1);

    // 實務測試。
    const std::vector<Product> catalog{{1001, "keyboard"},
                                       {1020, "mouse"},
                                       {1100, "monitor"}};
    assert(practical_has_sku(catalog, 1020));
    assert(!practical_has_sku(catalog, 9999));

    std::cout << "binary_search：基礎、LeetCode、商品查詢測試通過\n";
}

/*
 * 面試快問快答：
 * Q: binary_search 是否保證找到第一個重複值？ A: 不回 iterator，無此保證。
 * Q: 為何 sorted vector 常比 set 查詢快？ A: 都是 O(log N)，但 vector 連續記憶體
 *    cache locality 較好；代價是中間插入 O(N)。
 * 陷阱總結：未排序、比較器方向不一致、直接解參考 end，三者都必須靠測試防止。
 * 練習：把 Product 版本改成 descending sku，並同步修改排序與搜尋比較器。
 */

/*
 * 【教科書補充：binary_search 的真正契約】
 * - 完整排序是最常見的充分條件；標準真正要求的是範圍對查詢 key 同時形成兩側 partition。
 * - 若該 partition 關係或比較的非對稱性不成立，行為未定義，不只是「可能回錯答案」。
 * - heterogeneous comparator 要能處理 comp(element,key) 與 comp(key,element)，兩者語意須一致。
 * - 降冪資料必須配對降冪 comparator；資料與 comparator 任一方改變，都要重建同一 invariant。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'binary_search.cpp' -o '/tmp/codex_cpp_C_Algorithm_binary_search_binary_search' && '/tmp/codex_cpp_C_Algorithm_binary_search_binary_search'
//
// === 預期輸出（節錄）===
// binary_search：基礎、LeetCode、商品查詢測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
