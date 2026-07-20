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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1929. Concatenation of Array（陣列串接）
// 題目：輸入 nums，回傳長度 2N 且 answer[i]=answer[i+N]=nums[i]；例如
// [1,2,1] 回 [1,2,1,1,2,1]。
// 為何使用本章主題：兩次 std::copy 可把同一來源依序寫入預先配置的輸出，第一次
// 回傳的 output iterator 正好是第二段起點。
// 思路：1. 建立 2N 個已存在的目的元素；2. 複製 nums 到前半；3. 從回傳 iterator
// 再複製一次到後半。
// 複雜度：時間 O(N)、額外空間 O(N)，N 為 nums 的元素數，輸出實際含 2N 項。
// 易錯點：目的 vector 必須 resize 而非只 reserve；兩次複製的輸出範圍都要有足夠容量。
// -----------------------------------------------------------------------------
std::vector<int> leetcode_get_concatenation(const std::vector<int>& nums) {
    std::vector<int> answer(nums.size() * 2U);
    auto out = std::copy(nums.begin(), nums.end(), answer.begin());
    std::copy(nums.begin(), nums.end(), out);
    return answer;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】公開日誌匯出過濾
// 情境：內部 log 可能含 `SECRET` 行，匯出給外部前只保留未命中敏感標記的完整字串，
// 並維持原始順序。
// 為何使用本章主題：copy_if 同時表達篩選與穩定複製，back_inserter 讓未知輸出筆數
// 的 vector 安全成長，比先猜大小後管理尾端簡單。
// 設計：1. 對每行搜尋敏感標記；2. predicate 只接受未命中的行；3. 由 back_inserter
// append 到 result。
// 成本：時間 O(C)、額外空間 O(K)，C 為所有被檢查字元量、K 為匯出內容大小。
// 上線注意：單純 substring 不能取代結構化脫敏；需涵蓋大小寫、編碼、多行欄位並記錄拒絕筆數。
// -----------------------------------------------------------------------------
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

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'copy.cpp' -o '/tmp/codex_cpp_C_Algorithm_modifying_copy' && '/tmp/codex_cpp_C_Algorithm_modifying_copy'
//
// === 預期輸出（節錄）===
// copy：容量、重疊方向、LC1929、log 匯出測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
