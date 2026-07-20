/*
 * 第 24 章：Type erasure
 *
 * 模板保留具體型別；type erasure 則隱藏型別，只留下共同操作。std::function、std::any
 * 都是例子。它適合執行期異質集合與穩定邊界，成本可能包括間接呼叫、配置與較弱的
 * 靜態資訊。這不是「void* 亂轉」：好的 erasure wrapper 仍提供型別安全介面。
 */

#include <cassert>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

// LeetCode 278：First Bad Version。Predicate 的具體 lambda 型別被 std::function 擦除。
int leetcode_first_bad(int count, const std::function<bool(int)>& is_bad) {
    int left = 1;
    int right = count;
    while (left < right) {
        const int mid = left + (right - left) / 2;
        if (is_bad(mid)) {
            right = mid;
        } else {
            left = mid + 1;
        }
    }
    return left;
}

class Command {
public:
    template <typename Callable>
    explicit Command(Callable callable)
        : self_(std::make_unique<Model<Callable>>(std::move(callable))) {}

    Command(Command&&) noexcept = default;
    Command& operator=(Command&&) noexcept = default;
    Command(const Command&) = delete;
    Command& operator=(const Command&) = delete;

    std::string execute() { return self_->execute(); }

private:
    struct Concept {
        virtual ~Concept() = default;
        virtual std::string execute() = 0;
    };

    template <typename Callable>
    struct Model final : Concept {
        explicit Model(Callable value) : callable(std::move(value)) {}
        std::string execute() override { return callable(); }
        Callable callable;
    };

    std::unique_ptr<Concept> self_;
};

struct BackupJob {
    std::string target;
    std::string operator()() const { return "backup:" + target; }
};

// 【實務情境】工作佇列同時保存 functor 與 lambda，而來源型別不必共同繼承。
void practical_command_queue_test() {
    std::vector<Command> commands;
    commands.emplace_back(BackupJob{"db"});
    commands.emplace_back([count = 3] { return "emails:" + std::to_string(count); });

    assert(commands[0].execute() == "backup:db");
    assert(commands[1].execute() == "emails:3");
}

int main() {
    const int first_bad = leetcode_first_bad(10, [](int version) { return version >= 4; });
    assert(first_bad == 4);

    practical_command_queue_test();

    // 同一 wrapper 可保存完全不同、互不繼承的 callable 型別。
    Command command([] { return std::string{"done"}; });
    assert(command.execute() == "done");

    std::cout << "type erasure 測試完成\n";
}

/*
 * 【生命週期】Command 擁有 Model；lambda 若 capture reference，外部物件仍必須活到 execute。
 * 【成本】本例每個 Command 一次 heap allocation + virtual call；small-buffer optimization 可改善。
 * 【陷阱】std::function 要求目標可複製（C++23 有 move_only_function）；本例刻意做 move-only。
 * 【面試】type erasure 與 inheritance 差異？來源型別不需繼承共同 base，wrapper 內部代為適配。
 * 【練習】加入 clone() 讓 Command 可複製，並處理不可複製 callable 的限制。
 */

/*
 * 【教科書補充：erased wrapper 也要定義空狀態】
 * - Command move 後來源 self_ 為空；目前 execute() 只允許在非 moved-from 物件呼叫，否則解參考 null。
 * - 完整 wrapper 應提供 bool/empty 檢查或讓空呼叫丟明確例外，並約束 callable 回傳可轉成 string。
 * - first_bad 另要求 count>=1、predicate 可呼叫且結果在版本序列上單調；type erasure 不會驗這些語意。
 * - callable 若 capture reference，Command 擁有 closure 仍不等於擁有被 capture 的外部物件。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '24_type_erasure.cpp' -o '/tmp/codex_cpp_C_Template_24_type_erasure' && '/tmp/codex_cpp_C_Template_24_type_erasure'
//
// === 預期輸出（節錄）===
// type erasure 測試完成
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
