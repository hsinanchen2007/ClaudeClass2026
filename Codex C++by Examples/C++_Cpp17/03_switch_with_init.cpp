/*
 * C++17 教科書：switch statement with initializer
 *
 * `switch (init; condition)` 與 if-init 同理：先建立 parse/result/lock 等暫存物件，
 * 再對 condition 分支。init variable 在所有 case 與整個 switch scope 都有效。
 * switch condition 仍須是 integral/enumeration 類型，不能直接 switch std::string。
 *
 * 【設計】先 parse 字串成 enum，再 switch enum，能把 validation 與 dispatch 分離。
 * 【case scope】若 case 內宣告有 non-trivial initialization 的變數，使用 `{}` 建立 scope，
 * 避免 jump across initialization 編譯錯誤。
 * 【fallthrough】預設每個 case 結尾 break/return；刻意貫穿加 [[fallthrough]]。
 * 【常見陷阱】case 內宣告物件卻沒加 scope，可能發生 jump across initialization 編譯錯誤。
 * 【面試題】initializer 變數何時析構？離開整個 switch（包括被選 case）時。
 */

#include <cassert>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

namespace basic {
int parse_code(const std::string& text) { return std::stoi(text); }

std::string classify(const std::string& text) {
    switch (const int code = parse_code(text); code / 100) {
        case 2: return "success";
        case 4: return "client-error";
        case 5: return "server-error";
        default: return "other";
    }
}

void demo() {
    assert(classify("204") == "success");
    assert(classify("503") == "server-error");
}
}  // namespace basic

namespace leetcode {
// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 682. Baseball Game（棒球比賽計分）
// 題目：依序處理分數、+、D、C 並回傳有效分數總和；["5","2","C","D","+"] 得 30。
// 為何使用本章主題：switch-init 將 token.front() 命名為 operation，字元只在該次分派範圍內存在。
// 思路：1. 逐 token 取第一字元；2. 三種命令更新 scores，其他 token 解析為整數；3. 最後 accumulate。
// 複雜度：N 為操作數；時間 O(N)、額外空間 O(N)。
// 易錯點：token 不可為空；+ 需兩筆、D/C 需一筆，本實作依賴題目保證操作序列合法。
// -----------------------------------------------------------------------------
int leetcode_cal_points(const std::vector<std::string>& operations) {
    std::vector<int> scores;
    for (const std::string& token : operations) {
        switch (const char operation = token.front(); operation) {
            case '+': scores.push_back(scores[scores.size() - 1U] + scores[scores.size() - 2U]); break;
            case 'D': scores.push_back(scores.back() * 2); break;
            case 'C': scores.pop_back(); break;
            default: scores.push_back(std::stoi(token)); break;
        }
    }
    return std::accumulate(scores.begin(), scores.end(), 0);
}

void leetcode_test() {
    assert(leetcode_cal_points({"5", "2", "C", "D", "+"}) == 30);
}
}  // namespace leetcode

namespace practical {
// -----------------------------------------------------------------------------
// 【日常實務範例】服務事件代碼分派
// 情境：外部傳入 1/2/3 事件代碼，服務要映射成 allocate、release 或 refresh 動作。
// 為何使用本章主題：switch-init 先轉成 scoped Event，並把 event 作用域限制在分派 statement 內。
// 設計：1. 將 raw_event 轉 Event；2. 每個合法 enumerator 直接回動作；3. 未知值離開 switch 後丟例外。
// 成本：單次分派時間與空間皆 O(1)。
// 上線注意：static_cast 不會驗 raw 值；目前以最終 throw 防守，正式協定還要記錄未知代碼與版本。
// -----------------------------------------------------------------------------
enum class Event { start = 1, stop = 2, heartbeat = 3 };

std::string practical_dispatch(int raw_event) {
    switch (const Event event = static_cast<Event>(raw_event); event) {
        case Event::start: return "allocate";
        case Event::stop: return "release";
        case Event::heartbeat: return "refresh";
    }
    throw std::invalid_argument("未知事件代碼");
}

void practical_test() {
    assert(practical_dispatch(1) == "allocate");
    assert(practical_dispatch(3) == "refresh");
}
}  // namespace practical

int main() {
    basic::demo();
    leetcode::leetcode_test();
    practical::practical_test();
    std::cout << "switch-init：HTTP、Baseball Game、event dispatch 測試通過\n";
}

// 【延伸練習】傳入 99，驗證 unknown 分支；再比較 switch-init 與先宣告 event 的作用域。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '03_switch_with_init.cpp' -o '/tmp/codex_cpp_C_Cpp17_03_switch_with_init' && '/tmp/codex_cpp_C_Cpp17_03_switch_with_init'
//
// === 預期輸出（節錄）===
// switch-init：HTTP、Baseball Game、event dispatch 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
