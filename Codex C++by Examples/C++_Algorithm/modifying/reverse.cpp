/*
 * std::reverse / reverse_copy：反轉範圍
 * ====================================
 * reverse 原地交換首尾，需 bidirectional iterator，做 floor(N/2) 次 swap，O(N)。
 * reverse_copy 不改來源，目的端需足夠空間/使用 inserter；來源與目的不可危險重疊。
 * reverse 不改 size，但元素位置改變；vector iterator 通常仍指同一位置而非原物件，
 * 所以不要誤以為 iterator 會「跟著元素走」。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 344. Reverse String（反轉字串）
// 題目：以字元陣列 s 輸入，必須原地反轉並使用 O(1) 額外空間；例如
// ['h','e','l','l','o'] 變 ['o','l','l','e','h']。
// 為何使用本章主題：std::reverse 封裝首尾交換，直接滿足原地與常數額外空間要求。
// 思路：1. 將 begin/end 傳給 reverse；2. 演算法交換對稱位置直到中點。
// 複雜度：時間 O(N)、額外空間 O(1)，N 為字元數，執行 floor(N/2) 次交換。
// 易錯點：end 是尾後 iterator；此處 vector<char> 以字元為單位，不涉及 UTF-8 byte 字串切割問題。
// -----------------------------------------------------------------------------
void leetcode_reverse_string(std::vector<char>& text) {
    std::reverse(text.begin(), text.end());
}

// -----------------------------------------------------------------------------
// 【日常實務範例】錯誤 Breadcrumb 反向顯示
// 情境：診斷路徑以 root 到 leaf 儲存，例如 service/parser/token；錯誤訊息要由最具體
// 的 token 往外顯示，同時保留原 path 供其他分析。
// 為何使用本章主題：reverse_copy 直接反向複製到新容器，不修改來源；相較先 copy
// 再 reverse 少一個獨立步驟並清楚表達保留原資料。
// 設計：1. 依 path 大小 reserve；2. 由 reverse_copy 透過 back_inserter 寫出 leaf->root。
// 成本：時間 O(N)、額外空間 O(N)，N 為 breadcrumb 層數。
// 上線注意：這只是顯示順序，不是檔案路徑 canonicalization；輸出配置失敗時來源仍保持不變。
// -----------------------------------------------------------------------------
std::vector<std::string> practical_reverse_breadcrumb(
    const std::vector<std::string>& path) {
    std::vector<std::string> result;
    result.reserve(path.size());
    std::reverse_copy(path.begin(), path.end(), std::back_inserter(result));
    return result;
}

int main() {
    std::vector<int> values{1, 2, 3, 4};
    std::reverse(values.begin() + 1, values.end());
    assert((values == std::vector<int>{1, 4, 3, 2}));

    std::vector<char> text{'h', 'e', 'l', 'l', 'o'};
    leetcode_reverse_string(text);
    assert((text == std::vector<char>{'o', 'l', 'l', 'e', 'h'}));

    const std::vector<std::string> path{"service", "parser", "token"};
    assert((practical_reverse_breadcrumb(path) ==
            std::vector<std::string>{"token", "parser", "service"}));
    assert(path.front() == "service");
    std::cout << "reverse：子範圍、LC344、breadcrumb 測試通過\n";
}

/*
 * Unicode 陷阱：反轉 UTF-8 std::string 的 byte 會破壞多 byte code point；需要先
 * 解碼為 Unicode code points/grapheme clusters。面試：forward_list 為何不能
 * reverse 演算法？它沒有 --iterator；但有 member reverse()。
 * 練習：用 reverse 三次法手寫 rotate right。
 *
 * 【LeetCode 不變量】
 * LC344 也可雙指標 while(left<right)；std::reverse 已封裝同樣交換。空範圍與
 * 單元素範圍自然不做事，不需特判。
 *
 * 【實務選擇】
 * reverse_breadcrumb 使用 reverse_copy，明確保留原始 path 供後續診斷；若原資料
 * 不再使用，原地 reverse 可省配置。輸出數量已知時先 reserve，可避免成長重配。
 * 反轉只是視圖需求時，也可考慮 reverse_iterator/ranges::reverse_view 避免複製。
 *
 * 面試複雜度：時間 O(N)，交換 floor(N/2) 次，額外空間 O(1)；reverse_copy 輸出
 * 需 O(N) storage。易錯陷阱是把 end 當最後元素解參考，end 是尾後位置。
 *
 * LeetCode 也要測偶數/奇數、空與單字元。實務 path 若包含 symlink/`..`，單純反轉
 * 不是路徑正規化；顯示需求與檔案系統語意要分開。
 * 練習：用 reverse 三次實作 rotate，並與 std::rotate 的結果逐項比較。
 * 進階：用 reverse_iterator 產生唯讀輸出，確認沒有複製元素。
 * 測試：UTF-8 範例應刻意展示 byte reverse 為何錯，再交由 Unicode library 處理。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'reverse.cpp' -o '/tmp/codex_cpp_C_Algorithm_modifying_reverse' && '/tmp/codex_cpp_C_Algorithm_modifying_reverse'
//
// === 預期輸出（節錄）===
// reverse：子範圍、LC344、breadcrumb 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
