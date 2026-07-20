/*
 * Lambda 教科書 08：std::bind
 *
 * std::bind 固定 callable 的部分 arguments，留下 std::placeholders::_1... 給未來呼叫。
 * bound arguments 預設 decay-copy；要保存 reference 必須 std::ref/std::cref。member function
 * pointer 可綁 object/pointer。回傳的 bind object 型別未指定，通常用 auto 保存。
 *
 * 【現代建議】lambda 通常更清楚、型別診斷更好，也能精準控制 move/reference；bind 在維護
 * 舊 callback adapters 或需要 composition 時仍會遇到。nested bind expression 規則尤其難讀。
 * 【placeholder】_1 代表未來 call 的第一 argument，不是 bind 當下第一個 argument。
 * 【生命週期】std::ref 不延長對象 lifetime；callback 活得更久時仍會 dangling。
 * 【常見陷阱】bound argument 預設 decay-copy；忘記 std::ref 會只改副本，nested bind 又難讀。
 * 【面試題】為何 `std::bind(f, x)` 修改不到原 x？預設複製，要 `std::ref(x)`。
 */

#include <cstdlib>
#include <functional>
#include <iostream>
#include <string>
#include <utility>

namespace {

// 不使用 assert，讓測試在 -DNDEBUG 建置中仍會執行。
void expect(const bool condition) {
    if (!condition) std::abort();
}

}  // namespace

namespace basic {
int multiply(int left, int right) { return left * right; }

void demo() {
    using namespace std::placeholders;
    const auto double_value = std::bind(&multiply, _1, 2);
    expect(double_value(6) == 12);

    int value = 1;
    const auto increment = std::bind([](int& target) { ++target; }, std::ref(value));
    increment();
    expect(value == 2);
}
}  // namespace basic

namespace leetcode {
// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 278. First Bad Version（第一個錯誤的版本）
// 題目：版本 1..n 中，從某版起全部為 bad，找出第一個 bad；n=5、first_bad=4 時回傳 4。
// 為何使用本章主題：bind 固定模擬 oracle 的 first_bad，只留下 version placeholder；真實題目由
// 平台提供 isBadVersion API，不需綁定第二參數，因此這是 std::bind 的教學模擬。
// 思路：建立一元 bad predicate；在閉區間做二分；bad 時收右界，good 時把左界移到 middle+1。
// 複雜度：時間 O(log N)、額外空間 O(1)，N 是版本總數，每輪呼叫一次 oracle。
// 易錯點：middle 用 left+(right-left)/2 避免加法溢位；契約要求 1<=first_bad<=versions。
// -----------------------------------------------------------------------------
bool is_bad(int version, int first_bad) { return version >= first_bad; }

int leetcode_first_bad_version(int versions, int first_bad) {
    using namespace std::placeholders;
    const auto bad = std::bind(&is_bad, _1, first_bad);
    int left = 1;
    int right = versions;
    while (left < right) {
        const int middle = left + (right - left) / 2;
        if (bad(middle)) right = middle;
        else left = middle + 1;
    }
    return left;
}

void leetcode_test() {
    expect(leetcode_first_bad_version(5, 4) == 4);
    expect(leetcode_first_bad_version(1, 1) == 1);
}
}  // namespace leetcode

namespace practical {
// -----------------------------------------------------------------------------
// 【日常實務範例】擁有 Worker 的延後處理工作
// 情境：排程器先建立具有倍率設定的 Worker 工作，稍後才提供待處理整數。
// 為何使用本章主題：std::bind 將 Worker 物件移入 bind object，並以 _1 保留執行期輸入；
// 相較綁參考，工作離開 factory 後不會懸空，不過等價 lambda 通常更易讀。
// 設計：Worker 保存 factor；factory 綁定成員函式與 Worker ownership；呼叫 task 時代入 value。
// 成本：建立時移動一個 Worker、每次處理 O(1)，bind object 佔用一份 Worker 儲存空間。
// 上線注意：大型 Worker 的複製/移動成本需量測；bind 物件的 cv 與參數轉送規則也要符合目標函式。
// -----------------------------------------------------------------------------
class Worker {
public:
    explicit Worker(int factor) : factor_(factor) {}
    int process(int value) const { return value * factor_; }
private:
    int factor_;
};

// task 取得 Worker ownership；傳入 temporary 時，物件會被移入 bind object 而不會懸空。
auto practical_task(Worker worker) {
    using namespace std::placeholders;
    return std::bind(&Worker::process, std::move(worker), _1);
}

void practical_test() {
    const auto task = practical_task(Worker{3});
    expect(task(7) == 21);
    // task 自己持有 Worker，因此離開 factory 後仍可安全呼叫。
}
}  // namespace practical

int main() {
    basic::demo();
    leetcode::leetcode_test();
    practical::practical_test();
    std::cout << "std::bind：placeholder/reference、First Bad Version、owned task 測試通過\n";
}

// 【延伸練習】用 lambda 重寫 practical_task，比較 ownership、錯誤訊息與 placeholder 可讀性。

/*
 * 【教科書補充：bind expression 的轉換規則】
 * - 一般 bound value 儲存在 bind object，呼叫時以該 object 的 cv-lvalue 形式傳入；不會每次自動 move。
 * - 因此 move-only 值若最終 callable 參數要求 by-value，bind object 可能無法呼叫；lambda capture 更直觀。
 * - 多餘的 call-time arguments 會被忽略，但仍會求值；帶副作用的多餘參數仍會執行。
 * - overload set 需先 cast 到確切 function pointer；nested bind expression 會被視為組合並展開，不是普通值。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '08_std_bind.cpp' -o '/tmp/codex_cpp_C_Lambda_08_std_bind' && '/tmp/codex_cpp_C_Lambda_08_std_bind'
//
// === 預期輸出（節錄）===
// std::bind：placeholder/reference、First Bad Version、owned task 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
