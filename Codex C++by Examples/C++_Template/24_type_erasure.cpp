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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 278. First Bad Version（第一個錯誤的版本）
// 題目：版本 1..n 從某版起全部為 bad，找第一個 bad；n=10 且 4 起為 bad 時答案 4。
// 為何使用本章主題：std::function 擦除 lambda/functor 的具體型別，只暴露 bool(int) oracle；
// 原題由平台提供固定 API，不需要 type erasure，這是 runtime predicate 注入的教學改寫。
// 思路：在閉區間維護 left/right；middle 為 bad 就收右界，否則把左界移到 middle+1。
// 複雜度：時間 O(log N)、額外空間 O(1)，N 是 count，每輪有一次 type-erased 間接呼叫。
// 易錯點：count 必須至少 1、predicate 非空且單調；介面未驗最後候選，假設一定存在 bad 版本。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【日常實務範例】異質工作命令佇列
// 情境：排程器要在同一 vector 保存 BackupJob 與捕獲狀態 lambda，之後以一致 execute 介面執行。
// 為何使用本章主題：Command 內部以 Concept/Model 擦除來源 callable 型別，來源不需共同繼承；
// 相較 std::function，本 wrapper 刻意為 move-only，仍保留型別安全的 string() 邊界。
// 設計：constructor 配置 Model<Callable>；vector 保存 Command；execute 經 virtual Concept 呼叫具體 callable。
// 成本：每個 Command 至少一次 heap allocation，每次執行一次 virtual call，另有結果字串成本。
// 上線注意：moved-from Command 的 self_ 為空不可 execute；reference capture 仍可能懸空，例外也需隔離記錄。
// -----------------------------------------------------------------------------
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
