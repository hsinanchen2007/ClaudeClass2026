/*
 * Lambda 教科書 06：std::function type erasure
 *
 * std::function<R(Args...)> 可保存多種相容 callable（function、lambda、functor、bind result），
 * 呼叫端只看到固定 signature。代價是 type erasure、間接呼叫，且大型 target 可能 heap
 * allocation；small-buffer optimization 常見但標準不保證大小或是否發生。
 *
 * 【ownership】C++20 的 std::function target 必須 CopyConstructible，所以含 unique_ptr 的
 * move-only lambda 不能直接放入；C++23 有 std::move_only_function（另章再談）。
 * 【空狀態】default/null std::function 在 bool context 為 false；呼叫會丟 std::bad_function_call。
 * 【選擇】短期 template parameter/auto callback 可保留 inline；需 heterogeneous storage、ABI
 * 邊界或 runtime replacement 時才用 std::function。
 * 【常見陷阱】呼叫 empty std::function 會丟 bad_function_call，且 move-only target 在 C++20 不相容。
 * 【面試題】std::function 是否保證不配置？不保證。
 */

#include <cassert>
#include <functional>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace basic {
void demo() {
    std::function<int(int)> transform = [](int value) { return value * 2; };
    assert(transform && transform(6) == 12);
    transform = [](int value) { return value + 10; };
    assert(transform(6) == 16);

    const std::function<void()> empty;
    assert(!empty);
    // empty();  // 會丟 std::bad_function_call；預設執行路徑不觸發。
}
}  // namespace basic

namespace leetcode {
// LeetCode 70：Climbing Stairs。std::function 允許 recursive lambda 自我呼叫。
// memo 讓時間 O(n)、空間 O(n)；純遞迴會退化成 exponential。
int leetcode_climb_stairs(int steps) {
    std::vector<int> memo(static_cast<std::size_t>(steps + 1), 0);
    std::function<int(int)> solve = [&memo, &solve](int remaining) -> int {
        if (remaining <= 2) return remaining;
        int& cached = memo[static_cast<std::size_t>(remaining)];
        if (cached == 0) cached = solve(remaining - 1) + solve(remaining - 2);
        return cached;
    };
    return solve(steps);
}

void leetcode_test() {
    assert(leetcode_climb_stairs(5) == 8);
    assert(leetcode_climb_stairs(10) == 89);
}
}  // namespace leetcode

// 【實務案例】HTTP router：異質 handler 經 type erasure 放進同一 map，支援 runtime 註冊。
namespace practical {
using Handler = std::function<std::string(const std::string&)>;

class Router {
public:
    void add(std::string path, Handler handler) {
        handlers_.emplace(std::move(path), std::move(handler));
    }

    std::string practical_dispatch(const std::string& path, const std::string& body) const {
        const auto found = handlers_.find(path);
        if (found == handlers_.end()) return "404";
        return found->second(body);
    }

private:
    std::map<std::string, Handler> handlers_;
};

void practical_test() {
    Router router;
    router.add("/echo", [](const std::string& body) { return "echo:" + body; });
    assert(router.practical_dispatch("/echo", "ok") == "echo:ok");
    assert(router.practical_dispatch("/missing", "") == "404");
}
}  // namespace practical

int main() {
    basic::demo();
    leetcode::leetcode_test();
    practical::practical_test();
    std::cout << "std::function：type erasure、recursive DP、router 測試通過\n";
}

// 【延伸練習】比較 template callback、function pointer、std::function 的 ownership、配置與 inline。

/*
 * 【教科書補充：type erasure 不會替演算法驗證輸入】
 * - Fibonacci 類示範須拒絕負 steps，並在 int 溢位前設定上限；std::function 不會捕捉 signed-overflow UB。
 * - recursive closure 若 capture 外層 std::function&，只能在該物件原位且存活時呼叫；搬移或逃離 scope 會破壞借用。
 * - 每層遞迴都經 type-erased 間接呼叫；教學可讀性可以接受，hot path 宜用迴圈或自遞迴 lambda。
 * - empty std::function 與 target 拋出的例外是兩個不同 failure channel，都應在介面契約中處理。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '06_std_function.cpp' -o '/tmp/codex_cpp_C_Lambda_06_std_function' && '/tmp/codex_cpp_C_Lambda_06_std_function'
//
// === 預期輸出（節錄）===
// std::function：type erasure、recursive DP、router 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
