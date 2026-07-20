/*
 * std::swap(string,string)：泛型程式中的交換介面
 *
 * 對 std::string 有適當 overload，效果呼叫 member swap。泛型模板常使用：
 *   using std::swap; swap(a,b);
 * 讓 ADL 能找到自訂型別專屬 swap，又保留 std::swap fallback。不要在 std namespace
 * 為使用者型別任意新增 overload（標準允許的特化規則非常有限）。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <utility>

namespace {

void basic_demo() {
    std::string first = "left";
    std::string second = "right";
    std::swap(first, second);
    assert(first == "right" && second == "left");
}

// LeetCode 344（Reverse String）：std::swap 交換兩端字元。
void leetcode_reverse_string(std::string& text) {
    for (std::size_t left = 0U, right = text.size(); left < right; ++left) {
        --right;
        if (left >= right) break;
        std::swap(text[left], text[right]);
    }
}

// 實務：確保 pair 的 key 依字典序在前，便於建立無向邊 canonical key。
std::string practical_canonical_edge(std::string first, std::string second) {
    if (second < first) std::swap(first, second);
    return first + "->" + second;
}

}  // namespace

int main() {
    basic_demo();
    std::string word = "hello";
    leetcode_reverse_string(word);
    assert(word == "olleh");
    assert(practical_canonical_edge("nodeB", "nodeA") == "nodeA->nodeB");
    std::cout << "std::swap: tests passed\n";
}

/*
 * 【面試】為何泛型程式先 `using std::swap` 再無限定呼叫？同時啟用 std fallback 與 ADL。
 * 【陷阱】`std::swap(a,b)` 寫死可能略過自訂型別在自己 namespace 的高效 swap。
 * 【練習】定義含大型 string 的 Record，提供 noexcept member swap 與 free swap。
 */

/*
 * 【泛型 swap 慣用法】
 * template<class T>
 * void algorithm(T& a, T& b) {
 *     using std::swap;
 *     swap(a,b); // unqualified，讓 ADL 找到 T namespace 的 overload
 * }
 *
 * 【為何重要】大型自訂型別若只走 generic copy/move swap，可能較慢或 noexcept 條件較差；
 * 專屬 swap 可直接交換成員。std::string 已由標準庫提供合適 overload。
 *
 * 【陷阱】不要替一般使用者型別在 namespace std 隨意新增 overload；在型別自己的
 * namespace 定義 free swap，內部呼叫 member swap。
 * 【面試題】member swap 與 std::swap(string) 結果相同，前者直接；後者適合泛型語境。
 */
