/*
 * C++11 教科書：統一大括號初始化（brace initialization）
 *
 * T{args...} 可初始化 scalar、aggregate、container 與 class，並避免 most-vexing parse。
 * 優點是阻止 narrowing，例如 int{3.14} 編譯失敗。空大括號 T{} 通常做 value
 * initialization：數值歸零、pointer 為 nullptr。
 *
 * 【選擇規則】若 class 同時有 initializer_list constructor，它通常優先；
 * vector<int>(3, 9) 是三個 9，vector<int>{3, 9} 是兩個元素 3、9，語意完全不同。
 * 【常見陷阱】initializer_list overload 優先，不能只看大括號內數字猜 constructor 語意。
 * 【aggregate】公開資料成員、無阻礙 aggregate 條件時可依序初始化；欄位順序改變可能
 * 破壞呼叫端，正式 API 可改用具名 constructor/builder。
 * 【面試題】為何 auto x{1} 與 auto x = {1} 推導可能不同？後者是 initializer_list。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

namespace basic {
struct Point {
    int x;
    int y;
};

void demo() {
    int zero{};
    Point origin{};
    Point target{3, 4};
    std::vector<int> values{1, 2, 3};
    assert(zero == 0 && origin.x == 0 && origin.y == 0);
    assert(target.x == 3 && target.y == 4 && values.size() == 3U);

    // int narrowed{2.5};  // 正確地編譯失敗，避免靜默遺失小數。
}
}  // namespace basic

namespace leetcode {
// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 121. Best Time to Buy and Sell Stock（買賣股票的最佳時機）
// 題目：輸入每日價格，只能先買後賣一次，回傳最大獲利；[7,1,5,3,6,4] 的答案是 5。
// 為何使用本章主題：大括號把 minimum 與 answer 明確初始化，避免未初始化狀態；演算法本身並不依賴 brace init。
// 思路：1. 保存目前最低買價；2. 對每天計算今日賣出的獲利；3. 更新最低價與最大獲利。
// 複雜度：N 為天數；時間 O(N)、額外空間 O(1)。
// 易錯點：空輸入要回 0；必須先買後賣，且價格差若超過 int 範圍就需更寬的型別。
// -----------------------------------------------------------------------------
int max_profit(const std::vector<int>& prices) {
    if (prices.empty()) {
        return 0;
    }
    int minimum{prices.front()};
    int answer{};
    for (const int price : prices) {
        minimum = std::min(minimum, price);
        answer = std::max(answer, price - minimum);
    }
    return answer;
}

void test() {
    assert(max_profit({7, 1, 5, 3, 6, 4}) == 5);
    assert(max_profit({7, 6, 4, 3, 1}) == 0);
}
}  // namespace leetcode

namespace practical {
// -----------------------------------------------------------------------------
// 【日常實務範例】伺服器連線設定初始化
// 情境：啟動服務時要一次建立 host、port 與 TLS 選項，並辨識空的預設設定。
// 為何使用本章主題：aggregate brace init 讓三個欄位按宣告順序完整初始化，ServerConfig{} 也會安全地值初始化各欄位。
// 設計：1. 以大括號建立 production 設定；2. valid 檢查 host 非空與 port 非零；3. 空大括號用來測失敗路徑。
// 成本：驗證 host 是否為空為 O(1)，設定物件空間隨 host 字串長度 H 成長。
// 上線注意：欄位順序變更會破壞 aggregate 呼叫點；還要驗 port、主機名稱格式、TLS 憑證與秘密資料來源。
// -----------------------------------------------------------------------------
struct ServerConfig {
    std::string host;
    unsigned short port;
    bool tls;
};

bool valid(const ServerConfig& config) {
    return !config.host.empty() && config.port != 0U;
}

void test() {
    const ServerConfig production{"api.example.test", 443, true};
    const ServerConfig invalid{};
    assert(valid(production) && production.tls);
    assert(!valid(invalid));
}
}  // namespace practical

void leetcode_test() { leetcode::test(); }
void practical_test() { practical::test(); }

int main() {
    basic::demo();
    leetcode_test();
    practical_test();
    std::cout << "brace init：零初始化、股票題、伺服器設定測試通過\n";
}

// 【延伸練習】列出 vector<int>(3, 9) 與 vector<int>{3, 9} 的內容，再設計不易誤讀的 factory。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '05_brace_init.cpp' -o '/tmp/codex_cpp_C_Cpp11_05_brace_init' && '/tmp/codex_cpp_C_Cpp11_05_brace_init'
//
// === 預期輸出（節錄）===
// brace init：零初始化、股票題、伺服器設定測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
