/*
 * std::copy / copy_n / copy_if / copy_backward：複製範圍
 * =====================================================
 * copy(first,last,out) 逐一指定到輸出，回傳最後寫入位置的下一格。
 * 目的端必須有足夠空間；若要讓 vector 自動增長，使用 back_inserter。
 * 複雜度 O(N)。來源與目的若危險重疊，copy 的行為未定義：向右搬使用
 * copy_backward；向左搬才可用 copy。trivially copyable 型別實作可能最佳化成
 * memmove，但程式不能依賴未獲契約允許的重疊。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

// LeetCode 1929：Concatenation of Array。
std::vector<int> leetcode_get_concatenation(const std::vector<int>& nums) {
    std::vector<int> answer(nums.size() * 2U);
    auto out = std::copy(nums.begin(), nums.end(), answer.begin());
    std::copy(nums.begin(), nums.end(), out);
    return answer;
}

// 實務：只匯出可公開的 log 行；back_inserter 負責 push_back。
std::vector<std::string> practical_export_public_logs(
    const std::vector<std::string>& logs) {
    std::vector<std::string> result;
    std::copy_if(logs.begin(), logs.end(), std::back_inserter(result),
                 [](const std::string& line) {
                     return line.find("SECRET") == std::string::npos;
                 });
    return result;
}

int main() {
    const std::vector<int> source{1, 2, 3};
    std::vector<int> fixed(source.size());
    const auto end = std::copy(source.begin(), source.end(), fixed.begin());
    assert(end == fixed.end());
    assert(fixed == source);

    // 同一 vector 向右位移，必須從尾端 copy_backward。
    std::vector<int> shifted{1, 2, 3, 0};
    std::copy_backward(shifted.begin(), shifted.begin() + 3, shifted.end());
    assert((shifted == std::vector<int>{1, 1, 2, 3}));

    assert((leetcode_get_concatenation({1, 2, 1}) ==
            std::vector<int>{1, 2, 1, 1, 2, 1}));
    assert((practical_export_public_logs({"ok", "SECRET token", "done"}) ==
            std::vector<std::string>{"ok", "done"}));

    std::cout << "copy：容量、重疊方向、LC1929、log 匯出測試通過\n";
}

/*
 * 面試 Q&A：copy 為何不能讓 vector 變大？iterator 演算法不知道容器本體，只能
 * 對既有位置指定。copy_if 最壞仍 O(N)，輸出數量 <=N。
 * 練習：以 copy_n 複製前 k 筆，先處理 k 大於 source.size() 的輸入契約。
 *
 * 【LeetCode 解題連結】
 * LC1929 的關鍵不是手寫兩個 loop，而是理解 output iterator：第一次 copy 的
 * 回傳值正好是第二段起點。這種「串接回傳 iterator」在資料管線很常見。
 *
 * 【實務選擇】
 * - 目的大小已知：先 resize，再 copy 到 begin，配置次數可預測。
 * - 目的大小未知：reserve 估計容量，再搭配 back_inserter。
 * - 型別很大且來源之後不用：考慮 std::move，不要無條件 copy。
 * - 跨 byte buffer 且可能重疊：依契約使用 memmove，而不是賭 std::copy 實作。
 *
 * 面試延伸：copy_if 是穩定的，保留項目順序不變；它回傳輸出尾端，可直接計算
 * 實際產生數量。輸出 iterator 若指向固定陣列，呼叫者必須自行證明容量足夠。
 * 易錯陷阱：不要把 reserve 當 resize；reserve 只改 capacity，copy 到 begin 仍沒有
 * 已建構元素可指定。要嘛 resize，要嘛用 back_inserter。
 */
